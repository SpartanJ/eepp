#ifndef EE_UI_CSS_PROPERTYIDSET_HPP
#define EE_UI_CSS_PROPERTYIDSET_HPP

#include <bitset>
#include <eepp/ui/css/propertydefinition.hpp>
#include <set>

namespace EE { namespace UI { namespace CSS {

class PropertyIdSetIterator;

class EE_API PropertyIdSet {
  private:
	std::set<Uint32> mIds;

  public:
	void insert( Uint32 id ) { mIds.insert( id ); }

	void clear() { mIds.clear(); }

	void erase( Uint32 id ) { mIds.erase( id ); }

	bool empty() const { return mIds.empty(); }

	bool contains( Uint32 id ) const { return mIds.count( id ) == 1; }

	size_t size() const { return mIds.size(); }

	// Union with another set
	PropertyIdSet& operator|=( const PropertyIdSet& other ) {
		mIds.insert( other.mIds.begin(), other.mIds.end() );
		return *this;
	}

	PropertyIdSet operator|( const PropertyIdSet& other ) const {
		PropertyIdSet result = *this;
		result |= other;
		return result;
	}

	// Intersection with another set
	PropertyIdSet& operator&=( const PropertyIdSet& other ) {
		if ( mIds.size() > 0 && other.mIds.size() > 0 ) {
			for ( auto it = mIds.begin(); it != mIds.end(); )
				if ( other.mIds.count( *it ) == 0 )
					it = mIds.erase( it );
				else
					++it;
		} else {
			mIds.clear();
		}
		return *this;
	}

	PropertyIdSet operator&( const PropertyIdSet& other ) const {
		PropertyIdSet result;
		if ( mIds.size() > 0 && other.mIds.size() > 0 ) {
			for ( Uint32 id : mIds )
				if ( other.mIds.count( id ) == 1 )
					result.mIds.insert( id );
		}
		return result;
	}

	// Iterator support. Iterates through all the PropertyIds that are set (contained).
	// @note: Modifying the container invalidates the iterators. Only const_iterators are provided.
	inline PropertyIdSetIterator begin() const;
	inline PropertyIdSetIterator end() const;

	// Erases the property id represented by a valid iterator. Invalidates any previous iterators.
	// @return A new valid iterator pointing to the next element or end().
	inline PropertyIdSetIterator erase( const PropertyIdSetIterator& it );
};

class EE_API PropertyIdSetIterator {
  public:
	using CustomIdsIt = std::set<Uint32>::const_iterator;

	PropertyIdSetIterator() : container( nullptr ), custom_ids_iterator() {}
	PropertyIdSetIterator( const PropertyIdSet* container, CustomIdsIt custom_ids_iterator ) :
		container( container ), custom_ids_iterator( custom_ids_iterator ) {}

	PropertyIdSetIterator& operator++() {
		++custom_ids_iterator;
		return *this;
	}

	bool operator==( const PropertyIdSetIterator& other ) const {
		return container == other.container && custom_ids_iterator == other.custom_ids_iterator;
	}

	bool operator!=( const PropertyIdSetIterator& other ) const { return !( *this == other ); }

	Uint32 operator*() const { return *custom_ids_iterator; }

  private:
	const PropertyIdSet* container;
	CustomIdsIt custom_ids_iterator;
	friend PropertyIdSetIterator PropertyIdSet::erase( const PropertyIdSetIterator& );
};

PropertyIdSetIterator PropertyIdSet::begin() const {
	return PropertyIdSetIterator( this, mIds.begin() );
}

PropertyIdSetIterator PropertyIdSet::end() const {
	return PropertyIdSetIterator( this, mIds.end() );
}

PropertyIdSetIterator PropertyIdSet::erase( const PropertyIdSetIterator& it_in ) {
	PropertyIdSetIterator it = it_in;
	it.custom_ids_iterator = mIds.erase( it.custom_ids_iterator );
	return it;
}

}}} // namespace EE::UI::CSS

#endif // EE_UI_CSS_PROPERTYIDSET_HPP
