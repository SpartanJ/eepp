#pragma once
#include <eepp/core/containers.hpp>

#ifdef max
#undef max
#endif
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <list>
#include <optional>
#include <type_traits>

namespace EE {

// -----------------------------------------------------------------------------
//  DynamicLRU (general-purpose, list-based)
// -----------------------------------------------------------------------------
template <std::size_t Capacity, typename KeyT = std::uint64_t,
		  typename ValueT = std::array<char, 64>>
class DynamicLRU {
  private:
	static_assert( Capacity > 0, "Capacity must be greater than zero for DynamicLRU" );

	using ListPair = std::pair<KeyT, ValueT>;
	std::list<ListPair> mCacheList;
	UnorderedMap<KeyT, typename std::list<ListPair>::iterator> mCacheMap;

  public:
	DynamicLRU() { mCacheMap.reserve( Capacity ); }

	std::optional<ValueT> get( const KeyT& key ) {
		auto it = mCacheMap.find( key );
		if ( it == mCacheMap.end() )
			return std::nullopt;
		// Move accessed element to the front of the list (most recently used)
		mCacheList.splice( mCacheList.begin(), mCacheList, it->second );
		return it->second->second;
	}

	void put( KeyT key, ValueT value ) {
		auto it = mCacheMap.find( key );

		// Key already exists, update value and move to front
		if ( it != mCacheMap.end() ) {
			it->second->second = std::move( value );
			mCacheList.splice( mCacheList.begin(), mCacheList, it->second );
			return;
		}

		// Key doesn't exist, check for capacity
		if ( mCacheList.size() == Capacity ) {
			// Evict the least recently used element (the one at the back)
			KeyT lruKey = mCacheList.back().first;
			mCacheMap.erase( lruKey );
			mCacheList.pop_back();
		}

		// Insert the new element at the front
		mCacheList.emplace_front( key, std::move( value ) );
		mCacheMap[key] = mCacheList.begin();
	}

	void clear() {
		mCacheList.clear();
		mCacheMap.clear();
	}

	template <typename Predicate> void eraseIf( Predicate predicate ) {
		for ( auto it = mCacheList.begin(); it != mCacheList.end(); ) {
			if ( predicate( it->first, it->second ) ) {
				mCacheMap.erase( it->first );
				it = mCacheList.erase( it );
			} else {
				++it;
			}
		}
	}

	[[nodiscard]] std::size_t size() const { return mCacheList.size(); }
};

// -----------------------------------------------------------------------------
//  StaticLRU (contiguous array-based, generalized)
// -----------------------------------------------------------------------------
template <std::size_t Capacity, typename KeyT = std::uint64_t,
		  typename ValueT = std::array<char, 64>>
class StaticLRU {
  public:
	static_assert( Capacity > 0, "Capacity must be greater than zero for StaticLRU" );
	static_assert( Capacity < ( 1 << 16 ) - 2, "Capacity must be less than 65534 for StaticLRU" );
	static constexpr std::size_t N = Capacity;
	static constexpr std::size_t HASH_SZ = 2 * N; // Load factor <= 0.5
	static constexpr std::uint16_t NONE = std::numeric_limits<std::uint16_t>::max();
	static constexpr std::uint16_t DELETED_IDX = NONE - 1; // Tombstone for deleted slots

	struct Entry {
		std::uint16_t idx = NONE;
	};

	StaticLRU() {
		std::fill( prev_.begin(), prev_.end(), NONE );
		std::fill( next_.begin(), next_.end(), NONE );
		std::fill( table_.begin(), table_.end(), Entry{ NONE } );
	}

	std::optional<ValueT> get( const KeyT& key ) noexcept {
		const std::uint16_t idx = find_index( key );
		if ( idx >= N )
			return std::nullopt;

		unlink( idx );
		push_front( idx );
		return vals_[idx];
	}

	void put( KeyT key, ValueT value ) noexcept {
		const std::uint16_t idx = find_index( key );

		// Hit: update existing entry and move to front
		if ( idx < N ) {
			vals_[idx] = std::move( value );
			unlink( idx );
			push_front( idx );
			return;
		}

		// Miss: need to insert a new entry
		std::uint16_t target_idx;
		if ( used_ < N ) {
			// There's free space, allocate a new index
			target_idx = static_cast<std::uint16_t>( used_++ );
		} else {
			// Cache is full, evict the least recently used item (tail)
			target_idx = tail_;
			if ( target_idx >= N )
				return;

			// O(1) removal from hash table using pre-stored position and a tombstone
			const std::uint16_t old_pos = hash_pos_[target_idx];
			if ( old_pos >= HASH_SZ )
				return;
			table_[old_pos].idx = DELETED_IDX;

			unlink( target_idx );
		}

		keys_[target_idx] = key;
		vals_[target_idx] = std::move( value );

		// Insert into hash table using linear probing
		// We can reuse DELETED_IDX slots
		const std::size_t h = std::hash<KeyT>{}( key ) % HASH_SZ;
		for ( std::size_t probe = 0; probe < HASH_SZ; ++probe ) {
			const std::size_t pos = ( h + probe ) % HASH_SZ;
			if ( table_[pos].idx == NONE || table_[pos].idx == DELETED_IDX ) {
				table_[pos].idx = target_idx;
				hash_pos_[target_idx] = pos; // Store the hash table position
				break;
			}
		}

		push_front( target_idx );
	}

	void clear() {
		for ( std::size_t i = 0; i < used_; ++i ) {
			keys_[i] = KeyT{};
			vals_[i] = ValueT{};
		}
		std::fill( table_.begin(), table_.end(), Entry{ NONE } );
		std::fill( prev_.begin(), prev_.end(), NONE );
		std::fill( next_.begin(), next_.end(), NONE );
		head_ = tail_ = NONE;
		used_ = 0;
	}

	template <typename Predicate> void eraseIf( Predicate predicate ) {
		std::vector<std::pair<KeyT, ValueT>> retained;
		retained.reserve( used_ );
		for ( std::uint16_t idx = head_; idx < N; idx = next_[idx] ) {
			if ( !predicate( keys_[idx], vals_[idx] ) )
				retained.emplace_back( std::move( keys_[idx] ), std::move( vals_[idx] ) );
		}

		clear();
		for ( auto it = retained.rbegin(); it != retained.rend(); ++it )
			put( std::move( it->first ), std::move( it->second ) );
	}

	[[nodiscard]] std::size_t size() const noexcept { return used_; }

  private:
	// Data storage (SoA)
	alignas( 64 ) std::array<KeyT, N> keys_{};
	alignas( 64 ) std::array<ValueT, N> vals_{};

	// Doubly-linked list for LRU ordering
	alignas( 64 ) std::array<std::uint16_t, N> prev_{};
	alignas( 64 ) std::array<std::uint16_t, N> next_{};
	std::uint16_t head_ = NONE;
	std::uint16_t tail_ = NONE;
	std::size_t used_ = 0;

	// Hash table (open addressing with linear probing)
	alignas( 64 ) std::array<Entry, HASH_SZ> table_{};
	// Reverse map: from data index to hash table position for O(1) eviction
	alignas( 64 ) std::array<std::uint16_t, N> hash_pos_{};

	void unlink( std::uint16_t i ) noexcept {
		if ( i >= N )
			return;

		const std::uint16_t previous = prev_[i];
		const std::uint16_t next = next_[i];
		if ( previous < N )
			next_[previous] = next;
		if ( next < N )
			prev_[next] = previous;
		if ( head_ == i )
			head_ = next;
		if ( tail_ == i )
			tail_ = previous;
	}

	void push_front( std::uint16_t i ) noexcept {
		if ( i >= N )
			return;

		prev_[i] = NONE;
		next_[i] = head_;
		if ( head_ < N )
			prev_[head_] = i;
		head_ = i;
		if ( tail_ == NONE )
			tail_ = i;
	}

	// Finds the storage index for a given key.
	// Returns NONE if the key is not found.
	[[nodiscard]] std::uint16_t find_index( const KeyT& key ) const noexcept {
		const std::size_t h = std::hash<KeyT>{}( key ) % HASH_SZ;
		for ( std::size_t probe = 0; probe < HASH_SZ; ++probe ) {
			const std::size_t pos = ( h + probe ) % HASH_SZ;
			const auto& entry = table_[pos];

			if ( entry.idx == NONE ) {
				// Empty slot means the key cannot be further down the probe chain.
				return NONE;
			}
			if ( entry.idx < N && keys_[entry.idx] == key ) {
				// Found the key.
				return entry.idx;
			}
			// If entry.idx is DELETED_IDX, we continue probing.
		}
		return NONE; // Not found after checking the whole table
	}
};

// -----------------------------------------------------------------------------
//  Hybrid LRU: Compile-time selection
// -----------------------------------------------------------------------------
template <std::size_t Capacity, typename KeyT = std::uint64_t,
		  typename ValueT = std::array<char, 64>>
class LRUCache {
  private:
	static_assert( Capacity > 0, "Capacity must be greater than zero for LRUCache" );

	// Threshold for choosing the static implementation. If the estimated memory usage
	// is below this, the faster, allocation-free StaticLRU is used.
	static constexpr std::size_t STATIC_THRESHOLD_BYTES = 1 << 20; // 1 MB

	static constexpr std::size_t estimate_static_size() noexcept {
		constexpr std::size_t key_size = sizeof( KeyT );
		constexpr std::size_t val_size = sizeof( ValueT );
		constexpr std::size_t idx_size = sizeof( std::uint16_t );
		constexpr std::size_t entry_size = sizeof( std::uint16_t );

		constexpr std::size_t data_mem = ( key_size + val_size ) * Capacity;
		constexpr std::size_t lru_mem = ( idx_size * 2 ) * Capacity;	// prev/next
		constexpr std::size_t hash_mem = entry_size * ( 2 * Capacity ); // table
		constexpr std::size_t reverse_hash_mem = idx_size * Capacity;	// hash_pos

		// Sum and add a fudge factor for alignment/padding
		return static_cast<std::size_t>( ( data_mem + lru_mem + hash_mem + reverse_hash_mem ) *
										 1.2 );
	}

	static constexpr bool use_static =
		( Capacity > 0 && estimate_static_size() < STATIC_THRESHOLD_BYTES );

	using Impl = std::conditional_t<use_static, StaticLRU<Capacity, KeyT, ValueT>,
									DynamicLRU<Capacity, KeyT, ValueT>>;
	Impl impl_;

  public:
	LRUCache() = default;

	// Pass key by value if small and trivially copyable, otherwise by const reference.
	using KeyParamT = std::conditional_t<sizeof( KeyT ) <= 2 * sizeof( void* ) &&
											 std::is_trivially_copyable_v<KeyT>,
										 KeyT, const KeyT&>;

	std::optional<ValueT> get( KeyParamT key ) { return impl_.get( key ); }
	void put( KeyT key, ValueT value ) { impl_.put( std::move( key ), std::move( value ) ); }
	void clear() { impl_.clear(); }
	template <typename Predicate> void eraseIf( Predicate predicate ) {
		impl_.eraseIf( std::move( predicate ) );
	}

	static constexpr std::size_t capacity() { return Capacity; }
	[[nodiscard]] std::size_t size() const { return impl_.size(); }
	static constexpr bool is_static() { return use_static; }
};

} // namespace EE
