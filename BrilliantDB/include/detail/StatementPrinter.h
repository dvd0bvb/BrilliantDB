#pragma once

#include <string>
#include <sstream>
#include <type_traits>

#include "detail/Statement.h"
#include "detail/TypePrinter.h"
#include "detail/TupleUtils.h"
#include "detail/Table.h"
#include "detail/Column.h"

namespace BrilliantDB
{
	template<class T>
	struct StatementPrinter;

	template<class O, class F, class... Cs>
	struct StatementPrinter<Column<O, F, Cs...>>
	{
		using statement_type = Column<O, F, Cs...>;

		template<class C>
		std::string operator() (const statement_type& statement, const C& context)
		{
			std::stringstream ss;
			ss << "'" << statement.sName << "' " << TypePrinter<F>::Print() << ' ';
			for (const auto& c : statement.vConstraints)
			{
				ss << c << ' ';
			}
			return ss.str();
		}
	};

	template<class O1,class F1, class O2, class F2>
	struct StatementPrinter<ForeignKey<O1, F1, O2, F2>>
	{
		using statement_type = ForeignKey<O1, F1, O2, F2>;

		template<class C>
		std::string operator() (const statement_type& statement, const C& context)
		{
			std::stringstream ss;
			ss << "FOREIGN KEY ('" << context.GetColumnName(statement.pMember)<< "') "
				<< "REFERENCES '" << context.template GetTableName<O2>() 
				<< "' ('" << context.GetColumnName(statement.pReference) << "')";
			return ss.str();
		}
	};

	template<class P, class... Cs>
	struct StatementPrinter<Table<P, Cs...>>
	{
		using statement_type = Table<P, Cs...>;

		template<class C>
		std::string operator() (const statement_type& statement, const C& context)
		{
			std::stringstream ss;
			ss << "CREATE TABLE IF NOT EXISTS '" << statement.sName << "' ( ";
			std::size_t i = 0;
			TupleUtils::for_each_tuple(statement.tCols, [&](auto& col) {
				ss << Print(col, context);
				if (i < (std::tuple_size<std::tuple<Cs...>>::value - 1))
				{
					ss << ", ";
				}
				i++;
				});
			ss << ");";
			return ss.str();
		}
	};

	template<class T, class... Ts>
	struct StatementPrinter<InsertStatement<T, Ts...>>
	{
		using statement_type = InsertStatement<T, Ts...>;

		template<class C>
		std::string operator() (const statement_type& statement, const C& context)
		{
			auto& table = context.template GetTable<T>();
			std::string sInsert = "INSERT INTO '" + table.sName + "' (";
			std::string sValues = "Values (";
			std::size_t i = 0;
			TupleUtils::for_each_tuple(table.tCols, [&](auto& col) {
				if (!is_foreign_key<std::decay_t<decltype(col)>>::value && 
					std::find(col.vConstraints.cbegin(), col.vConstraints.cend(), Constraint::primary_key) == col.vConstraints.cend())
				{
					sInsert += "\"" + col.sName + "\"";
					sValues += "?";
					if (i < std::tuple_size<decltype(table.tCols)>::value - table.iNumForeignKeys - 1)
					{
						sInsert += ", ";
						sValues += ", ";
					}
				}
				i++;
				});
			sInsert += ") ";
			sValues += ')';
			return sInsert + sValues;
		}
	};

	template<class T, class... Ts>
	struct StatementPrinter<GetStatement<T, Ts...>>
	{
		using statement_type = GetStatement<T, Ts...>;

		template<class C>
		std::string operator() (const statement_type& statement, const C& context)
		{
			auto& table = context.template GetTable<T>();
			std::stringstream ss;

			ss << "SELECT * FROM '" << table.sName << '\'';
			TupleUtils::for_each_tuple(statement.tItems, [&](auto& item) {
				ss << Print(item, context);
				});
			return ss.str();
		}
	};

	template<class T, class... Ts>
	struct StatementPrinter<UpdateStatement<T, Ts...>>
	{
		using statement_type = UpdateStatement<T, Ts...>;

		template<class C>
		std::string operator() (const statement_type& statement, const C& context)
		{
			auto& table = context.template GetTable<T>();
			std::stringstream ss;

			ss << "UPDATE '" << table.sName << '\'';
			if (std::tuple_size_v<std::tuple<Ts...>>)
			{
				TupleUtils::for_each_tuple(statement.tItems, [&](auto& item) {
					ss << Print(item, context);
					});
			}
			return ss.str();
		}
	};

	template<class... Ts>
	struct StatementPrinter<WhereStatement<Ts...>>
	{
		using statement_type = WhereStatement<Ts...>;

		template<class C>
		std::string operator() (const statement_type& statement, const C& context)
		{
			std::stringstream ss;
			if (std::tuple_size_v<std::tuple<Ts...>>)
			{
				ss << " WHERE ";
				TupleUtils::for_each_tuple(statement.tItems, [&](auto& item) {
					ss << Print(item, context);
					});
			}
			return ss.str();
		}
	};

	template<class... Ts>
	struct StatementPrinter<SetStatement<Ts...>>
	{
		using statement_type = SetStatement<Ts...>;

		template<class C>
		std::string operator() (const statement_type& statement, const C& context)
		{
			std::stringstream ss;
			if (std::tuple_size_v<std::tuple<Ts...>>)
			{
				ss << " SET ";
				std::size_t i = 0;
				TupleUtils::for_each_tuple(statement.tItems, [&](auto& item) {
					ss << Print(item, context);
					if (i < std::tuple_size_v<std::tuple<Ts...>> - 1)
					{
						ss << ", ";
					}
					++i;
					});
			}
			return ss.str();
		}
	};

	template<class T, class... Ts>
	struct StatementPrinter <DeleteStatement<T, Ts...>>
	{
		using statement_type = DeleteStatement<T, Ts...>;

		template<class C>
		std::string operator() (const statement_type& statement, const C& context)
		{
			auto& table = context.template GetTable<T>();
			std::stringstream ss;
			ss << "DELETE FROM " << table.sName;
			TupleUtils::for_each_tuple(statement.tItems, [&ss,&context](auto& item) {
				ss << Print(item, context);
				});
			ss << ";";
			return ss.str();
		}
	};

	template<class T, class U>
	struct StatementPrinter<C<T, U>>
	{
		using statement_type = C<T, U>;

		template<class C>
		std::string operator() (const statement_type& statement, const C& context)
		{
			std::stringstream ss;
			ss << (statement.bNot ? " NOT " : "") << context.GetColumnName(statement.pMember) << statement.comp << "?";
			return ss.str();
		}
	};

	template<class T, class U>
	struct StatementPrinter<LogicalC<T, U, logical_c::_and>>
	{
		using statement_type = LogicalC<T, U, logical_c::_and>;

		template<class C>
		std::string operator() (const statement_type& statement, const C& context)
		{
			std::stringstream ss;
			ss << Print(std::get<0>(statement.t), context) << " AND " << Print(std::get<1>(statement.t), context);
			return ss.str();
		}
	};

	template<class T, class U>
	struct StatementPrinter<LogicalC<T, U, logical_c::_or>>
	{
		using statement_type = LogicalC<T, U, logical_c::_or>;

		template<class C>
		std::string operator() (const statement_type& statement, const C& context)
		{
			std::stringstream ss;
			ss << Print(std::get<0>(statement.t), context) << " OR " << Print(std::get<1>(statement.t), context);
			return ss.str();
		}
	};

	template<class T, class C>
	std::string Print(const T& statement, const C& context)
	{
		return StatementPrinter<T>()(statement, context);
	}
}