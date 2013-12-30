//
//  RNSQLite3Internal.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SQLITE3INTERNAL_H__
#define __RAYNE_SQLITE3INTERNAL_H__

#define RN_TARGET_SUPPORT_SQLITE 0

#if RN_TARGET_SUPPORT_SQLITE

#include <sqlite3.h>
#include "RNBase.h"

namespace RN
{
	namespace SQL
	{
		static inline void CheckError(sqlite3 *connection, int result)
		{
			if(result != SQLITE_OK)
			{
				std::string error(sqlite3_errmsg(connection));
				throw Exception(Exception::Type::InconsistencyException, error);
			}
		}
		
		static inline void CheckError(sqlite3 *connection)
		{
			CheckError(connection, sqlite3_errcode(connection));
		}
		
		
		
		class Statement
		{
		public:
			typedef sqlite3_destructor_type SqliteType;
			
			Statement(sqlite3 *connection, const std::string& query) :
				_connection(connection),
				_statement(nullptr)
			{
				sqlite3_prepare_v2(_connection, query.c_str(), static_cast<int>(query.length()), &_statement, nullptr);
				CheckError(_connection);
			}
			
			~Statement()
			{
				if(_statement)
					sqlite3_finalize(_statement);
			}
			
			
			void BindNull(int index)
			{
				sqlite3_bind_null(_statement, index);
			}
			
			void BindBlob(int index, const void *data, int size, SqliteType type = SQLITE_STATIC)
			{
				sqlite3_bind_blob(_statement, index, data, size, type);
			}
			
			void BindText(int index, const std::string& text, SqliteType type = SQLITE_STATIC)
			{
				sqlite3_bind_text(_statement, index, text.c_str(), static_cast<int>(text.length()), type);
			}
			
			void BindText(int index, const char *text, int size, SqliteType type = SQLITE_STATIC)
			{
				sqlite3_bind_text(_statement, index, text, size, type);
			}
			
			void BindDouble(int index, double val)
			{
				sqlite3_bind_double(_statement, index, val);
			}
			
			void BindInt(int index, int val)
			{
				sqlite3_bind_int(_statement, index, val);
			}
			
			void BindInt64(int index, int64 val)
			{
				sqlite3_bind_int64(_statement, index, val);
			}
			
			
			void Reset()
			{
				sqlite3_reset(_statement);
			}
			
			bool Step()
			{
				int result = sqlite3_step(_statement);
				
				if(result != SQLITE_ROW && result != SQLITE_DONE)
					CheckError(_connection);
				
				return (result == SQLITE_ROW);
			}
			
			
			
			int GetColumnCount()
			{
				return sqlite3_column_count(_statement);
			}
			
			int GetColumnType(int column)
			{
				return sqlite3_column_type(_statement, column);
			}
			
			
			
			const void *GetColumnBlob(int column, int& size)
			{
				const void *blob = sqlite3_column_blob(_statement, column);
				size = sqlite3_column_bytes(_statement, column);
				
				return blob;
			}
			
			double GetColumnDouble(int column)
			{
				return sqlite3_column_double(_statement, column);
			}
			
			int GetColumnInt(int column)
			{
				return sqlite3_column_int(_statement, column);
			}
			
			int64 GetColumnInt64(int column)
			{
				return sqlite3_column_int64(_statement, column);
			}
			
			std::string GetColumnText(int column)
			{
				const char *text = reinterpret_cast<const char *>(sqlite3_column_text(_statement, column));
				int size = size = sqlite3_column_bytes(_statement, column);
				
				return std::string(text, size);
			}
			
		private:
			sqlite3 *_connection;
			sqlite3_stmt *_statement;
		};
		
		
		static inline void Query(sqlite3 *connection, const std::string& query)
		{
			SQL::Statement statement(connection, query);
			statement.Step();
		}
		
		static inline bool CheckTable(sqlite3 *connection, const std::string& table)
		{
			SQL::Statement statement(connection, "SELECT name FROM sqlite_master WHERE type='table' AND name=?");
			statement.BindText(1, table);
			
			bool result = statement.Step();
			return result;
		}
	}
}

#endif /* RN_TARGET_SUPPORT_SQLITE */

#endif /* __RAYNE_SQLITE3INTERNAL_H__ */
