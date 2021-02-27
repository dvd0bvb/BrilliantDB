#pragma once

#include <concepts>

namespace BrilliantDB
{
	template<class T>
	concept RelativelySmallInt = std::integral<T> && sizeof(T) <= sizeof(int);

	template<class T>
	concept RelativelyLargeInt = std::integral<T> && sizeof(T) > sizeof(int);
}