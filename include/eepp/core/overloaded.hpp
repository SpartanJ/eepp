#pragma once

namespace EE {

// helper class for std::visit
// see: https://en.cppreference.com/w/cpp/utility/variant/visit
template <typename... Ts> struct Overloaded : Ts... {
	using Ts::operator()...;
};

template <typename... Ts> Overloaded( Ts... ) -> Overloaded<Ts...>;

}
