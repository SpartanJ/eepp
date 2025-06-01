#include "gitplugin.hpp"
#include "gitbranchmodel.hpp"
#include "gitstatusmodel.hpp"
#include <eepp/graphics/primitives.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uiiconthememanager.hpp>
#include <eepp/ui/uiloader.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uistackwidget.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <nlohmann/json.hpp>

using namespace EE::UI;
using namespace EE::UI::Doc;

using namespace std::literals;

using json = nlohmann::json;
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
#define GIT_THREADED 1
#else
#define GIT_THREADED 0
#endif

namespace ecode {

static constexpr auto DEFAULT_HIGHLIGHT_COLOR = "var(--font-highlight)"sv;
static constexpr auto GIT_STATUS_UPDATE_TAG = String::hash( "git::status-update" );

std::string GitPlugin::statusTypeToString( Git::GitStatusType type ) {
	switch ( type ) {
		case Git::GitStatusType::Untracked:
			return i18n( "git_untracked", "Untracked" );
		case Git::GitStatusType::Unmerged:
			return i18n( "git_unmerged", "Unmerged" );
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
	PluginBase( pluginManager ), mHighlightStyleColor( DEFAULT_HIGHLIGHT_COLOR ) {
	if ( sync ) {
		load( pluginManager );
	} else {
#if defined( GIT_THREADED ) && GIT_THREADED == 1
		mThreadPool->run( [this, pluginManager] { load( pluginManager ); } );
#else
		load( pluginManager );
#endif
	}
}

GitPlugin::~GitPlugin() {
	waitUntilLoaded();
	mShuttingDown = true;
	if ( mStatusButton )
		mStatusButton->close();

	if ( mSidePanel && mTab )
		mSidePanel->removeTab( mTab );

	endModelStyler();

	if ( getUISceneNode() )
		getUISceneNode()->removeActionsByTag( GIT_STATUS_UPDATE_TAG );

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

	mGit = std::make_unique<Git>( pluginManager->getWorkspaceFolder() );
	mGit->setSilent( mSilent );
	mGitFound = !mGit->getGitPath().empty();
	mProjectPath = mRepoSelected = mGit->getProjectPath();

	if ( getUISceneNode() ) {
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

	mModelStylerId = projectView->getModel()->subsribeModelStyler(
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
		projectView->getModel()->unsubsribeModelStyler( mModelStylerId );
		mModelStylerId = 0;
	}
}

void GitPlugin::updateUINow( bool force ) {
	if ( !mGit || !getUISceneNode() )
		return;

	if ( !mProjectPath.empty() )
		getUISceneNode()->runOnMainThread( [this] { buildSidePanelTab(); } );

	updateStatus( force );
	updateBranches();
}

void GitPlugin::updateUI() {
	if ( !mGit || !getUISceneNode() )
		return;

	getUISceneNode()->debounce( [this] { updateUINow(); }, mRefreshFreq, GIT_STATUS_UPDATE_TAG );
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
		mStatusButton->getTextBox()->setUsingCustomStyling( true );

		mStatusButton->on( Event::MouseClick, [this]( const Event* event ) {
			if ( nullptr == mTab )
				return;
			mTab->setTabSelected();
			if ( event->asMouseEvent()->getFlags() & EE_BUTTON_RMASK )
				mPanelSwicher->getListBox()->setSelected( 0 );
			else if ( mGitStatus.totalInserts || mGitStatus.totalDeletions )
				mPanelSwicher->getListBox()->setSelected( 1 );
		} );

		if ( mStatusBar->getNextNode() == nullptr ||
			 mStatusBar->getNextNode()->getId() != "doc_info" ) {
			auto docInfo = mStatusBar->find( "doc_info" );
			if ( docInfo != nullptr && docInfo->getParent() == mStatusButton->getParent() ) {
				mStatusButton->toPosition( docInfo->getNodeIndex() );
			}
		}
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
								   *mStatusButton->getTextBox()->getTextCache() );

	mStatusButton->invalidateDraw();
}

void GitPlugin::updateStatus( bool force ) {
	if ( !mGit || !mGitFound || mRunningUpdateStatus )
		return;

	if ( !mGit || mGit->getGitFolder().empty() ) {
		getUISceneNode()->runOnMainThread( [this] { updateStatusBarSync(); } );
		return;
	}

	mRunningUpdateStatus++;
	mThreadPool->run(
		[this, force] {
			if ( !mGit || mGit->getGitFolder().empty() ) {
				getUISceneNode()->runOnMainThread( [this] { updateStatusBarSync(); } );
				return;
			}

			auto prevBranch = updateReposBranches();
			Git::Status prevGitStatus;
			{
				Lock l( mGitStatusMutex );
				prevGitStatus = mGitStatus;
			}
			Git::Status newGitStatus = mGit->status( mStatusRecurseSubmodules );
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
				if ( !force && mGitBranches == prevBranch && mGitStatus == prevGitStatus )
					return;
			}

			getUISceneNode()->runOnMainThread( [this] { updateStatusBarSync(); } );
		},
		[this]( auto ) { mRunningUpdateStatus--; } );
}

PluginRequestHandle GitPlugin::processMessage( const PluginMessage& msg ) {
	switch ( msg.type ) {
		case PluginMessageType::WorkspaceFolderChanged: {
			if ( mGit ) {
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
					getUISceneNode()->runOnMainThread( [this] {
						if ( mProjectPath.empty() ) {
							hideSidePanel();
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

	Text::wrapText( str, PixelDensity::dpToPx( 400 ), tooltip->getFontStyleConfig(),
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
								   *tooltip->getTextCache() );

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
	listeners.push_back(
		editor->addEventListener( Event::OnCursorPosChange, [this, editor]( const Event* ) {
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
				  "Git binary not found.\nPlease check that git is accesible via PATH" ) );
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

	const auto checkOutFn = [this, branch]( bool createLocal ) {
		mLoader->setVisible( true );
		mThreadPool->run( [this, branch, createLocal] {
			auto result =
				createLocal ? mGit->checkoutAndCreateLocalBranch( branch.name, "", repoSelected() )
							: mGit->checkout( branch.name, repoSelected() );
			if ( result.success() ) {
				{
					std::string repoSel = repoSelected();
					Lock l( mGitBranchMutex );
					mGitBranches[repoSel] = branch.name;
				}
				if ( mBranchesTree->getModel() ) {
					if ( createLocal )
						updateBranches();
					else
						mBranchesTree->getModel()->invalidate( Model::DontInvalidateIndexes );
				}
			} else {
				showMessage( LSPMessageType::Warning, result.result );
			}
			getUISceneNode()->runOnMainThread( [this] { mLoader->setVisible( false ); } );
		} );
	};

	if ( branch.type == Git::RefType::Remote ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::YES_NO, i18n( "git_create_local_branch", "Create local branch?" ) );
		msgBox->on( Event::OnConfirm, [checkOutFn]( const Event* ) { checkOutFn( true ); } );
		msgBox->on( Event::OnCancel, [checkOutFn]( const Event* ) { checkOutFn( false ); } );
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
		runAsync(
			[this, branch]() { return mGit->mergeBranch( branch.name, false, repoSelected() ); },
			true, true, true, true, true );
	} );
	msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	msgBox->setTitle( i18n( "git_confirm", "Confirm" ) );
	msgBox->center();
	msgBox->showWhenReady();
}

void GitPlugin::pull( const std::string& repoPath ) {
	runAsync( [this, repoPath]() { return mGit->pull( repoPath ); }, true, true, true );
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

void GitPlugin::commit( const std::string& repoPath ) {
	if ( !mGitStatus.hasStagedChanges( mGit->repoName( repoPath, true ) ) ) {
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
	txtEdit->setLineWrapMode( LineWrapMode::Letter );
	txtEdit->setText( mLastCommitMsg );

	UICheckBox* chkAmmend = UICheckBox::New();
	chkAmmend->setLayoutMargin( Rectf( 0, 8, 0, 0 ) )
		->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent )
		->setLayoutGravity( UI_HALIGN_LEFT | UI_VALIGN_CENTER )
		->setClipType( ClipType::None )
		->setParent( msgBox->getLayoutCont()->getFirstChild() )
		->setId( "git-ammend" );
	chkAmmend->setText( i18n( "git_ammend", "Ammend last commit" ) );
	chkAmmend->toPosition( 2 );
	chkAmmend->setTooltipText( getUISceneNode()->getKeyBindings().getShortcutString(
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

	txtEdit->getDocument().setCommand(
		"commit-ammend", [chkAmmend] { chkAmmend->setChecked( !chkAmmend->isChecked() ); } );
	txtEdit->getKeyBindings().addKeybind( { KEY_L, KeyMod::getDefaultModifier() },
										  "commit-ammend" );

	txtEdit->getDocument().setCommand(
		"commit-push", [chkPush] { chkPush->setChecked( !chkPush->isChecked() ); } );
	txtEdit->getKeyBindings().addKeybind( { KEY_P, KeyMod::getDefaultModifier() }, "commit-push" );

	txtEdit->getDocument().setCommand( "commit-bypass-hook", [chkBypassHook] {
		chkBypassHook->setChecked( !chkBypassHook->isChecked() );
	} );
	txtEdit->getKeyBindings().addKeybind( { KEY_B, KeyMod::getDefaultModifier() },
										  "commit-bypass-hook" );

	msgBox->on( Event::OnConfirm, [this, msgBox, chkAmmend, chkBypassHook, chkPush,
								   repoPath]( const Event* ) {
		std::string msg( msgBox->getTextEdit()->getText().toUtf8() );
		if ( msg.empty() )
			return;
		bool ammend = chkAmmend->isChecked();
		bool bypassHook = chkBypassHook->isChecked();
		bool pushCommit = chkPush->isChecked();

		msgBox->closeWindow();
		runAsync(
			[this, msg, ammend, bypassHook, pushCommit, repoPath]() {
				std::optional<Git::Branch> branch = getBranchFromRepoPath( repoPath );
				bool pushNewBranch = branch && !branch->name.empty() && branch->remote.empty();
				auto res = mGit->commit( msg, ammend, bypassHook, repoPath );
				if ( res.success() ) {
					mLastCommitMsg.clear();
					if ( pushCommit ) {
						if ( pushNewBranch )
							return mGit->pushNewBranch( branch->name, repoPath );
						return mGit->push( repoPath );
					}
				} else
					mLastCommitMsg = msg;
				return res;
			},
			true, true, true, true, true );
	} );

	msgBox->on( Event::OnCancel, [this, msgBox]( const Event* ) {
		mLastCommitMsg = msgBox->getTextEdit()->getText();
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
		false, true );
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
	if ( files.empty() )
		return;
	runAsync(
		[this, files]() { return mGit->add( fixFilePaths( files ), mGit->repoPath( files[0] ) ); },
		true, false );
}

void GitPlugin::unstage( const std::vector<std::string>& files ) {
	if ( files.empty() )
		return;
	runAsync(
		[this, files]() {
			return mGit->reset( fixFilePaths( files ), mGit->repoPath( files[0] ) );
		},
		true, false );
}

void GitPlugin::discard( const std::vector<std::string>& files ) {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::OK_CANCEL,
		i18n( "git_confirm_discard_changes", "Are you sure you want to discard all file changes?" )
			.toUtf8() );

	msgBox->on( Event::OnConfirm, [this, files]( auto ) {
		runAsync(
			[this, files]() {
				return mGit->restore( fixFilePaths( files ), mGit->repoPath( files[0] ) );
			},
			true, true );
	} );
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
	getUISceneNode()->runOnMainThread( [this, file] {
		mManager->getLoadFileFn()( mGit->getProjectPath() + file, []( auto, auto ) {} );
	} );
}

void GitPlugin::diff( const Git::DiffMode mode, const std::string& repoPath ) {
	mThreadPool->run( [this, mode, repoPath] {
		auto res = mGit->diff( mode, repoPath );
		if ( res.fail() )
			return;

		std::string repoName = this->repoName( repoPath );
		getUISceneNode()->runOnMainThread( [this, mode, res, repoName] {
			auto ret = mManager->getSplitter()->createEditorInNewTab();
			auto doc = ret.second->getDocumentRef();
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
			doc->setDefaultFileName( repoName + "-" + modeName + ".diff" );
			ret.second->setSyntaxDefinition(
				SyntaxDefinitionManager::instance()->getByLSPName( "diff" ) );
			doc->textInput( res.result, false );
			doc->moveToStartOfDoc();
			doc->resetUndoRedo();
		} );
	} );
}

void GitPlugin::diff( const std::string& file, bool isStaged ) {
	mThreadPool->run( [this, file, isStaged] {
		auto res = mGit->diff( fixFilePath( file ), isStaged, mGit->repoPath( file ) );
		if ( res.fail() )
			return;

		getUISceneNode()->runOnMainThread( [this, file, res] {
			auto ret = mManager->getSplitter()->createEditorInNewTab();
			auto doc = ret.second->getDocumentRef();
			doc->setDefaultFileName( FileSystem::fileNameFromPath( file ) + ".diff" );
			doc->textInput( res.result, false );
			doc->moveToStartOfDoc();
			doc->resetUndoRedo();
			ret.second->setSyntaxDefinition(
				SyntaxDefinitionManager::instance()->getByLSPName( "diff" ) );
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
				   !icon.empty() ? findIcon( icon )->getSize( PixelDensity::dpToPxI( 12 ) )
								 : nullptr,
				   KeyBindings::keybindFormat( mKeyBindings[txtKey] ) )
			->setId( txtKey );
	};

	addFn( "git-blame", "Git Blame" );

	menu->addSubMenu( i18n( "git", "Git" ),
					  mManager->getUISceneNode()
						  ->findIcon( "source-control" )
						  ->getSize( PixelDensity::dpToPxI( 12 ) ),
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

void GitPlugin::updateBranches( bool force ) {
	if ( !mGit || !mGitFound || ( mRunningUpdateBranches && !force ) )
		return;

	if ( !mGit || mGit->getGitFolder().empty() ) {
		getUISceneNode()->runOnMainThread( [this] { updateBranchesUI( nullptr ); } );
		return;
	}

	mRunningUpdateBranches++;
	mThreadPool->run(
		[this] {
			if ( !mGit || mGit->getGitFolder().empty() ) {
				getUISceneNode()->runOnMainThread( [this] { updateBranchesUI( nullptr ); } );
				return;
			}

			auto prevBranch = updateReposBranches();
			auto branches = mGit->getAllBranchesAndTags( Git::RefType::All, {}, repoSelected() );
			auto hash = GitBranchModel::hashBranches( branches );
			auto model = GitBranchModel::asModel( std::move( branches ), hash, this );

			if ( mBranchesTree && mBranchesTree->getModel() &&
				 static_cast<GitBranchModel*>( mBranchesTree->getModel() )->getHash() == hash ) {
				if ( prevBranch != mGitBranches )
					mBranchesTree->getModel()->invalidate( Model::DontInvalidateIndexes );
				return;
			}

			getUISceneNode()->runOnMainThread( [this, model] { updateBranchesUI( model ); } );
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
		mTab = mSidePanel->add( i18n( "source_control", "Source Control" ), mTabContents,
								icon ? icon->getSize( PixelDensity::dpToPx( 12 ) ) : nullptr );
		mTab->setId( "source_control_tab" );
		mTab->setTextAsFallback( true );
		return;
	}
	if ( mTab )
		return;
	if ( mSidePanel == nullptr )
		getUISceneNode()->bind( "panel", mSidePanel );
	static constexpr auto STYLE = R"html(
	<style>
	#git_branches_tree ScrollBar,
	#git_status_tree ScrollBar {
		opacity: 0;
		transition: opacity 0.15;
	}
	#git_branches_tree:hover ScrollBar,
	#git_branches_tree ScrollBar.dragging,
	#git_branches_tree ScrollBar:focus-within,
	#git_status_tree:hover ScrollBar,
	#git_status_tree ScrollBar.dragging,
	#git_status_tree ScrollBar:focus-within {
		opacity: 1;
	}
	.git_highlight_style > treeview::cell::text {
		color: %s;
	}
	treeview::row treeview::cell.git_highlight_style_clear,
	treeview::row:selected .git_highlight_style > treeview::cell::text,
	treeview::row:selected .git_highlight_style > treeview::cell::text {
		color: var(--font);
	}
	.git_highlight_style > treeview::cell::icon {
		foreground-image: icon(circle, 8dpru), icon(circle-filled, 8dpru);
		foreground-position: 80%% 80%%, 80%% 80%%;
		foreground-tint: black, %s;
	}
	.git_highlight_style_clear > treeview::cell::icon {
		foreground-image: none, none;
	}
	</style>
	<RelativeLayout id="git_panel" lw="mp" lh="mp">
		<vbox id="git_content" lw="mp" lh="mp">
			<DropDownList id="git_panel_switcher" lw="mp" lh="22dp" border-type="inside" border-right-width="0" border-left-width="0" border-top-width="0" border-bottom-left-radius="0" border-bottom-right-radius="0" />
			<StackWidget id="git_panel_stack" lw="mp" lh="0" lw8="1">
				<vbox id="git_branches" lw="mp" lh="wc">
					<hbox lw="mp" lh="wc" padding="4dp">
						<DropDownList id="git_repo" lw="0" lh="wc" lw8="1" />
						<PushButton id="branch_pull" text="@string(git_pull, Pull)" tooltip="@string(pull_branch, Pull Branch)" text-as-fallback="true" icon="icon(repo-pull, 12dp)" margin-left="2dp" />
						<PushButton id="branch_push" text="@string(git_push, Push)" tooltip="@string(push_branch, Push Branch)" text-as-fallback="true" icon="icon(repo-push, 12dp)" margin-left="2dp" />
						<PushButton id="branch_add" text="@string(git_add_branch, Add Branch)" tooltip="@string(add_branch, Add Branch)" text-as-fallback="true" icon="icon(add, 12dp)" margin-left="2dp" />
					</hbox>
					<TreeView id="git_branches_tree" lw="mp" lh="0" lw8="1" />
				</vbox>
				<vbox id="git_status" lw="mp" lh="mp">
					<TreeView id="git_status_tree" lw="mp" lh="mp" />
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
	mTabContents = getUISceneNode()->loadLayoutFromString( String::format( STYLE, color, color ) );
	mTab = mSidePanel->add( i18n( "source_control", "Source Control" ), mTabContents,
							icon ? icon->getSize( PixelDensity::dpToPx( 12 ) ) : nullptr );
	mTab->setId( "source_control_tab" );
	mTab->setTextAsFallback( true );

	mTabContents->bind( "git_panel_switcher", mPanelSwicher );
	mTabContents->bind( "git_panel_stack", mStackWidget );
	mTabContents->bind( "git_branches_tree", mBranchesTree );
	mTabContents->bind( "git_status_tree", mStatusTree );
	mTabContents->bind( "git_content", mGitContentView );
	mTabContents->bind( "git_no_content", mGitNoContentView );
	mTabContents->bind( "git_panel_loader", mLoader );
	mTabContents->bind( "git_repo", mRepoDropDown );

	mTabContents->find( "branch_pull" )->onClick( [this]( auto ) { pull( repoSelected() ); } );
	mTabContents->find( "branch_push" )->onClick( [this]( auto ) { push( repoSelected() ); } );
	mTabContents->find( "branch_add" )->onClick( [this]( auto ) { branchCreate(); } );

	mBranchesTree->setAutoExpandOnSingleColumn( true );
	mBranchesTree->setHeadersVisible( false );
	mBranchesTree->setExpandersAsIcons( true );
	mBranchesTree->setIndentWidth( PixelDensity::dpToPx( 16 ) );
	mBranchesTree->setScrollViewType( UIScrollableWidget::Inclusive );
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
	listBox->addListBoxItems( { i18n( "branches", "Branches" ), i18n( "status", "Status" ) } );
	mStackMap.resize( 2 );
	mStackMap[0] = mTabContents->find<UIWidget>( "git_branches" );
	mStackMap[1] = mTabContents->find<UIWidget>( "git_status" );
	listBox->setSelected( 0 );

	mPanelSwicher->on( Event::OnItemSelected, [this, listBox]( const Event* ) {
		mStackWidget->setActiveWidget( mStackMap[listBox->getItemSelectedIndex()] );
	} );

	mStatusTree->setAutoColumnsWidth( true );
	mStatusTree->setHeadersVisible( false );
	mStatusTree->setExpandersAsIcons( true );
	mStatusTree->setScrollViewType( UIScrollableWidget::Inclusive );
	mStatusTree->setIndentWidth( PixelDensity::dpToPx( 4 ) );
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
					openFileStatusMenu( *file );
					break;
				}
				case ModelEventType::Open: {
					diff( file->file, file->report.type == Git::GitStatusType::Staged );
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
				updateBranches( true );
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

void GitPlugin::openFileStatusMenu( const Git::DiffFile& file ) {
	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->setId( "git_file_status_menu" );

	menuAdd( menu, "git-open-file", i18n( "git_open_file", "Open File" ), "file" );
	menuAdd( menu, "git-diff", i18n( "git_open_diff", "Open Diff" ), "diff-single" );

	if ( file.report.type != Git::GitStatusType::Staged ) {
		menuAdd( menu, "git-stage", i18n( "git_stage", "Stage" ), "diff-added" );
	} else {
		menuAdd( menu, "git-unstage", i18n( "git_unstage", "Unstage" ), "diff-removed" );
	}

	menu->addSeparator();

	if ( file.report.type != Git::GitStatusType::Staged )
		menuAdd( menu, "git-discard", i18n( "git_discard", "Discard" ) );

	menu->on( Event::OnItemClicked, [this, file]( const Event* event ) {
		if ( !mGit )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		std::string id( item->getId() );
		if ( id == "git-stage" ) {
			stage( { file.file } );
		} else if ( id == "git-unstage" ) {
			unstage( { file.file } );
		} else if ( id == "git-discard" ) {
			discard( file.file );
		} else if ( id == "git-open-file" ) {
			openFile( file.file );
		} else if ( id == "git-diff" ) {
			diff( file.file, file.report.type == Git::GitStatusType::Staged );
		}
	} );

	menu->showOverMouseCursor();
}

void GitPlugin::runAsync( std::function<Git::Result()> fn, bool _updateStatus, bool _updateBranches,
						  bool displaySuccessMsg, bool updateBranchesOnError,
						  bool updateStatusOnError ) {
	if ( !mGit )
		return;
	mLoader->setVisible( true );
	mThreadPool->run( [this, fn, _updateStatus, _updateBranches, displaySuccessMsg,
					   updateBranchesOnError, updateStatusOnError] {
		auto res = fn();
		mLoader->runOnMainThread( [this] { mLoader->setVisible( false ); } );
		if ( res.fail() || displaySuccessMsg ) {
			showMessage( LSPMessageType::Warning, res.result );
			if ( _updateBranches && updateBranchesOnError )
				updateBranches();
			if ( _updateStatus && updateStatusOnError )
				updateStatus( true );
			return;
		}
		if ( _updateBranches )
			updateBranches();

		if ( _updateStatus )
			updateStatus( true );
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
