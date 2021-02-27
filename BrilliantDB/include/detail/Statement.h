#pragma once

#include <concepts>
#include "detail/KeyTypes.h"

namespace BrilliantDB
{
	template<class... Ts>
	struct Statement
	{
		std::tuple<Ts...> tItems;
	};

	template<class T, class... Ts>
	struct InsertStatement : Statement<Ts...>
	{
		using TableType = T;
		InsertStatement(const std::tuple<Ts...>& t) : Statement<Ts...>{ t } {}
	};

	template<class T, class... Ts>
	[[nodiscard]] InsertStatement<T, Ts...> Insert(const std::tuple<Ts...>& t)
	{
		return InsertStatement<T, Ts...>(t);
	}

	template<class T, class... Ts>
	struct GetStatement : Statement<Ts...>
	{
		using TableType = T;
	};

	template<class T, class... Ts>
	[[nodiscard]] GetStatement<T, Ts...> Select(Ts&&... args)
	{
		return { std::forward_as_tuple(args...) };
	}

	template<class T, class... Ts>
	struct UpdateStatement : Statement<Ts...>
	{
		using TableType = T;
	};

	template<class T, class... Ts>
	[[nodiscard]] UpdateStatement<T, Ts...> Update(Ts&&... args)
	{
		return { std::forward_as_tuple(args...) };
	}

	template<class... Ts>
	struct WhereStatement : Statement<Ts...> 
	{
		WhereStatement(const std::tuple<Ts...>& t) : Statement<Ts...>{ t } {}
	};

	template<class... Ts>
	[[nodiscard]] WhereStatement<Ts...> Where(Ts&&... args)
	{
		return { std::forward_as_tuple(args...) };
	}

	template<class... Ts>
	struct SetStatement : Statement<Ts...> 
	{
		SetStatement(const std::tuple<Ts...>& t) : Statement<Ts...>{ t } {}
	};

	template<class... Ts>
	[[nodiscard]] SetStatement<Ts...> Set(Ts&&... args)
	{
		return { std::forward_as_tuple(args...) };
	}

	template<class T, class... Ts>
	struct DeleteStatement : Statement<Ts...>
	{
		using TableType = T;
	};

	template<class T, class... Ts>
	[[nodiscard]] DeleteStatement<T, Ts...> Delete(Ts&&... args)
	{
		return { std::forward_as_tuple(args...) };
	}

	enum class comparator
	{
		equal,
		less,
		less_eq,
		great,
		great_eq,
		not_equal
	};

	std::ostream& operator<< (std::ostream& os, comparator c)
	{
		switch (c)
		{
		case comparator::equal:
			os << "=";
			break;
		case comparator::less:
			os << "<";
			break;
		case comparator::less_eq:
			os << "<=";
			break;
		case comparator::great:
			os << ">";
			break;
		case comparator::great_eq:
			os << ">=";
			break;
		case comparator::not_equal:
			os << "<>";
			break;
		}
		return os;
	}

	template<class T, class U>
	struct C
	{
		using TableType = T;
		using FieldType = U;

		explicit C(U T::* p) : pMember(p) {}

		C& operator== (const U& v)
		{
			comp = comparator::equal;
			value = v;
			return *this;
		}

		C& operator< (const U& v)
		{
			comp = comparator::less;
			value = v;
			return *this;
		}

		C& operator<= (const U& v)
		{
			comp = comparator::less_eq;
			value = v;
			return *this;
		}

		C& operator> (const U& v)
		{
			comp = comparator::great;
			value = v;
			return *this;
		}

		C& operator>= (const U& v)
		{
			comp = comparator::great_eq;
			value = v;
			return *this;
		}

		C& operator!= (const U& v)
		{
			comp = comparator::not_equal;
			value = v;
			return *this;
		}

		C& operator! ()
		{
			bNot = !bNot;
			return *this;
		}

		bool bNot = false;
		comparator comp = comparator::equal;
		U T::* pMember;
		U value;
	};

	template<class T>
	struct is_col_statement : std::false_type {};

	template<class T, class U>
	struct is_col_statement<C<T, U>> : std::true_type {};

	template<class T>
	concept ColStatement = is_col_statement<std::decay_t<T>>::value;

	enum class logical_c
	{
		_and,
		_or
	};

	template<ColStatement T, ColStatement U, logical_c V>
	struct LogicalC
	{
		LogicalC(T c1, U c2) : t(std::make_tuple(std::move(c1), std::move(c2))) {}
		std::tuple<T, U> t;
	};

	template<class T, class U, logical_c V>
	struct is_col_statement<LogicalC<T, U, V>> : std::true_type {};

	template<ColStatement T, ColStatement U>
	LogicalC<T, U, logical_c::_and> operator&& (const T& c1, const U& c2)
	{
		return { c1, c2 };
	}

	template<ColStatement T, ColStatement U>
	LogicalC<T, U, logical_c::_or> operator|| (const T& c1, const U& c2)
	{
		return { c1, c2 };
	}
}