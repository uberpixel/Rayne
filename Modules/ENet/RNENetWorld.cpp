//
//  RNENetWorld.cpp
//  Rayne-ENet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNENetWorld.h"
#include "enet/enet.h"

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

namespace RN
{
	RNDefineMeta(ENetWorld, SceneAttachment)

	ENetWorld *ENetWorld::_instance = nullptr;

	ENetWorld* ENetWorld::GetInstance()
	{
		return _instance;
	}

	ENetWorld::ENetWorld() : _hosts(new Array())
	{
		RN_ASSERT(!_instance, "There already is an ENetWorld!");

		if(enet_initialize() != 0)
		{
			RNDebug("Failed initializing enet.");
			return;
		}

		_instance = this;
	}
		
	ENetWorld::~ENetWorld()
	{
		_hosts->Release();

		_instance = nullptr;
		enet_deinitialize();
	}

	void ENetWorld::Update(float delta)
	{
		_hosts->Enumerate<ENetHost>([&](ENetHost *host, size_t index, bool &stop) {
			host->Update(delta);
		});
	}

	void ENetWorld::AddHost(ENetHost *host)
	{
		_hosts->AddObject(host);
	}

	void ENetWorld::RemoveHost(ENetHost *host)
	{
		_hosts->RemoveObject(host);
	}

	void ENetWorld::Ping(String *ip, size_t repetitions)
	{
		RNDebug("Pinging " << ip << ".");
		
		struct PingPacket
		{
			struct icmp header;
			char padding[64 - sizeof(struct icmp)]; //Extra padding to get to minimum packet size
		};
		
		struct sockaddr_in addr;
		PingPacket packet;
		
		int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_ICMP);
		if(sock < 0)
		{
			RNDebug("Ping failed opening ICMP socket! Need to be root user!?");
		}
		
		struct timeval tv_out;
		tv_out.tv_sec = 1;
		tv_out.tv_usec = 0;
		
		// setting timeout of recv setting
//		int ttl_val = 64;
//		setsockopt(sock, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val));
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_out, sizeof(tv_out));
		
		std::vector<double> pingTimes;
		pingTimes.reserve(repetitions);

		// Send the packet
		for(int i = 0; i < repetitions; i++)
		{
			RNDebug("Ping " << i);
			
			ENetAddress address;
			enet_address_set_host(&address, ip->GetUTF8String());
			
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = address.host;

			// Initialize the ICMP header
			memset(&packet.header, 0, sizeof(packet));
			packet.header.icmp_type = ICMP_ECHO;
			packet.header.icmp_code = 0;
			packet.header.icmp_hun.ih_idseq.icd_id = 1337;
			packet.header.icmp_hun.ih_idseq.icd_seq = i+1;
			
			//Calculate checksum
			int packetLength = sizeof(packet);
			unsigned short *buf = reinterpret_cast<unsigned short*>(&packet);
			unsigned int checksum = 0;
			for(checksum = 0; packetLength > 1; packetLength -= 2)
			{
				checksum += *buf++;
			}
			if(packetLength == 1)
			{
				checksum += *(unsigned char*)buf;
			}
			checksum = (checksum >> 16) + (checksum & 0xFFFF);
			checksum += (checksum >> 16);
			packet.header.icmp_cksum = ~checksum;
			
			struct timespec startTime;
			struct timespec endTime;
			clock_gettime(CLOCK_MONOTONIC, &startTime);
			if(sendto(sock, &packet, sizeof(packet), 0, (struct sockaddr*) &addr, sizeof(addr)) < 0)
			{
				RNDebug("Ping failed sending!");
				continue;
			}
			
			//Waiting for answer to ping
			sockaddr_in r_addr;
			socklen_t addr_len = sizeof(r_addr);

			RNDebug("Ping waiting for response!");
			while(1)
			{
				if(recvfrom(sock, &packet, sizeof(packet), 0, (struct sockaddr*) &r_addr, &addr_len) <= 0)
				{
					RNDebug("Ping packet receive failed!");
					break;
				}
				else
				{
					//The 69 here seems to be a kernel bug on some systems (including macOS): https://blog.benjojo.co.uk/post/linux-icmp-type-69
					if((packet.header.icmp_type == ICMP_ECHOREPLY || packet.header.icmp_type == 69) && packet.header.icmp_code == 0) //It is an echo response!
					{
						clock_gettime(CLOCK_MONOTONIC, &endTime);
						double timeElapsed = (endTime.tv_sec- startTime.tv_sec) * 1000.0 + ((double)(endTime.tv_nsec - startTime.tv_nsec))/1000000.0;
						pingTimes.push_back(timeElapsed);
						RNDebug("Ping success: " << timeElapsed);
						break;
					}
					else
					{
						RNDebug("Ping error: Packet received with ICMP type " << packet.header.icmp_type << " and code " << packet.header.icmp_code);
					}
				}
			}
			
			usleep(100);
		}
		
		std::sort(pingTimes.begin(), pingTimes.end());
		
		double averagePing = 0.0;
		int filterSize = pingTimes.size() / 4;
		for(int i = filterSize; i < pingTimes.size() - filterSize; i++)
		{
			RNDebug("Ping " << pingTimes[i]);
			averagePing += pingTimes[i];
		}
		averagePing /= pingTimes.size() - 2 * filterSize;
		
		RNDebug("Pinging Finished with average of " << averagePing << "ms");
	}
}
