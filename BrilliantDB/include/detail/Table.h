#pragma once

#include <compare>
#include <string>
#include <sstream>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include "detail/TupleUtils.h"
#include "detail/TypePrinter.h"
#include "detail/Column.h"

namespace BrilliantDB
{
	struct TableInfo
	{
		int iColId = 0;
		std::string sColName;
		std::string sColType;
		bool bNotNull = true;
		std::string sDefaultVal;
		int iPk = 0;
	};

	bool operator< (const TableInfo& lh, const TableInfo& rh)
	{
		return lh.iColId < rh.iColId;
	}

	template<class P, class... Cs>
		struct Table
	{
		Table(std::string name, std::tuple<Cs...> t) : iNumForeignKeys(0), sName(std::move(name)), tCols(std::move(t))
		{
			TupleUtils::for_each_tuple(tCols, [&](auto& col) {
				if (is_foreign_key<std::decay_t<decltype(col)>>::value) { iNumForeignKeys++; }
				});
		}

		template<class C> 
		auto GetColumn() const
		{
			auto t = TupleUtils::BuildFromOther([](auto& item)->auto requires std::is_same_v<C, typename std::decay_t<decltype(item)>::FieldType> {
				return item;
				}, tCols);
			return std::get<0>(t);
		}

		std::vector<TableInfo> GetTableInfo() const
		{
			int iCol = 0;
			std::vector<TableInfo> v;
			TupleUtils::for_each_tuple(tCols, [&v, &iCol](auto& col) {
				if constexpr (!is_foreign_key<std::decay_t<decltype(col)>>::value)
				{
					TableInfo info;
					info.iColId = iCol++;
					info.sColName = col.sName;
					info.sColType = TypePrinter<typename std::decay_t<decltype(col)>::FieldType>::Print();
					info.bNotNull = std::find(col.vConstraints.cbegin(), col.vConstraints.cend(), Constraint::not_null) != col.vConstraints.cend();
					info.iPk = std::find(col.vConstraints.cbegin(), col.vConstraints.cend(), Constraint::primary_key) != col.vConstraints.cend();
					//info.sDefaultVal = don't have default value support at this time
					v.push_back(info);
				}
				});
			return v;
		}

		std::size_t iNumForeignKeys;
		std::string sName;
		std::tuple<Cs...> tCols;

		using PrimaryType = P;
	};
}