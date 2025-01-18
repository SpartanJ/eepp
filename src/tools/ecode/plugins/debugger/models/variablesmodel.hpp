#pragma once

#include "../dap/protocol.hpp"
#include <eepp/ui/models/model.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Models;

namespace EE::UI {
class UISceneNode;
class UITreeView;
} // namespace EE::UI

namespace ecode {

using namespace dap;

class DebuggerClient;

using VariablePath = std::vector<std::string>;

} // namespace ecode

namespace std {
template <> struct hash<ecode::VariablePath> {
	size_t operator()( const ecode::VariablePath& path ) const {
		size_t h = 0;
		for ( const auto& child : path )
			h = hashCombine( h, std::hash<std::string>{}( child ) );
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

class VariablesHolder {
  public:
	VariablesHolder( UISceneNode* sceneNode );

	void addVariables( const int variablesReference, std::vector<Variable>&& vars );

	void addChild( ModelVariableNode::NodePtr child );

	void upsertRootChild( Variable&& );

	void clear( bool all = false );

	void saveExpandedState( const ModelIndex& index );

	bool restoreExpandedState( const ExpandedState::Location& location, DebuggerClient* client,
							   UITreeView* uiVariables );

	std::shared_ptr<VariablesModel> getModel() { return mModel; }

  protected:
	Mutex mMutex;
	std::shared_ptr<ModelVariableNode> mRootNode;
	std::shared_ptr<VariablesModel> mModel;
	std::unordered_map<int, ModelVariableNode::NodePtr> mNodeMap;
	std::optional<ExpandedState::Location> mCurrentLocation;
	std::unordered_map<ExpandedState::Location, std::unordered_set<VariablePath>> mExpandedStates;

	ModelVariableNode::NodePtr getNodeByReference( int variablesReference );

	VariablePath buildVariablePath( ModelVariableNode* node ) const;

	bool resolvePath( std::vector<std::string> path, DebuggerClient* client,
					  UITreeView* uiVariables, ModelVariableNode::NodePtr parentNode, int pathPos );
};

} // namespace ecode
