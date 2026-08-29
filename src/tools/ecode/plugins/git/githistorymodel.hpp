#ifndef ECODE_GITHISTORYMODEL_HPP
#define ECODE_GITHISTORYMODEL_HPP

#include "git.hpp"
#include <eepp/ui/models/model.hpp>
#include <memory>

using namespace EE;
using namespace EE::UI::Models;

namespace ecode {
class GitPlugin;

class GitHistoryModel : public Model {
  public:
	enum class NodeType : uint8_t { Commit, LoadMore, Loading, Error, Empty };
	enum Column { Subject, Date, Author, Hash };
	struct Node {
		Git::Commit commit;
		Git::HistoryQuery query;
		Node* parent{ nullptr };
		std::vector<std::unique_ptr<Node>> children;
		String subject;
		String author;
		String date;
		String hash;
		String tooltip;
		String message;
		String error;
		NodeType type{ NodeType::Commit };
		bool childrenLoaded{ false };
		bool childrenLoading{ false };
		bool retryAppend{ false };
	};

	static std::shared_ptr<GitHistoryModel> asModel( GitPlugin* plugin ) {
		return std::make_shared<GitHistoryModel>( plugin );
	}

	explicit GitHistoryModel( GitPlugin* plugin ) : mPlugin( plugin ) {}

	size_t treeColumn() const { return Subject; }

	size_t rowCount( const ModelIndex& index = {} ) const;

	size_t columnCount( const ModelIndex& = {} ) const { return 4; }

	std::string columnName( const size_t& column ) const;

	ModelIndex parentIndex( const ModelIndex& index ) const;

	ModelIndex index( int row, int column, const ModelIndex& parent = {} ) const;

	Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const;

	bool classModelRoleEnabled() { return true; }

	bool tooltipModelRoleEnabled() { return true; }

	Node* node( const ModelIndex& index ) const;

	ModelIndex indexForNode( const Node* node, int column = 0 ) const;

	ModelIndex indexForCommit( std::string_view hash, int column = Subject ) const;

	void setRootLoading();

	void setRootPage( Git::HistoryPage page, const Git::HistoryQuery& query );

	void setRootError( std::string error );

	void setChildrenLoading( Node* node );

	void setChildrenPage( Node* node, Git::HistoryPage page, const Git::HistoryQuery& query );

	void setChildrenError( Node* node, std::string error, const Git::HistoryQuery& query );

	void setPageLoading( Node* node );

	void setPageError( Node* node, std::string error );

	void appendPage( Node* loadMore, Git::HistoryPage page );

	Git::HistoryQuery mergeQuery( const Node* node, size_t limit ) const;

  private:
	using Nodes = std::vector<std::unique_ptr<Node>>;
	Nodes mRoots;
	GitPlugin* mPlugin{ nullptr };

	Nodes& siblings( Node* parent );

	const Nodes& siblings( const Node* parent ) const;

	std::unique_ptr<Node> commitNode( Git::Commit commit, Node* parent,
									  const Git::HistoryQuery& query ) const;

	void fillPage( Nodes& nodes, Node* parent, Git::HistoryPage page,
				   const Git::HistoryQuery& query );

	void replaceChildren( Node* parent, Nodes children );
};

} // namespace ecode

#endif
