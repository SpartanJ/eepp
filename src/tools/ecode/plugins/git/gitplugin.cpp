#include "gitplugin.hpp"
#include <eepp/graphics/primitives.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <nlohmann/json.hpp>

using namespace EE::UI::Doc;

using json = nlohmann::json;
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
#define GIT_THREADED 1
#else
#define GIT_THREADED 0
#endif

namespace ecode {

static const char* GIT_EMPTY = "";
static const char* GIT_SUCCESS = "theme-success";
static const char* GIT_ERROR = "theme-error";
static const char* GIT_BOLD = "bold";
static const char* GIT_NOT_BOLD = "notbold";

class GitBranchModel : public Model {
  public:
	static std::shared_ptr<GitBranchModel> asModel( std::vector<Git::Branch>&& branches,
													GitPlugin* gitPlugin ) {
		return std::make_shared<GitBranchModel>( std::move( branches ), gitPlugin );
	}

	enum Column { Name, Remote, Type, LastCommit };

	struct BranchData {
		std::string branch;
		std::vector<Git::Branch> data;
	};

	std::string refTypeToString( Git::RefType type ) {
		switch ( type ) {
			case Git::RefType::Head:
				return mPlugin->i18n( "git_local_branches", "Local Branches" ).toUtf8();
			case Git::RefType::Remote:
				return mPlugin->i18n( "git_remote_branches", "Remote Branches" ).toUtf8();
			case Git::RefType::Tag:
				return mPlugin->i18n( "git_tags", "Tags" ).toUtf8();
			default:
				break;
		}
		return "";
	}

	GitBranchModel( std::vector<Git::Branch>&& branches, GitPlugin* gitPlugin ) :
		mPlugin( gitPlugin ) {
		std::map<std::string, std::vector<Git::Branch>> branchTypes;
		for ( auto& branch : branches ) {
			auto& type = branchTypes[refTypeToString( branch.type )];
			type.emplace_back( std::move( branch ) );
		}
		for ( auto& branch : branchTypes ) {
			mBranches.emplace_back(
				BranchData{ std::move( branch.first ), std::move( branch.second ) } );
		}
	}

	size_t treeColumn() const { return Column::Name; }

	size_t rowCount( const ModelIndex& index ) const {
		if ( !index.isValid() )
			return mBranches.size();
		if ( index.internalId() == -1 )
			return mBranches[index.row()].data.size();
		return 0;
	}

	size_t columnCount( const ModelIndex& ) const { return 4; }

	ModelIndex parentIndex( const ModelIndex& index ) const {
		if ( !index.isValid() || index.internalId() == -1 )
			return {};
		return createIndex( index.internalId(), index.column(), &mBranches[index.internalId()],
							-1 );
	}

	ModelIndex index( int row, int column, const ModelIndex& parent ) const {
		if ( row < 0 || column < 0 )
			return {};
		if ( !parent.isValid() )
			return createIndex( row, column, &mBranches[row], -1 );
		if ( parent.internalData() )
			return createIndex( row, column, &mBranches[parent.row()].data[row], parent.row() );
		return {};
	}

	Variant data( const ModelIndex& index, ModelRole role ) const {
		switch ( role ) {
			case ModelRole::Display: {
				if ( index.internalId() == -1 ) {
					if ( index.column() == Column::Name )
						return Variant( mBranches[index.row()].branch.c_str() );
					return Variant( GIT_EMPTY );
				}
				const Git::Branch& branch = mBranches[index.internalId()].data[index.row()];
				switch ( index.column() ) {
					case Column::Name:
						return Variant( branch.name.c_str() );
					case Column::Remote:
						return Variant( branch.remote.c_str() );
					case Column::Type:
						return Variant( branch.typeStr() );
					case Column::LastCommit:
						return Variant( branch.lastCommit.c_str() );
				}
				return Variant( GIT_EMPTY );
			}
			case ModelRole::Class: {
				if ( index.internalId() == -1 )
					return Variant( GIT_BOLD );
				const Git::Branch& branch = mBranches[index.internalId()].data[index.row()];
				if ( branch.name == mPlugin->gitBranch() )
					return Variant( GIT_BOLD );
				return Variant( GIT_NOT_BOLD );
			}
			default:
				break;
		}
		return {};
	}

	virtual bool classModelRoleEnabled() { return true; }

  protected:
	std::vector<BranchData> mBranches;
	GitPlugin* mPlugin;
};

class GitStatusModel : public Model {
  public:
	static std::shared_ptr<GitStatusModel> asModel( Git::FilesStatus&& status ) {
		return std::make_shared<GitStatusModel>( std::move( status ) );
	}

	struct RepoStatus {
		std::string repo;
		std::vector<Git::DiffFile> files;
	};

	enum Column { File, Inserted, Removed, FileStatus };

	GitStatusModel( Git::FilesStatus&& status ) {
		mStatus.reserve( status.size() );
		for ( auto& s : status )
			mStatus.emplace_back( RepoStatus{ std::move( s.first ), std::move( s.second ) } );
	}

	size_t treeColumn() const { return Column::File; }

	size_t rowCount( const ModelIndex& index ) const {
		if ( !index.isValid() )
			return mStatus.size();
		if ( index.internalId() == -1 )
			return mStatus[index.row()].files.size();
		return 0;
	}

	size_t columnCount( const ModelIndex& ) const { return 4; }

	ModelIndex parentIndex( const ModelIndex& index ) const {
		if ( !index.isValid() || index.internalId() == -1 )
			return {};
		return createIndex( index.internalId(), index.column(), &mStatus[index.internalId()], -1 );
	}

	ModelIndex index( int row, int column, const ModelIndex& parent ) const {
		if ( row < 0 || column < 0 )
			return {};
		if ( !parent.isValid() )
			return createIndex( row, column, &mStatus[row], -1 );
		if ( parent.internalData() )
			return createIndex( row, column, &mStatus[parent.row()].files[row], parent.row() );
		return {};
	}

	Variant data( const ModelIndex& index, ModelRole role ) const {
		if ( role == ModelRole::Display ) {
			if ( index.internalId() == -1 ) {
				if ( index.column() == Column::File )
					return Variant( mStatus[index.row()].repo.c_str() );
				return Variant( GIT_EMPTY );
			}
			const Git::DiffFile& s = mStatus[index.internalId()].files[index.row()];
			switch ( index.column() ) {
				case Column::File:
					return Variant( s.file.c_str() );
				case Column::Inserted:
					return Variant( s.inserts );
				case Column::Removed:
					return Variant( s.deletes );
				case Column::FileStatus:
					return Variant( std::string( static_cast<char>( s.status ), 1 ) );
			}
		} else if ( role == ModelRole::Class ) {
			switch ( index.column() ) {
				case Column::Inserted:
					return Variant( GIT_SUCCESS );
				case Column::Removed:
					return Variant( GIT_ERROR );
				default:
					break;
			}
		}
		return Variant( GIT_EMPTY );
	}

	virtual bool classModelRoleEnabled() { return true; }

  protected:
	std::vector<RepoStatus> mStatus;
};

Plugin* GitPlugin::New( PluginManager* pluginManager ) {
	return eeNew( GitPlugin, ( pluginManager, false ) );
}

Plugin* GitPlugin::NewSync( PluginManager* pluginManager ) {
	return eeNew( GitPlugin, ( pluginManager, true ) );
}

GitPlugin::GitPlugin( PluginManager* pluginManager, bool sync ) : PluginBase( pluginManager ) {
	if ( sync ) {
		load( pluginManager );
	} else {
#if defined( GIT_THREADED ) && GIT_THREADED == 1
		mThreadPool->run( [&, pluginManager] { load( pluginManager ); } );
#else
		load( pluginManager );
#endif
	}
}

GitPlugin::~GitPlugin() {
	mShuttingDown = true;
	if ( mStatusButton )
		mStatusButton->close();
	if ( mSidePanel && mTab )
		mSidePanel->removeTab( mTab );
}

void GitPlugin::load( PluginManager* pluginManager ) {
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
	mGitFound = !mGit->getGitPath().empty();

	if ( getUISceneNode() ) {
		updateStatusBar();
		// updateBranches();
	}

	subscribeFileSystemListener();
	mReady = true;
	fireReadyCbs();
	setReady();
}

void GitPlugin::updateUINow( bool force ) {
	if ( !mGit || !getUISceneNode() )
		return;

	updateStatusBar( force );
	// updateBranches();
}

void GitPlugin::updateUI() {
	if ( !mGit || !getUISceneNode() )
		return;

	getUISceneNode()->debounce( [this] { updateUINow(); }, mRefreshFreq,
								String::hash( "git::status-update" ) );
}

void GitPlugin::updateStatusBarSync() {
	if ( !mStatusBar )
		getUISceneNode()->bind( "status_bar", mStatusBar );
	if ( !mStatusBar )
		return;

	if ( !mStatusButton ) {
		mStatusButton = UIPushButton::New();
		mStatusButton->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::MatchParent );
		mStatusButton->setParent( mStatusBar );
		mStatusButton->setId( "status_git" );
		mStatusButton->setClass( "status_but" );
		mStatusButton->setIcon( getManager()
									->getUISceneNode()
									->findIcon( "source-control" )
									->getSize( PixelDensity::dpToPxI( 10 ) ) );
		mStatusButton->reloadStyle( true, true );
		mStatusButton->getTextBox()->setUsingCustomStyling( true );
		auto childCount = mStatusBar->getChildCount();
		if ( childCount > 2 )
			mStatusButton->toPosition( mStatusBar->getChildCount() - 2 );
	}

	mStatusButton->setVisible( !mGit->getGitFolder().empty() );

	if ( mGit->getGitFolder().empty() )
		return;

	std::string text( mStatusBarDisplayModifications &&
							  ( mGitStatus.totalInserts || mGitStatus.totalDeletions )
						  ? String::format( "%s (+%d / -%d)", mGitBranch.c_str(),
											mGitStatus.totalInserts, mGitStatus.totalDeletions )
						  : mGitBranch );

	mStatusButton->setText( text );

	if ( !mStatusBarDisplayModifications )
		return;

	if ( !mStatusCustomTokenizer.has_value() ) {
		std::vector<SyntaxPattern> patterns;
		auto fontColor( getVarColor( "--font" ) );
		auto insertedColor( getVarColor( "--theme-success" ) );
		auto deletedColor( getVarColor( "--theme-error" ) );
		patterns.emplace_back(
			SyntaxPattern( { ".*%((%+%d+)%s/%s(%-%d+)%)" }, { "normal", "keyword", "keyword2" } ) );
		SyntaxDefinition syntaxDef( "custom_build", {}, std::move( patterns ) );
		SyntaxColorScheme scheme( "status_bar_git",
								  { { "normal"_sst, { fontColor } },
									{ "keyword"_sst, { insertedColor } },
									{ "keyword2"_sst, { deletedColor } } },
								  {} );
		mStatusCustomTokenizer = { std::move( syntaxDef ), std::move( scheme ) };
	}

	SyntaxTokenizer::tokenizeText( mStatusCustomTokenizer->def, mStatusCustomTokenizer->scheme,
								   *mStatusButton->getTextBox()->getTextCache() );

	mStatusButton->invalidateDraw();
}

void GitPlugin::updateStatusBar( bool force ) {
	if ( !mGit || !mGitFound || !mStatusBarDisplayBranch )
		return;
	mThreadPool->run( [this, force] {
		if ( !mGit )
			return;

		if ( !mGit->getGitFolder().empty() ) {
			auto prevBranch = mGitBranch;
			mGitBranch = mGit->branch();
			auto prevGitStatus = mGitStatus;
			mGitStatus = mGit->status( mStatusRecurseSubmodules );
			if ( !force && mGitBranch == prevBranch && mGitStatus == prevGitStatus )
				return;
		} else if ( !mStatusButton ) {
			return;
		}

		getUISceneNode()->runOnMainThread( [this] { updateStatusBarSync(); } );
	} );
}

PluginRequestHandle GitPlugin::processMessage( const PluginMessage& msg ) {
	switch ( msg.type ) {
		case PluginMessageType::WorkspaceFolderChanged: {
			if ( mGit ) {
				mGit->setProjectPath( msg.asJSON()["folder"] );
				updateUINow( true );
			}
			break;
		}
		case ecode::PluginMessageType::UIReady: {
			updateUINow();
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

	if ( String::startsWith( file.getFilepath(), mGit->getGitFolder() ) &&
		 ( file.getExtension() == "lock" || file.isDirectory() ) )
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
									  i18n( "commit", "commit" ).capitalize().toUtf8().c_str(),
									  blame.commitHash.c_str(), blame.commitShortHash.c_str(),
									  i18n( "author", "author" ).capitalize().toUtf8().c_str(),
									  blame.author.c_str(), blame.authorEmail.c_str(),
									  i18n( "date", "date" ).capitalize().toUtf8().c_str(),
									  blame.date.c_str(), blame.commitMessage.c_str() )
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
		patterns.emplace_back( SyntaxPattern( { "([%w:]+)%s(.*)%(([%w%.-]+@[%w-]+%.%w+)%)" },
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

std::string GitPlugin::gitBranch() const {
	return mGitBranch;
}

void GitPlugin::onRegisterListeners( UICodeEditor* editor, std::vector<Uint32>& listeners ) {
	listeners.push_back(
		editor->addEventListener( Event::OnCursorPosChange, [this, editor]( const Event* ) {
			if ( mTooltipInfoShowing )
				hideTooltip( editor );
		} ) );
}

void GitPlugin::onBeforeUnregister( UICodeEditor* editor ) {
	for ( auto& kb : mKeyBindings )
		editor->getKeyBindings().removeCommandKeybind( kb.first );
}

void GitPlugin::onUnregisterDocument( TextDocument* doc ) {
	for ( auto& kb : mKeyBindings )
		doc->removeCommand( kb.first );
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
				   !icon.empty() ? mManager->getUISceneNode()->findIcon( icon )->getSize(
									   PixelDensity::dpToPxI( 12 ) )
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

void GitPlugin::updateBranches() {
	if ( !mGit || !mGitFound )
		return;

	mThreadPool->run( [this] {
		if ( !mGit )
			return;

		if ( !mGit->getGitFolder().empty() ) {
			if ( mGitBranch.empty() )
				mGitBranch = mGit->branch();
			auto branches = mGit->getAllBranchesAndTags();
			auto model = GitBranchModel::asModel( std::move( branches ), this );
			getUISceneNode()->runOnMainThread( [this, model] { updateSidePanelTab( model ); } );
		}
	} );
}

void GitPlugin::updateSidePanelTab( std::shared_ptr<GitBranchModel> model ) {
	buildSidePanelTab();

	UITreeView* tree = mSidePanel->find<UITreeView>( "git_branches_tree" );
	tree->setModel( model );
	tree->setColumnsVisible( { GitBranchModel::Name } );
}

void GitPlugin::buildSidePanelTab() {
	if ( mTab )
		return;
	if ( mSidePanel == nullptr )
		getUISceneNode()->bind( "panel", mSidePanel );
	UIIcon* icon = getUISceneNode()->findIcon( "source-control" );
	UIWidget* node = getUISceneNode()->loadLayoutFromString(
		R"html(
			<vbox id="git_branches" lw="mp" lh="wc" padding="4dp">
				<!-- <hbox lw="mp" lh="wc" margin-bottom="4dp">
					<Input id="branch_filter" lw="0" lw8="1" lh="0" />
					<PushButton id="branch_add" text="@string(add_branch, Add Branch)" tooltip="@string(add_branch, Add Branch)" text-as-fallback="true" icon="icon(add, 12dp)" margin-left="2dp" />
				</hbox> -->
				<TreeView id="git_branches_tree" lw="mp" lh="0" lw8="1" />
			</vbox>
		)html" );
	mTab = mSidePanel->add( getUISceneNode()->i18n( "source_control", "Source Control" ), node,
							icon ? icon->getSize( PixelDensity::dpToPx( 12 ) ) : nullptr );
	mTab->setId( "source_control" );
	mTab->setTextAsFallback( true );

	UITreeView* tree = mSidePanel->find<UITreeView>( "git_branches_tree" );
	tree->setAutoExpandOnSingleColumn( true );
	tree->setHeadersVisible( false );
}

} // namespace ecode
