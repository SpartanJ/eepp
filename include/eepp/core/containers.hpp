#ifndef EE_CONTAINERS_HPP
#define EE_CONTAINERS_HPP

#ifdef EE_DEBUG
#define EEPP_NO_THIRDPARTY_CONTAINERS
#endif

#ifdef EEPP_NO_THIRDPARTY_CONTAINERS
#include <unordered_map>
#include <unordered_set>
#else
#include <eepp/thirdparty/robin_hood.h>
#endif

namespace EE {

#ifdef EEPP_NO_THIRDPARTY_CONTAINERS

template <typename Key, typename Value> using UnorderedMap = std::unordered_map<Key, Value>;

template <typename Key> using UnorderedSet = std::unordered_set<Key>;

#else

template <typename Key, typename Value>
using UnorderedMap = robin_hood::unordered_flat_map<Key, Value>;

template <typename Key> using UnorderedSet = robin_hood::unordered_flat_set<Key>;

#endif

} // namespace EE

template <typename... T>
inline std::size_t hashCombine( std::size_t h1, std::size_t h2, T... other ) noexcept {
	if constexpr ( sizeof...( other ) == 0 ) {
		h1 ^= h2 + 0x9e3779b9 + ( h1 << 6 ) + ( h1 >> 2 );
		return h1;
	} else {
		return hashCombine( h1, hashCombine( h2, other... ) );
	}
}

#endif // EE_CONTAINERS_HPP
