#include "variablesmodel.hpp"
#include "../debuggerclient.hpp"
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitreeview.hpp>

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
	mRootNode( std::make_shared<ModelVariableNode>( "Root", 0 ) ),
	mModel( std::make_shared<VariablesModel>( mRootNode, sceneNode ) ) {
	mNodeMap[0] = mRootNode;
}

void VariablesHolder::addVariables( const int variablesReference, std::vector<Variable>&& vars ) {
	Lock l( mutex );
	auto parentNode = getNodeByReference( variablesReference );
	if ( !parentNode ) {
		auto node = mRootNode->getChildRecursive( variablesReference );
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
				mNodeMap[child->var.variablesReference] = *found;

			continue;
		}

		invalidateIndexes = true;

		auto child = std::make_shared<ModelVariableNode>( std::move( var ), parentNode );
		parentNode->addChild( child );

		if ( child->var.variablesReference != 0 )
			mNodeMap[child->var.variablesReference] = child;
	}

	mModel->invalidate( invalidateIndexes ? Model::UpdateFlag::InvalidateAllIndexes
										  : Model::UpdateFlag::DontInvalidateIndexes );
}

void VariablesHolder::addChild( ModelVariableNode::NodePtr child ) {
	Lock l( mutex );
	mRootNode->addChild( child );
	mNodeMap[child->var.variablesReference] = child;
	mModel->invalidate( Model::UpdateFlag::InvalidateAllIndexes );
}

void VariablesHolder::upsertRootChild( Variable&& var ) {
	Lock l( mutex );
	for ( size_t i = 0; i < mRootNode->children.size(); i++ ) {
		auto child = mRootNode->children[i];
		if ( child->getName() == var.name ) {
			auto newChild = std::make_shared<ModelVariableNode>( std::move( var ), mRootNode );
			mNodeMap[newChild->var.variablesReference] = newChild;
			mRootNode->children[i] = std::move( newChild );
			mModel->invalidate( Model::UpdateFlag::DontInvalidateIndexes );
			return;
		}
	}
	auto newChild = std::make_shared<ModelVariableNode>( std::move( var ), mRootNode );
	addChild( newChild );
}

void VariablesHolder::clear( bool all ) {
	Lock l( mutex );
	mRootNode->clear();
	if ( all ) {
		mNodeMap.clear();
		mExpandedStates.clear();
		mCurrentLocation = {};
	}
}

ModelVariableNode::NodePtr VariablesHolder::getNodeByReference( int variablesReference ) {
	auto it = mNodeMap.find( variablesReference );
	return ( it != mNodeMap.end() ) ? it->second : nullptr;
}

VariablePath VariablesHolder::buildVariablePath( ModelVariableNode* node ) const {
	VariablePath path;
	while ( node && node != mRootNode.get() ) {
		path.push_back( node->getName() );
		node = node->parent ? node->parent.get() : nullptr;
	}
	std::reverse( path.begin(), path.end() );
	return path;
}

void VariablesHolder::saveExpandedState( const ModelIndex& index ) {
	if ( !mCurrentLocation )
		return;

	ModelVariableNode* node = static_cast<ModelVariableNode*>( index.internalData() );
	if ( !node )
		return;

	auto nodePath = buildVariablePath( node );
	mExpandedStates[*mCurrentLocation].insert( std::move( nodePath ) );
}

void VariablesHolder::resolvePath( std::vector<std::string> path, DebuggerClient* client,
								   UITreeView* uiVariables, ModelVariableNode::NodePtr parentNode,
								   int pathPos ) {
	if ( path.empty() || !parentNode )
		return;

	auto currentNodeOpt = parentNode->getChild( path[pathPos] );
	if ( !currentNodeOpt )
		return;

	auto currentNode = *currentNodeOpt;

	if ( currentNode->getVariablesReference() > 0 ) {
		const auto onVariablesRecieved = [this, uiVariables, path, currentNode, pathPos,
										  client]( const int variablesReference,
												   std::vector<Variable>&& vars ) {
			addVariables( variablesReference, std::move( vars ) );

			uiVariables->runOnMainThread(
				[uiVariables, path] { uiVariables->selectRowWithPath( path ); } );

			auto nextPos = pathPos + 1;
			if ( nextPos < static_cast<Int64>( path.size() ) ) {
				resolvePath( path, client, uiVariables, currentNode, nextPos );
			}
		};

		client->variables( currentNode->getVariablesReference(), Variable::Type::Both,
						   onVariablesRecieved );
	}
}

void VariablesHolder::restoreExpandedState( const ExpandedState::Location& location,
											DebuggerClient* client, UITreeView* uiVariables ) {
	mCurrentLocation = location;

	auto it = mExpandedStates.find( location );
	if ( it == mExpandedStates.end() )
		return;

	for ( const VariablePath& path : it->second )
		resolvePath( path, client, uiVariables, mRootNode, 0 );
}

} // namespace ecode
