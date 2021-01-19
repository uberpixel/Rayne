#!/usr/bin/env python3
#
# Copyright (C) 2016 Google, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

"""Generate Vulkan dispatch table.
"""

import os
import sys

class Command(object):
    PLATFORM = 0
    LOADER = 1
    INSTANCE = 2
    DEVICE = 3

    def __init__(self, name, dispatch):
        self.name = name
        self.dispatch = dispatch
        self.ty = self._get_type()

    @staticmethod
    def valid_c_typedef(c):
        return (c.startswith("typedef") and
                c.endswith(");") and
                "*PFN_vkVoidFunction" not in c)

    @classmethod
    def from_c_typedef(cls, c):
        name_begin = c.find("*PFN_vk") + 7
        name_end = c.find(")(", name_begin)
        name = c[name_begin:name_end]

        dispatch_begin = name_end + 2
        dispatch_end = c.find(" ", dispatch_begin)
        dispatch = c[dispatch_begin:dispatch_end]
        if not dispatch.startswith("Vk"):
            dispatch = None

        return cls(name, dispatch)

    def _get_type(self):
        if self.dispatch:
            if self.dispatch in ["VkDevice", "VkQueue", "VkCommandBuffer"]:
                return self.DEVICE
            else:
                return self.INSTANCE
        else:
            if self.name in ["GetInstanceProcAddr"]:
                return self.PLATFORM
            else:
                return self.LOADER

    def __repr__(self):
        return "Command(name=%s, dispatch=%s)" % \
                (repr(self.name), repr(self.dispatch))

class Extension(object):
    def __init__(self, name, version, guard=None, commands=[]):
        self.name = name
        self.version = version
        self.guard = guard
        self.commands = commands[:]

    def add_command(self, cmd):
        self.commands.append(cmd)

    def __repr__(self):
        lines = []
        lines.append("Extension(name=%s, version=%s, guard=%s, commands=[" %
                (repr(self.name), repr(self.version), repr(self.guard)))

        for cmd in self.commands:
            lines.append("    %s," % repr(cmd))

        lines.append("])")

        return "\n".join(lines)
