#ifndef EE_CONTAINERS_HPP
#define EE_CONTAINERS_HPP

#ifdef EEPP_NO_THIRDPARTY_CONTAINERS
#include <unordered_map>
#include <unordered_set>
#else
#include <eepp/thirdparty/robin_hood.h>
#endif

namespace EE {

#ifdef EEPP_NO_THIRDPARTY_CONTAINERS

template <typename Key, typename Value> using UnorderedMap = std::ununordered_map<Key, Value>;

template <typename Key, typename Value> using UnorderedSet = std::unordered_set<Key, Value>;

#else

template <typename Key, typename Value>
using UnorderedMap = robin_hood::unordered_flat_map<Key, Value>;

template <typename Key, typename Value>
using UnorderedSet = robin_hood::unordered_flat_set<Key, Value>;

#endif

} // namespace EE

#endif // EE_CONTAINERS_HPP
