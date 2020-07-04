#include <eepp/ui/abstract/model.hpp>
#include <eepp/ui/uiabstractview.hpp>

namespace EE { namespace UI { namespace Abstract {

void Model::didUpdate( unsigned flags ) {
	if ( onUpdate )
		onUpdate();
	forEachView( [&]( UIAbstractView* view ) { view->didUpdateModel( flags ); } );
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

ModelIndex Model::sibling( int row, int column, const ModelIndex& parent ) const {
	if ( !parent.isValid() )
		return index( row, column, {} );
	int rowCount = this->rowCount( parent );
	if ( row < 0 || row > rowCount )
		return {};
	return index( row, column, parent );
}

bool Model::acceptsDrag( const ModelIndex&, const std::string& dataType ) {
	return false;
}

}}} // namespace EE::UI::Abstract
