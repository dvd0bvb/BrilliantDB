#pragma once

#include <system_error>
#include <sqlite3.h>

namespace BrilliantDB
{
	class SqliteErrorCat : public std::error_category
	{
	public:
		const char* name() const noexcept
		{
			return "SQLite error";
		}

		std::string message(int i) const
		{
			return sqlite3_errstr(i);
		}
	};

	inline void ThrowError(sqlite3* pDb)
	{
		throw std::system_error(sqlite3_errcode(pDb), SqliteErrorCat()), sqlite3_errmsg(pDb);
	}
}