#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/models/model.hpp>

namespace EE { namespace UI { namespace Models {

void Model::onModelUpdate( unsigned flags ) {
	if ( mOnUpdate )
		mOnUpdate();
	for ( auto client : mClients )
		client->onModelUpdated( flags );
	forEachView( [&]( UIAbstractView* view ) { view->onModelUpdate( flags ); } );
}

void Model::forEachView( std::function<void( UIAbstractView* )> callback ) {
	for ( auto view : mViews )
		callback( view );
}

void Model::unregisterView( UIAbstractView* view ) {
	mViews.erase( view );
}

void Model::registerClient( Model::Client* client ) {
	mClients.insert( client );
}

void Model::unregisterClient( Model::Client* client ) {
	mClients.erase( client );
}

void Model::refreshView() {
	forEachView( [&]( UIAbstractView* view ) { view->invalidateDraw(); } );
}

void Model::registerView( UIAbstractView* view ) {
	mViews.insert( view );
}

ModelIndex Model::createIndex( int row, int column, const void* data,
							   const Int64& internalId ) const {
	return ModelIndex( *this, row, column, const_cast<void*>( data ), internalId );
}

void Model::setOnUpdate( const std::function<void()>& onUpdate ) {
	mOnUpdate = onUpdate;
}

void Model::invalidate( unsigned int flags ) {
	onModelUpdate( flags );
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

}}} // namespace EE::UI::Models
