#pragma once

#include <algorithm>
#include <concepts>
#include <tuple>
#include <type_traits>
#include <sqlite3.h>

#include "detail/RowExtractor.h"
#include "detail/StatementPrinter.h"
#include "detail/Connection.h"
#include "detail/PreparedStatement.h"

namespace BrilliantDB
{
	//Db_Impl allows us to use tuple-like subclass unfolding to find and store tables
	template<class... Ts>
	struct Db_Impl;

	template<>
	struct Db_Impl<> 
	{
		template <class F>
		void ForEachTable(F&& f) {}

		template<class F>
		void ForEachTable(F&& f) const {}
	};

	template<class T, class... Ts>
	struct Db_Impl<T, Ts...> : public Db_Impl<Ts...>
	{
		Db_Impl(T t, Ts... ts) : tTable(std::move(t)), Db_Impl<Ts...>(ts...) {}

		T tTable;

		template<class U> requires std::same_as<std::decay_t<U>,typename T::PrimaryType>
		const T& GetTable() const
		{
			return tTable;
		}

		template<class U> requires !std::same_as<std::decay_t<U>,typename T::PrimaryType>
		const auto& GetTable() const
		{
			return Db_Impl<Ts...>::template GetTable<U>();
		}

		template <class F>
		void ForEachTable(F&& f) const
		{
			f(tTable);
			Db_Impl<Ts...>::ForEachTable(f);
		}

		template <class F>
		void ForEachTable(F&& f)
		{
			f(tTable);
			Db_Impl<Ts...>::ForEachTable(f);
		}
	};

	template<class... Ts>
	struct Database : public Db_Impl<Ts...>
	{
		Database(std::string sDir, Ts&&... tables) noexcept(false); //connection can throw on instantiation

		template<class T> auto Insert(const T& t) const;
		template<class T> auto Get(primary_key_t k) const;
		template<class T> void Update(const T& t) const;
		template<class T, class... Us> std::vector<T> GetAll(Us&&... Args) const;
		template<class T, class... Us> void UpdateAll(Us&&... args) const;
		template<class T> void Remove(const T& t) const;
		template<class T, class... Us> void RemoveAll(Us&&... args) const;
		
		template<class T> std::string GetTableName() const { return Db_Impl<Ts...>::template GetTable<T>().sName; }
		template<class T, class U> std::string GetColumnName(U T::* p) const;

		void UpdateSchema() const;
		template<class T> std::vector<TableInfo> GetTableInfo() const;

		template<class U, class... Us>
		std::optional<U> Execute(const PreparedStatement<GetStatement<U, Us...>>& stmt) const
		{
			switch (sqlite3_step(stmt.pStmt))
			{
			case SQLITE_DONE:
				return std::nullopt;
			case SQLITE_ROW:
				return Build<U>(stmt, Db_Impl<Ts...>::template GetTable<U>().tCols);
			default:
				ThrowError(connection.pDb);
			}
		}

		template<class U>
		auto Execute(const PreparedStatement<U>& stmt) const
		{
			switch (sqlite3_step(stmt.pStmt))
			{
			case SQLITE_DONE:
				return false;
			case SQLITE_ROW:
				return true;
			default:
				ThrowError(connection.pDb);
			}
			return false;
		}

		Connection connection;
	};

	template<class... Ts>
	Database<Ts...>::Database(std::string sDir, Ts&&... tables) noexcept(false) : Db_Impl<Ts...>(std::forward<Ts>(tables)...),
		connection(std::move(sDir))
	{
		
	}

	template<class... Ts>
	template<class T, class U>
	[[nodiscard]] std::string Database<Ts...>::GetColumnName(U T::* p) const
	{
		std::string ret; 
		auto& table = Db_Impl<Ts...>::template GetTable<T>();
		TupleUtils::for_each_tuple(table.tCols, [&ret, &p](auto& col) {
			using ColType = std::decay_t<decltype(col)>;
			if constexpr (std::is_same_v<typename ColType::FieldType, U> &&
				!is_foreign_key<ColType>::value)
			{
				if (col.pMember == p)
				{
					ret = col.sName;
				}
			}
		});
		return ret;
		//throw if can't find?
	}

	template<class... Ts>
	template<class T>
	[[nodiscard]] auto Database<Ts...>::Insert(const T& t) const
	{
		auto table = Db_Impl<Ts...>::template GetTable<T>();
		auto tpl = TupleUtils::BuildFromOther([&](const auto& item)->auto
			requires !is_foreign_key<std::decay_t<decltype(item)>>::value &&
			!is_primary_key<std::decay_t<decltype(item)>>::value
			{
				return C(item.pMember) == t.*item.pMember;
			}, table.tCols);

		auto ins = BrilliantDB::Insert<T>(tpl);
		auto stmt = Prepare(BrilliantDB::Insert<T>(tpl), *this);
		while (Execute(stmt));
		stmt.Finalize(connection);
		return primary_key_t{ sqlite3_last_insert_rowid(connection.pDb) };
	}

	template<class... Ts>
	template<class T>
	[[nodiscard]] auto Database<Ts...>::Get(primary_key_t k) const
	{
		auto table = Db_Impl<Ts...>::template GetTable<T>();
		auto col = table.template GetColumn<primary_key_t>();
		auto stmt = Prepare(Select<T>(Where(C(col.pMember) == k)), *this);
		auto obj = Execute(stmt);
		stmt.Finalize(connection);
		return obj;
	}

	template<class... Ts>
	template<class T>
	void Database<Ts...>::Update(const T& t) const
	{
		auto& table = Db_Impl<Ts...>::template GetTable<T>();

		auto tpl = TupleUtils::BuildFromOther([&](const auto& item)->auto
			requires !is_foreign_key<std::decay_t<decltype(item)>>::value &&
				!is_primary_key<std::decay_t<decltype(item)>>::value
			{
				return C(item.pMember) == t.*item.pMember;
			}, table.tCols);
		
		auto col = table.template GetColumn<primary_key_t>();
		UpdateAll<T>(SetStatement{ tpl }, Where(C(col.pMember) == t.*col.pMember));
	}

	template<class... Ts>
	template<class T, class... Us>
	[[nodiscard]] std::vector<T> Database<Ts...>::GetAll(Us&&... args) const
	{
		std::vector<T> vRet;
		auto table = Db_Impl<Ts...>::template GetTable<T>();
		auto col = table.template GetColumn<primary_key_t>();
		auto stmt = Prepare(Select<T>(std::forward<Us>(args)...), *this);
		while (auto obj = Execute(stmt))
		{
			vRet.push_back(*obj);
		}
		stmt.Finalize(connection);
		return vRet;
	}

	template<class... Ts>
	template<class T, class... Us>
	void Database<Ts...>::UpdateAll(Us&&... args) const
	{
		auto stmt = Prepare(BrilliantDB::Update<T>(std::forward<Us>(args)...), *this);
		while (Execute(stmt));
		stmt.Finalize(connection);
	}

	template<class... Ts>
	template<class T>
	void Database<Ts...>::Remove(const T& t) const
	{
		auto& table = Db_Impl<Ts...>::template GetTable<T>();
		auto col = table.template GetColumn<primary_key_t>();
		RemoveAll<T>(Where(C(col.pMember) == t.*col.pMember));
	}

	template<class... Ts>
	template<class T, class... Us>
	void Database<Ts...>::RemoveAll(Us&&... args) const
	{
		auto stmt = Prepare(Delete<T>(std::forward<Us>(args)...), *this);
		while (Execute(stmt));
		stmt.Finalize(connection);
	}

	template<class... Ts>
	void Database<Ts...>::UpdateSchema() const
	{
		Db_Impl<Ts...>::ForEachTable([&](auto& table) {
			auto vInfo = GetTableInfo<typename std::decay_t<decltype(table)>::PrimaryType>();
			auto vTableInfo = table.GetTableInfo();
			std::vector<TableInfo> vDiff;
			std::set_difference(vTableInfo.cbegin(), vTableInfo.cend(), vInfo.cbegin(), vInfo.cend(), std::back_inserter(vDiff));

			for (auto& info : vDiff)
			{
				std::string sql = "ALTER TABLE " + table.sName + " ADD COLUMN " + info.sColName + " " + info.sColType;
				if (info.iPk) { sql += " PRIMARY KEY"; }
				if (info.bNotNull) { sql += " NOT NULL"; }
				if (info.sDefaultVal.length()) { sql += " DEFAULT " + info.sDefaultVal; }
				if (sqlite3_exec(connection.pDb, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK)
				{
					ThrowError(connection.pDb);
				}
			}
			});
	}

	template<class... Ts>
	template<class T>
	std::vector<TableInfo> Database<Ts...>::GetTableInfo() const
	{
		auto table = Db_Impl<Ts...>::template GetTable<T>();
		std::vector<TableInfo> vRet;
		std::string sql = "PRAGMA table_info('" + table.sName + "');";
		if (sqlite3_exec(connection.pDb,sql.c_str(), 
			[](void* data, int argc, char** argv, char**) -> int {
				auto& ret = *static_cast<std::vector<TableInfo>*>(data);
				int i = 0;
				while (i < argc)
				{
					TableInfo info;
					info.iColId = std::atoi(argv[i++]);
					info.sColName = argv[i++];
					info.sColType = argv[i++];
					info.bNotNull = !!std::atoi(argv[i++]);
					info.sDefaultVal = argv[i] ? argv[i] : ""; i++;
					info.iPk = std::atoi(argv[i++]);
					ret.push_back(info);
				}
				return 0;
			}, &vRet, nullptr) != SQLITE_OK)
		{
			ThrowError(connection.pDb);
		}
		return vRet;
	}

	//Maker functions
	template<class... Ts>
	[[nodiscard]] Database<Ts...> MakeDatabase(std::string dir, Ts&&... tables)
	{
		Database<Ts...> db{ std::move(dir), std::forward<Ts>(tables)... };
		db.ForEachTable([&db](auto& table) {
			if (sqlite3_exec(db.connection.pDb, Print(table, db).c_str(), NULL, NULL, NULL) != SQLITE_OK)
			{
				ThrowError(db.connection.pDb);
			}
			});
		db.UpdateSchema();
		return db;
	}

	template<class P, class... Cs>
	[[nodiscard]] Table<P, Cs...> MakeTable(std::string name, Cs... cols)
	{
		return { std::move(name), std::make_tuple(cols...) };
	}

	template <class O, class F, class... Cs>
	[[nodiscard]] Column<O, F, Cs...> MakeColumn(std::string name, F O::* ptr, Cs... args)
	{
		return Column<O, F, Cs...>{name, ptr, args... };
	}

	template <class O1, ForeignKeyType F1, class O2, PrimaryKeyType F2>
	[[nodiscard]] ForeignKey<O1, F1, O2, F2> MakeForeignKey(F1 O1::* ptr, F2 O2::* ref)
	{
		return { "", ptr, ref };
	}
}