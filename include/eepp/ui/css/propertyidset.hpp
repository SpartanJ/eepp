#ifndef EE_UI_CSS_PROPERTYIDSET_HPP
#define EE_UI_CSS_PROPERTYIDSET_HPP

#include <bitset>
#include <eepp/core/debug.hpp>
#include <eepp/ui/css/propertyids.hpp>

namespace EE { namespace UI { namespace CSS {

class PropertyIdSetIterator;

namespace detail {
inline std::size_t bitsetFindFirst( const std::bitset<512>& bits ) {
	for ( std::size_t i = 0; i < bits.size(); ++i )
		if ( bits.test( i ) )
			return i;
	return bits.size();
}

inline std::size_t bitsetFindNext( const std::bitset<512>& bits, std::size_t from ) {
	for ( std::size_t i = from + 1; i < bits.size(); ++i )
		if ( bits.test( i ) )
			return i;
	return bits.size();
}
} // namespace detail

/**
 * @brief Fixed-capacity set of dense PropertyId values.
 *
 * Backed by a 512-bit std::bitset, so construction, clearing, copying, union,
 * intersection, and iteration perform zero heap allocations. Only successfully
 * registered property definitions occupy bits; unknown names never enter this
 * set. Iteration yields present IDs in ascending dense ID order.
 */
class EE_API PropertyIdSet {
  private:
	std::bitset<512> mBits;

  public:
	void insert( PropertyId id ) {
		const auto underlying = static_cast<std::size_t>( id );
		if ( underlying == 0 )
			return; // Invalid is a no-op
		if ( underlying >= mBits.size() ) {
			eeASSERTM( false, "PropertyId out of PropertyIdSet capacity" );
			return;
		}
		mBits.set( underlying );
	}

	void clear() { mBits.reset(); }

	void erase( PropertyId id ) {
		const auto underlying = static_cast<std::size_t>( id );
		if ( underlying == 0 )
			return; // Invalid is a no-op
		if ( underlying >= mBits.size() ) {
			eeASSERTM( false, "PropertyId out of PropertyIdSet capacity" );
			return;
		}
		mBits.reset( underlying );
	}

	bool empty() const { return mBits.none(); }

	bool contains( PropertyId id ) const {
		const auto underlying = static_cast<std::size_t>( id );
		return underlying != 0 && underlying < mBits.size() && mBits.test( underlying );
	}

	size_t size() const { return mBits.count(); }

	// Union with another set
	PropertyIdSet& operator|=( const PropertyIdSet& other ) {
		mBits |= other.mBits;
		return *this;
	}

	PropertyIdSet operator|( const PropertyIdSet& other ) const {
		PropertyIdSet result = *this;
		result |= other;
		return result;
	}

	// Intersection with another set
	PropertyIdSet& operator&=( const PropertyIdSet& other ) {
		mBits &= other.mBits;
		return *this;
	}

	PropertyIdSet operator&( const PropertyIdSet& other ) const {
		PropertyIdSet result = *this;
		result &= other;
		return result;
	}

	bool operator==( const PropertyIdSet& other ) const { return mBits == other.mBits; }

	bool operator!=( const PropertyIdSet& other ) const { return mBits != other.mBits; }

	// Iterator support. Iterates through all the PropertyIds that are set (contained).
	inline PropertyIdSetIterator begin() const;
	inline PropertyIdSetIterator end() const;

	// Erases the property id represented by a valid iterator. Invalidates any previous iterators.
	// @return A new valid iterator pointing to the next element or end().
	inline PropertyIdSetIterator erase( const PropertyIdSetIterator& it );

	friend class PropertyIdSetIterator;
};

class EE_API PropertyIdSetIterator {
  public:
	PropertyIdSetIterator() : container( nullptr ), bitIndex( 0 ) {}

	explicit PropertyIdSetIterator( const PropertyIdSet* container, std::size_t bitIndex ) :
		container( container ), bitIndex( bitIndex ) {}

	PropertyIdSetIterator& operator++() {
		if ( container ) {
			std::size_t next = detail::bitsetFindNext( container->mBits, bitIndex );
			bitIndex = ( next == container->mBits.size() ) ? container->mBits.size() : next;
		}
		return *this;
	}

	bool operator==( const PropertyIdSetIterator& other ) const {
		return container == other.container && bitIndex == other.bitIndex;
	}

	bool operator!=( const PropertyIdSetIterator& other ) const { return !( *this == other ); }

	PropertyId operator*() const { return static_cast<PropertyId>( bitIndex ); }

  private:
	const PropertyIdSet* container;
	std::size_t bitIndex;
	friend PropertyIdSetIterator PropertyIdSet::erase( const PropertyIdSetIterator& );
};

PropertyIdSetIterator PropertyIdSet::begin() const {
	const std::size_t first = detail::bitsetFindFirst( mBits );
	const std::size_t index = ( first == mBits.size() ) ? mBits.size() : first;
	return PropertyIdSetIterator( this, index );
}

PropertyIdSetIterator PropertyIdSet::end() const {
	return PropertyIdSetIterator( this, mBits.size() );
}

PropertyIdSetIterator PropertyIdSet::erase( const PropertyIdSetIterator& it_in ) {
	PropertyIdSetIterator it = it_in;
	if ( it.container == this && it.bitIndex < mBits.size() ) {
		mBits.reset( it.bitIndex );
		++it;
	}
	return it;
}

static_assert( sizeof( PropertyIdSet ) == 64, "PropertyIdSet must be a fixed 64-byte bitset" );

}}} // namespace EE::UI::CSS

#endif // EE_UI_CSS_PROPERTYIDSET_HPP
