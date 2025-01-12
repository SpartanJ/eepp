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

	void clear();

	ModelVariableNode::NodePtr getNodeByReference( int variablesReference );

	std::shared_ptr<ModelVariableNode> rootNode;
	std::shared_ptr<VariablesModel> model;
	std::unordered_map<int, ModelVariableNode::NodePtr> nodeMap;
};

} // namespace ecode
