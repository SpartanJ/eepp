#include <eepp/ui/models/widgettreemodel.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI { namespace Models {

std::shared_ptr<WidgetTreeModel> WidgetTreeModel::New( Node* node ) {
	return std::shared_ptr<WidgetTreeModel>( new WidgetTreeModel( node ) );
}

WidgetTreeModel::WidgetTreeModel( Node* node ) : mRoot( node ) {
	eeASSERT( mRoot );
}

size_t WidgetTreeModel::rowCount( const ModelIndex& index ) const {
	if ( !index.isValid() )
		return mRoot->getChildCount();
	Node* node = static_cast<Node*>( index.data() );
	return node->getChildCount();
}

size_t WidgetTreeModel::columnCount( const ModelIndex& ) const {
	return 1;
}

Variant WidgetTreeModel::data( const ModelIndex& index, Model::Role role ) const {
	const char* EMPTY = "";
	Node* node = static_cast<Node*>( index.data() );
	if ( role == Role::Display ) {
		if ( node->isWidget() ) {
			return Variant( node->asType<UIWidget>()->getElementTag().c_str() );
		} else if ( node->isUISceneNode() ) {
			return Variant( "UISceneNode" );
		} else if ( node->isSceneNode() ) {
			return Variant( "SceneNode" );
		} else {
			return Variant( node->getId().empty() ? "Node" : node->getId() );
		}
	}
	return Variant( EMPTY );
}

ModelIndex WidgetTreeModel::index( int row, int column, const ModelIndex& parent ) const {
	if ( !parent.isValid() )
		return createIndex( row, column, mRoot );
	Node* parentNode = static_cast<Node*>( parent.data() );
	return createIndex( row, column, parentNode->getChildAt( row ) );
}

ModelIndex WidgetTreeModel::parentIndex( const ModelIndex& index ) const {
	if ( !index.isValid() )
		return {};
	Node* node = static_cast<Node*>( index.data() );
	if ( node == mRoot )
		return {};
	return createIndex( node->getParent()->getNodeIndex(), 0, node->getParent() );
}

void WidgetTreeModel::update() {
	onModelUpdate();
}

}}} // namespace EE::UI::Models
