#pragma once

#include <concepts>
#include <string>
#include <type_traits>
#include <sqlite3.h>

namespace BrilliantDB
{
	enum class Key
	{
		primary,
		foreign
	};

	template<class T, Key K>
	struct key_t
	{
		key_t() requires std::signed_integral<T> : _t(-1) {}
		key_t(T t) : _t(std::move(t)) {}

		template<class U, Key L> requires std::same_as<T, std::decay_t<U>>
		key_t(const key_t<U, L>& other)
		{
			_t = other._t;
		}

		//implicit conversion for convenience
		operator T() { return _t; }

		T _t;

		using type = T;
	};

	template<class T, Key K>
	std::ostream& operator<< (std::ostream& os, const key_t<T, K>& k)
	{
		return os << k._t;
	}

	using primary_key_t = key_t<sqlite3_int64, Key::primary>;
	using foreign_key_t = key_t<sqlite3_int64, Key::foreign>;

	//concepts used to enforce key types in template instantiations see ForeignKey
	template<typename T>
	concept PrimaryKeyType = std::is_same_v<primary_key_t, std::decay_t<T>>;

	template<typename T>
	concept ForeignKeyType = std::is_same_v<foreign_key_t, std::decay_t<T>>;
}

namespace std
{
	template<class T, BrilliantDB::Key K>
	string to_string(BrilliantDB::key_t<T, K> k)
	{
		return to_string(k._t);
	}
}