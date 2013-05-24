#!/usr/bin/python

import sleekxmpp
import json
import os
import subprocess
from optparse import OptionParser

basedir = os.path.dirname(os.path.realpath(__file__))
config = json.load(open(os.path.join(basedir, 'config.json')))

class XMPPBot(sleekxmpp.ClientXMPP):

	def __init__(self, jid, password, msg):
		super(XMPPBot, self).__init__(jid, password)

		self.msg = msg

		self.add_event_handler('session_start', self.start)
		self.add_event_handler('message', self.message)

		self.register_plugin('xep_0030')
		self.register_plugin('xep_0199')

	def start(self, event):
		self.send_presence()
		self.get_roster()

		for recipient in config['recipients']:
			self.send_message(mto=recipient, mbody=self.msg)

		self.disconnect(wait=True)

def GitCommand(command, dir):
	pipe = subprocess.Popen(command, shell=True, cwd=dir, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	(out, error) = pipe.communicate()
	pipe.wait()

	return out.strip()


def GitCommitData(since, until, dir):
	data = GitCommand('git log {0}..{1} --format="%s - %an"'.format(since, until), dir)
	result = data.strip()
	return result.split('\n')


def ParseCommit(gitdir, oldrev, revision):
	if revision == '0000000000000000000000000000000000000000':
		return None

	branch  = GitCommand('git branch --contains {0}'.format(revision), gitdir)
	author  = GitCommand('git show --quiet --format="%an" {0}'.format(revision), gitdir)

	if oldrev == '0000000000000000000000000000000000000000':
		return '{0} published branch {1}'.format(author, branch)


	commits = GitCommitData(oldrev, revision, gitdir)
	verb    = 'commit' if len(commits) == 1 else 'commits'
	message = '{0} pushed {1} {2} to {3}'.format(author, len(commits), verb, branch)

	for commit in commits:
		message += '\n'
		message += commit

	return message


if __name__ == '__main__':
	optp = OptionParser()
	optp.add_option("-o", "--oldrev", dest="oldrev", help="The old revision")
	optp.add_option("-n", "--newrev", dest="newrev", help="The new revision")
	optp.add_option("-g", "--git", dest="git", help="The repo directory")

	(opts, args) = optp.parse_args()

	message = ParseCommit(opts.git, opts.oldrev, opts.newrev)
	if message is not None:
		xmpp = XMPPBot(config['sender']['jid'], config['sender']['pw'], message)

		if xmpp.connect():
			xmpp.process(block=True)
