#ifndef EE_STLCONTAINERS_HPP
#define EE_STLCONTAINERS_HPP

namespace EE {

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

	typedef eeVector<Uint8>::type 		eeVectorUint8;
	typedef eeVector<Uint16>::type 	eeVectorUint16;
	typedef eeVector<Uint32>::type 	eeVectorUint32;
	typedef eeVector<Int8>::type 		eeVectorInt8;
	typedef eeVector<Int16>::type 		eeVectorInt16;
	typedef eeVector<Int32>::type 		eeVectorInt32;
	typedef eeVector<eeFloat>::type 	eeVectorFloat;
}

#endif
