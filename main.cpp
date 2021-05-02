#include <cstddef>
#include <type_traits>
#include <iostream>
#include <utility>

struct base
{
	int a;
	int v;
};

struct derrived : base
{
	int first;
	int second;
};


/////////////////////////////////////////////
template<typename T>
struct always_bool
{
	using type = bool;
};

/////////////////////////////////////////////
template <class T>
constexpr T unsafe_declval() noexcept {
	typename std::remove_reference<T>::type* ptr = 0;
	ptr += 42; // suppresses 'null pointer dereference' warnings
	return static_cast<T>(*ptr);
}

///////////////////// Structure that can be converted to reference to anything
struct ubiq_lref_constructor {
	std::size_t ignore;
	template <class Type> constexpr operator Type&() const && noexcept {  // tweak for template_unconstrained.cpp like cases
		return unsafe_declval<Type&>();
	};

	template <class Type> constexpr operator Type&() const & noexcept {  // tweak for optional_chrono.cpp like cases
		return unsafe_declval<Type&>();
	};
};

///////////////////// Structure that can be converted to rvalue reference to anything
struct ubiq_rref_constructor {
	std::size_t ignore;
	template <class Type> /*constexpr*/ operator Type() const && noexcept {  // Allows initialization of rvalue reference fields and move-only types
		return unsafe_declval<Type>();
	};
};



template <class T>
struct tag {
	friend constexpr bool loophole(tag<T>);
};

template <class T, bool value>
struct loophole_t {
	friend constexpr bool loophole(tag<T>) { return value; };
};

template <class Derived, class U>
constexpr bool static_assert_non_inherited() noexcept {
	sizeof(loophole_t<Derived, std::is_base_of<U, Derived>::value>);
	return true;
}

template <class Derived>
struct ubiq_lref_base_asserting {
	template <class Type> constexpr operator Type&() const &&  // tweak for template_unconstrained.cpp like cases
		noexcept(static_assert_non_inherited<Derived, Type>())  // force the computation of assert function
	{
		return unsafe_declval<Type&>();
	};

	template <class Type> constexpr operator Type&() const &  // tweak for optional_chrono.cpp like cases
		noexcept(static_assert_non_inherited<Derived, Type>())  // force the computation of assert function
	{
		return unsafe_declval<Type&>();
	};
};

template <class Derived>
struct ubiq_rref_base_asserting {
	template <class Type> /*constexpr*/ operator Type() const &&  // Allows initialization of rvalue reference fields and move-only types
		noexcept(static_assert_non_inherited<Derived, Type>())  // force the computation of assert function
	{
		return unsafe_declval<Type>();
	};
};

template <class T, std::size_t I0, std::size_t... I, class /*Enable*/ = typename std::enable_if<std::is_copy_constructible<T>::value>::type>
constexpr auto is_first_not_base(std::index_sequence<I0, I...>) noexcept
-> typename always_bool<decltype(T{ ubiq_lref_base_asserting<T>{}, ubiq_lref_constructor{I}... }) > ::type
{
	return loophole(tag<T>{});
}

template <class T, std::size_t I0, std::size_t... I, class /*Enable*/ = typename std::enable_if<!std::is_copy_constructible<T>::value>::type>
constexpr auto is_first_not_base(std::index_sequence<I0, I...>) noexcept
-> typename always_bool<decltype(T{ ubiq_rref_base_asserting<T>{}, ubiq_rref_constructor{I}... }) > ::type
{
	return loophole(tag<T>{});
}

template <class T>
constexpr bool is_first_not_base(std::index_sequence<>) noexcept
{
	return false;
}




int main() {
  // TODO: detect count of fields by boost.pfr
	bool status = is_first_not_base<base>(std::make_index_sequence<2>{});
	std::cout << std::boolalpha << status << std::endl;
	std::cout << "done" << std::endl;
	return 0;
}
