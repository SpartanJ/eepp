#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/models/model.hpp>

namespace EE { namespace UI { namespace Models {

void Model::onModelUpdate( unsigned flags ) {
	if ( mOnUpdate )
		mOnUpdate();
	forEachView( [&]( UIAbstractView* view ) { view->onModelUpdate( flags ); } );
}

void Model::forEachView( std::function<void( UIAbstractView* )> callback ) {
	for ( auto view : mViews )
		callback( view );
}

void Model::unregisterView( UIAbstractView* view ) {
	mViews.erase( view );
}

void Model::registerView( UIAbstractView* view ) {
	mViews.insert( view );
}

ModelIndex Model::createIndex( int row, int column, const void* data ) const {
	return ModelIndex( *this, row, column, const_cast<void*>( data ) );
}

void Model::setOnUpdate( const std::function<void()>& onUpdate ) {
	mOnUpdate = onUpdate;
}

ModelIndex Model::sibling( int row, int column, const ModelIndex& parent ) const {
	if ( !parent.isValid() )
		return index( row, column, {} );
	int rowCount = this->rowCount( parent );
	if ( row < 0 || row > rowCount )
		return {};
	return index( row, column, parent );
}

bool Model::acceptsDrag( const ModelIndex&, const std::string& ) {
	return false;
}

}}} // namespace EE::UI::Model
