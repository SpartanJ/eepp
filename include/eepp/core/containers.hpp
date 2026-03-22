#ifndef EE_CONTAINERS_HPP
#define EE_CONTAINERS_HPP

#include <eepp/config.hpp>

#if defined( EEPP_NO_THIRDPARTY_CONTAINERS ) || ( defined( EE_DEBUG ) && defined( EE_COMPILER_MSVC ) )
#include <unordered_map>
#include <unordered_set>
#else
#include <eepp/core/small_vector.hpp>
#include <eepp/thirdparty/unordered_dense.h>
#endif

namespace EE {

#if defined( EEPP_NO_THIRDPARTY_CONTAINERS ) || ( defined( EE_DEBUG ) && defined( EE_COMPILER_MSVC ) )

template <typename Key, typename Value> using UnorderedMap = std::unordered_map<Key, Value>;

template <typename Key> using UnorderedSet = std::unordered_set<Key>;

// Fallbacks for the stack-allocated versions when in debug mode
template <typename Key, typename Value, size_t MinInlineCapacity = 16>
using SmallUnorderedMap = std::unordered_map<Key, Value>;

template <typename Key, size_t MinInlineCapacity = 16>
using SmallUnorderedSet = std::unordered_set<Key>;

#else

template <typename Key, typename Value>
using UnorderedMap = ankerl::unordered_dense::map<Key, Value>;

template <typename Key> using UnorderedSet = ankerl::unordered_dense::set<Key>;

// -------------------------------------------------------------------------
// Stack-allocated Hash Maps for small datasets
// -------------------------------------------------------------------------
// By passing EE::SmallVector as the 5th template argument, we tell the map
// to use our SBO (Small Buffer Optimization) storage instead of std::vector.
template <typename Key, typename Value, size_t MinInlineCapacity = 16>
using SmallUnorderedMap =
	ankerl::unordered_dense::map<Key, Value, ankerl::unordered_dense::hash<Key>, std::equal_to<Key>,
								 EE::SmallVector<std::pair<Key, Value>, MinInlineCapacity>>;

template <typename Key, size_t MinInlineCapacity = 16>
using SmallUnorderedSet =
	ankerl::unordered_dense::set<Key, ankerl::unordered_dense::hash<Key>, std::equal_to<Key>,
								 EE::SmallVector<Key, MinInlineCapacity>>;

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
