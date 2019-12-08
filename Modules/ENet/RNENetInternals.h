//
//  RNENetInternals.h
//  Rayne-ENet
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ENETINTERNALS_H_
#define __RAYNE_ENETINTERNALS_H_

#include "RNENetClient.h"
#include "RNENetServer.h"

#if RN_ENET_USE_BOTAN_DTLS_ENCRYPTION
	#include <botan/tls_server.h>
	#include <botan/tls_client.h>
	#include <botan/tls_callbacks.h>
	#include <botan/tls_session_manager.h>
	#include <botan/tls_policy.h>
	#include <botan/auto_rng.h>
	#include <botan/certstor.h>
	#include <botan/pkcs8.h>
	#include <botan/pk_keys.h>
#endif

namespace RN
{
#if RN_ENET_USE_BOTAN_DTLS_ENCRYPTION
	class ENetClientEncryptor : public Botan::Credentials_Manager, Botan::TLS::Callbacks
	{
	public:
		ENetClientEncryptor(ENetClient *client);
		~ENetClientEncryptor();
		
		void SendData(RN::Data *data);
		void ReceivedData(RN::Data *data);
		
	private:
		//Botan::TLS::Callbacks implementation
		void tls_emit_data(const uint8_t data[], size_t size) override;
		void tls_record_received(uint64_t seq_no, const uint8_t data[], size_t size) override;
		void tls_alert(Botan::TLS::Alert alert) override;
		bool tls_session_established(const Botan::TLS::Session& session) override;
		
		//Botan::Credentials_Manager implementation
		std::vector<Botan::Certificate_Store*> trusted_certificate_authorities(const std::string& type, const std::string& context) override;
		std::vector<Botan::X509_Certificate> cert_chain(const std::vector<std::string>& cert_key_types, const std::string& type, const std::string& context) override;
		Botan::Private_Key* private_key_for(const Botan::X509_Certificate& cert, const std::string& type, const std::string& context) override;
		
		Botan::TLS::Client _botanClient;
		ENetClient *_client;
	};
#endif
	class ENetClientEncryptorSharedInternals
	{
	public:
		ENetClientEncryptorSharedInternals(RN::String *trustedCertStorePath);
		~ENetClientEncryptorSharedInternals();
		
#if RN_ENET_USE_BOTAN_DTLS_ENCRYPTION
		Botan::AutoSeeded_RNG botanRNG;
		Botan::TLS::Session_Manager_In_Memory botanSessionManager;
		Botan::TLS::Strict_Policy botanPolicy;
		Botan::X509_Certificate certificate;
		std::vector<Botan::Certificate_Store*> certificateStore;
#endif
	};

#if RN_ENET_USE_BOTAN_DTLS_ENCRYPTION
	class ENetClientEncryptorContext
	{
	public:
		ENetClient *client;
		ENetClientEncryptor *encryptor;
	};


	class ENetServerEncryptor : public Botan::Credentials_Manager, Botan::TLS::Callbacks
	{
	public:
		ENetServerEncryptor(ENetServer *server, RN::uint16 clientID);
		~ENetServerEncryptor();
		
		void SendData(RN::Data *data);
		void ReceivedData(RN::Data *data);
		
	private:
		//Botan::TLS::Callbacks implementation
		void tls_emit_data(const uint8_t data[], size_t size) override;
		void tls_record_received(uint64_t seq_no, const uint8_t data[], size_t size) override;
		void tls_alert(Botan::TLS::Alert alert) override;
		bool tls_session_established(const Botan::TLS::Session& session) override;
		
		//Botan::Credentials_Manager implementation
		std::vector<Botan::Certificate_Store*> trusted_certificate_authorities(const std::string& type, const std::string& context) override;
		std::vector<Botan::X509_Certificate> cert_chain(const std::vector<std::string>& cert_key_types, const std::string& type, const std::string& context) override;
		Botan::Private_Key* private_key_for(const Botan::X509_Certificate& cert, const std::string& type, const std::string& context) override;
		
		Botan::TLS::Server _botanServer;
		
		ENetServer *_server;
		RN::uint16 _clientID;
	};
#endif

	class ENetServerEncryptorSharedInternals
	{
	public:
		ENetServerEncryptorSharedInternals(RN::String *privateKeyPath, RN::String *certificatePath);
		~ENetServerEncryptorSharedInternals();
		
#if RN_ENET_USE_BOTAN_DTLS_ENCRYPTION
		Botan::AutoSeeded_RNG botanRNG;
		Botan::TLS::Session_Manager_In_Memory botanSessionManager;
		Botan::TLS::Strict_Policy botanPolicy;
		Botan::Private_Key *privateKey;
		Botan::X509_Certificate *certificate;
#endif
	};

#if RN_ENET_USE_BOTAN_DTLS_ENCRYPTION
	class ENetServerEncryptorContext
	{
	public:
		ENetServer *server;
		ENetServerEncryptor **encryptors;
	};
#endif
}

#endif /* defined(__RAYNE_ENETINTERNALS_H_) */
