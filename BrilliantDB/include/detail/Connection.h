#pragma once

#include <fstream>
#include <string>
#include <sqlite3.h>
#include "detail/SqliteError.h"

namespace BrilliantDB
{
	struct Connection
	{
		Connection(std::string sDir) : sDirectory(std::move(sDir)) { Open(); }
		~Connection() 
		{ 
			Close();
		}

		Connection(const Connection& other) = delete;
		Connection(Connection&& other) noexcept : pDb(other.pDb), sDirectory(other.sDirectory)
		{
			other.pDb = nullptr;
		}

		Connection& operator= (const Connection& other) = delete;
		Connection operator= (Connection&& other) noexcept
		{
			return Connection(std::move(other));
		}

		void Open() noexcept(false);
		void Close() noexcept(false);

		sqlite3* pDb = nullptr;
		const std::string sDirectory;
	};

	void Connection::Open() noexcept(false)
	{
		if (sqlite3_open(sDirectory.c_str(), &pDb) != SQLITE_OK)
		{
			ThrowError(pDb);
		}
	}

	void Connection::Close() noexcept(false)
	{
		if (sqlite3_close(pDb) != SQLITE_OK)
		{
			ThrowError(pDb);
		}
		pDb = nullptr;
	}
}