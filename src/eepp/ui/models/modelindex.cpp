#include <eepp/ui/models/model.hpp>
#include <eepp/ui/models/modelindex.hpp>

namespace EE { namespace UI { namespace Models {

ModelIndex ModelIndex::sibling( int row, int column ) const {
	if ( !isValid() )
		return {};
	eeASSERT( model() );
	return model()->index( row, column, parent() );
}

ModelIndex ModelIndex::siblingAtColumn( int column ) const {
	if ( !isValid() )
		return {};
	return sibling( row(), column );
}

std::string ModelIndex::toString() const {
	return String::format( "ModelIndex{col: %ld - row: %ld, internalId: %ld}", mColumn, mRow,
						   mInternalId );
}

Variant ModelIndex::data( ModelRole role ) const {
	if ( !isValid() )
		return {};
	eeASSERT( model() );
	return model()->data( *this, role );
}

ModelIndex ModelIndex::parent() const {
	return mModel ? mModel->parentIndex( *this ) : ModelIndex();
}

}}} // namespace EE::UI::Models
