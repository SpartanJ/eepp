#include <eepp/ui/models/persistentmodelindex.hpp>

namespace EE { namespace UI { namespace Models {

PersistentModelIndex::PersistentModelIndex( ModelIndex const& index ) {
	if ( !index.isValid() )
		return;

	auto* model = const_cast<Model*>( index.model() );
	m_handle = model->registerPersistentIndex( index );
}

int PersistentModelIndex::row() const {
	if ( !hasValidHandle() )
		return -1;
	return m_handle.lock()->m_index.row();
}

int PersistentModelIndex::column() const {
	if ( !hasValidHandle() )
		return -1;
	return m_handle.lock()->m_index.column();
}

PersistentModelIndex PersistentModelIndex::parent() const {
	if ( !hasValidHandle() )
		return {};
	return { m_handle.lock()->m_index.parent() };
}

PersistentModelIndex PersistentModelIndex::siblingAtColumn( int column ) const {
	if ( !hasValidHandle() )
		return {};

	return { m_handle.lock()->m_index.siblingAtColumn( column ) };
}

Variant PersistentModelIndex::data( ModelRole role ) const {
	if ( !hasValidHandle() )
		return {};
	return { m_handle.lock()->m_index.data( role ) };
}

PersistentModelIndex::operator ModelIndex() const {
	if ( !hasValidHandle() )
		return {};
	else
		return m_handle.lock()->m_index;
}

bool PersistentModelIndex::operator==( PersistentModelIndex const& other ) const {
	bool is_this_valid = hasValidHandle();
	bool is_other_valid = other.hasValidHandle();

	if ( !is_this_valid && !is_other_valid )
		return true;
	if ( is_this_valid != is_other_valid )
		return false;

	return m_handle.lock()->m_index == other.m_handle.lock()->m_index;
}

bool PersistentModelIndex::operator!=( PersistentModelIndex const& other ) const {
	return !( *this == other );
}

bool PersistentModelIndex::operator==( ModelIndex const& other ) const {
	if ( !hasValidHandle() ) {
		return !other.isValid();
	}

	return m_handle.lock()->m_index == other;
}

bool PersistentModelIndex::operator!=( ModelIndex const& other ) const {
	return !( *this == other );
}

}}} // namespace EE::UI::Models
