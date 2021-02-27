#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <typeindex>
#include "detail/KeyTypes.h"

namespace BrilliantDB
{
	enum class Constraint
	{
		primary_key,
		auto_increment,
		not_null
	};

	inline std::ostream& operator<< (std::ostream& os, Constraint c)
	{
		switch (c)
		{
		case Constraint::primary_key:
			os << "PRIMARY KEY";
			break;
		case Constraint::auto_increment:
			os << "AUTOINCREMENT";
			break;
		case Constraint::not_null:
			os << "NOT NULL";
			break;
		}
		return os;
	}

	template<class O, class F, class... Cs>
	struct Column
	{
		Column(std::string name, F O::* ptr, Cs... constraints) :
			sName(std::move(name)),
			pMember(ptr),
			vConstraints{ constraints... }
		{
		}
		virtual ~Column() = default;

		const std::string sName;
		F O::* pMember;
		std::vector<Constraint> vConstraints;

		using FieldType = F;
	};

	template<class O1, ForeignKeyType F1, class O2, PrimaryKeyType F2, class... Cs>
	struct ForeignKey : public Column<O1, F1, Cs...>
	{
		ForeignKey(std::string name, F1 O1::* p1, F2 O2::* p2, Cs... constraints) :
			Column<O1, F1, Cs...>(name, p1, constraints...),
			pReference(p2) {}

		F2 O2::* pReference;
	};

	template<class T>
	struct is_foreign_key : std::false_type {};

	template<class O1, ForeignKeyType F1, class O2, PrimaryKeyType F2, class... Cs>
	struct is_foreign_key<ForeignKey<O1, F1, O2, F2, Cs...>> : std::true_type {};

	template<class>
	struct is_primary_key : std::false_type {};

	template<class O, PrimaryKeyType F, class... Cs>
	struct is_primary_key<Column<O, F, Cs...>> : std::true_type {};
}