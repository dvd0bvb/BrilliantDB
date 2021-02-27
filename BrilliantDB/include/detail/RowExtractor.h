#pragma once

#include <optional>
#include <string>
#include <vector>

#include "detail/GeneralConcepts.h"
#include "detail/KeyTypes.h"
#include "detail/PreparedStatement.h"

namespace BrilliantDB
{
	struct RowExtractor
	{
		template<RelativelyLargeInt T, class S>
		T Extract(const PreparedStatement<S>& stmt)
		{
			return static_cast<T>(sqlite3_column_int64(stmt.pStmt, iIndex++));
		}

		template<RelativelySmallInt T, class S>
		T Extract(const PreparedStatement<S>& stmt)
		{
			return static_cast<T>(sqlite3_column_int(stmt.pStmt, iIndex++));
		}

		template<std::floating_point T, class S>
		T Extract(const PreparedStatement<S>& stmt)
		{
			return static_cast<T>(sqlite3_column_double(stmt.pStmt, iIndex++));
		}

		template<PrimaryKeyType T, class S>
		T Extract(const PreparedStatement<S>& stmt)
		{
			return T(sqlite3_column_int64(stmt.pStmt, iIndex++));
		}

		template<ForeignKeyType T, class S>
		T Extract(const PreparedStatement<S>& stmt)
		{
			return T(sqlite3_column_int64(stmt.pStmt, iIndex++));
		}

		template<class T, class S> requires std::same_as<std::decay_t<T>, std::string>
		std::string Extract(const PreparedStatement<S>& stmt)
		{
			auto ret = reinterpret_cast<const char*>(sqlite3_column_text(stmt.pStmt, iIndex++));
			if (ret) { return ret; }
			return {};
		}

		template<class T, class S> requires std::same_as<std::decay_t<T>, std::vector<char>>
		std::vector<char> Extract(const PreparedStatement<S>& stmt)
		{
			auto buffer = static_cast<const char*>(sqlite3_column_blob(stmt.pStmt, iIndex));
			std::size_t sz = sqlite3_column_bytes(stmt.pStmt, iIndex);
			iIndex++;
			if (sz) { return { buffer, buffer + sz }; }
			return {};
		}

		int iIndex = 0;
	};

	template<class T, class... Ts, class S>
	std::optional<T> Build(const PreparedStatement<S>& stmt, const std::tuple<Ts...>& cols)
	{
		RowExtractor extractor;
		auto obj = std::make_optional<T>();
		TupleUtils::for_each_tuple(cols, [&](auto& col) 
			requires !is_foreign_key<T>::value 
			{
			(*obj).*col.pMember = extractor.Extract<typename std::decay_t<decltype(col)>::FieldType>(stmt);
			});
		return obj;
	}
}