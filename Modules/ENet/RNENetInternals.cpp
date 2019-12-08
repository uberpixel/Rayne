//
//  RNENetInternals.cpp
//  Rayne-ENet
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNENetInternals.h"

namespace RN
{
#if RN_ENET_USE_BOTAN_DTLS_ENCRYPTION
	ENetClientEncryptor::ENetClientEncryptor(ENetClient *client) : _botanClient(*this, client->_encryptorSharedInternals->botanSessionManager, *this, client->_encryptorSharedInternals->botanPolicy, client->_encryptorSharedInternals->botanRNG, Botan::TLS::Server_Information("", 0), Botan::TLS::Protocol_Version::DTLS_V12), _client(client)
	{

	}

	ENetClientEncryptor::~ENetClientEncryptor()
	{
		
	}

	void ENetClientEncryptor::SendData(RN::Data *data)
	{
		_botanClient.send(data->GetBytes<uint8_t>(), data->GetLength());
	}

	void ENetClientEncryptor::ReceivedData(RN::Data *data)
	{
		_botanClient.received_data(data->GetBytes<uint8_t>(), data->GetLength());
	}

	void ENetClientEncryptor::tls_emit_data(const uint8_t data[], size_t size)
	{
		//RN::Data *dataPacket = RN::Data::WithBytes(data, size);
		//_client->SendPacket(dataPacket);
		RNDebug("tls_emit_data");
	}

	void ENetClientEncryptor::tls_record_received(uint64_t seq_no, const uint8_t data[], size_t size)
	{
		//RN::Data *dataPacket = RN::Data::WithBytes(data, size);
		//_client->ProcessPacket(dataPacket);
		RNDebug("tls_record_received");
	}

	void ENetClientEncryptor::tls_alert(Botan::TLS::Alert alert)
	{
		// handle a tls alert received from the tls server
		RNDebug("tls_alert");
	}

	bool ENetClientEncryptor::tls_session_established(const Botan::TLS::Session& session)
	{
		RNDebug("tls_session_established");
		return false;
	}


	std::vector<Botan::Certificate_Store*> ENetClientEncryptor::trusted_certificate_authorities(const std::string& type, const std::string& context)
	{
		return _client->_encryptorSharedInternals->certificateStore;
	}

	std::vector<Botan::X509_Certificate> ENetClientEncryptor::cert_chain(const std::vector<std::string>& cert_key_types, const std::string& type, const std::string& context)
	{
		return std::vector<Botan::X509_Certificate>();
	}

	Botan::Private_Key* ENetClientEncryptor::private_key_for(const Botan::X509_Certificate& cert, const std::string& type, const std::string& context)
	{
		return nullptr;
	}
#endif

	ENetClientEncryptorSharedInternals::ENetClientEncryptorSharedInternals(RN::String *trustedCertStorePath)
#if RN_ENET_USE_BOTAN_DTLS_ENCRYPTION
		: botanSessionManager(botanRNG)
#endif
	{
#if RN_ENET_USE_BOTAN_DTLS_ENCRYPTION
//		_certificateStore.push_back(new Botan::System_Certificate_Store);
		
//#if RN_PLATFORM_ANDROID
/*		RN::String *directory = RN::FileManager::GetSharedInstance()->GetPathForLocation(RN::FileManager::Location::SaveDirectory);
		RN::String *caFilePath = directory->Copy();
		caFilePath->AppendPathComponent(RNCSTR("aws_cacert.pem"));*/
		
		RN::Data *data = RN::Data::WithContentsOfFile(trustedCertStorePath);
		certificate = Botan::X509_Certificate(data->GetBytes<uint8_t>(), data->GetLength());
		certificateStore.push_back(new Botan::Certificate_Store_In_Memory(certificate));
//		SafeRelease(caFilePath);
//#endif
#endif
	}

	ENetClientEncryptorSharedInternals::~ENetClientEncryptorSharedInternals()
	{
		
	}


#if RN_ENET_USE_BOTAN_DTLS_ENCRYPTION
	ENetServerEncryptor::ENetServerEncryptor(ENetServer *server, RN::uint16 clientID) : _botanServer(*this, server->_encryptorSharedInternals->botanSessionManager, *this, server->_encryptorSharedInternals->botanPolicy, server->_encryptorSharedInternals->botanRNG, true), _server(server), _clientID(clientID)
	{
		
	}

	ENetServerEncryptor::~ENetServerEncryptor()
	{
		
	}
	
	void ENetServerEncryptor::tls_emit_data(const uint8_t data[], size_t size)
	{
		//RN::Data *dataPacket = RN::Data::WithBytes(data, size);
		//_server->SendPacket(dataPacket, _clientID, 0, false);
		RNDebug("tls_emit_data");
	}

	void ENetServerEncryptor::tls_record_received(uint64_t seq_no, const uint8_t data[], size_t size)
	{
		RNDebug("tls_record_received");
	}

	void ENetServerEncryptor::tls_alert(Botan::TLS::Alert alert)
	{
		RNDebug("tls_alert");
	}

	bool ENetServerEncryptor::tls_session_established(const Botan::TLS::Session& session)
	{
		RNDebug("tls_session_established");
		return false;
	}


	std::vector<Botan::Certificate_Store*> ENetServerEncryptor::trusted_certificate_authorities(const std::string& type, const std::string& context)
	{
		return std::vector<Botan::Certificate_Store*>();
	}

	std::vector<Botan::X509_Certificate> ENetServerEncryptor::cert_chain(const std::vector<std::string>& cert_key_types, const std::string& type, const std::string& context)
	{
		if(_server->_encryptorSharedInternals->certificate)
		{
			return { *_server->_encryptorSharedInternals->certificate };
		}
		
		return std::vector<Botan::X509_Certificate>();
	}

	Botan::Private_Key* ENetServerEncryptor::private_key_for(const Botan::X509_Certificate& cert, const std::string& type, const std::string& context)
	{
		return _server->_encryptorSharedInternals->privateKey;
	}
#endif

	ENetServerEncryptorSharedInternals::ENetServerEncryptorSharedInternals(String *privateKeyPath, String *certificatePath)
#if RN_ENET_USE_BOTAN_DTLS_ENCRYPTION
		: botanSessionManager(botanRNG)
#endif
	{
#if RN_ENET_USE_BOTAN_DTLS_ENCRYPTION
		//TODO: Add an assert if either of the parameters is null
		
		privateKey = Botan::PKCS8::load_key(privateKeyPath->GetUTF8String(), botanRNG);
		certificate = new Botan::X509_Certificate(certificatePath->GetUTF8String());
#endif
	}

	ENetServerEncryptorSharedInternals::~ENetServerEncryptorSharedInternals()
	{
#if RN_ENET_USE_BOTAN_DTLS_ENCRYPTION
		delete privateKey;
		delete certificate;
#endif
	}
}
