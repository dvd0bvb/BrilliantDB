#pragma once

#include <string>
#include "detail/KeyTypes.h"

namespace BrilliantDB
{
	template<class T>
	struct TypePrinter
	{ };

	struct IntPrinter { static std::string Print() { return "INTEGER"; } };
	struct TextPrinter { static std::string Print() { return "TEXT"; } };
	struct RealPrinter { static std::string Print() { return "REAL"; } };
	struct BlobPrinter { static std::string Print() { return "BLOB"; } };

	template<class T, Key K>
	struct TypePrinter<key_t<T, K>> : public TypePrinter<T> {};

	template<>
	struct TypePrinter<bool> : public IntPrinter {};

	template<>
	struct TypePrinter<unsigned char> : public IntPrinter {};

	template<>
	struct TypePrinter<signed char> : public IntPrinter {};

	template<>
	struct TypePrinter<char> : public IntPrinter {};

	template<>
	struct TypePrinter<unsigned short> : public IntPrinter {};

	template<>
	struct TypePrinter<short> : public IntPrinter {};

	template<>
	struct TypePrinter<unsigned int> : public IntPrinter {};

	template<>
	struct TypePrinter<int> : public IntPrinter {};

	template<>
	struct TypePrinter<unsigned long> : public IntPrinter {};

	template<>
	struct TypePrinter<long> : public IntPrinter {};

	template<>
	struct TypePrinter<unsigned long long> : public IntPrinter {};

	template<>
	struct TypePrinter<long long> : public IntPrinter {};

	template<>
	struct TypePrinter<std::string> : public TextPrinter {};

	template<>
	struct TypePrinter<std::wstring> : public TextPrinter {};

	template<>
	struct TypePrinter<const char*> : public TextPrinter {};

	template<>
	struct TypePrinter<float> : public RealPrinter {};

	template<>
	struct TypePrinter<double> : public RealPrinter {};

	template<>
	struct TypePrinter<std::vector<char>> : public BlobPrinter {};
}