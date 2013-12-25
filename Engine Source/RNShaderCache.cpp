//
//  RNShaderCache.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNShaderCache.h"
#include "RNSQLite3Internal.h"
#include "RNShader.h"
#include "RNPathManager.h"
#include "RNLogging.h"
#include "RNLockGuard.h"

#define kRNShaderCacheDatabaseVersion 1
#define kRNShaderCacheCacheTableSQL "CREATE TABLE cache (hash TEXT PRIMARY KEY, lookup TEXT, type INTEGER, format INTEGER, binary BLOB)"

namespace RN
{
	struct ShaderCacheInternal
	{
		bool hasSupport;
#if RN_TARGET_SUPPORT_SQLITE
		sqlite3 *connection;
#endif
	};
	
	
	RNDeclareSingleton(ShaderCache)
	
	ShaderCache::ShaderCache()
	{
#if RN_TARGET_SUPPORT_SQLITE
		_internals->hasSupport = SupportsCaching();
		_internals->connection = nullptr;
#else
		_internals->hasSupport = false;
#endif
	}
	
	ShaderCache::~ShaderCache()
	{
#if RN_TARGET_SUPPORT_SQLITE
		if(_internals->connection)
			sqlite3_close(_internals->connection);
#endif
	}
	
	void ShaderCache::InitializeDatabase()
	{
#if RN_TARGET_SUPPORT_SQLITE
		if(_internals->hasSupport)
		{
			std::string path = PathManager::Join(PathManager::SaveDirectory(), "shadercache.sqlite3");
			sqlite3_open(path.c_str(), &_internals->connection);
			
			bool coldCache = false;
			
			if(!SQL::CheckTable(_internals->connection, "manifest"))
			{
				SQL::Query(_internals->connection, "CREATE TABLE manifest (version INTEGER)");
				SQL::Query(_internals->connection, kRNShaderCacheCacheTableSQL);
				
				BumpVersion();
				coldCache = true;
			}
			else
			{
				// Check the version and update if needed
				SQL::Statement versionCheck(_internals->connection, "SELECT version FROM manifest ORDER BY version DESC LIMIT 1");
				versionCheck.Step();
				
				int version = versionCheck.GetColumnInt(0);
				
				if(version < kRNShaderCacheDatabaseVersion)
					coldCache = UpdateDatabase();
				
				if(version > kRNShaderCacheDatabaseVersion || coldCache)
				{
					coldCache = true;
					
					SQL::Query(_internals->connection, "DROP TABLE cache");
					SQL::Query(_internals->connection, kRNShaderCacheCacheTableSQL);
					
					BumpVersion();
				}
			}
			
			
			if(coldCache)
				RNDebug("Shader cache is completely cold!");
		}
#endif
	}
	
	void ShaderCache::BumpVersion()
	{
#if RN_TARGET_SUPPORT_SQLITE
		SQL::Statement addVersion(_internals->connection, "INSERT INTO manifest (version) VALUES(?)");
		addVersion.BindInt(1, kRNShaderCacheDatabaseVersion);
		addVersion.Step();
#endif
	}
	
	bool ShaderCache::UpdateDatabase()
	{
		return false;
	}
	
	
	
	void ShaderCache::InvalidateCacheEntries(Shader *shader)
	{
#if RN_TARGET_SUPPORT_SQLITE
		LockGuard<Mutex> lock(_lock);
		
		std::string hash = std::move(shader->GetFileHash());
		
		SQL::Statement statement(_internals->connection, "DELETE FROM cache WHERE hash=?");
		statement.BindText(1, hash);
		statement.Step();
#endif
	}
	
	void ShaderCache::CacheShaderProgram(Shader *shader, ShaderProgram *program, const ShaderLookup& lookup)
	{
#if RN_TARGET_SUPPORT_SQLITE
#if GL_ARB_get_program_binary
		if(_internals->hasSupport)
		{
			GLint size;
			gl::GetProgramiv(program->program, GL_PROGRAM_BINARY_LENGTH, &size);
			
			if(!size)
				return;
			
			
			GLint length;
			GLenum format;
			uint8 *buffer = new uint8[size];
			
			gl::GetProgramBinary(program->program, size, &length, &format, buffer);
			
			
			std::string hash = std::move(shader->GetFileHash());
			
			LockGuard<Mutex> lock(_lock);
			
			SQL::Statement statement(_internals->connection, "INSERT INTO cache (hash, lookup, type, format, binary) VALUES(?, ?, ?, ?, ?)");
			statement.BindText(1, hash);
			statement.BindText(2, lookup.mangledDefines);
			statement.BindInt(3, lookup.type);
			statement.BindInt(4, format);
			statement.BindBlob(5, buffer, length);
			
			statement.Step();
			
			delete [] buffer;
		}
#endif
#endif
	}

	ShaderProgram *ShaderCache::DequeShaderProgram(Shader *shader, const ShaderLookup& lookup)
	{
#if RN_TARGET_SUPPORT_SQLITE
		if(!_internals->connection)
			return nullptr;
		
		LockGuard<Mutex> lock(_lock);
		
		std::string hash = std::move(shader->GetFileHash());
		
		SQL::Statement statement(_internals->connection, "SELECT (binary, format) FROM cache WHERE hash=? AND lookup=? AND type=?");
		statement.BindText(1, hash);
		statement.BindText(2, lookup.mangledDefines);
		statement.BindInt(3, lookup.type);
		
		if(statement.Step())
		{
			GLsizei length;
			const void *binary = statement.GetColumnBlob(1, length);
			GLenum format = static_cast<GLenum>(statement.GetColumnInt(2));
			
			ShaderProgram *program = new ShaderProgram;
			program->program = gl::CreateProgram();
			
			gl::ProgramBinary(program->program, format, binary, length);
			
			GLint status;
			gl::GetProgramiv(program->program, GL_LINK_STATUS, &status);
			
			if(!status)
			{
				delete program;
				return nullptr;
			}
			
			program->ReadLocations();
		}
#endif
		return nullptr;
	}
	
	
	
	bool ShaderCache::SupportsCaching()
	{
#if RN_TARGET_SUPPORT_SQLITE
		static bool hasSupport;
		static std::once_flag flag;
		
		std::call_once(flag, [&]() {
			hasSupport = gl::SupportsFeature(gl::Feature::ShaderBinary);
			
			if(hasSupport)
			{
				GLint formats = 0;
				gl::GetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &formats);
				
				hasSupport = (formats > 0);
			}
		});
		
		return hasSupport;
#else
		return false;
#endif
	}
}