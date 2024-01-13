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
	Node* node = static_cast<Node*>( index.internalData() );
	return node->getChildCount();
}

size_t WidgetTreeModel::columnCount( const ModelIndex& ) const {
	return Column::Count;
}

std::string WidgetTreeModel::columnName( const size_t& col ) const {
	switch ( col ) {
		case Column::PseudoClasses:
			return "Pseudo Classes";
		case Column::Classes:
			return "Classes";
		case Column::Type:
			return "Type";
		case Column::ID:
			return "ID";
	}
	return "";
}

Variant WidgetTreeModel::data( const ModelIndex& index, ModelRole role ) const {
	const char* EMPTY = "";
	Node* node = static_cast<Node*>( index.internalData() );

	if ( role == ModelRole::Display ) {
		switch ( index.column() ) {
			case Column::PseudoClasses: {
				if ( node->isWidget() ) {
					return Variant(
						String::join( node->asType<UIWidget>()->getStyleSheetPseudoClasses() ) );
				}
				break;
			}
			case Column::Classes: {
				if ( node->isWidget() ) {
					return Variant(
						String::join( node->asType<UIWidget>()->getStyleSheetClasses() ) );
				}
				break;
			}
			case Column::Type: {
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
			case Column::ID:
				return Variant( node->getId() );
		}
	}

	return Variant( EMPTY );
}

ModelIndex WidgetTreeModel::index( int row, int column, const ModelIndex& parent ) const {
	if ( !parent.isValid() )
		return createIndex( row, column, mRoot );
	Node* parentNode = static_cast<Node*>( parent.internalData() );
	return createIndex( row, column, parentNode->getChildAt( row ) );
}

ModelIndex WidgetTreeModel::parentIndex( const ModelIndex& index ) const {
	if ( !index.isValid() )
		return {};
	Node* node = static_cast<Node*>( index.internalData() );
	if ( node == mRoot )
		return {};
	return createIndex( node->getParent()->getNodeIndex(), 0, node->getParent() );
}

ModelIndex WidgetTreeModel::getRoot() const {
	return createIndex( 0, 0, mRoot );
}

ModelIndex WidgetTreeModel::getModelIndex( const Node* node ) const {
	eeASSERT( node != nullptr );
	const Node* fNode = node;
	while ( fNode ) {
		if ( fNode == mRoot )
			break;
		fNode = fNode->getParent();
	}
	if ( fNode != mRoot || nullptr == node->getParent() )
		return {};
	return createIndex( node->getNodeIndex(), 0, node );
}

}}} // namespace EE::UI::Models
