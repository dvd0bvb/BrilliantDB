#pragma once

#include <concepts>
#include <tuple>

namespace TupleUtils
{
	template<class L, class... Ts>
	void for_each_tuple(const std::tuple<Ts...>& t, L&& l)
	{
		std::apply([&l](auto&... item) { (..., l(item)); }, t);
	}

	template<class L, class T>
	concept BuilderCallable = requires (L l, T t) { l(t); };

	template<class L>
	class TupleBuilder
	{
	public:
		TupleBuilder(const L& l) : _l(l) {}

		template<class... Ts>
		constexpr auto operator() (Ts&&... args) const
		{
			std::tuple<> t;
			return Build(t, std::forward<Ts>(args)...);
		}

	private:
		constexpr auto Build(const std::tuple<>& tpl) const
		{
			return tpl;
		}

		template<class... Us, class T, class... Ts>
		constexpr auto Build(const std::tuple<Us...>& tpl, T&& t, Ts&&... args) const requires BuilderCallable<L, T>
		{
			auto _tpl = std::tuple_cat(tpl, std::make_tuple(_l(std::forward<T>(t))));
			return Build(_tpl, std::forward<Ts>(args)...);
		}

		template<class... Us, class T, class... Ts>
		constexpr auto Build(const std::tuple<Us...>& tpl, T&& t, Ts&&... args) const requires !BuilderCallable<L, T>
		{
			return Build(tpl, std::forward<Ts>(args)...);
		}

		template<class... Us, class T>
		constexpr auto Build(const std::tuple<Us...>& tpl, T&& t) const requires BuilderCallable<L, T>
		{
			return std::tuple_cat(tpl, std::make_tuple(_l(std::forward<T>(t))));
		}

		template<class... Us, class T>
		constexpr auto Build(const std::tuple<Us...>& tpl, const T& t) const requires !BuilderCallable<L, T>
		{
			return tpl;
		}

		L _l;
	};

	template<class L, class... Ts>
	auto BuildFromOther(const L& l, const std::tuple<Ts...>& t)
	{
		return std::apply(TupleBuilder(l), t);
	}

	template<class T>
	struct is_tuple : std::false_type {};

	template<class... Ts>
	struct is_tuple<std::tuple<Ts...>> : std::true_type {};

	template<class T>
	concept TupleType = is_tuple<std::decay_t<T>>::value;

	template<class T>
	auto constexpr Flatten(T&& t) 
	{
		return std::make_tuple(std::forward<T>(t));
	}

	template<TupleType T> auto constexpr Flatten(T&& t);

	template<TupleType T, std::size_t... Is>
	auto constexpr Flatten(T&& t, std::index_sequence<Is...>)
	{
		return std::tuple_cat(Flatten(std::get<Is>(std::forward<T>(t)))...);
	}

	template<TupleType T>
	auto constexpr Flatten(T&& t)
	{
		return Flatten(std::forward<T>(t), std::make_index_sequence<std::tuple_size_v<std::decay_t<decltype(t)>>>());
	}
}