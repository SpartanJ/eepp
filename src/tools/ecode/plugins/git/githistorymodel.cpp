#include "githistorymodel.hpp"
#include "gitplugin.hpp"
#include <algorithm>
#include <eepp/system/sys.hpp>
#include <functional>
#include <iterator>

namespace ecode {

size_t GitHistoryModel::rowCount( const ModelIndex& index ) const {
	if ( !index.isValid() )
		return mRoots.size();
	const Node* item = node( index );
	return item ? item->children.size() : 0;
}

std::string GitHistoryModel::columnName( const size_t& column ) const {
	switch ( column ) {
		case Subject:
			return mPlugin->i18n( "git_history_subject", "Subject" );
		case Author:
			return mPlugin->i18n( "git_history_author", "Author" );
		case Date:
			return mPlugin->i18n( "git_history_date", "Date" );
		case Hash:
			return mPlugin->i18n( "git_history_hash", "Hash" );
	}
	return {};
}

GitHistoryModel::Node* GitHistoryModel::node( const ModelIndex& index ) const {
	return index.isValid() ? static_cast<Node*>( index.internalData() ) : nullptr;
}

const GitHistoryModel::Nodes& GitHistoryModel::siblings( const Node* parent ) const {
	return parent ? parent->children : mRoots;
}

GitHistoryModel::Nodes& GitHistoryModel::siblings( Node* parent ) {
	return parent ? parent->children : mRoots;
}

ModelIndex GitHistoryModel::indexForNode( const Node* item, int column ) const {
	if ( !item )
		return {};
	const auto& items = siblings( item->parent );
	for ( size_t row = 0; row < items.size(); ++row )
		if ( items[row].get() == item )
			return createIndex( row, column, item );
	return {};
}

ModelIndex GitHistoryModel::indexForCommit( std::string_view hash, int column ) const {
	std::function<const Node*( const Nodes& )> find = [&]( const Nodes& nodes ) -> const Node* {
		for ( const auto& item : nodes ) {
			if ( item->type == NodeType::Commit && item->commit.hash == hash )
				return item.get();
			if ( const Node* found = find( item->children ) )
				return found;
		}
		return nullptr;
	};
	return indexForNode( find( mRoots ), column );
}

ModelIndex GitHistoryModel::parentIndex( const ModelIndex& index ) const {
	Node* item = node( index );
	return item ? indexForNode( item->parent ) : ModelIndex{};
}

ModelIndex GitHistoryModel::index( int row, int column, const ModelIndex& parent ) const {
	if ( row < 0 || column < 0 )
		return {};
	const auto& items = siblings( node( parent ) );
	if ( static_cast<size_t>( row ) >= items.size() )
		return {};
	return createIndex( row, column, items[row].get() );
}

Variant GitHistoryModel::data( const ModelIndex& index, ModelRole role ) const {
	const Node* item = node( index );
	if ( !item )
		return {};
	if ( role == ModelRole::Class ) {
		if ( item->type == NodeType::WorkingTree )
			return Variant( "git_history_working_tree" );
		if ( item->type == NodeType::LoadMore || item->type == NodeType::Error )
			return Variant( "git_history_action" );
		if ( item->type != NodeType::Commit && item->type != NodeType::WorkingTree )
			return Variant( "git_history_secondary" );
		if ( item->commit.isMerge() )
			return Variant( "git_history_merge" );
		return Variant( "" );
	}
	if ( role == ModelRole::Tooltip ) {
		if ( item->type == NodeType::Error )
			return Variant( &item->error );
		if ( item->type != NodeType::Commit )
			return Variant( &item->message );
		return Variant( &item->tooltip );
	}
	if ( role != ModelRole::Display )
		return {};
	if ( item->type != NodeType::Commit && item->type != NodeType::WorkingTree )
		return index.column() == Subject ? Variant( &item->message ) : Variant( "" );
	switch ( index.column() ) {
		case Subject:
			return Variant( &item->subject );
		case Author:
			return Variant( &item->author );
		case Date:
			return Variant( &item->date );
		case Hash:
			return Variant( &item->hash );
	}
	return {};
}

std::unique_ptr<GitHistoryModel::Node>
GitHistoryModel::commitNode( Git::Commit commit, Node* parent,
							 const Git::HistoryQuery& query ) const {
	auto item = std::make_unique<Node>();
	item->parent = parent;
	item->commit = std::move( commit );
	item->subject = String::fromUtf8( item->commit.subject );
	item->author = String::fromUtf8( item->commit.authorName );
	item->date = Sys::epochToString( item->commit.commitTime );
	item->hash = String::fromUtf8( item->commit.shortHash );
	item->tooltip = String::format( "%s\n%s <%s>\n%s", item->commit.hash, item->commit.authorName,
									item->commit.authorEmail, item->commit.message );
	item->message = item->date + String{ " · " } + item->author + String{ " · " } + item->hash;
	if ( item->commit.parents.size() == 2 ) {
		item->query = query;
		auto loading = std::make_unique<Node>();
		loading->type = NodeType::Loading;
		loading->parent = item.get();
		loading->message = mPlugin->i18n( "git_history_loading", "Loading..." );
		item->children.emplace_back( std::move( loading ) );
	}
	return item;
}

std::unique_ptr<GitHistoryModel::Node>
GitHistoryModel::workingTreeNode( const Git::Status& status, const std::string& repoName ) const {
	auto repo = status.files.find( repoName );
	if ( repo == status.files.end() || repo->second.empty() )
		return {};
	size_t changed = 0;
	size_t staged = 0;
	for ( const auto& file : repo->second ) {
		if ( file.report.type == Git::GitStatusType::Staged )
			++staged;
		else
			++changed;
	}
	auto item = std::make_unique<Node>();
	item->type = NodeType::WorkingTree;
	item->hasWorkingTreeChanges = changed > 0;
	item->hasStagedChanges = staged > 0;
	item->subject = mPlugin->i18n( "git_working_tree_index", "Working Tree / Index" );
	item->message = String::format(
		mPlugin->i18n( "git_working_tree_summary", "%zu changed, %zu staged" ).toUtf8(), changed,
		staged );
	item->tooltip = item->subject + String{ "\n" } + item->message;
	return item;
}

void GitHistoryModel::fillPage( Nodes& nodes, Node* parent, Git::HistoryPage page,
								const Git::HistoryQuery& query ) {
	for ( auto& commit : page.commits )
		nodes.emplace_back( commitNode( std::move( commit ), parent, query ) );
	if ( page.hasMore && !nodes.empty() && !nodes.back()->commit.parents.empty() ) {
		auto more = std::make_unique<Node>();
		more->type = NodeType::LoadMore;
		more->parent = parent;
		more->message = mPlugin->i18n( "git_history_load_older", "Load older commits" );
		more->query = query;
		more->query.continuation = nodes.back()->commit.parents[0];
		nodes.emplace_back( std::move( more ) );
	}
}

void GitHistoryModel::replaceChildren( Node* parent, Nodes children ) {
	auto& current = parent->children;
	const ModelIndex parentIndex = indexForNode( parent );
	if ( !current.empty() && beginDeleteRows( parentIndex, 0, current.size() - 1 ) ) {
		current.clear();
		endDeleteRows();
	}
	if ( !children.empty() ) {
		beginInsertRows( parentIndex, 0, children.size() - 1 );
		current = std::move( children );
		endInsertRows();
	}
	// Views are not Model::Client instances, so structural notifications alone do not rebuild a
	// UITreeView's flattened rows and scrollable content height.
	invalidate( Model::DontInvalidateIndexes );
}

void GitHistoryModel::setRootLoading() {
	mRoots.clear();
	auto item = std::make_unique<Node>();
	item->type = NodeType::Loading;
	item->message = mPlugin->i18n( "git_history_loading", "Loading..." );
	mRoots.emplace_back( std::move( item ) );
	invalidate();
}

void GitHistoryModel::setRootPage( Git::HistoryPage page, const Git::HistoryQuery& query,
								   const Git::Status& status, const std::string& repoName ) {
	mRoots.clear();
	if ( auto item = workingTreeNode( status, repoName ) )
		mRoots.emplace_back( std::move( item ) );
	fillPage( mRoots, nullptr, std::move( page ), query );
	if ( mRoots.empty() ) {
		auto item = std::make_unique<Node>();
		item->type = NodeType::Empty;
		item->message = mPlugin->i18n( "git_history_empty", "No commits yet" );
		mRoots.emplace_back( std::move( item ) );
	}
	invalidate();
}

void GitHistoryModel::setWorkingTreeStatus( const Git::Status& status,
											const std::string& repoName ) {
	auto item = workingTreeNode( status, repoName );
	const bool hasItem = !mRoots.empty() && mRoots.front()->type == NodeType::WorkingTree;
	if ( hasItem && !item ) {
		beginDeleteRows( {}, 0, 0 );
		mRoots.erase( mRoots.begin() );
		endDeleteRows();
	} else if ( !hasItem && item ) {
		beginInsertRows( {}, 0, 0 );
		mRoots.insert( mRoots.begin(), std::move( item ) );
		endInsertRows();
	} else if ( hasItem && item ) {
		mRoots.front()->message = std::move( item->message );
		mRoots.front()->tooltip = std::move( item->tooltip );
		mRoots.front()->hasWorkingTreeChanges = item->hasWorkingTreeChanges;
		mRoots.front()->hasStagedChanges = item->hasStagedChanges;
	}
	invalidate( Model::DontInvalidateIndexes );
}

void GitHistoryModel::setRootError( std::string error ) {
	mRoots.clear();
	auto item = std::make_unique<Node>();
	item->type = NodeType::Error;
	item->message = mPlugin->i18n( "git_history_error", "Could not load Git history" ) +
					String{ " · " } + mPlugin->i18n( "git_history_retry", "Retry" );
	item->error = error.empty() ? item->message : String::fromUtf8( error );
	mRoots.emplace_back( std::move( item ) );
	invalidate();
}

void GitHistoryModel::setChildrenLoading( Node* item ) {
	if ( !item )
		return;
	item->childrenLoading = true;
	if ( item->children.size() == 1 && ( item->children.front()->type == NodeType::Error ||
										 item->children.front()->type == NodeType::Loading ) ) {
		item->children.front()->type = NodeType::Loading;
		item->children.front()->message = mPlugin->i18n( "git_history_loading", "Loading..." );
		item->children.front()->error.clear();
		invalidate( Model::DontInvalidateIndexes );
	}
}

void GitHistoryModel::setChildrenPage( Node* item, Git::HistoryPage page,
									   const Git::HistoryQuery& query ) {
	if ( !item )
		return;
	Nodes children;
	fillPage( children, item, std::move( page ), query );
	if ( children.empty() ) {
		auto child = std::make_unique<Node>();
		child->type = NodeType::Empty;
		child->parent = item;
		child->message =
			mPlugin->i18n( "git_history_no_additional_commits", "No additional commits" );
		children.emplace_back( std::move( child ) );
	}
	item->childrenLoading = false;
	item->childrenLoaded = true;
	replaceChildren( item, std::move( children ) );
}

void GitHistoryModel::setChildrenError( Node* item, std::string error,
										const Git::HistoryQuery& query ) {
	if ( !item )
		return;
	Nodes children;
	auto child = std::make_unique<Node>();
	child->type = NodeType::Error;
	child->parent = item;
	child->query = query;
	child->message = mPlugin->i18n( "git_history_error", "Could not load Git history" ) +
					 String{ " · " } + mPlugin->i18n( "git_history_retry", "Retry" );
	child->error = error.empty() ? child->message : String::fromUtf8( error );
	children.emplace_back( std::move( child ) );
	item->childrenLoading = false;
	replaceChildren( item, std::move( children ) );
}

void GitHistoryModel::setPageLoading( Node* item ) {
	if ( !item || item->childrenLoading )
		return;
	item->childrenLoading = true;
	// Keep the semantic node type intact: appendPage() must still identify this row as the
	// pagination cursor when the asynchronous request completes.
	item->message = mPlugin->i18n( "git_history_loading", "Loading..." );
	item->error.clear();
	invalidate( Model::DontInvalidateIndexes );
}

void GitHistoryModel::setPageError( Node* item, std::string error ) {
	if ( !item )
		return;
	item->type = NodeType::Error;
	item->childrenLoading = false;
	item->retryAppend = true;
	item->message = mPlugin->i18n( "git_history_error", "Could not load Git history" ) +
					String{ " · " } + mPlugin->i18n( "git_history_retry", "Retry" );
	item->error = error.empty() ? item->message : String::fromUtf8( error );
	invalidate( Model::DontInvalidateIndexes );
}

void GitHistoryModel::appendPage( Node* loadMore, Git::HistoryPage page ) {
	if ( !loadMore || ( loadMore->type != NodeType::LoadMore && !loadMore->retryAppend ) )
		return;
	Node* parent = loadMore->parent;
	auto& items = siblings( parent );
	auto pos = std::find_if( items.begin(), items.end(),
							 [loadMore]( const auto& item ) { return item.get() == loadMore; } );
	if ( pos == items.end() )
		return;
	Git::HistoryQuery query = loadMore->query;
	Nodes additions;
	fillPage( additions, parent, std::move( page ), query );
	const size_t row = std::distance( items.begin(), pos );
	const ModelIndex parentIndex = indexForNode( parent );
	if ( !beginDeleteRows( parentIndex, row, row ) )
		return;
	items.erase( pos );
	endDeleteRows();
	if ( additions.empty() ) {
		invalidate( Model::DontInvalidateIndexes );
		return;
	}
	beginInsertRows( parentIndex, row, row + additions.size() - 1 );
	items.insert( items.begin() + row, std::make_move_iterator( additions.begin() ),
				  std::make_move_iterator( additions.end() ) );
	endInsertRows();
	invalidate( Model::DontInvalidateIndexes );
}

Git::HistoryQuery GitHistoryModel::mergeQuery( const Node* item, size_t limit ) const {
	Git::HistoryQuery query;
	query.limit = limit;
	if ( !item || item->commit.parents.size() < 2 )
		return query;
	query.revision = item->commit.parents[1];
	query.exclusions = item->query.exclusions;
	query.exclusions.emplace_back( item->commit.parents[0] );
	return query;
}

} // namespace ecode
