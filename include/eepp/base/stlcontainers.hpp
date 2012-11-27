#ifndef EE_STLCONTAINERS_HPP
#define EE_STLCONTAINERS_HPP

#include <eepp/base/memorymanager.hpp>
#include <stack>
#include <set>
#include <deque>

namespace EE {
	template <typename T, typename A = eeAllocator<T> >
	struct eeStack {
		#ifdef EE_MEMORY_MANAGER
		typedef typename std::stack<T, A> type;
		#else
		typedef typename std::stack<T> type;
		#endif
	};

	template <typename T, typename A = eeAllocator<T> >
	struct eeDeque {
		#ifdef EE_MEMORY_MANAGER
		typedef typename std::deque<T, A> type;
		#else
		typedef typename std::deque<T> type;
		#endif
	};

	template <typename T, typename A = eeAllocator<T> >
	struct eeVector {
		#ifdef EE_MEMORY_MANAGER
		typedef typename std::vector<T, A> type;
		#else
		typedef typename std::vector<T> type;
		#endif
	};

	template <typename T, typename A = eeAllocator<T> >
	struct eeList {
		#ifdef EE_MEMORY_MANAGER
		typedef typename std::list<T, A> type;
		#else
		typedef typename std::list<T> type;
		#endif
	};

	template <typename T, typename P = std::less<T>, typename A = eeAllocator<T> >
	struct eeSet {
		#ifdef EE_MEMORY_MANAGER
		typedef typename std::set<T, P, A> type;
		#else
		typedef typename std::set<T, P> type;
		#endif
	};

	template <typename K, typename V, typename P = std::less<K>, typename A = eeAllocator< std::pair<const K, V> > >
	struct eeMap {
		#ifdef EE_MEMORY_MANAGER
		typedef typename std::map<K, V, P, A> type;
		#else
		typedef typename std::map<K, V, P> type;
		#endif
	};

	template <typename K, typename V, typename P = std::less<K>, typename A = eeAllocator< std::pair<const K, V> > >
	struct eeMultimap {
		#ifdef EE_MEMORY_MANAGER
		typedef typename std::multimap<K, V, P, A> type;
		#else
		typedef typename std::multimap<K, V, P> type;
		#endif
	};
}

#endif
