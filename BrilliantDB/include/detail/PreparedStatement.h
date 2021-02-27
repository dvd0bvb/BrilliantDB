#pragma once

#include <functional>
#include <string>
#include <sqlite3.h>
#include "detail/Connection.h"
#include "detail/Statement.h"
#include "detail/StatementPrinter.h"
#include "detail/SqliteError.h"
#include "detail/GeneralConcepts.h"
#include "detail/TupleUtils.h"
#include "detail/KeyTypes.h"

namespace BrilliantDB
{
	template<class S>
	struct PreparedStatement
	{
		using StatementType = S;

		template<class C>
		PreparedStatement(const S& statement, const C& context)
		{
			if (sqlite3_prepare_v2(context.connection.pDb, Print(statement, context).c_str(), -1, &pStmt, nullptr) != SQLITE_OK)
			{
				ThrowError(context.connection.pDb);
			}
		}

		void Finalize(const Connection& conn)
		{
			if (sqlite3_finalize(pStmt) != SQLITE_OK)
			{
				ThrowError(conn.pDb);
			}
		}

		sqlite3_stmt* pStmt = nullptr;
	};

	template<class S>
	struct Binder
	{
		explicit Binder(const Connection& c, const PreparedStatement<S>& s) : conn(c), stmt(s) {}

		template<RelativelyLargeInt T>
		int Bind(T t) { return sqlite3_bind_int64(stmt.pStmt, iIndex++, t); }

		template<RelativelySmallInt T>
		int Bind(T t) { return sqlite3_bind_int(stmt.pStmt, iIndex++, t); }

		template<std::floating_point T>
		int Bind(T t) { return sqlite3_bind_double(stmt.pStmt, iIndex++, t); }

		int Bind(const char* t) { return sqlite3_bind_text(stmt.pStmt, iIndex++, t, -1, SQLITE_TRANSIENT); }

		template<PrimaryKeyType T>
		int Bind(T t) { return Bind<T::type>(t._t); }

		template<ForeignKeyType T>
		int Bind(T t) { return Bind<T::type>(t._t); }

		int Bind(const std::string& s) { return Bind(s.c_str()); }

		template<ColStatement T>
		int Bind(const T& c) { return Bind(c.value); }

		int Bind(const std::vector<char>& v)
		{
			if (v.size())
			{
				return sqlite3_bind_blob(stmt.pStmt, iIndex++, (const void*)&v.front(), v.size(), SQLITE_TRANSIENT);
			}
			else
			{
				return sqlite3_bind_blob(stmt.pStmt, iIndex++, "", 0, SQLITE_TRANSIENT);
			}
		}

		template<class T>
		void operator() (T&& t)
		{
			if (Bind(t) != SQLITE_OK)
			{
				ThrowError(conn.pDb);
			}
		}

		int iIndex = 1; //binder indices start at 1 not 0
		const Connection& conn;
		const PreparedStatement<S>& stmt;
	};

	struct Extractor
	{
		template<class... Ts>
		auto operator() (const Statement<Ts...>& t) const
		{
			return TupleUtils::BuildFromOther(*this, TupleUtils::Flatten(t.tItems));
		}

		template<class T, class U, logical_c V>
		auto operator() (const LogicalC<T, U, V>& t) const
		{
			return TupleUtils::BuildFromOther(*this, TupleUtils::Flatten(t.t));
		}

		template<ColStatement T>
		auto operator() (const T& t) const
		{
			return t;
		}
	};

	template<class S, class C>
	PreparedStatement<S> Prepare(const S& statement, const C& context)
	{
		PreparedStatement stmt(statement, context);
		auto tExtracted = TupleUtils::Flatten(TupleUtils::BuildFromOther(Extractor(), statement.tItems));
		TupleUtils::for_each_tuple(tExtracted, Binder{ context.connection, stmt });
		return stmt;
	}
}