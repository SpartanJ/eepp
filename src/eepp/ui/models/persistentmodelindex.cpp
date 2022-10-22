#include <eepp/ui/models/persistentmodelindex.hpp>

namespace EE { namespace UI { namespace Models {

PersistentModelIndex::PersistentModelIndex( ModelIndex const& index ) {
	if ( !index.isValid() )
		return;

	auto* model = const_cast<Model*>( index.model() );
	mHandle = model->registerPersistentIndex( index );
}

int PersistentModelIndex::row() const {
	if ( !hasValidHandle() )
		return -1;
	return mHandle.lock()->mIndex.row();
}

int PersistentModelIndex::column() const {
	if ( !hasValidHandle() )
		return -1;
	return mHandle.lock()->mIndex.column();
}

PersistentModelIndex PersistentModelIndex::parent() const {
	if ( !hasValidHandle() )
		return {};
	return { mHandle.lock()->mIndex.parent() };
}

PersistentModelIndex PersistentModelIndex::siblingAtColumn( int column ) const {
	if ( !hasValidHandle() )
		return {};

	return { mHandle.lock()->mIndex.siblingAtColumn( column ) };
}

Variant PersistentModelIndex::data( ModelRole role ) const {
	if ( !hasValidHandle() )
		return {};
	return { mHandle.lock()->mIndex.data( role ) };
}

PersistentModelIndex::operator ModelIndex() const {
	if ( !hasValidHandle() )
		return {};
	else
		return mHandle.lock()->mIndex;
}

bool PersistentModelIndex::operator==( PersistentModelIndex const& other ) const {
	bool isThisValid = hasValidHandle();
	bool isOtherValid = other.hasValidHandle();

	if ( !isThisValid && !isOtherValid )
		return true;
	if ( isThisValid != isOtherValid )
		return false;

	return mHandle.lock()->mIndex == other.mHandle.lock()->mIndex;
}

bool PersistentModelIndex::operator!=( PersistentModelIndex const& other ) const {
	return !( *this == other );
}

bool PersistentModelIndex::operator==( ModelIndex const& other ) const {
	if ( !hasValidHandle() ) {
		return !other.isValid();
	}

	return mHandle.lock()->mIndex == other;
}

bool PersistentModelIndex::operator!=( ModelIndex const& other ) const {
	return !( *this == other );
}

}}} // namespace EE::UI::Models
