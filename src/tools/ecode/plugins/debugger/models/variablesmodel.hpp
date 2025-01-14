#pragma once

#include "../dap/protocol.hpp"
#include <eepp/ui/models/model.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Models;

namespace EE::UI {
class UISceneNode;
}

namespace ecode {

using namespace dap;

class DebuggerClient;

struct VariablePath {
	std::string scopeName;
	std::string variableName;
	std::vector<std::string> childPath;

	bool operator==( const VariablePath& other ) const {
		return scopeName == other.scopeName && variableName == other.variableName &&
			   childPath == other.childPath;
	}
};

} // namespace ecode

namespace std {
template <> struct hash<ecode::VariablePath> {
	size_t operator()( const ecode::VariablePath& path ) const {
		size_t h = std::hash<std::string>{}( path.scopeName );
		h ^= std::hash<std::string>{}( path.variableName ) + 0x9e3779b9 + ( h << 6 ) + ( h >> 2 );
		for ( const auto& child : path.childPath ) {
			h ^= std::hash<std::string>{}( child ) + 0x9e3779b9 + ( h << 6 ) + ( h >> 2 );
		}
		return h;
	}
};

} // namespace std

namespace ecode {

struct ExpandedState {
	struct Location {
		std::string filePath;
		int lineNumber{};
		int frameIndex{};

		bool operator==( const Location& other ) const {
			return filePath == other.filePath && lineNumber == other.lineNumber &&
				   frameIndex == other.frameIndex;
		}
	};

	Location location;
	std::unordered_set<VariablePath> expandedPaths;
};

} // namespace ecode

namespace std {
template <> struct hash<ecode::ExpandedState::Location> {
	size_t operator()( const ecode::ExpandedState::Location& loc ) const {
		return hashCombine( std::hash<std::string>{}( loc.filePath ),
							std::hash<int>{}( loc.lineNumber ),
							std::hash<int>{}( loc.frameIndex ) );
	}
};
} // namespace std

namespace ecode {

struct ModelVariableNode : public std::enable_shared_from_this<ModelVariableNode> {
	using NodePtr = std::shared_ptr<ModelVariableNode>;

	ModelVariableNode( Variable&& var, NodePtr parent );

	ModelVariableNode( const std::string& name, int variablesReference, NodePtr parent = nullptr );

	virtual ~ModelVariableNode();

	const std::vector<NodePtr>& getChildren() const;

	const std::string& getName() const;

	int getVariablesReference() const;

	void clear();

	std::optional<NodePtr> getChildRecursive( int variablesReference );

	std::optional<NodePtr> getChild( const std::string& name );

	std::optional<NodePtr> getChild( int variablesReference );

	void addChild( NodePtr child );

	NodePtr getParent() const;

	NodePtr parent{ nullptr };
	Variable var;
	std::vector<NodePtr> children;
};

class VariablesModel : public Model {
  public:
	enum Columns { Name, Value, Type };

	VariablesModel( ModelVariableNode::NodePtr rootNode, UISceneNode* sceneNode );

	ModelIndex index( int row, int column, const ModelIndex& parent = ModelIndex() ) const override;

	ModelIndex parentIndex( const ModelIndex& index ) const override;

	size_t rowCount( const ModelIndex& index = ModelIndex() ) const override;

	bool hasChilds( const ModelIndex& index = ModelIndex() ) const override;

	size_t columnCount( const ModelIndex& ) const override;

	std::string columnName( const size_t& colIdx ) const override;

	Variant data( const ModelIndex& index, ModelRole role ) const override;

  protected:
	ModelVariableNode::NodePtr rootNode;
	UISceneNode* mSceneNode;
};

struct VariablesHolder {
	VariablesHolder( UISceneNode* sceneNode );

	void addVariables( const int variablesReference, std::vector<Variable>&& vars );

	void addChild( ModelVariableNode::NodePtr child );

	void upsertRootChild( Variable&& );

	void clear( bool all = false );

	ModelVariableNode::NodePtr getNodeByReference( int variablesReference );

	Mutex mutex;
	std::shared_ptr<ModelVariableNode> rootNode;
	std::shared_ptr<VariablesModel> model;
	std::unordered_map<int, ModelVariableNode::NodePtr> nodeMap;

	std::unordered_map<ExpandedState::Location, std::unordered_set<VariablePath>> expandedStates;
	std::optional<ExpandedState::Location> currentLocation;

	VariablePath buildVariablePath( ModelVariableNode* node ) const;
	void saveExpandedState( const ModelIndex& index );
	void restoreExpandedState( const ExpandedState::Location& location, DebuggerClient* client );
};

} // namespace ecode
