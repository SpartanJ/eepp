#include "variablesmodel.hpp"
#include <eepp/ui/uiscenenode.hpp>

namespace ecode {

ModelVariableNode::ModelVariableNode( Variable&& var, NodePtr parent ) :
	parent( parent ), var( std::move( var ) ) {}

ModelVariableNode::ModelVariableNode( const std::string& name, int variablesReference,
									  NodePtr parent ) :
	parent( parent ) {
	var.name = name;
	var.variablesReference = variablesReference;
}

ModelVariableNode::~ModelVariableNode() {}

const std::string& ModelVariableNode::getName() const {
	return var.name;
}

int ModelVariableNode::getVariablesReference() const {
	return var.variablesReference;
}

std::optional<ModelVariableNode::NodePtr>
ModelVariableNode::getChildRecursive( int variablesReference ) {
	if ( var.variablesReference == variablesReference )
		return shared_from_this();

	for ( const auto& child : children ) {
		auto found = child->getChild( variablesReference );
		if ( found )
			return found;
	}
	return {};
}

void ModelVariableNode::clear() {
	children.clear();
}

void ModelVariableNode::addChild( NodePtr child ) {
	children.emplace_back( child );
}

ModelVariableNode::NodePtr ModelVariableNode::getParent() const {
	return parent;
}

std::optional<ModelVariableNode::NodePtr> ModelVariableNode::getChild( int variablesReference ) {
	auto found = std::find_if( children.begin(), children.end(),
							   [variablesReference]( const NodePtr& child ) {
								   return child->var.variablesReference == variablesReference;
							   } );
	return ( found != children.end() ) ? *found : std::optional<NodePtr>{};
}

std::optional<ModelVariableNode::NodePtr> ModelVariableNode::getChild( const std::string& name ) {
	auto found = std::find_if( children.begin(), children.end(), [&name]( const NodePtr& child ) {
		return child->var.name == name;
	} );
	return ( found != children.end() ) ? *found : std::optional<NodePtr>{};
}

const std::vector<ModelVariableNode::NodePtr>& ModelVariableNode::getChildren() const {
	return children;
}

VariablesModel::VariablesModel( ModelVariableNode::NodePtr rootNode, UISceneNode* sceneNode ) :
	rootNode( rootNode ), mSceneNode( sceneNode ) {}

ModelIndex VariablesModel::index( int row, int column, const ModelIndex& parent ) const {
	ModelVariableNode* parentNode = parent.isValid()
										? static_cast<ModelVariableNode*>( parent.internalData() )
										: rootNode.get();

	if ( row >= 0 && row < static_cast<int>( parentNode->getChildren().size() ) ) {
		ModelVariableNode::NodePtr childNode = parentNode->getChildren()[row];
		return createIndex( row, column, childNode.get() );
	}

	return ModelIndex();
}

ModelIndex VariablesModel::parentIndex( const ModelIndex& index ) const {
	if ( !index.isValid() )
		return ModelIndex();

	ModelVariableNode* childNode = static_cast<ModelVariableNode*>( index.internalData() );
	ModelVariableNode::NodePtr parentNode = childNode->getParent();

	if ( parentNode == nullptr || parentNode == rootNode )
		return ModelIndex();

	ModelVariableNode::NodePtr grandParentNode = parentNode->getParent();
	if ( grandParentNode == nullptr )
		grandParentNode = rootNode;

	int row = std::distance( grandParentNode->getChildren().begin(),
							 std::find_if( grandParentNode->getChildren().begin(),
										   grandParentNode->getChildren().end(),
										   [parentNode]( ModelVariableNode::NodePtr node ) {
											   return node.get() == parentNode.get();
										   } ) );

	return createIndex( row, Columns::Name, parentNode.get() );
}

size_t VariablesModel::rowCount( const ModelIndex& index ) const {
	ModelVariableNode* parentNode =
		index.isValid() ? static_cast<ModelVariableNode*>( index.internalData() ) : rootNode.get();

	return static_cast<int>( parentNode->getChildren().size() );
}

bool VariablesModel::hasChilds( const ModelIndex& index ) const {
	if ( !index.isValid() )
		return !rootNode->children.empty();
	ModelVariableNode* node = static_cast<ModelVariableNode*>( index.internalData() );
	return !node->children.empty() || node->var.variablesReference > 0;
}

size_t VariablesModel::columnCount( const ModelIndex& ) const {
	return 3;
}

std::string VariablesModel::columnName( const size_t& colIdx ) const {
	switch ( colIdx ) {
		case Columns::Name:
			return mSceneNode->i18n( "variable_name", "Variable Name" );
		case Columns::Value:
			return mSceneNode->i18n( "value", "Value" );
		case Columns::Type:
			return mSceneNode->i18n( "type", "Type" );
	}
	return "";
}

Variant VariablesModel::data( const ModelIndex& index, ModelRole role ) const {
	static const char* EMPTY = "";
	if ( !index.isValid() )
		return EMPTY;

	ModelVariableNode* node = static_cast<ModelVariableNode*>( index.internalData() );

	if ( role == ModelRole::Display ) {
		switch ( index.column() ) {
			case 0:
				return Variant( node->var.name.c_str() );
			case 1:
				return Variant( node->var.value.c_str() );
			case 2:
				return Variant( node->var.type ? node->var.type->c_str() : EMPTY );
		}
	}

	return EMPTY;
}

VariablesHolder::VariablesHolder( UISceneNode* sceneNode ) :
	rootNode( std::make_shared<ModelVariableNode>( "Root", 0 ) ),
	model( std::make_shared<VariablesModel>( rootNode, sceneNode ) ) {
	nodeMap[0] = rootNode;
}

void VariablesHolder::addVariables( const int variablesReference, std::vector<Variable>&& vars ) {
	Lock l( mutex );
	auto parentNode = getNodeByReference( variablesReference );
	if ( !parentNode ) {
		auto node = rootNode->getChildRecursive( variablesReference );
		if ( !node )
			return;
		parentNode = *node;
	}

	bool invalidateIndexes = false;

	for ( auto& var : vars ) {
		if ( var.name.empty() )
			continue;

		auto found = parentNode->getChild( var.name );
		if ( found ) {
			ModelVariableNode::NodePtr child = *found;
			child->var = std::move( var );

			if ( child->var.variablesReference != 0 )
				nodeMap[child->var.variablesReference] = *found;

			continue;
		}

		invalidateIndexes = true;

		auto child = std::make_shared<ModelVariableNode>( std::move( var ), parentNode );
		parentNode->addChild( child );

		if ( child->var.variablesReference != 0 )
			nodeMap[child->var.variablesReference] = child;
	}

	model->invalidate( invalidateIndexes ? Model::UpdateFlag::InvalidateAllIndexes
										 : Model::UpdateFlag::DontInvalidateIndexes );
}

void VariablesHolder::addChild( ModelVariableNode::NodePtr child ) {
	Lock l( mutex );
	rootNode->addChild( child );
	nodeMap[child->var.variablesReference] = child;
	model->invalidate( Model::UpdateFlag::InvalidateAllIndexes );
}

void VariablesHolder::upsertRootChild( Variable&& var ) {
	Lock l( mutex );
	for ( size_t i = 0; i < rootNode->children.size(); i++ ) {
		auto child = rootNode->children[i];
		if ( child->getName() == var.name ) {
			auto newChild = std::make_shared<ModelVariableNode>( std::move( var ), rootNode );
			nodeMap[newChild->var.variablesReference] = newChild;
			rootNode->children[i] = std::move( newChild );
			model->invalidate( Model::UpdateFlag::DontInvalidateIndexes );
			return;
		}
	}
	auto newChild = std::make_shared<ModelVariableNode>( std::move( var ), rootNode );
	addChild( newChild );
}

void VariablesHolder::clear( bool all ) {
	Lock l( mutex );
	rootNode->clear();
	if ( all ) {
		nodeMap.clear();
	}
}

ModelVariableNode::NodePtr VariablesHolder::getNodeByReference( int variablesReference ) {
	auto it = nodeMap.find( variablesReference );
	return ( it != nodeMap.end() ) ? it->second : nullptr;
}

} // namespace ecode
