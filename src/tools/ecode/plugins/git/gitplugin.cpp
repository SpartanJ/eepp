#include "gitplugin.hpp"
#include "../../settingspage.hpp"
#include "gitbranchmodel.hpp"
#include "githistorymodel.hpp"
#include "githistorytreeview.hpp"
#include "gitstatusmodel.hpp"
#include <eepp/graphics/image.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/tools/uidiffview.hpp>
#include <eepp/ui/tools/uimergeview.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uidropdownmodellist.hpp>
#include <eepp/ui/uiiconthememanager.hpp>
#include <eepp/ui/uiloader.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uistackwidget.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>
#include <eepp/window/engine.hpp>
#include <nlohmann/json.hpp>

using namespace EE::UI;
using namespace EE::UI::Doc;

using namespace std::literals;

using json = nlohmann::json;

namespace ecode {

GitHistoryRefModel::GitHistoryRefModel( std::shared_ptr<GitBranchModel> source, String headLabel ) :
	mSource( std::move( source ) ), mHeadLabel( std::move( headLabel ) ) {
	if ( !mSource )
		return;
	for ( size_t groupRow = 0; groupRow < mSource->rowCount( {} ); ++groupRow ) {
		const ModelIndex group = mSource->index( groupRow, GitBranchModel::Name, {} );
		for ( size_t row = 0; row < mSource->rowCount( group ); ++row ) {
			const ModelIndex index = mSource->index( row, GitBranchModel::HistoryDisplay, group );
			if ( mSource->branchRef( index ).type != Git::RefType::Stash )
				mSourceIndexes.emplace_back( index );
		}
	}
}

ModelIndex GitHistoryRefModel::index( int row, int column, const ModelIndex& parent ) const {
	if ( parent.isValid() || row < 0 || static_cast<size_t>( row ) >= rowCount() || column != 0 )
		return {};
	return createIndex( row, column );
}

Variant GitHistoryRefModel::data( const ModelIndex& index, ModelRole role ) const {
	if ( role != ModelRole::Display || !index.isValid() || index.row() < 0 ||
		 static_cast<size_t>( index.row() ) >= rowCount() )
		return {};
	if ( index.row() == 0 )
		return Variant( &mHeadLabel );
	return mSource->data( mSourceIndexes[index.row() - 1], role );
}

std::string GitHistoryRefModel::revision( size_t index ) const {
	if ( index == 0 || index > mSourceIndexes.size() )
		return "HEAD";
	const auto& branch = mSource->branchRef( mSourceIndexes[index - 1] );
	switch ( branch.type ) {
		case Git::RefType::Head:
			return "refs/heads/" + branch.name;
		case Git::RefType::Remote:
			return "refs/remotes/" + branch.name;
		case Git::RefType::Tag:
			return "refs/tags/" + branch.name;
		default:
			return "HEAD";
	}
}

bool GitHistoryRefModel::isRevision( size_t index, std::string_view revision ) const {
	if ( index == 0 || index > mSourceIndexes.size() )
		return revision == "HEAD";
	const auto& branch = mSource->branchRef( mSourceIndexes[index - 1] );
	std::string_view prefix;
	switch ( branch.type ) {
		case Git::RefType::Head:
			prefix = "refs/heads/";
			break;
		case Git::RefType::Remote:
			prefix = "refs/remotes/";
			break;
		case Git::RefType::Tag:
			prefix = "refs/tags/";
			break;
		default:
			return false;
	}
	return revision.size() == prefix.size() + branch.name.size() &&
		   revision.starts_with( prefix ) && revision.substr( prefix.size() ) == branch.name;
}

void GitPlugin::registerSettings( SettingsPage& page ) {
	page.addGroup( i18n( "general", "General" ) );
	page.addText( "ui-refresh-frequency", "/config/ui_refresh_frequency",
				  i18n( "git_ui_refresh_frequency", "Refresh Frequency" ),
				  i18n( "git_ui_refresh_frequency_desc",
						"How often Git information in the interface is refreshed." ),
				  mRefreshFreq.toString(), []( const std::string& text ) {
					  Time value;
					  return SettingsPage::parseNonNegativeSettingsTime( text, value );
				  } );
	page.addBool( "statusbar-display-branch", "/config/statusbar_display_branch",
				  i18n( "git_statusbar_display_branch", "Show Branch in Status Bar" ),
				  i18n( "git_statusbar_display_branch_desc",
						"Display the current Git branch in the status bar." ),
				  true );
	page.addBool( "filetree-highlight-changes", "/config/filetree_highlight_changes",
				  i18n( "git_filetree_highlight_changes", "Highlight File Tree Changes" ),
				  i18n( "git_filetree_highlight_changes_desc",
						"Highlight files with Git changes in the file tree." ),
				  true );
	page.addText( "filetree-highlight-style-color", "/config/filetree_highlight_style_color",
				  i18n( "git_filetree_highlight_style_color", "File Tree Highlight Color" ),
				  i18n( "git_filetree_highlight_style_color_desc",
						"CSS color used to highlight changed files." ),
				  "var(--font-highlight)" );
	page.addBool( "statusbar-display-modifications", "/config/statusbar_display_modifications",
				  i18n( "git_statusbar_display_modifications", "Show Modifications in Status Bar" ),
				  i18n( "git_statusbar_display_modifications_desc",
						"Display repository modification counts in the status bar." ),
				  true );
	page.addBool( "status-recurse-submodules", "/config/status_recurse_submodules",
				  i18n( "git_status_recurse_submodules", "Recurse into Submodules" ),
				  i18n( "git_status_recurse_submodules_desc",
						"Include submodule changes when collecting repository status." ),
				  true );
	page.addBool( "silent", "/config/silent", i18n( "git_silent", "Silent Git Logs" ),
				  i18n( "git_silent_desc", "Hide non-critical Git plugin log messages." ), true );
}

static constexpr auto DEFAULT_HIGHLIGHT_COLOR = "var(--font-highlight)"sv;
static constexpr auto GIT_STATUS_UPDATE_TAG = String::hash( "git::status-update" );

static std::string writeGitBlobTempFile( const std::string& contents,
										 const std::string& sourceFilePath ) {
	if ( contents.empty() )
		return "";

	std::string ext( FileSystem::fileExtension( sourceFilePath ) );
	std::string path( Sys::getTempPath() + ".ecode-git-diff-" + String::randString( 16 ) );
	if ( !ext.empty() )
		path += "." + ext;

	return FileSystem::fileWrite( path, contents ) ? path : "";
}

std::string GitPlugin::statusTypeToString( Git::GitStatusType type ) {
	switch ( type ) {
		case Git::GitStatusType::Untracked:
			return i18n( "git_untracked", "Untracked" );
		case Git::GitStatusType::Unmerged:
			return i18n( "git_conflicts", "Conflicts" );
		case Git::GitStatusType::Changed:
			return i18n( "git_changed", "Changed" );
		case Git::GitStatusType::Staged:
			return i18n( "git_staged", "Staged" );
		case Git::GitStatusType::Ignored:
			return i18n( "git_ignored", "Ignored" );
	}
	return "";
}

std::vector<std::string> GitPlugin::repos() {
	Lock l( mReposMutex );
	std::vector<std::string> ret;
	for ( const auto& repo : mRepos )
		ret.push_back( repo.first );
	return ret;
}

Plugin* GitPlugin::New( PluginManager* pluginManager ) {
	return eeNew( GitPlugin, ( pluginManager, false ) );
}

Plugin* GitPlugin::NewSync( PluginManager* pluginManager ) {
	return eeNew( GitPlugin, ( pluginManager, true ) );
}

GitPlugin::GitPlugin( PluginManager* pluginManager, bool sync ) :
	PluginBase( pluginManager ),
	mLifetime( this, getUISceneNode() ),
	mHighlightStyleColor( DEFAULT_HIGHLIGHT_COLOR ) {
	if ( sync ) {
		load( pluginManager );
	} else {
		mThreadPool->run( [this, pluginManager] { load( pluginManager ); } );
	}
}

GitPlugin::~GitPlugin() {
	mLifetime.invalidate();
	waitUntilLoaded();
	mShuttingDown = true;
	mConflictViewCloseConnection.disconnect();
	mConflictView = nullptr;
	mConflictSessions.clear();
	if ( mStatusButton )
		mStatusButton->close();

	if ( mSidePanel && mTab )
		mSidePanel->removeTab( mTab );

	endModelStyler();

	if ( getUISceneNode() )
		getUISceneNode()->removeActionsByTag( GIT_STATUS_UPDATE_TAG );

	if ( mStatusBar && mRepositionCbId )
		mStatusBar->removeEventListener( mRepositionCbId );

	{
		Lock l( mGitBranchMutex );
	}
	{
		Lock l( mGitStatusMutex );
	}
	{
		Lock l( mRepoMutex );
	}
	{
		Lock l( mReposMutex );
	}

	// TODO: Add a signal for these waits
	while ( mRunningUpdateStatus )
		Sys::sleep( Milliseconds( 1.f ) );

	while ( mRunningUpdateBranches )
		Sys::sleep( Milliseconds( 1.f ) );

	while ( mRunningHistoryRequests )
		Sys::sleep( Milliseconds( 1.f ) );

	while ( *mRunningAsyncTasks )
		Sys::sleep( Milliseconds( 1.f ) );
}

void GitPlugin::runAsyncTask( std::function<void()> task ) {
	auto runningTasks = mRunningAsyncTasks;
	++*runningTasks;
	mThreadPool->run( std::move( task ),
					  [runningTasks = std::move( runningTasks )]( auto ) { --*runningTasks; } );
}

void GitPlugin::load( PluginManager* pluginManager ) {
	Clock clock;
	AtomicBoolScopedOp loading( mLoading, true );
	pluginManager->subscribeMessages( this,
									  [this]( const auto& notification ) -> PluginRequestHandle {
										  return processMessage( notification );
									  } );

	std::string path = pluginManager->getPluginsPath() + "git.json";
	if ( FileSystem::fileExists( path ) ||
		 FileSystem::fileWrite( path, "{\n  \"config\":{},\n  \"keybindings\":{}\n}\n" ) ) {
		mConfigPath = path;
	}
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return;
	mConfigHash = String::hash( data );

	json j;
	try {
		j = json::parse( data, nullptr, true, true );
	} catch ( const json::exception& e ) {
		Log::error( "GitPlugin::load - Error parsing config from path %s, error: %s, config "
					"file content:\n%s",
					path.c_str(), e.what(), data.c_str() );
		// Recreate it
		j = json::parse( "{\n  \"config\":{},\n  \"keybindings\":{},\n}\n", nullptr, true, true );
	}

	bool updateConfigFile = false;

	if ( j.contains( "config" ) ) {
		auto& config = j["config"];

		if ( config.contains( "ui_refresh_frequency" ) )
			mRefreshFreq = Time::fromString( config.value( "ui_refresh_frequency", "5s" ) );
		else {
			config["ui_refresh_frequency"] = mRefreshFreq.toString();
			updateConfigFile = true;
		}

		if ( config.contains( "statusbar_display_branch" ) )
			mStatusBarDisplayBranch = config.value( "statusbar_display_branch", true );
		else {
			config["statusbar_display_branch"] = mStatusBarDisplayBranch;
			updateConfigFile = true;
		}

		if ( config.contains( "filetree_highlight_changes" ) )
			mFileTreeHighlightChanges = config.value( "filetree_highlight_changes", true );
		else {
			config["filetree_highlight_changes"] = mFileTreeHighlightChanges;
			updateConfigFile = true;
		}

		if ( config.contains( "filetree_highlight_style_color" ) ) {
			mHighlightStyleColor =
				config.value( "filetree_highlight_style_color", DEFAULT_HIGHLIGHT_COLOR );
		} else {
			config["filetree_highlight_style_color"] = mHighlightStyleColor;
			updateConfigFile = true;
		}

		if ( config.contains( "statusbar_display_modifications" ) )
			mStatusBarDisplayModifications =
				config.value( "statusbar_display_modifications", true );
		else {
			config["statusbar_display_modifications"] = mStatusBarDisplayModifications;
			updateConfigFile = true;
		}

		if ( config.contains( "status_recurse_submodules" ) )
			mStatusRecurseSubmodules = config.value( "status_recurse_submodules", true );
		else {
			config["status_recurse_submodules"] = mStatusRecurseSubmodules;
			updateConfigFile = true;
		}

		if ( config.contains( "silent" ) )
			mSilent = config.value( "silent", true );
		else {
			config["silent"] = mSilent;
			updateConfigFile = true;
		}
	}

	if ( mKeyBindings.empty() ) {
		mKeyBindings["git-blame"] = "alt+shift+b";
	}

	if ( j.contains( "keybindings" ) ) {
		auto& kb = j["keybindings"];
		auto list = { "git-blame" };
		for ( const auto& key : list ) {
			if ( kb.contains( key ) ) {
				if ( !kb[key].empty() )
					mKeyBindings[key] = kb[key];
			} else {
				kb[key] = mKeyBindings[key];
				updateConfigFile = true;
			}
		}
	}

	if ( updateConfigFile ) {
		std::string newData = j.dump( 2 );
		if ( newData != data ) {
			FileSystem::fileWrite( path, newData );
			mConfigHash = String::hash( newData );
		}
	}

	mGit = std::make_shared<Git>( pluginManager->getWorkspaceFolder() );
	mGit->setSilent( mSilent );
	mGitFound = !mGit->getGitPath().empty();
	mProjectPath = mRepoSelected = mGit->getProjectPath();

	if ( getUISceneNode() ) {
		mLifetime.setDispatcher( getUISceneNode() );
		initModelStyler();
		updateStatus();
		updateBranches();
	}

	subscribeFileSystemListener();
	mReady = true;
	fireReadyCbs();
	setReady( clock.getElapsedTime() );
}

void GitPlugin::initModelStyler() {
	if ( !mFileTreeHighlightChanges )
		return;

	auto projectView = getUISceneNode()->getRoot()->find<UITreeView>( "project_view" );
	if ( !projectView || !projectView->getModel() )
		return;

	if ( mModelChangedId == 0 ) {
		mModelChangedId =
			projectView->on( Event::OnModelChanged, [this]( auto ) { initModelStyler(); } );
	}

	mModelStylerId = projectView->getModel()->subscribeModelStyler(
		[this]( const ModelIndex& index, const void* data ) -> Variant {
			static const char* STYLE_MODIFIED = "git_highlight_style";
			static const char* STYLE_NONE = "git_highlight_style_clear";
			auto model = static_cast<const FileSystemModel*>( index.model() );
			auto node = static_cast<const FileSystemModel::Node*>( data );
			Lock l( mGitStatusFileCacheMutex );
			std::string_view nodePath = model->getNodeRelativePath( node );
			auto found = std::find_if( mGitStatusFilesCache.begin(), mGitStatusFilesCache.end(),
									   [&nodePath]( const std::string& key ) {
										   return std::string_view{ key } == nodePath;
									   } );
			if ( found != mGitStatusFilesCache.end() )
				return Variant( STYLE_MODIFIED );
			return Variant( STYLE_NONE );
		} );
}

void GitPlugin::endModelStyler() {
	if ( !mFileTreeHighlightChanges || mModelStylerId == 0 || !SceneManager::existsSingleton() ||
		 SceneManager::instance()->isShuttingDown() )
		return;

	auto projectView = getUISceneNode()->getRoot()->find<UITreeView>( "project_view" );
	if ( !projectView )
		return;

	if ( mModelChangedId ) {
		projectView->removeEventListener( mModelChangedId );
		mModelChangedId = 0;
	}

	if ( projectView->getModel() ) {
		projectView->getModel()->unsubscribeModelStyler( mModelStylerId );
		mModelStylerId = 0;
	}
}

void GitPlugin::updateUINow( bool force ) {
	if ( !mGit || !getUISceneNode() )
		return;
	if ( !mProjectPath.empty() )
		mLifetime.weakHandle().run( []( GitPlugin* plugin ) { plugin->buildSidePanelTab(); } );

	updateStatus( force );
	updateBranches();
}

void GitPlugin::updateUI() {
	if ( !mGit || !getUISceneNode() )
		return;

	const auto lifetime = mLifetime.weakHandle();
	getUISceneNode()->debounce(
		[lifetime] { lifetime.run( []( GitPlugin* plugin ) { plugin->updateUINow(); } ); },
		mRefreshFreq, GIT_STATUS_UPDATE_TAG );
}

void GitPlugin::updateStatusBarSync() {
	buildSidePanelTab();

	mGitContentView->setVisible( !mGit->getGitFolder().empty() )
		->setEnabled( !mGit->getGitFolder().empty() );
	mGitNoContentView->setVisible( !mGitContentView->isVisible() )
		->setEnabled( !mGitContentView->isEnabled() );

	if ( !mGit->getGitFolder().empty() ) {
		{
			Lock l( mGitStatusMutex );
			mStatusTree->setModel( GitStatusModel::asModel( mGitStatus.files, this ) );
		}
		mStatusTree->expandAll();
	} else {
		return;
	}

	if ( mConflictStateBar && mConflictStateText ) {
		auto* conflictSession = activeConflictSession();
		const bool hasState =
			conflictSession && ( !conflictSession->files.empty() ||
								 conflictSession->operation != Git::GitOperation::None );
		mConflictStateBar->setVisible( hasState );
		if ( hasState ) {
			String operation;
			switch ( conflictSession->operation ) {
				case Git::GitOperation::Merge:
					operation = i18n( "git_operation_merge", "Merging" );
					break;
				case Git::GitOperation::Rebase:
					operation = i18n( "git_operation_rebase", "Rebasing" );
					break;
				case Git::GitOperation::CherryPick:
					operation = i18n( "git_operation_cherry_pick", "Cherry-picking" );
					break;
				case Git::GitOperation::Revert:
					operation = i18n( "git_operation_revert", "Reverting" );
					break;
				case Git::GitOperation::StashApply:
					operation = i18n( "git_operation_stash_apply", "Applying stash" );
					break;
				case Git::GitOperation::None:
					operation = i18n( "git_conflicts", "Conflicts" );
					break;
			}
			mConflictStateText->setText(
				conflictSession->files.empty()
					? String::format(
						  ( conflictSession->operation == Git::GitOperation::Merge
								? i18n( "git_operation_ready_commit", "%s · Ready to commit" )
								: i18n( "git_operation_ready", "%s · Ready to continue" ) )
							  .toUtf8(),
						  operation.toUtf8().c_str() )
					: String::format(
						  ( conflictSession->files.size() == 1
								? i18n( "git_operation_conflict", "%s · %d conflict" )
								: i18n( "git_operation_conflicts", "%s · %d conflicts" ) )
							  .toUtf8(),
						  operation.toUtf8().c_str(),
						  static_cast<int>( conflictSession->files.size() ) ) );
			const bool canContinue = conflictSession->files.empty() &&
									 conflictSession->operation != Git::GitOperation::None &&
									 conflictSession->operation != Git::GitOperation::StashApply;
			auto* continueButton =
				mTabContents->find( "git_conflict_continue" )->asType<UIPushButton>();
			continueButton->setText( conflictSession->operation == Git::GitOperation::Merge
										 ? i18n( "git_commit_merge", "Commit" )
										 : i18n( "git_continue_operation", "Continue" ) );
			if ( auto* icon = getUISceneNode()->findIcon(
					 conflictSession->operation == Git::GitOperation::Merge ? "git-commit"
																			: "debug-continue" ) )
				continueButton->setIcon( icon->createDrawable( PixelDensity::dpToPxI( 12 ) ) );
			continueButton->setEnabled( canContinue );
			mTabContents->find( "git_conflict_abort" )
				->setEnabled( conflictSession->operation != Git::GitOperation::None &&
							  conflictSession->operation != Git::GitOperation::StashApply );
		}
	}

	if ( !mStatusBarDisplayBranch )
		return;

	if ( !mStatusBar )
		getUISceneNode()->bind( "status_bar", mStatusBar );
	if ( !mStatusBar )
		return;

	if ( !mStatusButton ) {
		mStatusButton = UIPushButton::New();
		mStatusButton->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::MatchParent );
		mStatusButton->setParent( mStatusBar );
		mStatusButton->setId( "git_status" );
		mStatusButton->setClass( "status_but" );
		mStatusButton->setIcon( iconDrawable( "source-control", 10 ) );
		mStatusButton->reloadStyle( true, true );
		mStatusButton->getTextView()->setUsingCustomStyling( true );

		mStatusButton->on( Event::MouseClick, [this]( const Event* event ) {
			if ( nullptr == mTab )
				return;
			mTab->setTabSelected();
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_RMASK )
				mPanelSwicher->getListBox()->setSelected( 0 );
			else if ( mGitStatus.totalInserts || mGitStatus.totalDeletions )
				mPanelSwicher->getListBox()->setSelected( 1 );
		} );

		const auto reposition = [this] {
			if ( mStatusBar == nullptr )
				return;
			if ( mStatusButton->getNextNode() == nullptr ||
				 mStatusButton->getNextNode()->getId() != "doc_info" ) {
				auto docInfo = mStatusBar->find( "doc_info" );
				if ( docInfo != nullptr && docInfo->getParent() == mStatusButton->getParent() ) {
					mStatusButton->toPosition( docInfo->getNodeIndex() );
				}
			}
		};

		reposition();

		mRepositionCbId =
			mStatusBar->on( Event::OnChildCountChanged, [reposition]( auto ) { reposition(); } );
	}

	mStatusButton->setVisible( !mGit->getGitFolder().empty() );

	if ( mGit->getGitFolder().empty() )
		return;

	std::string text;
	{
		Lock l( mGitStatusMutex );
		text = mStatusBarDisplayModifications &&
					   ( mGitStatus.totalInserts || mGitStatus.totalDeletions )
				   ? String::format( "%s (+%d / -%d)", gitBranch().c_str(), mGitStatus.totalInserts,
									 mGitStatus.totalDeletions )
				   : gitBranch();
	}
	mStatusButton->setText( text );

	if ( !mStatusBarDisplayModifications )
		return;

	if ( !mStatusCustomTokenizer.has_value() ) {
		std::vector<SyntaxPattern> patterns;
		auto fontColor( getVarColor( "--font" ) );
		auto insertedColor( getVarColor( "--theme-success" ) );
		auto deletedColor( getVarColor( "--theme-error" ) );
		patterns.emplace_back(
			SyntaxPattern( { ".*%((%+%d+)%s/%s(%-%d+)%)" }, { "normal", "keyword", "type" } ) );
		SyntaxDefinition syntaxDef( "custom_build", {}, std::move( patterns ) );
		SyntaxColorScheme scheme( "status_bar_git",
								  { { "normal"_sst, { fontColor } },
									{ "keyword"_sst, { insertedColor } },
									{ "type"_sst, { deletedColor } } },
								  {} );
		mStatusCustomTokenizer = { std::move( syntaxDef ), std::move( scheme ) };
	}

	SyntaxTokenizer::tokenizeText( mStatusCustomTokenizer->def, mStatusCustomTokenizer->scheme,
								   mStatusButton->getTextView()->getTextCache() );

	mStatusButton->invalidateDraw();
}

void GitPlugin::updateStatus( bool force ) {
	if ( !mGit || !mGitFound )
		return;
	if ( mRunningUpdateStatus ) {
		if ( force )
			mPendingForcedStatusUpdate = true;
		return;
	}

	if ( !mGit || mGit->getGitFolder().empty() ) {
		mLifetime.weakHandle().run( []( GitPlugin* plugin ) { plugin->updateStatusBarSync(); } );
		return;
	}

	mRunningUpdateStatus++;
	UnorderedSet<std::string> sessionRepos;
	for ( const auto& [repo, session] : mConflictSessions )
		if ( session )
			sessionRepos.insert( repo );
	const std::string selectedRepo = repoSelected();
	const auto lifetime = mLifetime.weakHandle();
	mThreadPool->run(
		[this, lifetime, force, selectedRepo, sessionRepos = std::move( sessionRepos )] {
			if ( !mGit || mGit->getGitFolder().empty() ) {
				lifetime.run( []( GitPlugin* plugin ) { plugin->updateStatusBarSync(); } );
				return;
			}

			auto prevBranch = updateReposBranches();
			Git::Status prevGitStatus;
			{
				Lock l( mGitStatusMutex );
				prevGitStatus = mGitStatus;
			}
			Git::Status newGitStatus = mGit->status( mStatusRecurseSubmodules );
			UnorderedSet<std::string> conflictRepos;
			for ( const auto& repo : newGitStatus.files ) {
				for ( const auto& file : repo.second )
					if ( file.report.type == Git::GitStatusType::Unmerged )
						conflictRepos.insert( mGit->repoPath( file.file ) );
			}
			if ( !selectedRepo.empty() )
				conflictRepos.insert( selectedRepo );
			conflictRepos.insert( sessionRepos.begin(), sessionRepos.end() );
			UnorderedMap<std::string, Git::ConflictState> conflictStates;
			for ( const auto& repo : conflictRepos )
				conflictStates.emplace( repo, mGit->conflictState( repo, false ) );
			for ( auto state = conflictStates.begin(); state != conflictStates.end(); ) {
				if ( !state->second.hasConflicts() &&
					 state->second.operation == Git::GitOperation::None &&
					 sessionRepos.find( state->first ) == sessionRepos.end() )
					state = conflictStates.erase( state );
				else
					++state;
			}
			UnorderedSet<std::string> cache;

			for ( const auto& status : newGitStatus.files ) {
				for ( const auto& file : status.second ) {
					std::string p( FileSystem::fileRemoveFileName( file.file ) );
					std::string lp;
					while ( p != lp ) {
						cache.insert( p );
						lp = p;
						p = FileSystem::removeLastFolderFromPath( p );
					}
					cache.insert( file.file );
				}
			}

			{
				Lock l( mGitStatusFileCacheMutex );
				mGitStatusFilesCache = std::move( cache );
			}

			{
				Lock l( mGitStatusMutex );
				mGitStatus = std::move( newGitStatus );
				if ( !force && conflictStates.empty() && mGitBranches == prevBranch &&
					 mGitStatus == prevGitStatus )
					return;
			}

			lifetime.run(
				[conflictStates = std::move( conflictStates )]( GitPlugin* plugin ) mutable {
					const bool selectStatusPanel = plugin->updateConflictSessions( conflictStates );
					plugin->updateStatusBarSync();
					if ( selectStatusPanel && plugin->mPanelSwicher )
						plugin->mPanelSwicher->getListBox()->setSelected( 1 );
				} );
		},
		[this, lifetime]( auto ) {
			--mRunningUpdateStatus;
			if ( mPendingForcedStatusUpdate.exchange( false ) )
				lifetime.run( []( GitPlugin* plugin ) { plugin->updateStatus( true ); } );
		} );
}

PluginRequestHandle GitPlugin::processMessage( const PluginMessage& msg ) {
	switch ( msg.type ) {
		case PluginMessageType::WorkspaceFolderChanged: {
			if ( mGit ) {
				{
					Lock l( mGitStatusMutex );
					mGitStatus = {};
				}
				if ( getUISceneNode() ) {
					mLifetime.weakHandle().run( []( GitPlugin* plugin ) {
						plugin->mConflictViewCloseConnection.disconnect();
						plugin->mConflictView = nullptr;
						plugin->mConflictSessions.clear();
						plugin->mActiveConflictRepo.clear();
						++plugin->mConflictGeneration;
						if ( plugin->mManager && plugin->mManager->getSplitter() )
							plugin->mManager->getSplitter()->removeTabWithOwnedWidgetId(
								"git_conflict_resolver" );
						if ( plugin->mPanelSwicher )
							plugin->mPanelSwicher->getListBox()->setSelected( 0 );
						if ( plugin->mStackWidget && !plugin->mStackMap.empty() )
							plugin->mStackWidget->setActiveWidget( plugin->mStackMap[0] );
					} );
				}
				mGit->setProjectPath( msg.asJSON()["folder"] );

				{
					Lock l( mGitBranchMutex );
					mGitBranches.clear();
				}

				{
					Lock l( mRepoMutex );
					mProjectPath = mRepoSelected = mGit->getProjectPath();
				}

				{
					Lock l( mReposMutex );
					mRepos.clear();
				}

				if ( getUISceneNode() && mSidePanel ) {
					mLifetime.weakHandle().run( []( GitPlugin* plugin ) {
						if ( plugin->mProjectPath.empty() ) {
							plugin->hideSidePanel();
						}
					} );
				}

				updateUINow( true );
				mInitialized = true;

				if ( mModelStylerId == 0 )
					initModelStyler();
			}
			break;
		}
		case ecode::PluginMessageType::UIReady: {
			mLifetime.setDispatcher( getUISceneNode() );
			if ( !mInitialized )
				updateUINow();
			if ( mModelStylerId == 0 )
				initModelStyler();
			break;
		}
		case ecode::PluginMessageType::UIThemeReloaded: {
			mStatusCustomTokenizer.reset();
			updateUINow( true );
			break;
		}
		default:
			break;
	}
	return PluginRequestHandle::empty();
}

void GitPlugin::onFileSystemEvent( const FileEvent& ev, const FileInfo& file ) {
	PluginBase::onFileSystemEvent( ev, file );

	if ( mShuttingDown || isLoading() )
		return;

	if ( file.isDirectory() )
		return;

	auto inGitFolder = file.getFilepath().find( "/.git/" ) != std::string::npos;
#if EE_PLATFORM == EE_PLATFORM_WIN
	inGitFolder |= file.getFilepath().find( "\\.git\\" ) != std::string::npos;
#endif

	if ( inGitFolder && file.getExtension() == "lock" )
		return;

	updateUI();
}

FileSystemListenerOptions GitPlugin::getFileSystemListenerOptions() const {
	auto options = Plugin::getFileSystemListenerOptions();
	const auto& workspace = getManager()->getWorkspaceFolder();
	if ( !workspace.empty() ) {
		FileSystemListenerFilter filter;
		filter.path = workspace;
		options.filters.emplace_back( std::move( filter ) );
	}
	return options;
}

void GitPlugin::displayTooltip( UICodeEditor* editor, const Git::Blame& blame,
								const Vector2f& position ) {
	// HACK: Gets the old font style to restore it when the tooltip is hidden
	UITooltip* tooltip = editor->createTooltip();
	if ( tooltip == nullptr )
		return;

	String str( blame.error.empty()
					? String::format( "%s: %s (%s)\n%s: %s (%s)\n%s: %s\n\n%s",
									  i18n( "commit", "Commit" ).toUtf8().c_str(),
									  blame.commitHash.c_str(), blame.commitShortHash.c_str(),
									  i18n( "author", "Author" ).toUtf8().c_str(),
									  blame.author.c_str(), blame.authorEmail.c_str(),
									  i18n( "date", "Date" ).toUtf8().c_str(), blame.date.c_str(),
									  blame.commitMessage.c_str() )
					: blame.error );

	Text::hardWrapText( str, PixelDensity::dpToPx( 400 ), tooltip->getFontStyleConfig(),
						editor->getTabWidth() );

	editor->setTooltipText( str );

	mTooltipInfoShowing = true;
	mOldBackgroundColor = tooltip->getBackgroundColor();
	if ( Color::Transparent == mOldBackgroundColor ) {
		tooltip->reloadStyle( true, true, true, true );
		mOldBackgroundColor = tooltip->getBackgroundColor();
	}
	mOldTextStyle = tooltip->getFontStyle();
	mOldTextAlign = tooltip->getHorizontalAlign();
	mOldDontAutoHideOnMouseMove = tooltip->dontAutoHideOnMouseMove();
	mOldUsingCustomStyling = tooltip->getUsingCustomStyling();
	tooltip->setHorizontalAlign( UI_HALIGN_LEFT );
	tooltip->setPixelsPosition( tooltip->getTooltipPosition( position ) );
	tooltip->setDontAutoHideOnMouseMove( true );
	tooltip->setUsingCustomStyling( true );
	tooltip->setData( String::hash( "git" ) );
	tooltip->setBackgroundColor( editor->getColorScheme().getEditorColor( "background"_sst ) );
	tooltip->getUIStyle()->setStyleSheetProperty( StyleSheetProperty(
		"background-color",
		editor->getColorScheme().getEditorColor( "background"_sst ).toHexString(), true,
		StyleSheetSelectorRule::SpecificityImportant ) );

	if ( !mTooltipCustomSyntaxDef.has_value() ) {
		static std::vector<SyntaxPattern> patterns;

		patterns.emplace_back( SyntaxPattern( { "([%w:]+)%s(%x+)%s%((%x+)%)" },
											  { "normal", "keyword", "number", "number" } ) );
		patterns.emplace_back( SyntaxPattern( { "([%w:]+)%s(.*)%(([%w%+%.-]+@[%w%.-]+%.%w+)%)" },
											  { "normal", "keyword", "function", "link" } ) );
		patterns.emplace_back( SyntaxPattern( { "([%w:]+)%s(%d%d%d%d%-%d%d%-%d%d[%s%d%-+:]+)" },
											  { "normal", "keyword", "warning" } ) );
		SyntaxDefinition syntaxDef( "custom_build", {}, std::move( patterns ) );
		mTooltipCustomSyntaxDef = std::move( syntaxDef );
	}

	SyntaxTokenizer::tokenizeText( *mTooltipCustomSyntaxDef, editor->getColorScheme(),
								   tooltip->getTextCache() );

	tooltip->notifyTextChangedFromTextCache();

	if ( editor->hasFocus() && !tooltip->isVisible() )
		tooltip->show();
}

void GitPlugin::hideTooltip( UICodeEditor* editor ) {
	mTooltipInfoShowing = false;
	UITooltip* tooltip = nullptr;
	if ( editor && ( tooltip = editor->getTooltip() ) && tooltip->isVisible() &&
		 tooltip->getData() == String::hash( "git" ) ) {
		editor->setTooltipText( "" );
		tooltip->hide();
		// Restore old tooltip state
		tooltip->setData( 0 );
		tooltip->setFontStyle( mOldTextStyle );
		tooltip->setHorizontalAlign( mOldTextAlign );
		tooltip->setUsingCustomStyling( mOldUsingCustomStyling );
		tooltip->setDontAutoHideOnMouseMove( mOldDontAutoHideOnMouseMove );
		tooltip->setBackgroundColor( mOldBackgroundColor );
	}
}

bool GitPlugin::onMouseLeave( UICodeEditor* editor, const Vector2i&, const Uint32& ) {
	hideTooltip( editor );
	return false;
}

std::string GitPlugin::gitBranch() {
	std::string repoSel = repoSelected();
	Lock l( mGitBranchMutex );
	return mGitBranches[repoSel];
}

void GitPlugin::onRegisterListeners( UICodeEditor* editor, std::vector<Uint32>& listeners ) {
	listeners.push_back( editor->on( Event::OnCursorPosChange, [this, editor]( const Event* ) {
		if ( mTooltipInfoShowing )
			hideTooltip( editor );
	} ) );
}

Color GitPlugin::getVarColor( const std::string& var ) {
	return Color::fromString(
		getUISceneNode()->getRoot()->getUIStyle()->getVariable( var ).getValue() );
}

void GitPlugin::blame( UICodeEditor* editor ) {
	if ( !mGitFound ) {
		editor->setTooltipText(
			i18n( "git_not_found",
				  "Git binary not found.\nPlease check that git is accessible via PATH" ) );
		return;
	}
	mThreadPool->run( [this, editor]() {
		auto blame = mGit->blame( editor->getDocument().getFilePath(),
								  editor->getDocument().getSelection().start().line() + 1 );
		editor->runOnMainThread( [this, editor, blame] {
			displayTooltip(
				editor, blame,
				editor->getScreenPosition( editor->getDocument().getSelection().start() )
					.getPosition() );
		} );
	} );
}

// Branch operations

void GitPlugin::checkout( Git::Branch branch ) {
	if ( !mGit )
		return;

	const auto lifetime = mLifetime.weakHandle();
	const auto git = mGit;
	const std::string repo = repoSelected();
	const auto checkOutFn = [this, branch, lifetime, git, repo]( bool createLocal ) {
		mLoader->setVisible( true );
		mThreadPool->run( [branch, createLocal, lifetime, git, repo] {
			auto result = createLocal ? git->checkoutAndCreateLocalBranch( branch.name, "", repo )
									  : git->checkout( branch.name, repo );
			lifetime.run(
				[branch, createLocal, repo, result = std::move( result )]( GitPlugin* plugin ) {
					if ( result.success() ) {
						{
							Lock l( plugin->mGitBranchMutex );
							plugin->mGitBranches[repo] = branch.name;
						}
						if ( plugin->mBranchesTree->getModel() ) {
							if ( createLocal )
								plugin->updateBranches();
							else
								plugin->mBranchesTree->getModel()->invalidate(
									Model::DontInvalidateIndexes );
						}
						plugin->invalidateHistory();
					} else {
						plugin->showMessage( LSPMessageType::Warning, result.result );
					}
					plugin->mLoader->setVisible( false );
				} );
		} );
	};

	if ( branch.type == Git::RefType::Remote ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::YES_NO, i18n( "git_create_local_branch", "Create local branch?" ) );
		msgBox->on( Event::OnConfirm, [checkOutFn]( const Event* ) { checkOutFn( true ); } );
		msgBox->on( Event::OnDiscard, [checkOutFn]( const Event* ) { checkOutFn( false ); } );
		msgBox->setTitle( i18n( "git_checkout", "Check Out" ) );
		msgBox->center();
		msgBox->showWhenReady();
		return;
	}

	checkOutFn( false );
}

void GitPlugin::branchRename( Git::Branch branch ) {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT,
		String::format(
			i18n( "git_rename_branch_ask", "Enter the new name for the branch: '%s'" ).toUtf8(),
			branch.name ) );
	msgBox->on( Event::OnConfirm, [this, branch, msgBox]( const Event* ) {
		std::string newName( msgBox->getTextInput()->getText().toUtf8() );
		if ( newName.empty() || branch.name == newName )
			return;
		msgBox->closeWindow();
		runAsync(
			[this, branch, newName]() {
				return mGit->renameBranch( branch.name, newName, repoSelected() );
			},
			false, true );
	} );
	msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	msgBox->setTitle( i18n( "git_rename_branch", "Rename Branch" ) );
	msgBox->center();
	msgBox->getTextInput()->setText( branch.name );
	msgBox->showWhenReady();
}

void GitPlugin::branchDelete( Git::Branch branch ) {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::OK_CANCEL,
		String::format( i18n( "git_confirm_branch_delete",
							  "Are you sure you want to delete the branch \"%s\"?" )
							.toUtf8(),
						branch.name ) );

	msgBox->on( Event::OnConfirm, [this, branch]( auto ) {
		runAsync( [this, branch]() { return mGit->deleteBranch( branch.name, repoSelected() ); },
				  false, true );
	} );
	msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	msgBox->setTitle( i18n( "git_confirm", "Confirm" ) );
	msgBox->center();
	msgBox->showWhenReady();
}

void GitPlugin::branchMerge( Git::Branch branch ) {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::OK_CANCEL,
		String::format(
			i18n( "git_confirm_branch_merge", "Are you sure you want to merge from branch \"%s\"?" )
				.toUtf8(),
			branch.name ) );

	msgBox->on( Event::OnConfirm, [this, branch]( auto ) {
		const std::string repoPath = repoSelected();
		runMergeLikeAsync(
			[branch, repoPath]( Git& git ) {
				return git.mergeBranch( branch.name, false, repoPath );
			},
			repoPath );
	} );
	msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	msgBox->setTitle( i18n( "git_confirm", "Confirm" ) );
	msgBox->center();
	msgBox->showWhenReady();
}

void GitPlugin::pull( const std::string& repoPath ) {
	runMergeLikeAsync( [repoPath]( Git& git ) { return git.pull( repoPath ); }, repoPath );
}

void GitPlugin::push( const std::string& repoPath ) {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::OK_CANCEL,
		i18n( "git_confirm_push_changes",
			  "Are you sure you want to push the local changes to the remote server?" ) );

	msgBox->on( Event::OnConfirm, [this, repoPath]( auto ) {
		runAsync(
			[this, repoPath]() {
				std::optional<Git::Branch> branch = getBranchFromRepoPath( repoPath );
				bool pushNewBranch = branch && !branch->name.empty() && branch->remote.empty();
				if ( pushNewBranch )
					return mGit->pushNewBranch( branch->name, repoPath );
				return mGit->push( repoPath );
			},
			true, true, true, true );
	} );
	msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	msgBox->setTitle( i18n( "git_confirm", "Confirm" ) );
	msgBox->center();
	msgBox->showWhenReady();
}

void GitPlugin::fetch( const std::string& repoPath ) {
	runAsync( [this, repoPath]() { return mGit->fetch( repoPath ); }, true, true, true );
}

void GitPlugin::branchCreate() {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT,
		i18n( "git_create_branch_ask",
			  "Create new branch at current branch (HEAD).\nEnter the name for the branch:" ) );
	msgBox->on( Event::OnConfirm, [this, msgBox]( const Event* ) {
		std::string newName( msgBox->getTextInput()->getText().toUtf8() );
		if ( newName.empty() )
			return;
		msgBox->closeWindow();
		runAsync( [this, newName]() { return mGit->createBranch( newName, true, repoSelected() ); },
				  false, true );
	} );
	msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	msgBox->setTitle( i18n( "git_add_branch", "Add Branch" ) );
	msgBox->center();
	msgBox->showWhenReady();
}

void GitPlugin::commit( const std::string& repoPath, bool mergeCommit ) {
	if ( !mergeCommit && !mGitStatus.hasStagedChanges( mGit->repoName( repoPath, true ) ) ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::OK, i18n( "git_nothing_to_commit", "Nothing to Commit" ) );
		msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
		msgBox->setTitle( i18n( "git_nothing_to_commit", "Nothing to Commit" ) );
		msgBox->center();
		msgBox->showWhenReady();
		return;
	}

	UIMessageBox* msgBox = UIMessageBox::New( UIMessageBox::TEXT_EDIT,
											  i18n( "git_commit_message", "Commit Message:" ) );

	UITextEdit* txtEdit = msgBox->getTextEdit();
	txtEdit->setLineWrapType( LineWrapType::Viewport );
	txtEdit->setLineWrapMode( LineWrapMode::Word );
	txtEdit->setText( mLastCommitMsg );

	UICheckBox* chkAmend = UICheckBox::New();
	chkAmend->setLayoutMargin( Rectf( 0, 8, 0, 0 ) )
		->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent )
		->setLayoutGravity( UI_HALIGN_LEFT | UI_VALIGN_CENTER )
		->setClipType( ClipType::None )
		->setParent( msgBox->getLayoutCont()->getFirstChild() )
		->setId( "git-amend" );
	chkAmend->setText( i18n( "git_amend", "Amend last commit" ) );
	chkAmend->setEnabled( !mergeCommit );
	chkAmend->toPosition( 2 );
	chkAmend->setTooltipText( getUISceneNode()->getKeyBindings().getShortcutString(
		{ KEY_A, KeyMod::getDefaultModifier() }, true ) );

	UICheckBox* chkBypassHook = UICheckBox::New();
	chkBypassHook->setLayoutMargin( Rectf( 0, 8, 0, 0 ) )
		->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent )
		->setLayoutGravity( UI_HALIGN_LEFT | UI_VALIGN_CENTER )
		->setClipType( ClipType::None )
		->setParent( msgBox->getLayoutCont()->getFirstChild() )
		->setId( "git-bypass-hook" );
	chkBypassHook->setText( i18n( "git_bypass_hook", "Bypass commit hook" ) );
	chkBypassHook->toPosition( 3 );
	chkBypassHook->setTooltipText( getUISceneNode()->getKeyBindings().getShortcutString(
		{ KEY_B, KeyMod::getDefaultModifier() }, true ) );

	UICheckBox* chkPush = UICheckBox::New();
	chkPush->setLayoutMargin( Rectf( 0, 8, 0, 0 ) )
		->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent )
		->setLayoutGravity( UI_HALIGN_LEFT | UI_VALIGN_CENTER )
		->setClipType( ClipType::None )
		->setParent( msgBox->getLayoutCont()->getFirstChild() )
		->setId( "git-push-commit" );
	chkPush->setText( i18n( "git_push_commit", "Push commit" ) );
	chkPush->toPosition( 4 );
	chkPush->setTooltipText( getUISceneNode()->getKeyBindings().getShortcutString(
		{ KEY_P, KeyMod::getDefaultModifier() }, true ) );

	if ( !mergeCommit ) {
		txtEdit->getDocument().setCommand(
			"commit-amend", [chkAmend] { chkAmend->setChecked( !chkAmend->isChecked() ); } );
		txtEdit->getKeyBindings().addKeybind( { KEY_L, KeyMod::getDefaultModifier() },
											  "commit-amend" );
	}

	txtEdit->getDocument().setCommand(
		"commit-push", [chkPush] { chkPush->setChecked( !chkPush->isChecked() ); } );
	txtEdit->getKeyBindings().addKeybind( { KEY_P, KeyMod::getDefaultModifier() }, "commit-push" );

	txtEdit->getDocument().setCommand( "commit-bypass-hook", [chkBypassHook] {
		chkBypassHook->setChecked( !chkBypassHook->isChecked() );
	} );
	txtEdit->getKeyBindings().addKeybind( { KEY_B, KeyMod::getDefaultModifier() },
										  "commit-bypass-hook" );

	msgBox->on( Event::OnConfirm, [this, msgBox, chkAmend, chkBypassHook, chkPush, repoPath,
								   mergeCommit]( const Event* ) {
		std::string msg( msgBox->getTextEdit()->getText().toUtf8() );
		if ( msg.empty() )
			return;
		bool amend = chkAmend->isChecked();
		bool bypassHook = chkBypassHook->isChecked();
		bool pushCommit = chkPush->isChecked();
		std::optional<Git::Branch> branch = getBranchFromRepoPath( repoPath );
		auto git = mGit;
		const auto lifetime = mLifetime.weakHandle();

		msgBox->closeWindow();
		runAsync(
			[git = std::move( git ), lifetime, branch = std::move( branch ), msg, amend, bypassHook,
			 pushCommit, repoPath, mergeCommit]() {
				bool pushNewBranch = branch && !branch->name.empty() && branch->remote.empty();
				auto res = git->commit( msg, amend, bypassHook, repoPath, mergeCommit );
				if ( res.success() ) {
					lifetime.run( [mergeCommit, repoPath]( GitPlugin* plugin ) {
						plugin->mLastCommitMsg.clear();
						plugin->invalidateHistory();
						if ( mergeCommit ) {
							++plugin->mConflictGeneration;
							plugin->mConflictSessions.erase( repoPath );
							if ( plugin->mActiveConflictRepo == repoPath ) {
								plugin->mActiveConflictRepo.clear();
								plugin->mManager->getSplitter()->removeTabWithOwnedWidgetId(
									"git_conflict_resolver" );
							}
						}
					} );
					if ( pushCommit ) {
						if ( pushNewBranch )
							return git->pushNewBranch( branch->name, repoPath );
						return git->push( repoPath );
					}
				} else {
					lifetime.run( [msg]( GitPlugin* plugin ) { plugin->mLastCommitMsg = msg; } );
				}
				return res;
			},
			true, true, true, true, true );
	} );

	msgBox->on( Event::OnDiscard, [this, msgBox]( const Event* ) {
		mLastCommitMsg = msgBox->getTextEdit()->getText();
	} );

	msgBox->on( Event::OnVisibleChange, [msgBox, txtEdit]( const Event* ) {
		if ( !msgBox->isVisible() )
			return;

		msgBox->getLayoutCont()->setLayoutSizePolicy( SizePolicy::MatchParent,
													  SizePolicy::MatchParent );

		msgBox->getLayoutCont()->getFirstChild()->asType<UIWidget>()->setLayoutSizePolicy(
			SizePolicy::MatchParent, SizePolicy::MatchParent );

		txtEdit->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::Fixed );
		txtEdit->setLayoutWeight( 1 );
	} );

	msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	msgBox->setTitle( i18n( "git_commit", "Commit" ) );
	msgBox->center();
	msgBox->showWhenReady();
}

void GitPlugin::fastForwardMerge( Git::Branch branch ) {
	runAsync(
		[this, branch]() {
			if ( branch.name == gitBranch() ) {
				auto res = mGit->fastForwardMerge( repoSelected() );
				if ( res.success() )
					return res;
				return mGit->mergeBranch( "", true, repoSelected() );
			}

			auto remoteBranch = mGit->getAllBranchesAndTags(
				Git::RefType::Remote, "refs/remotes/" + branch.remote, repoSelected() );
			if ( remoteBranch.empty() )
				return Git::Result{ "", -1 };

			return mGit->updateRef( branch.name, remoteBranch[0].lastCommit, repoSelected() );
		},
		false, true, false, false, false, true );
}

// Branch operations

// File operations

static bool isPath( const std::string& file ) {
	bool ret = !file.empty() && file[0] == '/';
#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( !ret )
		ret = LuaPattern::hasMatches( file, "%w:[\\/][\\/]" );
#endif
	return ret;
}

std::string GitPlugin::fixFilePath( const std::string& file ) {
	std::string path;
	if ( !isPath( file ) ) {
		path = ( mProjectPath + file );
	}
	return path;
}

std::vector<std::string> GitPlugin::fixFilePaths( const std::vector<std::string>& files ) {
	std::vector<std::string> paths;
	paths.reserve( files.size() );
	for ( const auto& file : files ) {
		if ( !isPath( file ) ) {
			paths.push_back( mProjectPath + file );
		} else {
			paths.push_back( file );
		}
	}
	return paths;
}

std::optional<Git::Branch> GitPlugin::getBranchFromRepoPath( const std::string& repoPath ) {
	Git::Branch branch;
	std::string branchName;

	if ( repoPath.empty() )
		return {};

	{
		Lock l( mGitBranchMutex );
		branchName = mGitBranches[repoPath];
	}

	if ( branchName.empty() )
		return {};

	if ( repoPath != repoSelected() || !mBranchesTree->getModel() ) {
		auto branches =
			mGit->getAllBranchesAndTags( Git::RefType::Head, "refs/heads/" + branchName, repoPath );
		if ( !branches.empty() )
			return branches.front();
	} else {
		auto modelShared = mBranchesTree->getModelShared();
		auto model = static_cast<const GitBranchModel*>( modelShared.get() );
		return model->branch( branchName );
	}

	return {};
}

void GitPlugin::stage( const std::vector<std::string>& files ) {
	runFileOperation( files, FileOperation::Stage );
}

void GitPlugin::unstage( const std::vector<std::string>& files ) {
	runFileOperation( files, FileOperation::Unstage );
}

void GitPlugin::runFileOperation( std::vector<std::string> files, FileOperation operation ) {
	if ( files.empty() )
		return;
	runAsync(
		[this, files = std::move( files ), operation]() {
			std::map<std::string, std::vector<std::string>> filesByRepo;
			for ( const auto& file : files )
				filesByRepo[mGit->repoPath( file )].emplace_back( file );

			Git::Result result;
			for ( auto& [repoPath, repoFiles] : filesByRepo ) {
				auto paths = fixFilePaths( repoFiles );
				switch ( operation ) {
					case FileOperation::Stage:
						result = mGit->add( paths, repoPath );
						break;
					case FileOperation::Unstage:
						result = mGit->reset( paths, repoPath );
						break;
					case FileOperation::Discard:
						result = mGit->restore( paths, repoPath );
						break;
				}
				if ( result.fail() )
					return result;
			}
			return result;
		},
		true, operation == FileOperation::Discard );
}

void GitPlugin::discard( const std::vector<std::string>& files ) {
	if ( files.empty() )
		return;
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::OK_CANCEL,
		i18n( "git_confirm_discard_changes", "Are you sure you want to discard all file changes?" )
			.toUtf8() );

	msgBox->on( Event::OnConfirm,
				[this, files]( auto ) { runFileOperation( files, FileOperation::Discard ); } );
	msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	msgBox->setTitle( i18n( "git_confirm", "Confirm" ) );
	msgBox->center();
	msgBox->showWhenReady();
}

void GitPlugin::discard( const std::string& file ) {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::OK_CANCEL,
		String::format( i18n( "git_confirm_discard_changes",
							  "Are you sure you want to discard the changes in file: \"%s\"?" )
							.toUtf8(),
						file ) );

	msgBox->on( Event::OnConfirm, [this, file]( auto ) {
		runAsync(
			[this, file]() { return mGit->restore( fixFilePath( file ), mGit->repoPath( file ) ); },
			true, true );
	} );
	msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	msgBox->setTitle( i18n( "git_confirm", "Confirm" ) );
	msgBox->center();
	msgBox->showWhenReady();
}

void GitPlugin::openFile( const std::string& file ) {
	mLifetime.weakHandle().run( [file]( GitPlugin* plugin ) {
		plugin->mManager->getLoadFileFn()( plugin->mGit->getProjectPath() + file,
										   []( auto, auto ) {} );
	} );
}

GitPlugin::GitConflictSession* GitPlugin::conflictSession( const std::string& repoPath ) {
	auto session = mConflictSessions.find( repoPath );
	return session != mConflictSessions.end() ? session->second.get() : nullptr;
}

GitPlugin::GitConflictSession* GitPlugin::activeConflictSession() {
	if ( auto* session = conflictSession( mActiveConflictRepo ) )
		return session;
	if ( mConflictSessions.empty() )
		return nullptr;
	mActiveConflictRepo = mConflictSessions.begin()->first;
	return mConflictSessions.begin()->second.get();
}

bool GitPlugin::updateConflictSessions(
	UnorderedMap<std::string, Git::ConflictState>& conflictStates ) {
	bool selectStatusPanel = false;
	for ( auto& [repo, conflictState] : conflictStates ) {
		if ( !conflictState.error.empty() )
			continue;
		if ( !conflictState.hasConflicts() && conflictState.operation == Git::GitOperation::None ) {
			const bool wasActive = mActiveConflictRepo == repo;
			++mConflictGeneration;
			mConflictSessions.erase( repo );
			if ( wasActive ) {
				mActiveConflictRepo.clear();
				mConflictViewCloseConnection.disconnect();
				mConflictView = nullptr;
				mManager->getSplitter()->removeTabWithOwnedWidgetId( "git_conflict_resolver" );
			}
			continue;
		}
		std::vector<std::string> files;
		files.reserve( conflictState.files.size() );
		for ( auto& conflict : conflictState.files )
			files.emplace_back( std::move( conflict.path ) );
		auto& session = mConflictSessions[repo];
		const bool stateChanged =
			session && ( session->files != files || session->operation != conflictState.operation );
		if ( !session ) {
			session = std::make_unique<GitConflictSession>();
			selectStatusPanel = true;
		} else if ( session->files.empty() && !files.empty() ) {
			selectStatusPanel = true;
		}
		session->repoPath = repo;
		session->files = std::move( files );
		session->operation = conflictState.operation;
		if ( session->generation == 0 || stateChanged )
			session->generation = ++mConflictGeneration;
		if ( session->currentFile >= session->files.size() )
			session->currentFile = 0;
		if ( mActiveConflictRepo.empty() )
			mActiveConflictRepo = repo;
	}
	if ( mActiveConflictRepo.empty() && !mConflictSessions.empty() )
		mActiveConflictRepo = mConflictSessions.begin()->first;
	return selectStatusPanel;
}

void GitPlugin::loadConflictResolverView( std::shared_ptr<TextDocument> resultDocument,
										  Git::ConflictFile conflict, std::string repository,
										  std::vector<std::string> files, size_t currentFile,
										  Git::GitOperation operation, Uint64 generation ) {
	auto* session = conflictSession( repository );
	if ( !resultDocument )
		return;
	if ( mShuttingDown || mActiveConflictRepo != repository || !session ||
		 generation != session->generation )
		return;
	auto toVersion = []( const std::optional<Git::ConflictStage>& stage, String label ) {
		MergeVersion version;
		version.label = std::move( label );
		if ( stage ) {
			version.present = true;
			version.text = String::fromUtf8( stage->contents );
			version.objectId = stage->objectId;
			version.mode = stage->mode;
		}
		return version;
	};
	String stage2Label;
	String stage3Label;
	switch ( operation ) {
		case Git::GitOperation::Merge:
			stage2Label = i18n( "git_merge_current_branch", "Current branch (ours)" );
			stage3Label = i18n( "git_merge_incoming_branch", "Incoming branch (theirs)" );
			break;
		case Git::GitOperation::Rebase:
			stage2Label = i18n( "git_merge_upstream", "Upstream" );
			stage3Label = i18n( "git_merge_replayed_commit", "Replayed commit (your change)" );
			break;
		case Git::GitOperation::None:
		case Git::GitOperation::CherryPick:
		case Git::GitOperation::Revert:
		case Git::GitOperation::StashApply:
			stage2Label = i18n( "git_merge_ours_stage", "Ours (stage 2)" );
			stage3Label = i18n( "git_merge_theirs_stage", "Theirs (stage 3)" );
			break;
	}
	MergeInput input;
	input.path = conflict.path;
	input.base = toVersion( conflict.base, i18n( "git_merge_base", "Base" ) );
	input.stage2 = toVersion( conflict.stage2, std::move( stage2Label ) );
	input.stage3 = toVersion( conflict.stage3, std::move( stage3Label ) );
	input.resultDocument = resultDocument;
	input.resultLabel = i18n( "git_merge_result", "Result" );
	input.missingVersionLabel = i18n( "git_merge_not_present", "Not present" );
	auto* mergeView = mConflictView && mManager->getSplitter()->ownedWidgetExists( mConflictView )
						  ? mConflictView
						  : UIMergeView::New();
	mergeView->load( std::move( input ) );
	mergeView->setRecreateConflictCallback( [this] { recreateConflict(); } );
	const String continueText = operation == Git::GitOperation::Merge
									? i18n( "git_commit_merge", "Commit" )
									: i18n( "git_continue_operation", "Continue" );
	const bool newView = mergeView != mConflictView;
	session->files = std::move( files );
	session->currentFile = currentFile;
	session->generation = generation;
	session->operation = operation;
	mConflictView = mergeView;
	if ( newView ) {
		const Uint32 defaultModifier = KeyMod::getDefaultModifier();
		const Uint32 abortModifier = defaultModifier | KeyMod::getDefaultSecondaryModifier();
		mergeView->setId( "git_conflict_resolver" );
		mergeView->addToolbarAction( "git-conflict-save-stage",
									 i18n( "git_save_and_stage", "Save & Stage" ),
									 { KEY_S, defaultModifier }, "git-branch-staged-changes",
									 [this] { saveAndStageConflict(); } );
		mergeView->addToolbarAction(
			"git-conflict-previous-file", i18n( "git_previous_conflict_file", "Previous File" ),
			{ KEY_PAGEUP, KeyMod::getDefaultSecondaryModifier() }, "arrow-circle-left",
			[this] { openAdjacentConflict( false ); } );
		mergeView->addToolbarAction(
			"git-conflict-next-file", i18n( "git_next_conflict_file", "Next File" ),
			{ KEY_PAGEDOWN, KeyMod::getDefaultSecondaryModifier() }, "arrow-circle-right",
			[this] { openAdjacentConflict( true ); } );
		mergeView->addToolbarAction(
			"git-conflict-continue", continueText, { KEY_RETURN, defaultModifier },
			operation == Git::GitOperation::Merge ? "git-commit" : "debug-continue",
			[this] { continueConflictOperation(); } );
		mergeView->addToolbarAction( "git-conflict-abort", i18n( "git_abort_operation", "Abort" ),
									 { KEY_A, abortModifier }, "discard",
									 [this] { abortConflictOperation(); } );
		mConflictViewCloseConnection =
			mergeView->connect( Event::OnClose, [this, mergeView]( const Event* ) {
				if ( mConflictView == mergeView ) {
					mConflictView = nullptr;
					if ( auto* session = activeConflictSession() )
						session->generation = ++mConflictGeneration;
				}
			} );
		mManager->getSplitter()->createWidget(
			mergeView,
			String::format( i18n( "git_merge_tab", "Merge: %s" ).toUtf8(),
							resultDocument->getFilename() ),
			true );
	} else {
		if ( auto* continueButton = mergeView->find<UIPushButton>( "git-conflict-continue" ) ) {
			continueButton->setText( continueText );
			if ( auto* icon = getUISceneNode()->findIcon(
					 operation == Git::GitOperation::Merge ? "git-commit" : "debug-continue" ) )
				continueButton->setIcon( icon->createDrawable( PixelDensity::dpToPxI( 12 ) ) );
		}
		auto tabs = mManager->getSplitter()->getTabFromOwnedWidgetId( mergeView->getId() );
		if ( !tabs.empty() ) {
			tabs.front().first->setText( String::format(
				i18n( "git_merge_tab", "Merge: %s" ).toUtf8(), resultDocument->getFilename() ) );
			tabs.front().second->setTabSelected( tabs.front().first );
		}
	}
}

void GitPlugin::openConflictResolver( const std::string& file ) {
	const std::string repository = mGit->repoPath( file );
	std::string absolutePath = mGit->getProjectPath() + file;
	std::string relativePath = absolutePath;
	FileSystem::filePathRemoveBasePath( repository, relativePath );
	const Uint64 generation = ++mConflictGeneration;
	auto& session = mConflictSessions[repository];
	if ( !session )
		session = std::make_unique<GitConflictSession>();
	session->repoPath = repository;
	session->generation = generation;
	mActiveConflictRepo = repository;
	auto git = mGit;
	const auto lifetime = mLifetime.weakHandle();
	const std::string stateErrorMessage =
		i18n( "git_conflict_state_failed", "Unable to read Git's conflict state." ).toUtf8();
	runAsyncTask( [git = std::move( git ), lifetime, repository,
				   relativePath = std::move( relativePath ),
				   absolutePath = std::move( absolutePath ), generation, stateErrorMessage] {
		auto state = git->conflictState( repository );
		if ( !state.error.empty() ) {
			lifetime.run( [stateErrorMessage]( GitPlugin* plugin ) {
				plugin->showMessage( LSPMessageType::Error, stateErrorMessage );
			} );
			return;
		}
		auto found = std::find_if(
			state.files.begin(), state.files.end(),
			[&relativePath]( const auto& conflict ) { return conflict.path == relativePath; } );
		if ( found == state.files.end() ) {
			lifetime.run( []( GitPlugin* plugin ) { plugin->updateStatus( true ); } );
			return;
		}
		const size_t currentFile = std::distance( state.files.begin(), found );
		std::vector<std::string> files;
		files.reserve( state.files.size() );
		for ( const auto& conflictFile : state.files )
			files.emplace_back( conflictFile.path );
		Git::ConflictFile conflict = std::move( state.files[currentFile] );
		lifetime.run( [lifetime, conflict = std::move( conflict ),
					   absolutePath = std::move( absolutePath ), repository,
					   files = std::move( files ), currentFile, operation = state.operation,
					   generation]( GitPlugin* plugin ) mutable {
			const auto* session = plugin->conflictSession( repository );
			if ( plugin->mShuttingDown || plugin->mActiveConflictRepo != repository || !session ||
				 generation != session->generation )
				return;
			if ( !conflict.workingTreeExists ) {
				plugin->showMessage(
					LSPMessageType::Warning,
					plugin
						->i18n(
							"git_conflict_missing_result",
							"This conflict requires choosing whether to keep or delete the file." )
						.toUtf8() );
				return;
			}
			auto openView = [plugin, conflict = std::move( conflict ), repository,
							 files = std::move( files ), currentFile, operation,
							 generation]( std::shared_ptr<TextDocument> document ) mutable {
				plugin->loadConflictResolverView( std::move( document ), std::move( conflict ),
												  std::move( repository ), std::move( files ),
												  currentFile, operation, generation );
			};
			if ( auto* tab = plugin->mManager->getSplitter()->isDocumentOpen( absolutePath, false,
																			  true ) ) {
				openView( tab->getOwnedWidget()->asType<UICodeEditor>()->getDocumentRef() );
				return;
			}
			auto document = std::make_shared<TextDocument>();
			document->loadAsyncFromFile(
				absolutePath, plugin->mThreadPool,
				[lifetime, document, openView = std::move( openView )]( TextDocument*,
																		bool success ) mutable {
					lifetime.run( [document, openView = std::move( openView ),
								   success]( GitPlugin* ) mutable {
						if ( success )
							openView( std::move( document ) );
					} );
				} );
		} );
	} );
}

void GitPlugin::recreateConflict() {
	auto* session = activeConflictSession();
	if ( !session || !mConflictView ||
		 !mManager->getSplitter()->ownedWidgetExists( mConflictView ) )
		return;
	const MergeInput& input = mConflictView->getInput();
	Git::ConflictFile conflict;
	conflict.path = input.path;
	const auto toStage = []( const MergeVersion& version,
							 Uint8 stage ) -> std::optional<Git::ConflictStage> {
		if ( !version.present )
			return std::nullopt;
		return Git::ConflictStage{ version.objectId, {}, version.mode, stage };
	};
	conflict.base = toStage( input.base, 1 );
	conflict.stage2 = toStage( input.stage2, 2 );
	conflict.stage3 = toStage( input.stage3, 3 );
	const std::string repo = session->repoPath;
	const std::string path = conflict.path;
	const Uint64 generation = session->generation;
	auto* view = mConflictView;
	auto git = mGit;
	const auto lifetime = mLifetime.weakHandle();
	runAsyncTask( [git = std::move( git ), lifetime, repo, path, conflict = std::move( conflict ),
				   generation, view]() mutable {
		auto result = git->restoreConflictStages( conflict, repo );
		auto state = git->conflictState( repo, false );
		lifetime.run( [result = std::move( result ), state = std::move( state ), repo, path,
					   generation, view]( GitPlugin* plugin ) mutable {
			auto* session = plugin->conflictSession( repo );
			if ( plugin->mShuttingDown || plugin->mActiveConflictRepo != repo || !session ||
				 generation != session->generation || plugin->mConflictView != view ||
				 !plugin->mManager->getSplitter()->ownedWidgetExists( view ) )
				return;
			auto restored = std::find_if(
				state.files.begin(), state.files.end(),
				[&path]( const Git::ConflictFile& file ) { return file.path == path; } );
			if ( result.fail() || restored == state.files.end() ) {
				plugin->showMessage(
					LSPMessageType::Error,
					result.result.empty()
						? plugin
							  ->i18n( "git_conflict_recreate_failed",
									  "Unable to restore the conflict in Git's index." )
							  .toUtf8()
						: result.result );
				return;
			}
			session->files.clear();
			session->files.reserve( state.files.size() );
			for ( const auto& file : state.files )
				session->files.emplace_back( file.path );
			session->currentFile =
				static_cast<size_t>( std::distance( state.files.begin(), restored ) );
			session->operation = state.operation;
			view->recreateConflict();
			if ( !view->getResultEditor()->getDocument().save() ) {
				plugin->showMessage( LSPMessageType::Warning,
									 plugin
										 ->i18n( "git_conflict_recreate_save_failed",
												 "The conflict was restored in Git's index, but "
												 "its marker text could not "
												 "be saved to disk." )
										 .toUtf8() );
			}
			plugin->updateStatus( true );
		} );
	} );
}

void GitPlugin::saveAndStageConflict() {
	auto* session = activeConflictSession();
	if ( !session || !mConflictView || session->currentFile >= session->files.size() )
		return;
	auto& document = mConflictView->getResultEditor()->getDocument();
	if ( !document.save() ) {
		showMessage(
			LSPMessageType::Error,
			i18n( "git_conflict_save_failed", "Unable to save the conflict result." ).toUtf8() );
		return;
	}
	if ( mConflictView->hasUnresolvedMarkerBlocks() )
		showMessage( LSPMessageType::Warning,
					 i18n( "git_conflict_markers_remain",
						   "The saved result still contains recognizable conflict markers." )
						 .toUtf8() );
	const std::string repo = session->repoPath;
	const std::string path = session->files[session->currentFile];
	const Uint64 generation = session->generation;
	auto git = mGit;
	const auto lifetime = mLifetime.weakHandle();
	runAsyncTask( [git = std::move( git ), lifetime, repo, path, generation] {
		auto result = git->resolveConflict( path, false, repo );
		auto state = git->conflictState( repo );
		lifetime.run( [result = std::move( result ), state = std::move( state ), repo, path,
					   generation]( GitPlugin* plugin ) mutable {
			auto* session = plugin->conflictSession( repo );
			if ( plugin->mShuttingDown || !session || generation != session->generation )
				return;
			if ( !state.error.empty() ) {
				plugin->showMessage( LSPMessageType::Error,
									 plugin
										 ->i18n( "git_conflict_state_failed",
												 "Unable to read Git's conflict state." )
										 .toUtf8() );
				return;
			}
			const bool unresolved =
				std::any_of( state.files.begin(), state.files.end(),
							 [&path]( const auto& conflict ) { return conflict.path == path; } );
			if ( result.fail() || unresolved ) {
				plugin->showMessage( LSPMessageType::Error,
									 result.result.empty()
										 ? plugin
											   ->i18n( "git_conflict_still_unmerged",
													   "Git still reports this path as unmerged." )
											   .toUtf8()
										 : result.result );
				return;
			}
			session->files.clear();
			session->files.reserve( state.files.size() );
			for ( const auto& conflict : state.files )
				session->files.emplace_back( conflict.path );
			session->currentFile = 0;
			session->operation = state.operation;
			session->generation = ++plugin->mConflictGeneration;
			if ( !state.files.empty() ) {
				std::string next = session->repoPath + state.files.front().path;
				FileSystem::filePathRemoveBasePath( plugin->mGit->getProjectPath(), next );
				plugin->openConflictResolver( next );
			}
			plugin->updateStatus( true );
		} );
	} );
}

void GitPlugin::openAdjacentConflict( bool next ) {
	auto* session = activeConflictSession();
	if ( !session || session->files.empty() )
		return;
	const size_t count = session->files.size();
	const size_t index =
		next ? ( session->currentFile + 1 ) % count : ( session->currentFile + count - 1 ) % count;
	std::string file = session->repoPath + session->files[index];
	FileSystem::filePathRemoveBasePath( mGit->getProjectPath(), file );
	openConflictResolver( file );
}

void GitPlugin::acceptConflictSide( const std::string& file, bool stage2 ) {
	const std::string repository = mGit->repoPath( file );
	std::string path = mGit->getProjectPath() + file;
	FileSystem::filePathRemoveBasePath( repository, path );
	const std::string noLongerUnmerged =
		i18n( "git_conflict_no_longer_unmerged", "Git no longer reports this path as unmerged." )
			.toUtf8();
	const std::string stateErrorMessage =
		i18n( "git_conflict_state_failed", "Unable to read Git's conflict state." ).toUtf8();
	mLoader->setVisible( true );
	auto git = mGit;
	const auto lifetime = mLifetime.weakHandle();
	runAsyncTask( [git = std::move( git ), lifetime, repository, path, stage2, noLongerUnmerged,
				   stateErrorMessage] {
		auto state = git->conflictState( repository );
		auto found =
			std::find_if( state.files.begin(), state.files.end(),
						  [&path]( const auto& conflict ) { return conflict.path == path; } );
		Git::Result result;
		if ( !state.error.empty() ) {
			result.returnCode = EXIT_FAILURE;
			result.result = stateErrorMessage;
		} else if ( found == state.files.end() ) {
			result.returnCode = EXIT_FAILURE;
			result.result = noLongerUnmerged;
		} else {
			const bool present = stage2 ? found->stage2.has_value() : found->stage3.has_value();
			result = git->acceptConflictStage( path, stage2, present, repository );
		}
		lifetime.run( [result = std::move( result )]( GitPlugin* plugin ) mutable {
			plugin->mLoader->setVisible( false );
			if ( plugin->mShuttingDown )
				return;
			if ( result.fail() )
				plugin->showMessage( LSPMessageType::Warning, result.result );
			plugin->updateStatus( true );
		} );
	} );
}

void GitPlugin::continueConflictOperation() {
	auto* session = activeConflictSession();
	if ( !session )
		return;
	const std::string repo = session->repoPath;
	const auto operation = session->operation;
	const Uint64 generation = session->generation;
	const std::string unresolvedMessage =
		i18n( "git_resolve_before_continue", "Resolve and stage all conflicts before continuing." )
			.toUtf8();
	const std::string stateErrorMessage =
		i18n( "git_conflict_state_failed", "Unable to read Git's conflict state." ).toUtf8();
	if ( operation == Git::GitOperation::Merge ) {
		mLoader->setVisible( true );
		auto git = mGit;
		const auto lifetime = mLifetime.weakHandle();
		runAsyncTask( [git = std::move( git ), lifetime, repo, generation, unresolvedMessage,
					   stateErrorMessage] {
			auto state = git->conflictState( repo, false );
			Git::Result message;
			if ( !state.error.empty() ) {
				message.returnCode = EXIT_FAILURE;
				message.result = stateErrorMessage;
			} else if ( state.hasConflicts() ) {
				message.returnCode = EXIT_FAILURE;
				message.result = unresolvedMessage;
			} else if ( state.operation != Git::GitOperation::Merge ) {
				message.returnCode = EXIT_FAILURE;
			} else {
				message = git->preparedMergeMessage( repo );
			}
			lifetime.run(
				[message = std::move( message ), repo, generation]( GitPlugin* plugin ) mutable {
					plugin->mLoader->setVisible( false );
					auto* session = plugin->conflictSession( repo );
					if ( plugin->mShuttingDown || !session || generation != session->generation )
						return;
					if ( message.fail() ) {
						plugin->showMessage(
							LSPMessageType::Warning,
							message.result.empty()
								? plugin
									  ->i18n( "git_no_continuable_operation",
											  "No continuable Git operation is active." )
									  .toUtf8()
								: message.result );
						return;
					}
					plugin->mLastCommitMsg = String::fromUtf8( message.result );
					plugin->commit( repo, true );
				} );
		} );
		return;
	}
	mLoader->setVisible( true );
	auto git = mGit;
	const auto lifetime = mLifetime.weakHandle();
	runAsyncTask( [git = std::move( git ), lifetime, repo, operation, generation, unresolvedMessage,
				   stateErrorMessage] {
		auto state = git->conflictState( repo );
		Git::Result result;
		if ( !state.error.empty() ) {
			result.returnCode = EXIT_FAILURE;
			result.result = stateErrorMessage;
		} else if ( state.hasConflicts() ) {
			result.returnCode = EXIT_FAILURE;
			result.result = unresolvedMessage;
		} else {
			result = git->continueOperation( operation, repo );
		}
		lifetime.run( [result = std::move( result ), repo,
					   generation]( GitPlugin* plugin ) mutable {
			plugin->mLoader->setVisible( false );
			auto* session = plugin->conflictSession( repo );
			if ( plugin->mShuttingDown || !session || generation != session->generation )
				return;
			if ( result.fail() ) {
				plugin->showMessage( LSPMessageType::Warning,
									 result.result.empty()
										 ? plugin
											   ->i18n( "git_no_continuable_operation",
													   "No continuable Git operation is active." )
											   .toUtf8()
										 : result.result );
				return;
			}
			++plugin->mConflictGeneration;
			plugin->mConflictSessions.erase( repo );
			if ( plugin->mActiveConflictRepo == repo ) {
				plugin->mActiveConflictRepo.clear();
				plugin->mManager->getSplitter()->removeTabWithOwnedWidgetId(
					"git_conflict_resolver" );
			}
			plugin->updateBranches();
			plugin->updateStatus( true );
		} );
	} );
}

void GitPlugin::abortConflictOperation() {
	if ( !activeConflictSession() )
		return;
	auto* message =
		UIMessageBox::New( UIMessageBox::OK_CANCEL, i18n( "git_confirm_abort_operation",
														  "Abort the current Git operation?" ) );
	message->setTitle( i18n( "git_confirm", "Confirm" ) );
	message->on( Event::OnConfirm, [this, message]( const Event* ) {
		message->closeWindow();
		auto* session = activeConflictSession();
		if ( !session )
			return;
		const std::string repo = session->repoPath;
		const auto operation = session->operation;
		const Uint64 generation = session->generation;
		mLoader->setVisible( true );
		auto git = mGit;
		const auto lifetime = mLifetime.weakHandle();
		runAsyncTask( [git = std::move( git ), lifetime, repo, operation, generation] {
			auto result = git->abortOperation( operation, repo );
			lifetime.run( [result = std::move( result ), repo,
						   generation]( GitPlugin* plugin ) mutable {
				plugin->mLoader->setVisible( false );
				auto* session = plugin->conflictSession( repo );
				if ( plugin->mShuttingDown || !session || generation != session->generation )
					return;
				if ( result.fail() ) {
					plugin->showMessage( LSPMessageType::Warning,
										 result.result.empty()
											 ? plugin
												   ->i18n( "git_no_abortable_operation",
														   "No abortable Git operation is active." )
												   .toUtf8()
											 : result.result );
					return;
				}
				++plugin->mConflictGeneration;
				plugin->mConflictSessions.erase( repo );
				if ( plugin->mActiveConflictRepo == repo ) {
					plugin->mActiveConflictRepo.clear();
					plugin->mManager->getSplitter()->removeTabWithOwnedWidgetId(
						"git_conflict_resolver" );
				}
				plugin->updateBranches();
				plugin->updateStatus( true );
			} );
		} );
	} );
	message->center();
	message->showWhenReady();
}

void GitPlugin::diff( const Git::DiffMode mode, const std::string& repoPath ) {
	const auto lifetime = mLifetime.weakHandle();
	mThreadPool->run( [this, mode, repoPath, lifetime] {
		auto res = mGit->diff( mode, repoPath );
		if ( res.fail() )
			return;

		lifetime.run( [mode, res = std::move( res ), repoPath]( GitPlugin* plugin ) {
			std::string modeName;
			switch ( mode ) {
				case Git::DiffHead: {
					modeName = "HEAD";
					break;
				}
				case Git::DiffStaged:
					modeName = "staged";
					break;
			}
			plugin->getPluginContext()->loadDiffFromMemory(
				res.result, UIDiffView::isMultiFileDiff( res.result ) ? modeName : "", "",
				repoPath );
		} );
	} );
}

void GitPlugin::diff( const std::string& file, Git::GitStatusType status ) {
	const auto lifetime = mLifetime.weakHandle();
	mThreadPool->run( [this, file, status, lifetime] {
		auto filePath = fixFilePath( file );
		if ( status == Git::GitStatusType::Untracked ) {
			lifetime.run( [filePath = std::move( filePath )]( GitPlugin* plugin ) {
				plugin->getPluginContext()->loadDiffFromPaths( "", filePath );
			} );
			return;
		}
		auto res =
			mGit->diff( filePath, status == Git::GitStatusType::Staged, mGit->repoPath( file ) );
		if ( res.fail() )
			return;

		auto result = std::move( res.result );
		std::string oldImagePath;
		std::string newImagePath( filePath );
		if ( EE::Graphics::Image::isImageExtension( filePath ) ) {
			std::string repoPath( mGit->repoPath( file ) );
			auto oldBlob = mGit->showFile(
				filePath, status == Git::GitStatusType::Staged ? "HEAD" : ":", repoPath );
			if ( oldBlob.success() )
				oldImagePath = writeGitBlobTempFile( oldBlob.result, filePath );

			if ( status == Git::GitStatusType::Staged ) {
				auto newBlob = mGit->showFile( filePath, ":", repoPath );
				if ( newBlob.success() )
					newImagePath = writeGitBlobTempFile( newBlob.result, filePath );
			}
		}

		lifetime.run( [result = std::move( result ), filePath = std::move( filePath ),
					   oldImagePath = std::move( oldImagePath ),
					   newImagePath = std::move( newImagePath )]( GitPlugin* plugin ) {
			plugin->getPluginContext()->loadDiffFromMemory(
				result, newImagePath.empty() ? filePath : newImagePath, oldImagePath );
		} );
	} );
}

void GitPlugin::diff( std::vector<Git::DiffFile> files ) {
	if ( files.empty() )
		return;
	const auto lifetime = mLifetime.weakHandle();
	mThreadPool->run( [this, files = std::move( files ), lifetime] {
		std::string patch;
		std::string repoPath;
		for ( const auto& file : files ) {
			auto filePath = fixFilePath( file.file );
			auto fileRepoPath = mGit->repoPath( file.file );
			if ( repoPath.empty() )
				repoPath = fileRepoPath;
			auto result =
				file.report.type == Git::GitStatusType::Untracked
					? mGit->diffUntracked( filePath, fileRepoPath )
					: mGit->diff( filePath, file.report.type == Git::GitStatusType::Staged,
								  fileRepoPath );
			if ( result.fail() )
				return;
			patch += result.result;
			if ( !patch.empty() && patch.back() != '\n' )
				patch += '\n';
		}
		lifetime.run( [patch = std::move( patch ),
					   repoPath = std::move( repoPath )]( GitPlugin* plugin ) {
			plugin->getPluginContext()->loadDiffFromMemory( patch, "selected files", "", repoPath );
		} );
	} );
}

// File operations

// Stash operations

void GitPlugin::stashPush( const std::vector<std::string>& files, const std::string& repoPath ) {

	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT,
		i18n( "git_stash_push", "Stash Local Changes\nName your stash (optional):" ) );

	UIRadioButton* rKeepIndex = UIRadioButton::New();
	rKeepIndex->setLayoutMargin( Rectf( 0, 8, 0, 0 ) )
		->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent )
		->setLayoutGravity( UI_HALIGN_LEFT | UI_VALIGN_CENTER )
		->setClipType( ClipType::None )
		->setParent( msgBox->getLayoutCont()->getFirstChild() )
		->setId( "git-stash-keep-index" );
	rKeepIndex->setText( i18n( "git_stash_keep_index", "Keep Index" ) );
	rKeepIndex->toPosition( 2 );

	UIRadioButton* rKeepWorkingTree = UIRadioButton::New();
	rKeepWorkingTree->setLayoutMargin( Rectf( 0, 8, 0, 0 ) )
		->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent )
		->setLayoutGravity( UI_HALIGN_LEFT | UI_VALIGN_CENTER )
		->setClipType( ClipType::None )
		->setParent( msgBox->getLayoutCont()->getFirstChild() )
		->setId( "git-stash-keep-working-tree" );
	rKeepWorkingTree->setText( i18n( "git_stash_keep_working_tree", "Keep Working Tree" ) );
	rKeepWorkingTree->toPosition( 3 );

	msgBox->on( Event::OnConfirm, [this, msgBox, rKeepIndex, rKeepWorkingTree, files,
								   repoPath]( const Event* ) {
		bool keepIndex = rKeepIndex->isActive();
		bool keepWorkingTree = rKeepWorkingTree->isActive();
		std::string message = msgBox->getTextInput()->getText().toUtf8();
		String::trimInPlace( message );
		String::trimInPlace( message, '\n' );
		msgBox->closeWindow();
		runAsync(
			[this, files, keepIndex, keepWorkingTree, repoPath, message]() {
				auto res = mGit->stashPush( fixFilePaths( files ), message, keepIndex, repoPath );
				if ( res.success() && keepWorkingTree )
					mGit->stashApply( "stash@{0}", true, repoPath );
				return res;
			},
			true, true );
	} );

	msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	msgBox->setTitle( i18n( "git_stash_save", "Save Stash" ) );
	msgBox->center();
	msgBox->showWhenReady();
}

void GitPlugin::stashApply( const Git::Branch& branch ) {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::OK_CANCEL,
		i18n( "git_confirm_apply_stash", "Apply a previously saved stash?" ).toUtf8() );

	UICheckBox* chkIndex = UICheckBox::New();
	chkIndex->setLayoutMargin( Rectf( 0, 8, 0, 0 ) )
		->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent )
		->setLayoutGravity( UI_HALIGN_LEFT | UI_VALIGN_CENTER )
		->setClipType( ClipType::None )
		->setParent( msgBox->getLayoutCont()->getFirstChild() )
		->setId( "git-restore-index" );
	chkIndex->setText( i18n( "git_restore_index", "Restore Index" ) );
	chkIndex->toPosition( 2 );
	chkIndex->setChecked( true );

	msgBox->on( Event::OnConfirm, [this, branch, chkIndex]( auto ) {
		runAsync(
			[this, branch, chkIndex]() {
				return mGit->stashApply( branch.remote, chkIndex->isChecked(), repoSelected() );
			},
			true, true );
	} );
	msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	msgBox->setTitle( i18n( "git_apply_stash_title", "Apply Stash" ) );
	msgBox->center();
	msgBox->showWhenReady();
}

void GitPlugin::stashDrop( const Git::Branch& branch ) {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::OK_CANCEL,
		i18n( "git_confirm_drop_stash", "Do you want to drop the selected stash?" ).toUtf8() );

	msgBox->on( Event::OnConfirm, [this, branch]( auto ) {
		runAsync( [this, branch]() { return mGit->stashDrop( branch.remote, repoSelected() ); },
				  true, true );
	} );
	msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	msgBox->setTitle( i18n( "git_drop_stash_title", "Drop Stash" ) );
	msgBox->center();
	msgBox->showWhenReady();
}

// Stash operations

void GitPlugin::onRegister( UICodeEditor* editor ) {
	PluginBase::onRegister( editor );

	for ( auto& kb : mKeyBindings ) {
		if ( !kb.second.empty() )
			editor->getKeyBindings().addKeybindString( kb.second, kb.first );
	}

	if ( !editor->hasDocument() )
		return;

	auto& doc = editor->getDocument();
	doc.setCommand( "git-blame", [this]( TextDocument::Client* client ) {
		blame( static_cast<UICodeEditor*>( client ) );
	} );
	doc.setCommand( "show-source-control-tab", [this]() {
		if ( mTab )
			mTab->setTabSelected();
	} );
	doc.setCommand( "git-pull", [this] { pull( projectPath() ); } );
	doc.setCommand( "git-push", [this] { push( projectPath() ); } );
	doc.setCommand( "git-fetch", [this] { fetch( projectPath() ); } );
	doc.setCommand( "git-commit", [this] { commit( projectPath() ); } );
}

void GitPlugin::onUnregister( UICodeEditor* editor ) {
	PluginBase::onUnregister( editor );
}

bool GitPlugin::onCreateContextMenu( UICodeEditor*, UIPopUpMenu* menu, const Vector2i& /*position*/,
									 const Uint32& /*flags*/ ) {
	if ( !mGitFound )
		return false;

	menu->addSeparator();

	auto* subMenu = UIPopUpMenu::New();
	subMenu->addClass( "gitplugin_menu" );

	auto addFn = [this, subMenu]( const std::string& txtKey, const std::string& txtVal,
								  const std::string& icon = "" ) {
		subMenu
			->add( i18n( txtKey, txtVal ),
				   !icon.empty() ? findIcon( icon )->createDrawable( PixelDensity::dpToPxI( 12 ) )
								 : nullptr,
				   KeyBindings::keybindFormat( mKeyBindings[txtKey] ) )
			->setId( txtKey );
	};

	addFn( "git-blame", "Git Blame" );

	menu->addSubMenu( i18n( "git", "Git" ),
					  mManager->getUISceneNode()
						  ->findIcon( "source-control" )
						  ->createDrawable( PixelDensity::dpToPxI( 12 ) ),
					  subMenu );

	return false;
}

bool GitPlugin::onKeyDown( UICodeEditor* editor, const KeyEvent& event ) {
	if ( event.getSanitizedMod() == 0 && event.getKeyCode() == KEY_ESCAPE && editor->getTooltip() &&
		 editor->getTooltip()->isVisible() ) {
		hideTooltip( editor );
	}

	return false;
}

std::unordered_map<std::string, std::string> GitPlugin::updateReposBranches() {
	mGit->getSubModules();

	bool reposEmpty = false;
	{
		Lock l( mReposMutex );
		reposEmpty = mRepos.empty();
	}
	if ( reposEmpty )
		updateRepos();

	Lock l( mGitBranchMutex );

	decltype( mGitBranches ) prevBranch;
	if ( mGitBranches.empty() || mLastBranchesUpdate.getElapsedTime() > Seconds( 1 ) ) {
		prevBranch = mGitBranches;
		mGitBranches = mGit->branches( repos() );
		mLastBranchesUpdate.restart();
	} else {
		Lock l( mGitBranchMutex );
		prevBranch = mGitBranches;
	}

	return prevBranch;
}

void GitPlugin::invalidateHistory() {
	++mHistoryGeneration;
	mHistoryLoaded = false;
	mHistoryRepo.clear();
	if ( mPanelSwicher && mPanelSwicher->getListBox()->getItemSelectedIndex() == 2 )
		reloadHistory();
}

void GitPlugin::updateHistoryHeader() {
	if ( !mHistoryRefDropDown || !mHistoryRefModel )
		return;
	auto rowCount = mHistoryRefModel->rowCount();
	for ( size_t i = 0; i < rowCount; ++i ) {
		if ( mHistoryRefModel->isRevision( i, mHistoryRevision ) ) {
			mUpdatingHistoryRefs = true;
			mHistoryRefDropDown->getListView()->getSelection().set(
				mHistoryRefModel->index( i, 0 ) );
			mUpdatingHistoryRefs = false;
			return;
		}
	}
}

void GitPlugin::updateHistoryRefs( const std::shared_ptr<GitBranchModel>& model ) {
	if ( !mHistoryRefDropDown )
		return;
	std::string currentBranch;
	{
		const std::string repo = repoSelected();
		Lock l( mGitBranchMutex );
		auto it = mGitBranches.find( repo );
		if ( it != mGitBranches.end() )
			currentBranch = it->second;
	}
	String headLabel{ "HEAD" };
	if ( !currentBranch.empty() )
		headLabel = String::fromUtf8( String::format( "HEAD · %s", currentBranch ) );
	auto historyRefModel = std::make_shared<GitHistoryRefModel>( model, std::move( headLabel ) );
	size_t selected = 0;
	bool found = false;
	for ( size_t i = 0; i < historyRefModel->rowCount(); ++i ) {
		if ( historyRefModel->isRevision( i, mHistoryRevision ) ) {
			selected = i;
			found = true;
			break;
		}
	}
	const bool revisionChanged = !found && mHistoryRevision != "HEAD";
	if ( !found )
		mHistoryRevision = "HEAD";
	mUpdatingHistoryRefs = true;
	mHistoryRefModel = std::move( historyRefModel );
	mHistoryRefDropDown->setModel( mHistoryRefModel );
	mHistoryRefDropDown->getListView()->getSelection().set(
		mHistoryRefModel->index( selected, 0 ) );
	mUpdatingHistoryRefs = false;
	if ( revisionChanged )
		invalidateHistory();
}

void GitPlugin::ensureHistoryLoaded() {
	const std::string repo = repoSelected();
	if ( !mHistoryLoaded || mHistoryRepo != repo )
		reloadHistory();
}

void GitPlugin::reloadHistory() {
	if ( mShuttingDown || !mGit || !mGitFound || !mHistoryTree )
		return;
	updateHistoryHeader();
	const std::string repo = repoSelected();
	const Uint64 generation = ++mHistoryGeneration;
	mHistoryRepo = repo;
	mHistoryLoaded = false;
	if ( !mHistoryModel ) {
		mHistoryModel = GitHistoryModel::asModel( this );
		mHistoryTree->setModel( mHistoryModel );
		mHistoryTree->setColumnsVisible( { GitHistoryModel::Subject } );
	}
	mHistoryTree->clearViewMetadata();
	mHistoryModel->setRootLoading();
	Git::HistoryQuery query;
	query.revision = mHistoryRevision;
	auto git = mGit;
	const auto lifetime = mLifetime.weakHandle();
	++mRunningHistoryRequests;
	mThreadPool->run(
		[git = std::move( git ), lifetime, repo, generation, query]() mutable {
			auto page = git->history( query, repo );
			lifetime.run(
				[repo, generation, query, page = std::move( page )]( GitPlugin* plugin ) mutable {
					if ( plugin->mShuttingDown || generation != plugin->mHistoryGeneration ||
						 repo != plugin->repoSelected() )
						return;
					plugin->mHistoryLoaded = true;
					if ( page.success() )
						plugin->mHistoryModel->setRootPage( std::move( page ), query );
					else
						plugin->mHistoryModel->setRootError( std::move( page.result ) );
				} );
		},
		[this]( auto ) { --mRunningHistoryRequests; } );
}

void GitPlugin::loadHistoryPage( GitHistoryModel::Node* node, Git::HistoryQuery query,
								 bool append ) {
	if ( !node || mShuttingDown || node->childrenLoading )
		return;
	if ( append && node->type != GitHistoryModel::NodeType::LoadMore && !node->retryAppend )
		return;
	if ( !append && ( node->type != GitHistoryModel::NodeType::Commit || node->childrenLoaded ) )
		return;
	const std::string repo = repoSelected();
	const Uint64 generation = mHistoryGeneration;
	if ( append )
		mHistoryModel->setPageLoading( node );
	else
		mHistoryModel->setChildrenLoading( node );
	auto git = mGit;
	const auto lifetime = mLifetime.weakHandle();
	++mRunningHistoryRequests;
	mThreadPool->run(
		[git = std::move( git ), lifetime, repo, generation, node, query = std::move( query ),
		 append]() mutable {
			auto page = git->history( query, repo );
			lifetime.run( [repo, generation, node, query = std::move( query ), append,
						   page = std::move( page )]( GitPlugin* plugin ) mutable {
				if ( plugin->mShuttingDown || generation != plugin->mHistoryGeneration ||
					 repo != plugin->repoSelected() )
					return;
				if ( page.fail() ) {
					if ( append )
						plugin->mHistoryModel->setPageError( node, std::move( page.result ) );
					else
						plugin->mHistoryModel->setChildrenError( node, std::move( page.result ),
																 query );
					return;
				}
				if ( append )
					plugin->mHistoryModel->appendPage( node, std::move( page ) );
				else
					plugin->mHistoryModel->setChildrenPage( node, std::move( page ), query );
				plugin->mHistoryTree->recalculateColumnsWidth();
			} );
		},
		[this]( auto ) { --mRunningHistoryRequests; } );
}

void GitPlugin::activateHistoryIndex( const ModelIndex& index, bool expand ) {
	if ( !mHistoryModel )
		return;
	auto* node = mHistoryModel->node( index );
	if ( !node )
		return;
	if ( node->type == GitHistoryModel::NodeType::LoadMore ) {
		if ( !expand && !node->childrenLoading )
			loadHistoryPage( node, node->query, true );
		return;
	}
	if ( node->type == GitHistoryModel::NodeType::Error && !expand ) {
		if ( node->retryAppend && !node->childrenLoading )
			loadHistoryPage( node, node->query, true );
		else if ( node->parent && !node->parent->childrenLoading )
			loadHistoryPage( node->parent, node->query, false );
		else
			reloadHistory();
		return;
	}
	if ( expand && node->type == GitHistoryModel::NodeType::Commit &&
		 node->commit.parents.size() == 2 && !node->childrenLoaded && !node->childrenLoading ) {
		loadHistoryPage( node, mHistoryModel->mergeQuery( node, 200 ), false );
	}
}

void GitPlugin::openCommitDetails( const Git::Commit& commit ) {
	if ( commit.hash.empty() )
		return;
	const std::string repo = repoSelected();
	const Uint64 generation = ++mCommitDetailsGeneration;
	mCommitDetailsCommit = commit;
	mCommitDetailsRepo = repo;

	const bool newView =
		!mCommitDetailsView || !mManager->getSplitter()->ownedWidgetExists( mCommitDetailsView );
	if ( newView ) {
		mCommitDetailsView = getUISceneNode()->loadLayoutFromString( R"xml(
			<vbox id="git_commit_details" lw="mp" lh="mp">
				<vbox lw="mp" lh="wc" padding="12dp">
					<hbox lw="mp" lh="wc">
						<TextView id="git_commit_metadata" lw="0" lw8="1" lh="wc"
								  word-wrap="true" focusable="false" />
						<PushButton id="git_commit_sha" text="Commit SHA"
									text-as-fallback="true" margin-left="8dp" />
					</hbox>
					<hbox lw="mp" lh="wc" margin-top="8dp">
						<TextView id="git_commit_subject" lw="0" lw8="1" lh="wc"
								  font-size="14dp" word-wrap="false" text-overflow="ellipsis"
								  focusable="false" />
						<PushButton id="git_commit_message_toggle"
									text="@string(git_expand_commit_description, Expand Commit Description)"
									icon="icon(unfold, 12dp)" text-as-fallback="true"
									margin-left="8dp" visible="false" />
					</hbox>
					<TextView id="git_commit_message" lw="mp" lh="wc" margin-top="8dp"
							  word-wrap="true" focusable="false" visible="false" />
					<TextView id="git_commit_parents" lw="mp" lh="wc" margin-top="4dp"
							  word-wrap="true" focusable="false" />
				</vbox>
				<hbox lw="mp" lh="wc" padding-left="8dp" padding-right="8dp"
					  padding-top="4dp" padding-bottom="4dp">
					<PushButton id="git_commit_files_toggle"
								tooltip="@string(git_collapse_all_files, Collapse All Files)"
								icon="icon(collapse-all, 12dp)" />
					<PushButton id="git_commit_mode_toggle"
								text="@string(git_split_diff, Split)"
								icon="icon(split-horizontal, 12dp)" text-as-fallback="true"
								margin-left="4dp" />
					<TextView id="git_commit_files_status" lw="0" lw8="1" lh="wc"
							  margin-left="8dp" layout_gravity="center_vertical" focusable="false" />
					<PushButton id="git_commit_github"
								text="@string(git_view_on_github, View on GitHub)"
								icon="icon(github, 12dp)" text-as-fallback="true"
								visible="false" />
				</hbox>
				<vbox id="git_commit_diff" lw="mp" lh="0" lw8="1" />
			</vbox>
		)xml" );
		mCommitDetailsSubject = mCommitDetailsView->find<UITextView>( "git_commit_subject" );
		mCommitDetailsMetadata = mCommitDetailsView->find<UITextView>( "git_commit_metadata" );
		mCommitDetailsParents = mCommitDetailsView->find<UITextView>( "git_commit_parents" );
		mCommitDetailsMessage = mCommitDetailsView->find<UITextView>( "git_commit_message" );
		mCommitDetailsStatus = mCommitDetailsView->find<UITextView>( "git_commit_files_status" );
		mCommitDetailsMessageToggle =
			mCommitDetailsView->find<UIPushButton>( "git_commit_message_toggle" );
		mCommitDetailsFilesToggle =
			mCommitDetailsView->find<UIPushButton>( "git_commit_files_toggle" );
		mCommitDetailsModeToggle =
			mCommitDetailsView->find<UIPushButton>( "git_commit_mode_toggle" );
		mCommitDetailsGitHub = mCommitDetailsView->find<UIPushButton>( "git_commit_github" );
		mCommitDetailsDiffContainer = mCommitDetailsView->find<UIWidget>( "git_commit_diff" );
		mCommitDetailsMessageToggle->onClick( [this]( const Event* ) {
			mCommitDetailsMessageExpanded = !mCommitDetailsMessageExpanded;
			mCommitDetailsMessage->setVisible( mCommitDetailsMessageExpanded );
			mCommitDetailsMessageToggle->setText(
				mCommitDetailsMessageExpanded
					? i18n( "git_collapse_commit_description", "Collapse Commit Description" )
					: i18n( "git_expand_commit_description", "Expand Commit Description" ) );
		} );
		mCommitDetailsFilesToggle->onClick( [this]( const Event* ) {
			mCommitDetailsFilesCollapsed = !mCommitDetailsFilesCollapsed;
			UIDiffView::setMultiFileCollapsed( mCommitDetailsDiff, mCommitDetailsFilesCollapsed );
			mCommitDetailsFilesToggle->setTooltipText(
				mCommitDetailsFilesCollapsed
					? i18n( "git_expand_all_files", "Expand All Files" )
					: i18n( "git_collapse_all_files", "Collapse All Files" ) );
			if ( auto* icon =
					 findIcon( mCommitDetailsFilesCollapsed ? "expand-all" : "collapse-all" ) )
				mCommitDetailsFilesToggle->setIcon(
					icon->createDrawable( PixelDensity::dpToPxI( 12 ) ) );
		} );
		mCommitDetailsModeToggle->onClick( [this]( const Event* ) {
			mCommitDetailsViewMode = mCommitDetailsViewMode == UIDiffView::ViewMode::Unified
										 ? UIDiffView::ViewMode::SideBySide
										 : UIDiffView::ViewMode::Unified;
			UIDiffView::setMultiFileViewMode( mCommitDetailsDiff, mCommitDetailsViewMode );
			mCommitDetailsModeToggle->setText( mCommitDetailsViewMode ==
													   UIDiffView::ViewMode::Unified
												   ? i18n( "git_split_diff", "Split" )
												   : i18n( "git_unified_diff", "Unified" ) );
			if ( auto* icon = findIcon( mCommitDetailsViewMode == UIDiffView::ViewMode::Unified
											? "split-horizontal"
											: "layout" ) )
				mCommitDetailsModeToggle->setIcon(
					icon->createDrawable( PixelDensity::dpToPxI( 12 ) ) );
		} );
		mCommitDetailsGitHub->onClick( [this]( const Event* ) {
			if ( !mCommitDetailsURL.empty() )
				Engine::instance()->openURI( mCommitDetailsURL );
		} );
		mCommitDetailsView->find<UIPushButton>( "git_commit_sha" )
			->onClick( [this]( const Event* ) {
				getUISceneNode()->getWindow()->getClipboard()->setText( mCommitDetailsCommit.hash );
			} );
		auto* view = mCommitDetailsView;
		mCommitDetailsCloseConnection =
			view->connect( Event::OnClose, [this, view]( const Event* ) {
				if ( mCommitDetailsView != view )
					return;
				++mCommitDetailsGeneration;
				mCommitDetailsView = nullptr;
				mCommitDetailsSubject = nullptr;
				mCommitDetailsMetadata = nullptr;
				mCommitDetailsParents = nullptr;
				mCommitDetailsMessage = nullptr;
				mCommitDetailsStatus = nullptr;
				mCommitDetailsMessageToggle = nullptr;
				mCommitDetailsFilesToggle = nullptr;
				mCommitDetailsModeToggle = nullptr;
				mCommitDetailsGitHub = nullptr;
				mCommitDetailsDiffContainer = nullptr;
				mCommitDetailsDiff = nullptr;
				mCommitDetailsMessageBody.clear();
				mCommitDetailsURL.clear();
				mCommitDetailsRepo.clear();
			} );
	}

	mCommitDetailsSubject->setText( String::fromUtf8( commit.subject ) );
	String metadata = String::fromUtf8( commit.authorName ) + "\n" +
					  Sys::epochToString( commit.commitTime ) + " - " +
					  String::fromUtf8( commit.authorEmail );
	mCommitDetailsMetadata->setText( metadata );
	String parents;
	for ( size_t i = 0; i < commit.parents.size(); ++i ) {
		if ( i )
			parents += ", ";
		parents += String::fromUtf8( commit.parents[i] );
	}
	mCommitDetailsMessageBody.clear();
	mCommitDetailsMessageExpanded = false;
	mCommitDetailsMessage->setText( "" );
	mCommitDetailsMessage->setVisible( false );
	mCommitDetailsMessageToggle->setVisible( false );
	mCommitDetailsParents->setVisible( !parents.empty() );
	mCommitDetailsParents->setText( i18n( "git_parents", "Parents" ) + ": " + parents );
	mCommitDetailsStatus->setText(
		i18n( "git_loading_changed_files", "Loading changed files..." ) );
	mCommitDetailsFilesCollapsed = false;
	mCommitDetailsFilesToggle->setTooltipText(
		i18n( "git_collapse_all_files", "Collapse All Files" ) );
	if ( auto* icon = findIcon( "collapse-all" ) )
		mCommitDetailsFilesToggle->setIcon( icon->createDrawable( PixelDensity::dpToPxI( 12 ) ) );
	mCommitDetailsURL.clear();
	mCommitDetailsGitHub->setVisible( false );
	mCommitDetailsDiff = nullptr;
	mCommitDetailsDiffContainer->closeAllChildren();
	auto* shaButton = mCommitDetailsView->find<UIPushButton>( "git_commit_sha" );
	shaButton->setText( String::fromUtf8( commit.shortHash ) );
	shaButton->setTooltipText( String::fromUtf8( commit.hash ) );

	const std::string tabName = commit.shortHash + " " + commit.subject;
	if ( newView ) {
		mManager->getSplitter()->createWidget( mCommitDetailsView, tabName, true );
	} else {
		auto tabs = mManager->getSplitter()->getTabFromOwnedWidgetId( mCommitDetailsView->getId() );
		if ( !tabs.empty() ) {
			tabs.front().first->setText( tabName );
			tabs.front().second->setTabSelected( tabs.front().first );
		}
	}
	if ( generation == mCommitDetailsGeneration )
		loadCommitFiles();
}

void GitPlugin::loadCommitFiles() {
	const Uint64 generation = mCommitDetailsGeneration;
	const std::string repo = mCommitDetailsRepo;
	const Git::Commit commit = mCommitDetailsCommit;
	auto git = mGit;
	const auto lifetime = mLifetime.weakHandle();
	runAsyncTask( [git = std::move( git ), lifetime, generation, repo, commit] {
		auto result = git->commitFiles( commit, repo );
		lifetime.run( [generation, repo, commit,
					   result = std::move( result )]( GitPlugin* plugin ) mutable {
			if ( plugin->mShuttingDown || generation != plugin->mCommitDetailsGeneration ||
				 repo != plugin->mCommitDetailsRepo || repo != plugin->repoSelected() ||
				 commit.hash != plugin->mCommitDetailsCommit.hash || !plugin->mCommitDetailsView ||
				 !plugin->mManager->getSplitter()->ownedWidgetExists( plugin->mCommitDetailsView ) )
				return;
			if ( result.fail() ) {
				plugin->mCommitDetailsStatus->setText(
					plugin->i18n( "git_changed_files_error", "Could not load changed files" ) +
					( result.result.empty() ? "" : ": " + result.result ) );
				return;
			}
			std::string message = std::move( result.message );
			const size_t subjectEnd = message.find_first_of( "\r\n" );
			const std::string_view subject{
				message.data(), subjectEnd == std::string::npos ? message.size() : subjectEnd };
			plugin->mCommitDetailsSubject->setText(
				String::fromUtf8( subject.empty() ? commit.subject : std::string{ subject } ) );
			std::string body;
			if ( subjectEnd != std::string::npos ) {
				size_t bodyStart = subjectEnd;
				while ( bodyStart < message.size() &&
						( message[bodyStart] == '\n' || message[bodyStart] == '\r' ) )
					++bodyStart;
				body = message.substr( bodyStart );
			}
			plugin->mCommitDetailsMessageBody = std::move( body );
			plugin->mCommitDetailsMessage->setText(
				String::fromUtf8( plugin->mCommitDetailsMessageBody ) );
			plugin->mCommitDetailsMessage->setVisible( false );
			plugin->mCommitDetailsMessageExpanded = false;
			plugin->mCommitDetailsMessageToggle->setVisible(
				!plugin->mCommitDetailsMessageBody.empty() );
			plugin->mCommitDetailsMessageToggle->setText(
				plugin->i18n( "git_expand_commit_description", "Expand Commit Description" ) );

			int totalInserts = 0;
			int totalDeletes = 0;
			for ( const auto& file : result.files ) {
				totalInserts += file.inserts;
				totalDeletes += file.deletes;
			}
			if ( result.files.empty() ) {
				plugin->mCommitDetailsStatus->setText(
					plugin->i18n( "git_no_changed_files", "No changed files" ) );
			} else {
				plugin->mCommitDetailsStatus->setText( String::format(
					plugin->i18n( "git_changed_files_summary", "Changed files (%zu)  +%d -%d" )
						.toUtf8(),
					result.files.size(), totalInserts, totalDeletes ) );
			}

			plugin->mCommitDetailsURL = std::move( result.commitURL );
			plugin->mCommitDetailsGitHub->setVisible( !plugin->mCommitDetailsURL.empty() );
			plugin->mCommitDetailsDiffContainer->closeAllChildren();
			plugin->mCommitDetailsDiff = nullptr;
			if ( !result.patch.empty() ) {
				plugin->mCommitDetailsDiff = UIDiffView::NewMultiFileDiffViewer(
					result.patch, repo, plugin->mCommitDetailsViewMode );
				plugin->mCommitDetailsDiff->setLayoutSizePolicy( SizePolicy::MatchParent,
																 SizePolicy::MatchParent );
				plugin->mCommitDetailsDiff->setParent( plugin->mCommitDetailsDiffContainer );
				for ( auto* diff : UIDiffView::multiFileDiffViews( plugin->mCommitDetailsDiff ) ) {
					if ( const auto* scheme = plugin->getPluginContext()->getCurrentColorScheme() )
						diff->setSyntaxColorScheme( *scheme );
				}
			}
			const bool hasDiff = plugin->mCommitDetailsDiff != nullptr;
			plugin->mCommitDetailsFilesToggle->setVisible( hasDiff );
			plugin->mCommitDetailsModeToggle->setVisible( hasDiff );
		} );
	} );
}

void GitPlugin::updateBranches( bool force ) {
	if ( !mGit || !mGitFound || ( mRunningUpdateBranches && !force ) )
		return;

	if ( !mGit || mGit->getGitFolder().empty() ) {
		mLifetime.weakHandle().run(
			[]( GitPlugin* plugin ) { plugin->updateBranchesUI( nullptr ); } );
		return;
	}

	const std::string requestedRepo = repoSelected();
	const auto lifetime = mLifetime.weakHandle();
	mRunningUpdateBranches++;
	mThreadPool->run(
		[this, requestedRepo, lifetime] {
			if ( !mGit || mGit->getGitFolder().empty() ) {
				lifetime.run( [requestedRepo]( GitPlugin* plugin ) {
					if ( requestedRepo == plugin->repoSelected() )
						plugin->updateBranchesUI( nullptr );
				} );
				return;
			}

			auto prevBranch = updateReposBranches();
			auto branches = mGit->getAllBranchesAndTags( Git::RefType::All, {}, requestedRepo );
			auto hash = GitBranchModel::hashBranches( branches );
			auto model = GitBranchModel::asModel( std::move( branches ), hash, this );

			bool branchChanged;
			{
				Lock l( mGitBranchMutex );
				branchChanged = prevBranch != mGitBranches;
			}
			lifetime.run( [model, hash, branchChanged, requestedRepo]( GitPlugin* plugin ) {
				if ( requestedRepo != plugin->repoSelected() )
					return;
				if ( plugin->mBranchesTree && plugin->mBranchesTree->getModel() &&
					 static_cast<GitBranchModel*>( plugin->mBranchesTree->getModel() )->getHash() ==
						 hash ) {
					if ( plugin->mBranchesTree->getModel() ) {
						if ( branchChanged )
							plugin->mBranchesTree->getModel()->invalidate(
								Model::DontInvalidateIndexes );
						plugin->updateHistoryRefs( std::static_pointer_cast<GitBranchModel>(
							plugin->mBranchesTree->getModelShared() ) );
					}
					if ( branchChanged )
						plugin->invalidateHistory();
				} else {
					plugin->updateBranchesUI( model );
					if ( branchChanged )
						plugin->invalidateHistory();
				}
			} );
		},
		[this]( auto ) { mRunningUpdateBranches--; } );
}

void GitPlugin::updateRepos() {
	if ( !mGit )
		return;

	auto subModules = mGit->getSubModules();
	std::sort( subModules.begin(), subModules.end() );

	std::vector<std::pair<std::string, std::string>> repos;
	repos.clear();
	repos.emplace_back( mProjectPath, FileSystem::fileNameFromPath( mProjectPath ) );
	for ( auto& subModule : subModules ) {
		std::string subModulePath = mProjectPath + subModule;
		repos.emplace_back( std::move( subModulePath ), FileSystem::fileNameFromPath( subModule ) );
	}

	Lock l( mReposMutex );
	if ( repos == mRepos )
		return;

	mRepos = std::move( repos );
}

void GitPlugin::updateBranchesUI( std::shared_ptr<GitBranchModel> model ) {
	buildSidePanelTab();

	if ( !model ) {
		mBranchesTree->setModel( model );
	} else {
		mBranchesTree->setModel( model );
		mBranchesTree->setColumnsVisible( { GitBranchModel::Name } );
		mBranchesTree->expandAll();
	}

	updateHistoryRefs( model );
	updateRepos();

	std::vector<String> items;
	decltype( mRepos ) repos;
	{
		Lock l( mReposMutex );
		repos = mRepos;
	}

	for ( const auto& repo : repos )
		items.push_back( repo.second );

	if ( repos.empty() || ( repos.size() == 1 && repos.begin()->second == "" ) ) {
		if ( !mRepoDropDown->getListBox()->isEmpty() )
			mRepoDropDown->getListBox()->clear();
		return;
	}

	if ( mRepoDropDown->getListBox()->getItemsText() != items ) {
		mRepoDropDown->getListBox()->clear();
		mRepoDropDown->getListBox()->addListBoxItems( items );
		mRepoDropDown->getListBox()->setSelected( repoName( repoSelected() ) );
	}
}

void GitPlugin::buildSidePanelTab() {
	if ( mTabContents && !mTab ) {
		if ( mProjectPath.empty() )
			return;
		UIIcon* icon = findIcon( "source-control" );
		mTab =
			mSidePanel->add( i18n( "source_control", "Source Control" ), mTabContents,
							 icon ? icon->createDrawable( PixelDensity::dpToPx( 12 ) ) : nullptr );
		mTab->setId( "source_control_tab" );
		mTab->setTextAsFallback( true );
		return;
	}
	if ( mTab )
		return;
	if ( mSidePanel == nullptr )
		getUISceneNode()->bind( "panel", mSidePanel );
	if ( !UIWidgetCreator::isWidgetRegistered( "GitHistoryTreeView" ) )
		UIWidgetCreator::registerWidget( "GitHistoryTreeView", GitHistoryTreeView::New );
	static constexpr auto STYLE = R"html(
	<style>
	#git_branches_tree ScrollBar,
	#git_status_tree ScrollBar,
	#git_history_tree ScrollBar {
		opacity: 0;
		transition: opacity 0.15;
	}
	#git_branches_tree:hover ScrollBar,
	#git_branches_tree ScrollBar.dragging,
	#git_branches_tree ScrollBar:focus-within,
	#git_status_tree:hover ScrollBar,
	#git_status_tree ScrollBar.dragging,
	#git_status_tree ScrollBar:focus-within,
	#git_history_tree:hover ScrollBar,
	#git_history_tree ScrollBar.dragging,
	#git_history_tree ScrollBar:focus-within {
		opacity: 1;
	}
	treeview::cell.git_history_secondary {
		color: var(--font-hint);
	}
	treeview::cell.git_history_hash {
		color: var(--font-hint);
		font-family: monospace;
	}
	treeview::cell.git_history_action {
		color: %s;
	}
	treeview::cell.git_history_merge {
		font-style: italic;
	}
	treeview::row:selected treeview::cell.git_history_secondary,
	treeview::row:selected treeview::cell.git_history_hash,
	treeview::row:selected treeview::cell.git_history_action {
		color: var(--font-selected-pressed);
	}
	treeview::cell.git_highlight_style {
		color: %s;
	}
	treeview::row:selected treeview::cell.git_highlight_style,
	treeview::row:selected treeview::cell.git_highlight_style {
		color: var(--font-selected-pressed);
		tint: var(--font-selected-pressed);
	}
	treeview::cell.git_highlight_style > treeview::cell::icon {
		foreground-image: icon(circle, 8dpru), icon(circle-filled, 8dpru);
		foreground-position: 80%% 80%%, 80%% 80%%;
		foreground-tint: black, %s;
	}
	treeview::cell.git_highlight_style_clear > treeview::cell::icon {
		foreground-image: none, none;
	}
	</style>
	<RelativeLayout id="git_panel" lw="mp" lh="mp">
		<vbox id="git_content" lw="mp" lh="mp">
			<DropDownList id="git_panel_switcher" lw="mp" lh="22dp" border-type="inside" border-right-width="0" border-left-width="0" border-top-width="0" border-bottom-left-radius="0" border-bottom-right-radius="0" />
			<vbox id="git_conflict_state" lw="mp" lh="wc" padding="4dp" visible="false">
				<TextView id="git_conflict_state_text" lw="mp" lh="wc" text-align="left|center_vertical" />
				<hbox lw="mp" lh="wc">
					<PushButton lw="0" lw8="0.5" id="git_conflict_continue" text="@string(git_continue_operation, Continue)" icon="icon(debug-continue, 12dp)" />
					<PushButton lw="0" lw8="0.5" id="git_conflict_abort" text="@string(git_abort_operation, Abort)" icon="icon(discard, 12dp)" margin-left="2dp" />
				</hbox>
			</vbox>
			<StackWidget id="git_panel_stack" lw="mp" lh="0" lw8="1">
				<vbox id="git_branches" lw="mp" lh="wc">
					<hbox lw="mp" lh="wc" padding="4dp">
						<DropDownList id="git_repo" lw="0" lh="wc" lw8="1" menu-width-mode="expand-if-needed" />
						<PushButton id="branch_pull" text="@string(git_pull, Pull)" tooltip="@string(pull_branch, Pull Branch)" text-as-fallback="true" icon="icon(repo-pull, 12dp)" margin-left="2dp" />
						<PushButton id="branch_push" text="@string(git_push, Push)" tooltip="@string(push_branch, Push Branch)" text-as-fallback="true" icon="icon(repo-push, 12dp)" margin-left="2dp" />
						<PushButton id="branch_add" text="@string(git_add_branch, Add Branch)" tooltip="@string(add_branch, Add Branch)" text-as-fallback="true" icon="icon(add, 12dp)" margin-left="2dp" />
					</hbox>
					<TreeView id="git_branches_tree" lw="mp" lh="0" lw8="1" />
				</vbox>
				<vbox id="git_status" lw="mp" lh="mp">
					<TreeView id="git_status_tree" lw="mp" lh="mp" />
				</vbox>
				<vbox id="git_history" lw="mp" lh="mp">
					<hbox lw="mp" lh="wc" padding="4dp">
						<DropDownModelList id="git_history_ref" lw="0" lw8="1" lh="wc" menu-width-mode="expand-if-needed" />
						<PushButton id="git_history_refresh" text="@string(git_history_refresh, Refresh)" icon="icon(refresh, 12dp)" text-as-fallback="true" />
					</hbox>
					<GitHistoryTreeView id="git_history_tree" lw="mp" lh="0" lw8="1" />
				</vbox>
			</StackWidget>
		</vbox>
		<TextView id="git_no_content" lw="mp" lh="wc" word-wrap="true" visible="false" text='@string(git_no_git_repo, "Current folder is not a Git repository.")' margin="8dp" text-align="center" />
		<Loader margin-top="32dp" id="git_panel_loader" indeterminate="true" lw="24dp" lh="24dp" outline-thickness="2dp" visible="false" layout_gravity="bottom|right" margin-bottom="24dp" margin-right="24dp" />
	</RelativeLayout>
	)html";
	UIIcon* icon = findIcon( "source-control" );
	std::string color =
		!mHighlightStyleColor.empty() && Color::isColorString( mHighlightStyleColor )
			? mHighlightStyleColor
			: std::string{ DEFAULT_HIGHLIGHT_COLOR };

	mTabContents = getUISceneNode()->loadLayoutFromString(
		String::format( STYLE, color, color, color ), nullptr, String::hash( "git_plugin_style" ) );

	mTab = mSidePanel->add( i18n( "source_control", "Source Control" ), mTabContents,
							icon ? icon->createDrawable( PixelDensity::dpToPx( 12 ) ) : nullptr );
	mTab->setId( "source_control_tab" );
	mTab->setTextAsFallback( true );

	mTabContents->bind( "git_panel_switcher", mPanelSwicher );
	mTabContents->bind( "git_panel_stack", mStackWidget );
	mTabContents->bind( "git_branches_tree", mBranchesTree );
	mTabContents->bind( "git_status_tree", mStatusTree );
	mTabContents->bind( "git_history_tree", mHistoryTree );
	mTabContents->bind( "git_history_ref", mHistoryRefDropDown );
	mTabContents->bind( "git_content", mGitContentView );
	mTabContents->bind( "git_no_content", mGitNoContentView );
	mTabContents->bind( "git_conflict_state", mConflictStateBar );
	mTabContents->bind( "git_conflict_state_text", mConflictStateText );
	mTabContents->bind( "git_panel_loader", mLoader );
	mTabContents->bind( "git_repo", mRepoDropDown );

	mTabContents->find( "branch_pull" )->onClick( [this]( auto ) { pull( repoSelected() ); } );
	mTabContents->find( "branch_push" )->onClick( [this]( auto ) { push( repoSelected() ); } );
	mTabContents->find( "branch_add" )->onClick( [this]( auto ) { branchCreate(); } );
	mTabContents->find( "git_history_refresh" )->onClick( [this]( auto ) { reloadHistory(); } );
	mHistoryRefDropDown->getListView()->setColumnsVisible( { 0 } );
	mHistoryRefDropDown->getListView()->setAutoExpandOnSingleColumn( true );
	mHistoryRefDropDown->on( Event::OnItemSelected, [this]( const Event* ) {
		if ( mUpdatingHistoryRefs )
			return;
		const ModelIndex selected = mHistoryRefDropDown->getListView()->getSelection().first();
		if ( !selected.isValid() || !mHistoryRefModel )
			return;
		const std::string revision = mHistoryRefModel->revision( selected.row() );
		if ( revision == mHistoryRevision )
			return;
		mHistoryRevision = revision;
		invalidateHistory();
	} );
	updateHistoryRefs(
		mBranchesTree && mBranchesTree->getModel()
			? std::static_pointer_cast<GitBranchModel>( mBranchesTree->getModelShared() )
			: nullptr );
	mTabContents->find( "git_conflict_continue" )->onClick( [this]( auto ) {
		continueConflictOperation();
	} );
	mTabContents->find( "git_conflict_abort" )->onClick( [this]( auto ) {
		abortConflictOperation();
	} );

	mBranchesTree->setAutoExpandOnSingleColumn( true );
	mBranchesTree->setHeadersVisible( false );
	mBranchesTree->setExpandersAsIcons( true );
	mBranchesTree->setIndentWidth( PixelDensity::dpToPx( 16 ) );
	mBranchesTree->setScrollViewType( ScrollViewType::Overlay );
	mBranchesTree->on( Event::OnModelEvent, [this]( const Event* event ) {
		const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
		if ( !modelEvent->getModelIndex().hasParent() )
			return;

		const Git::Branch* branch =
			static_cast<Git::Branch*>( modelEvent->getModelIndex().internalData() );

		switch ( modelEvent->getModelEventType() ) {
			case ModelEventType::Open: {
				if ( branch->type != Git::RefType::Stash )
					checkout( *branch );
				else
					stashApply( *branch );
				break;
			}
			case ModelEventType::OpenMenu: {
				openBranchMenu( *branch );
				break;
			}
			default:
				break;
		}
	} );

	mBranchesTree->on( Event::KeyDown, [this]( const Event* event ) {
		const KeyEvent* keyEvent = event->asKeyEvent();
		ModelIndex modelIndex = mBranchesTree->getSelection().first();
		if ( !modelIndex.isValid() || modelIndex.internalId() == -1 || !mBranchesTree->getModel() )
			return;
		Git::Branch branch =
			static_cast<const GitBranchModel*>( mBranchesTree->getModel() )->branch( modelIndex );

		switch ( keyEvent->getKeyCode() ) {
			case KEY_F7:
				branchCreate();
				break;
			case KEY_F2:
				branchRename( branch );
				break;
			case KEY_DELETE:
				if ( branch.type == Git::RefType::Stash )
					stashDrop( branch );
				else if ( branch.type == Git::RefType::Head )
					branchDelete( branch );
				break;
			default:
				break;
		}
	} );

	auto listBox = mPanelSwicher->getListBox();
	listBox->addListBoxItems( { i18n( "branches", "Branches" ), i18n( "status", "Status" ),
								i18n( "git_history", "History" ) } );
	mStackMap.resize( 3 );
	mStackMap[0] = mTabContents->find<UIWidget>( "git_branches" );
	mStackMap[1] = mTabContents->find<UIWidget>( "git_status" );
	mStackMap[2] = mTabContents->find<UIWidget>( "git_history" );

	mPanelSwicher->on( Event::OnItemSelected, [this, listBox]( const Event* ) {
		mStackWidget->setActiveWidget( mStackMap[listBox->getItemSelectedIndex()] );
		if ( listBox->getItemSelectedIndex() == 2 ) {
			if ( mBranchesTree && mBranchesTree->getModel() )
				updateHistoryRefs(
					std::static_pointer_cast<GitBranchModel>( mBranchesTree->getModelShared() ) );
			ensureHistoryLoaded();
		}
	} );
	listBox->setSelected( 0 );
	mStackWidget->setActiveWidget( mStackMap[0] );

	mHistoryTree->setAutoExpandOnSingleColumn( true );
	mHistoryTree->setFitAllColumnsToWidget( true );
	mHistoryTree->setHorizontalScrollMode( ScrollBarMode::AlwaysOff );
	mHistoryTree->setAutoColumnsWidth( true );
	mHistoryTree->setRowHeight( PixelDensity::dpToPx( 36 ) );
	mHistoryTree->setHeadersVisible( false );
	mHistoryTree->setExpandersAsIcons( true );
	mHistoryTree->setScrollViewType( ScrollViewType::Overlay );
	mHistoryTree->setIndentWidth( PixelDensity::dpToPx( 16 ) );
	mHistoryTree->on( Event::OnModelEvent, [this]( const Event* event ) {
		const auto* modelEvent = static_cast<const ModelEvent*>( event );
		if ( modelEvent->getModelEventType() == ModelEventType::OpenTree )
			activateHistoryIndex( modelEvent->getModelIndex(), true );
		else if ( modelEvent->getModelEventType() == ModelEventType::Open )
			activateHistoryIndex( modelEvent->getModelIndex(), false );
	} );
	mHistoryTree->setOnSelection( [this]( const ModelIndex& index ) {
		if ( !mHistoryModel )
			return;
		const auto* node = mHistoryModel->node( index );
		if ( node && node->type == GitHistoryModel::NodeType::Commit )
			openCommitDetails( node->commit );
	} );

	mStatusTree->setAutoColumnsWidth( true );
	mStatusTree->setHeadersVisible( false );
	mStatusTree->setExpandersAsIcons( true );
	mStatusTree->setScrollViewType( ScrollViewType::Overlay );
	mStatusTree->setIndentWidth( PixelDensity::dpToPx( 4 ) );
	mStatusTree->setSelectionKind( UIAbstractView::SelectionKind::Multiple );
	mStatusTree->on( Event::OnRowCreated, [this]( const Event* event ) {
		UITableRow* row = event->asRowCreatedEvent()->getRow();
		row->on( Event::MouseUp, [this, row]( const Event* event ) {
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_RMASK )
				mStatusTree->onOpenMenuModelIndex( row->getCurIndex(), event );
		} );
	} );
	mStatusTree->on( Event::OnModelEvent, [this]( const Event* event ) {
		const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
		auto modelShared = mStatusTree->getModelShared();
		if ( !modelShared )
			return;
		auto model = static_cast<GitStatusModel*>( modelShared.get() );

		if ( modelEvent->getModelIndex().internalId() == GitStatusModel::GitFile ) {
			const Git::DiffFile* file = model->file( modelEvent->getModelIndex() );
			if ( file == nullptr )
				return;

			switch ( modelEvent->getModelEventType() ) {
				case ModelEventType::OpenMenu: {
					std::vector<Git::DiffFile> files;
					files.reserve( mStatusTree->getSelection().size() );
					mStatusTree->getSelection().forEachIndex(
						[model, &files]( const ModelIndex& index ) {
							if ( const auto* selectedFile = model->file( index ) )
								files.emplace_back( *selectedFile );
						} );
					if ( files.empty() )
						files.emplace_back( *file );
					openFileStatusMenu( std::move( files ) );
					break;
				}
				case ModelEventType::Open: {
					if ( file->report.type == Git::GitStatusType::Unmerged )
						openConflictResolver( file->file );
					else
						diff( file->file, file->report.type );
					break;
				}
				default:
					break;
			}
		} else if ( modelEvent->getModelIndex().internalId() == GitStatusModel::Status ) {
			switch ( modelEvent->getModelEventType() ) {
				case ModelEventType::OpenMenu: {
					const auto* status = model->statusType( modelEvent->getModelIndex() );
					auto type = status->type;
					if ( type == Git::GitStatusType::Staged ||
						 type == Git::GitStatusType::Untracked ||
						 type == Git::GitStatusType::Changed ) {
						std::string repoPath;
						if ( !status->files.empty() )
							repoPath = mGit->repoPath( status->files.front().file );

						UIPopUpMenu* menu = UIPopUpMenu::New();
						menu->setId( "git_status_type_menu" );

						if ( type == Git::GitStatusType::Staged ) {
							menuAdd( menu, "git-commit", i18n( "git_commit", "Commit" ),
									 "git-commit" );
							menuAdd( menu, "git-diff-staged",
									 i18n( "git_diff_staged", "Diff Staged" ), "diff-multiple" );
							menuAdd( menu, "git-unstage-all",
									 i18n( "git_unstage_all", "Unstage All" ), "diff-removed" );
						}

						if ( type == Git::GitStatusType::Untracked ||
							 type == Git::GitStatusType::Changed )
							menuAdd( menu, "git-stage-all", i18n( "git_stage_all", "Stage All" ),
									 "diff-added" );

						if ( type == Git::GitStatusType::Changed ) {
							menu->addSeparator();
							menuAdd( menu, "git-discard-all",
									 i18n( "git_discard_all", "Discard All" ) );
						}

						menu->on( Event::OnItemClicked, [this, modelShared, repoPath,
														 type]( const Event* event ) {
							if ( !mGit || !modelShared )
								return;
							auto model = static_cast<GitStatusModel*>( modelShared.get() );
							UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
							std::string id( item->getId() );
							if ( id == "git-commit" ) {
								commit( repoPath );
							} else if ( id == "git-stage-all" ) {
								stage( model->getFiles( repoFullName( repoPath ),
														static_cast<Uint32>( type ) ) );
							} else if ( id == "git-unstage-all" ) {
								unstage( model->getFiles( repoFullName( repoPath ),
														  (Uint32)Git::GitStatusType::Staged ) );
							} else if ( id == "git-discard-all" ) {
								discard( model->getFiles( repoFullName( repoPath ),
														  (Uint32)Git::GitStatusType::Changed ) );
							} else if ( id == "git-diff-staged" ) {
								diff( Git::DiffMode::DiffStaged, repoPath );
							}
						} );

						menu->showOverMouseCursor();
					}

					break;
				}
				default:
					break;
			}
		} else if ( modelEvent->getModelIndex().internalId() == GitStatusModel::Repo ) {
			switch ( modelEvent->getModelEventType() ) {
				case ModelEventType::OpenMenu: {
					const auto* repo = model->repo( modelEvent->getModelIndex() );
					if ( repo == nullptr )
						return;

					std::string repoName = repo->repo;
					std::string repoPath = this->repoPath( repo->repo );
					if ( repoPath.empty() && !repo->type.empty() &&
						 !repo->type.front().files.empty() )
						repoPath = mGit->repoPath( repo->type.front().files.front().file );

					if ( repoPath.empty() )
						return;

					UIPopUpMenu* menu = UIPopUpMenu::New();
					menu->setId( "git_repo_type_menu" );

					if ( repo->hasStatusType( Git::GitStatusType::Staged ) ) {
						menuAdd( menu, "git-commit", i18n( "git_commit", "Commit" ), "git-commit" );
					}

					if ( repo->hasStatusType( Git::GitStatusType::Untracked ) ) {
						menuAdd( menu, "git-stage-all", i18n( "git_stage_all", "Stage All" ),
								 "diff-added" );
					}

					menuAdd( menu, "git-fetch", i18n( "git_fetch", "Fetch" ), "repo-fetch" );
					menuAdd( menu, "git-pull", i18n( "git_pull", "Pull" ), "repo-pull" );
					menuAdd( menu, "git-push", i18n( "git_push", "Push" ), "repo-push" );
					menuAdd( menu, "git-stash", i18n( "git_stash_all", "Stash All" ), "git-stash" );
					menuAdd( menu, "git-diff-head", i18n( "git_diff_head", "Diff HEAD" ),
							 "diff-multiple" );

					menu->on( Event::OnItemClicked,
							  [this, model, repoName, repoPath]( const Event* event ) {
								  if ( !mGit )
									  return;
								  UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
								  std::string id( item->getId() );
								  if ( id == "git-stash" ) {
									  stashPush( model->getFiles( repoName ), repoPath );
								  } else if ( id == "git-fetch" ) {
									  fetch( repoPath );
								  } else if ( id == "git-pull" ) {
									  pull( repoPath );
								  } else if ( id == "git-push" ) {
									  push( repoPath );
								  } else if ( id == "git-commit" ) {
									  commit( repoPath );
								  } else if ( id == "git-stage-all" ) {
									  stage( model->getFiles(
										  repoName, (Uint32)Git::GitStatusType::Untracked |
														(Uint32)Git::GitStatusType::Changed ) );
								  } else if ( id == "git-diff-head" ) {
									  diff( Git::DiffMode::DiffHead, repoPath );
								  }
							  } );

					menu->showOverMouseCursor();

					break;
				}
				default:
					break;
			}
		}
	} );

	mRepoDropDown->on( Event::OnItemSelected, [this]( const Event* ) {
		const auto& txt = mRepoDropDown->getListBox()->getItemSelectedText();

		for ( const auto& repo : mRepos ) {
			if ( txt == repo.second ) {
				{
					Lock l( mRepoMutex );
					mRepoSelected = repo.first;
				}
				mHistoryRevision = "HEAD";
				updateHistoryRefs( nullptr );
				invalidateHistory();
				updateBranches( true );
				updateStatus( true );
				break;
			}
		}
	} );
}

void GitPlugin::hideSidePanel() {
	if ( mSidePanel && mTab ) {
		mSidePanel->removeTab( mTab, false );
		mTab = nullptr;
	}
}

void GitPlugin::openBranchMenu( const Git::Branch& branch ) {
	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->setId( "git_branch_menu" );

	if ( branch.type != Git::RefType::Stash ) {
		menuAdd( menu, "git-fetch", i18n( "git_fetch", "Fetch" ), "repo-fetch" );

		if ( gitBranch() != branch.name ) {
			menuAdd( menu, "git-checkout", i18n( "git_checkout_ellipsis", "Check Out..." ),
					 "git-fetch" );
		}

		if ( branch.type == Git::RefType::Head ) {
			menuAdd( menu, "git-branch-rename", i18n( "git_rename", "Rename" ), "", { KEY_F2 } );
			menuAdd( menu, "git-pull", i18n( "git_pull", "Pull" ), "repo-pull" );
			if ( branch.ahead )
				menuAdd( menu, "git-push", i18n( "git_push", "Push" ), "repo-push" );
			if ( branch.behind )
				menuAdd( menu, "git-fast-forward-merge",
						 i18n( "git_fast_forward_merge", "Fast Forward Merge" ) );
			menu->addSeparator();
			menuAdd( menu, "git-branch-delete", i18n( "git_delete_branch", "Delete" ), "remove" );
		}

		menuAdd( menu, "git-merge-branch", i18n( "git_merge_branch", "Merge Branch" ),
				 "git-merge" );
		menuAdd( menu, "git-create-branch", i18n( "git_create_branch", "Create Branch" ),
				 "repo-forked", { KEY_F7 } );
	} else {
		menuAdd( menu, "git-stash-apply", i18n( "git_apply_stash", "Apply Stash" ),
				 "git-stash-apply" );
		menuAdd( menu, "git-stash-drop", i18n( "git_drop_stash", "Drop Stash" ), "git-stash-pop" );
	}

	menu->on( Event::OnItemClicked, [this, branch]( const Event* event ) {
		if ( !mGit )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		std::string id( item->getId() );
		if ( id == "git-checkout" ) {
			checkout( branch );
		} else if ( id == "git-pull" ) {
			pull( repoSelected() );
		} else if ( id == "git-push" ) {
			push( repoSelected() );
		} else if ( id == "git-branch-delete" ) {
			branchDelete( branch );
		} else if ( id == "git-branch-rename" ) {
			branchRename( branch );
		} else if ( id == "git-fetch" ) {
			fetch( repoSelected() );
		} else if ( id == "git-fast-forward-merge" ) {
			fastForwardMerge( branch );
		} else if ( id == "git-create-branch" ) {
			branchCreate();
		} else if ( id == "git-stash-apply" ) {
			stashApply( branch );
		} else if ( id == "git-stash-drop" ) {
			stashDrop( branch );
		} else if ( id == "git-merge-branch" ) {
			branchMerge( branch );
		}
	} );

	menu->showOverMouseCursor();
}

void GitPlugin::openFileStatusMenu( std::vector<Git::DiffFile> files ) {
	if ( files.empty() )
		return;

	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->setId( "git_file_status_menu" );

	const bool multiple = files.size() > 1;
	bool hasStaged = false;
	bool hasUnstaged = false;
	bool hasUnmerged = false;
	for ( const auto& file : files ) {
		hasStaged |= file.report.type == Git::GitStatusType::Staged;
		hasUnstaged |= file.report.type != Git::GitStatusType::Staged;
		hasUnmerged |= file.report.type == Git::GitStatusType::Unmerged;
	}
	if ( hasUnmerged && !multiple ) {
		menuAdd( menu, "git-resolve-conflict", i18n( "git_resolve_conflict", "Resolve Conflict" ),
				 "diff-modified" );
		menuAdd( menu, "git-accept-ours", i18n( "git_accept_ours", "Accept Ours" ) );
		menuAdd( menu, "git-accept-theirs", i18n( "git_accept_theirs", "Accept Theirs" ) );
		menu->on( Event::OnItemClicked, [this, file = files.front()]( const Event* event ) {
			const std::string id = event->getNode()->asType<UIMenuItem>()->getId();
			if ( id == "git-resolve-conflict" )
				openConflictResolver( file.file );
			else if ( id == "git-accept-ours" )
				acceptConflictSide( file.file, true );
			else if ( id == "git-accept-theirs" )
				acceptConflictSide( file.file, false );
		} );
		menu->showOverMouseCursor();
		return;
	}

	menuAdd( menu, "git-open-file",
			 multiple ? i18n( "git_open_files", "Open Files" )
					  : i18n( "git_open_file", "Open File" ),
			 "file" );
	menuAdd( menu, "git-diff",
			 multiple ? i18n( "git_open_diffs", "Open Diffs" )
					  : i18n( "git_open_diff", "Open Diff" ),
			 multiple ? "diff-multiple" : "diff-single" );

	if ( hasUnstaged )
		menuAdd( menu, "git-stage", i18n( "git_stage", "Stage" ), "diff-added" );
	if ( hasStaged )
		menuAdd( menu, "git-unstage", i18n( "git_unstage", "Unstage" ), "diff-removed" );

	menu->addSeparator();

	if ( hasUnstaged )
		menuAdd( menu, "git-discard", i18n( "git_discard", "Discard" ) );

	menu->on( Event::OnItemClicked,
			  [this, files = std::move( files )]( const Event* event ) mutable {
				  if ( !mGit )
					  return;
				  UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
				  std::string id( item->getId() );
				  std::vector<std::string> paths;
				  paths.reserve( files.size() );
				  if ( id == "git-stage" ) {
					  for ( const auto& file : files )
						  if ( file.report.type != Git::GitStatusType::Staged )
							  paths.emplace_back( file.file );
					  stage( paths );
				  } else if ( id == "git-unstage" ) {
					  for ( const auto& file : files )
						  if ( file.report.type == Git::GitStatusType::Staged )
							  paths.emplace_back( file.file );
					  unstage( paths );
				  } else if ( id == "git-discard" ) {
					  for ( const auto& file : files )
						  if ( file.report.type != Git::GitStatusType::Staged )
							  paths.emplace_back( file.file );
					  if ( paths.size() == 1 )
						  discard( paths.front() );
					  else
						  discard( paths );
				  } else if ( id == "git-open-file" ) {
					  for ( const auto& file : files )
						  openFile( file.file );
				  } else if ( id == "git-diff" ) {
					  if ( files.size() == 1 )
						  diff( files.front().file, files.front().report.type );
					  else
						  diff( std::move( files ) );
				  }
			  } );

	menu->showOverMouseCursor();
}

void GitPlugin::runAsync( std::function<Git::Result()> fn, bool _updateStatus, bool _updateBranches,
						  bool displaySuccessMsg, bool updateBranchesOnError,
						  bool updateStatusOnError, bool historyChanged ) {
	if ( !mGit )
		return;
	mLoader->setVisible( true );
	const auto lifetime = mLifetime.weakHandle();
	runAsyncTask( [lifetime, fn, _updateStatus, _updateBranches, displaySuccessMsg,
				   updateBranchesOnError, updateStatusOnError, historyChanged] {
		auto res = fn();
		lifetime.run( [res = std::move( res ), _updateStatus, _updateBranches, displaySuccessMsg,
					   updateBranchesOnError, updateStatusOnError,
					   historyChanged]( GitPlugin* plugin ) mutable {
			plugin->mLoader->setVisible( false );
			if ( res.success() && historyChanged )
				plugin->invalidateHistory();
			if ( res.fail() || displaySuccessMsg ) {
				plugin->showMessage( LSPMessageType::Warning, res.result );
				if ( _updateBranches && updateBranchesOnError )
					plugin->updateBranches();
				if ( _updateStatus && updateStatusOnError )
					plugin->updateStatus( true );
				return;
			}
			if ( _updateBranches )
				plugin->updateBranches();
			if ( _updateStatus )
				plugin->updateStatus( true );
		} );
	} );
}

void GitPlugin::runMergeLikeAsync( std::function<Git::Result( Git& )> fn,
								   const std::string& repoPath ) {
	if ( !mGit )
		return;
	mLoader->setVisible( true );
	auto git = mGit;
	const auto lifetime = mLifetime.weakHandle();
	runAsyncTask( [git = std::move( git ), lifetime, fn = std::move( fn ), repoPath] {
		auto result = fn( *git );
		Git::ConflictState conflicts;
		if ( result.fail() )
			conflicts = git->conflictState( repoPath );
		lifetime.run( [result = std::move( result ),
					   conflicts = std::move( conflicts )]( GitPlugin* plugin ) mutable {
			plugin->mLoader->setVisible( false );
			if ( result.success() )
				plugin->invalidateHistory();
			plugin->updateBranches();
			plugin->updateStatus( true );
			if ( result.fail() && !conflicts.hasConflicts() )
				plugin->showMessage( LSPMessageType::Warning, result.result );
		} );
	} );
}

void GitPlugin::menuAdd( UIMenu* menu, const std::string& cmd, const std::string& text,
						 const std::string& icon, const KeyBindings::Shortcut& forcedKeybinding ) {

	menu->add( text, iconDrawable( icon, 12 ),
			   forcedKeybinding.empty() ? KeyBindings::keybindFormat( mKeyBindings[cmd] )
										: getUISceneNode()->getKeyBindings().getShortcutString(
											  forcedKeybinding, true ) )
		->setId( cmd );
}

std::string GitPlugin::repoSelected() {
	Lock l( mRepoMutex );
	return mRepoSelected;
}

std::string GitPlugin::projectPath() {
	Lock l( mRepoMutex );
	return mProjectPath;
}

std::string GitPlugin::repoName( const std::string& repoPath ) {
	Lock l( mRepoMutex );
	for ( const auto& repo : mRepos )
		if ( repo.first == repoPath )
			return repo.second;
	return "";
}

std::string GitPlugin::repoFullName( const std::string& repoPath ) {
	Lock l( mRepoMutex );
	for ( const auto& repo : mRepos ) {
		if ( repo.first == repoPath ) {
			if ( repoPath != mProjectPath ) {
				auto fullName( repo.first );
				FileSystem::filePathRemoveBasePath( mProjectPath, fullName );
				return fullName;
			}
			return repo.second;
		}
	}
	return "";
}

std::string GitPlugin::repoPath( const std::string& repoName ) {
	Lock l( mRepoMutex );
	for ( const auto& repo : mRepos )
		if ( repo.second == repoName )
			return repo.first;
	return "";
}

} // namespace ecode
