#include "uieditor.hpp"
#include <args/args.hxx>
#define PUGIXML_HEADER_ONLY
#include <array>
#include <iostream>
#include <mutex>
#include <pugixml/pugixml.hpp>
#include <utility>

namespace uieditor {
/**
This is a real time visual editor for the UI module.
The layout files can be edited with any editor, and the layout changes can be seen live with this
editor. The preview uses an isolated nested UI scene, so project styles and resources cannot affect
the editor shell. Changes in the built-in editor are previewed directly from memory; external edits
are watched on disk. Project files can be created by hand and should look like this one:

<uiproject>
	<basepath>/optional/project/root/path</basepath>
	<font>
		<path>font</path>
	</font>
	<drawable>
		<path>drawable</path>
		<path>background</path>
	</drawable>
	<widget>
		<customWidget name="ScreenGame" replacement="RelativeLayout" />
	</widget>
	<layout width="1920" height="1080">
		<path>layout</path>
	</layout>
	<stylesheet path="style.css" />
</uiproject>

basepath is optional, otherwise it will take the base path from the xml file itself ( if the xml is
in /home/project.xml, basepath it going to be /home )

Layout width and height are the default/target window size for the project.

"path"s could be explicit files or directories.

customWidget are defined in the case you use special widgets in your application, you'll need to
indicate a valid replacement to be able to edit the file.
*/

class PreviewScene : public UISceneNode {
  public:
	explicit PreviewScene( EE::Window::Window* window ) : UISceneNode( window, false ) {}

	void clearDocument() {
		// Drain the close queue without running actions, timers, or scheduled document updates.
		getRoot()->closeAllChildren();
		checkClose();
	}
};

// A nested scene is updated by its owner, never also registered with SceneManager.
class PreviewHost : public UIWidget {
  public:
	explicit PreviewHost( App& app ) : UIWidget( "uieditor::preview" ), mApp( app ) {
		setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
		setClipType( ClipType::ContentBox );
		mScene = eeNew( PreviewScene, ( getUISceneNode()->getWindow() ) );
		mScene->setParent( this );
		mScene->setVisibleBoundsNode( this );
		subscribeScheduledUpdate();
	}

	UISceneNode* scene() const { return mScene; }

	void scheduledUpdate( const Time& time ) {
		mApp.update();
		UIWidget::scheduledUpdate( time );
		mScene->update( time );
	}

  private:
	App& mApp;
	UISceneNode* mScene;
};

// efsw callbacks run on a worker thread. Only the UI thread reads documents or changes scenes.
class UpdateListener : public efsw::FileWatchListener {
  public:
	void setFiles( const std::string& layout, const std::string& css, const std::string& baseCSS ) {
		std::lock_guard<Mutex> lock( mMutex );
		// Own these paths because the UI can select a different project while efsw is notifying.
		mFiles = { layout, css, baseCSS };
		mChanged = false;
	}

	void handleFileAction( efsw::WatchID, const std::string& dir, const std::string& filename,
						   efsw::Action action, const std::string& ) {
		if ( action != efsw::Actions::Modified && action != efsw::Actions::Add &&
			 action != efsw::Actions::Moved )
			return;
		std::lock_guard<Mutex> lock( mMutex );
		for ( const auto& file : mFiles ) {
			if ( file.size() == dir.size() + filename.size() &&
				 file.compare( 0, dir.size(), dir ) == 0 &&
				 file.compare( dir.size(), filename.size(), filename ) == 0 ) {
				mChanged = true;
				break;
			}
		}
	}

	bool takeChanges() {
		std::lock_guard<Mutex> lock( mMutex );
		return std::exchange( mChanged, false );
	}

  private:
	Mutex mMutex;
	std::array<std::string, 3> mFiles;
	bool mChanged{ false };
};

static std::string absolutePath( std::string path ) {
	if ( FileSystem::isRelativePath( path ) ) {
		std::string base = FileSystem::getCurrentWorkingDirectory();
		FileSystem::dirAddSlashAtEnd( base );
		path = base + path;
	}
	return path;
}

static URI fileURI( const std::string& path ) {
	URI uri;
	uri.setScheme( "file" );
	std::string uriPath = path;
	String::replaceAll( uriPath, "\\", "/" );
#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( !uriPath.empty() && uriPath.front() != '/' )
		uriPath.insert( uriPath.begin(), '/' );
#endif
	uri.setPath( uriPath );
	return uri;
}

void App::invalidatePreview() {
	mPreviewDirty = true;
	mReloadClock.restart();
}

static bool isFont( const std::string& path ) {
	std::string ext = FileSystem::fileExtension( path );
	return ext == "ttf" || ext == "otf" || ext == "woff" || ext == "woff2" || ext == "otb" ||
		   ext == "bdf" || ext == "ttc";
}

static bool isXML( const std::string& path ) {
	return FileSystem::fileExtension( path ) == "xml";
}

static bool isCSS( const std::string& path ) {
	return FileSystem::fileExtension( path ) == "css";
}

void App::loadConfig() {
	std::string path( Sys::getConfigPath( "eepp-uieditor" ) );
	if ( !FileSystem::fileExists( path ) )
		FileSystem::makeDir( path );
	FileSystem::dirAddSlashAtEnd( path );
	path += "config.ini";
	mIni.loadFromFile( path );
	std::string recent = mIni.getValue( "UIEDITOR", "recentprojects", "" );
	mRecentProjects = String::split( recent, ';' );
	recent = mIni.getValue( "UIEDITOR", "recentfiles", "" );
	mRecentFiles = String::split( recent, ';' );
}

void App::saveConfig() {
	mIni.setValue( "UIEDITOR", "recentprojects", String::join( mRecentProjects, ';' ) );
	mIni.setValue( "UIEDITOR", "recentfiles", String::join( mRecentFiles, ';' ) );
	mIni.writeFile();
}

void App::loadImage( std::string path ) {
	std::string name = FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( path ) );
	if ( auto texture = TextureFactory::instance()->loadFromFile( path ) ) {
		mPreviewScene->getResourceScope()->publishLocalDrawable(
			name, TextureRegion::New( std::move( texture ), name ) );
	}
}

void App::createWidgetInspector() {
	auto context = mAppUISceneNode->makeCurrent();
	UIWidgetInspector::create( mPreviewScene, mMenuIconSize );
}

void App::loadFont( std::string path ) {
	std::string name = FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( path ) );
	auto font = FontTrueType::New( name, *mPreviewScene->getResourceScope() );
	if ( !font->loadFromFile( path ) )
		mPreviewScene->getResourceScope()->eraseLocalFont( font.get() );
}

void App::loadImagesFromFolder( std::string folderPath ) {
	std::vector<std::string> files = FileSystem::filesGetInPath( folderPath );

	FileSystem::dirAddSlashAtEnd( folderPath );

	for ( auto it = files.begin(); it != files.end(); ++it )
		if ( Image::isImageExtension( *it ) )
			loadImage( folderPath + ( *it ) );
}

void App::loadFontsFromFolder( std::string folderPath ) {
	std::vector<std::string> files = FileSystem::filesGetInPath( folderPath );

	FileSystem::dirAddSlashAtEnd( folderPath );

	for ( auto it = files.begin(); it != files.end(); ++it )
		if ( isFont( *it ) )
			loadFont( folderPath + ( *it ) );
}

void App::loadLayoutsFromFolder( std::string folderPath ) {
	std::vector<std::string> files = FileSystem::filesGetInPath( folderPath );

	FileSystem::dirAddSlashAtEnd( folderPath );

	for ( auto it = files.begin(); it != files.end(); ++it )
		if ( isXML( *it ) )
			mLayouts[FileSystem::fileRemoveExtension( ( *it ) )] = ( folderPath + ( *it ) );
}

void App::loadBaseStyleSheet() {
	if ( !mUseDefaultTheme )
		return;
	updateWatches();
	openDocument( mBaseStyleSheet );
}

void App::toggleAppTheme() {
	mUseDefaultTheme = !mUseDefaultTheme;
	mPreviewScene->getUIThemeManager()->setDefaultTheme(
		mProjectThemes.empty() ? ( mUseDefaultTheme ? mTheme : UIThemePtr{} )
							   : mProjectThemes.back() );
	if ( mUseDefaultTheme )
		loadBaseStyleSheet();
	else
		updateWatches();
	// Native widgets retain properties with no replacement CSS declaration. Rebuild through the
	// normal preview path so disabling Breeze also restores their unstyled defaults.
	refreshPreview();
}

void App::loadStyleSheet( std::string cssPath ) {
	cssPath = absolutePath( std::move( cssPath ) );
	if ( !FileSystem::fileExists( cssPath ) )
		return;
	mCurrentStyleSheet = std::move( cssPath );
	loadBaseStyleSheet();
	updateWatches();
	openDocument( mCurrentStyleSheet );
	refreshPreview();
}

void App::updateWatches() {
	mListener->setFiles( mCurrentLayout, mCurrentStyleSheet,
						 mUseDefaultTheme ? mBaseStyleSheet : std::string{} );
	SmallVector<std::string, 3> folders;
	for ( const auto* file : { &mCurrentLayout, &mCurrentStyleSheet, &mBaseStyleSheet } ) {
		if ( file->empty() || ( file == &mBaseStyleSheet && !mUseDefaultTheme ) )
			continue;
		std::string folder = FileSystem::fileRemoveFileName( *file );
		if ( std::find( folders.begin(), folders.end(), folder ) == folders.end() )
			folders.emplace_back( std::move( folder ) );
	}
	for ( auto it = mWatches.begin(); it != mWatches.end(); ) {
		if ( std::find( folders.begin(), folders.end(), it->first ) == folders.end() ) {
			mFileWatcher->removeWatch( it->second );
			it = mWatches.erase( it );
		} else {
			++it;
		}
	}
	for ( auto& folder : folders ) {
		if ( mWatches.find( folder ) == mWatches.end() ) {
			auto watch = mFileWatcher->addWatch( folder, mListener );
			if ( watch > 0 )
				mWatches.emplace( std::move( folder ), watch );
		}
	}
}

std::pair<UITab*, UICodeEditor*> App::openDocument( const std::string& path ) {
	if ( mSplitter->isDocumentOpen( path ) )
		return { nullptr, mSplitter->findEditorFromPath( path ) };
	auto context = mAppUISceneNode->makeCurrent();
	return mSplitter->loadFileFromPathInNewTab( path );
}

std::pair<UITab*, UICodeEditor*> App::loadLayout( std::string file ) {
	file = absolutePath( std::move( file ) );
	if ( !FileSystem::fileExists( file ) )
		return { nullptr, nullptr };
	mCurrentLayout = std::move( file );
	updateWatches();
	refreshPreview();
	return openDocument( mCurrentLayout );
}

bool App::readSource( const std::string& path, std::string& source ) {
	if ( auto* editor = mSplitter->findEditorFromPath( path ); editor && editor->isDirty() ) {
		source = editor->getDocument().getText().toUtf8();
		return true;
	}
	return FileSystem::fileGet( path, source );
}

void App::refreshPreview() {
	mPreviewDirty = false;
	std::string layout, baseCSS, projectCSS;
	pugi::xml_document document;
	if ( !mCurrentLayout.empty() ) {
		if ( !readSource( mCurrentLayout, layout ) )
			return;
		auto result = document.load_buffer( layout.data(), layout.size(),
											pugi::parse_default | pugi::parse_ws_pcdata );
		if ( !result ) {
			// Keep the last valid preview while the user is typing incomplete XML.
			Log::error( "Couldn't preview %s: %s (offset %d)", mCurrentLayout.c_str(),
						result.description(), result.offset );
			return;
		}
	}
	if ( mUseDefaultTheme && !readSource( mBaseStyleSheet, baseCSS ) )
		return;
	if ( !mCurrentStyleSheet.empty() && !readSource( mCurrentStyleSheet, projectCSS ) )
		return;

	auto context = mPreviewScene->makeCurrent();
	clearPreviewDocument();
	mPreviewScene->setURIFromURL( fileURI( mCurrentLayout ) );
	if ( mUseDefaultTheme ) {
		mPreviewScene->combineStyleSheet( baseCSS, false, String::hash( mBaseStyleSheet ),
										  fileURI( mBaseStyleSheet ) );
	}
	if ( !mCurrentStyleSheet.empty() ) {
		mPreviewScene->combineStyleSheet( projectCSS, false, String::hash( mCurrentStyleSheet ),
										  fileURI( mCurrentStyleSheet ) );
	}
	mPreviewScene->loadLayoutNodes( document.first_child(), mPreviewScene->getRoot(),
									String::hash( mCurrentLayout ) );
	mPreviewScene->reloadStyle( true, true, true );
	mPreviewScene->invalidateDraw();
}

void App::clearPreviewDocument() {
	auto context = mPreviewScene->makeCurrent();
	// Retire deferred CSS/images/fonts before replacing the document. Project resources remain
	// in this scene's scope until closeProject(); author font faces belong to this document.
	mPreviewScene->invalidateAsyncResourceLoads();
	mPreviewScene->beginDocumentNavigation( mCurrentLayout.empty() ? URI{}
																   : fileURI( mCurrentLayout ) );
	static_cast<PreviewScene*>( mPreviewScene )->clearDocument();
	mPreviewScene->clearFontFaces();
	mPreviewScene->setStyleSheet( CSS::StyleSheet{}, false );
	// Author @glyph-icon rules can retain project fonts. Keep them in a replaceable theme, with
	// the shell's immutable icon theme as a lookup fallback.
	auto* icons = mPreviewScene->getUIIconThemeManager();
	if ( auto* theme = icons->getCurrentTheme() )
		icons->remove( theme );
	icons->setCurrentTheme( UIIconTheme::New( "preview-icons" ) );
	icons->setFallbackTheme( mAppUISceneNode->getUIIconThemeManager()->getCurrentThemeHandle() );
}

void App::showEditor( bool show ) {
	if ( show == mSidePanel->isVisible() )
		return;

	if ( show ) {
		mSidePanel->setVisible( true );
		mSidePanel->setParent( mProjectSplitter );
		mProjectSplitter->swap();
	} else {
		mSidePanel->setVisible( false );
		mSidePanel->setParent( mAppUISceneNode->getRoot() );
	}
}

void App::toggleEditor() {
	showEditor( !mSidePanel->isVisible() );
}

void App::updateRecentMenu( bool projects ) {
	auto context = mAppUISceneNode->makeCurrent();
	if ( !mUIMenuBar )
		return;
	auto* fileMenu = mUIMenuBar->getPopUpMenu( "File" );
	auto* item =
		fileMenu ? fileMenu->getItem( projects ? "Recent projects" : "Recent files" ) : nullptr;
	if ( !item )
		return;
	auto* menu = item->asType<UIMenuSubMenu>()->getSubMenu();
	menu->removeAll();
	for ( const auto& path : projects ? mRecentProjects : mRecentFiles )
		menu->add( path );
	auto& listener = projects ? mRecentProjectEventClickId : mRecentFilesEventClickId;
	if ( listener != 0xFFFFFFFF )
		menu->removeEventListener( listener );
	listener = menu->on( Event::OnItemClicked, [this, projects]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		std::string path = event->getNode()->asType<UIMenuItem>()->getText().toUtf8();
		if ( !FileSystem::fileExists( path ) || FileSystem::isDirectory( path ) )
			return;
		if ( projects )
			loadProject( std::move( path ) );
		else
			loadLayoutFile( std::move( path ) );
	} );
}

void App::resizeCb() {
	if ( !mPreviewScene || !mPreviewHost )
		return;
	if ( auto* viewMenu = mUIMenuBar ? mUIMenuBar->getPopUpMenu( "View" ) : nullptr ) {
		if ( auto* item = viewMenu->getItemId( "project-viewport" ) )
			item->asType<UIMenuCheckBox>()->setActive( mUseProjectViewport );
	}
	bool projectViewport =
		mUseProjectViewport && mProjectScreenSize.x > 0 && mProjectScreenSize.y > 0;
	mPreviewScene->setFollowParentSize( !projectViewport );
	if ( !projectViewport ) {
		if ( mPreviewScene->getScale() != 1.f )
			mPreviewScene->setScale( 1.f );
		mPreviewScene->setPosition( 0, 0 );
		return;
	}
	Sizef available = mPreviewHost->getPixelsSize();
	Float scale = eemin(
		1.f, eemin( available.x / mProjectScreenSize.x, available.y / mProjectScreenSize.y ) );
	mPreviewScene->setPixelsSize( mProjectScreenSize );
	mPreviewScene->setScale( scale, OriginPoint( OriginPoint::OriginTopLeft ) );
	mPreviewScene->setPosition( ( available.x - mProjectScreenSize.x * scale ) * 0.5f,
								( available.y - mProjectScreenSize.y * scale ) * 0.5f );
}

void App::resizeWindowToLayout() {
	if ( mProjectScreenSize.x <= 0 || mProjectScreenSize.y <= 0 )
		return;
	Sizef windowSize( mWindow->getSize().x, mWindow->getSize().y );
	Sizef chrome = windowSize - mPreviewHost->getPixelsSize();
	Sizef desired = mProjectScreenSize + chrome;
	Rect border = mWindow->getBorderSize();
	Sizei usable = Engine::instance()
					   ->getDisplayManager()
					   ->getDisplayIndex( mWindow->getCurrentDisplayIndex() )
					   ->getUsableBounds()
					   .getSize();
	mWindow->setSize(
		static_cast<Uint32>( eemin( desired.x, Float( usable.x - border.Left - border.Right ) ) ),
		static_cast<Uint32>( eemin( desired.y, Float( usable.y - border.Top - border.Bottom ) ) ) );
	mWindow->centerToDisplay();
}

UIWidget* App::createWidget( const std::string& widgetName ) {
	auto it = mWidgetRegistered.find( widgetName );
	return it != mWidgetRegistered.end() ? UIWidgetCreator::createFromName( it->second ) : nullptr;
}

std::string App::pathFix( std::string path ) {
	return FileSystem::isRelativePath( path ) ? mBasePath + path : path;
}

void App::loadUITheme( std::string themePath ) {
	TextureAtlasLoader tgl;
	tgl.setResourceScope( mPreviewScene->getResourceScope() );
	tgl.loadFromFile( themePath );

	std::string name(
		FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( themePath ) ) );

	auto uitheme =
		UITheme::loadFromTextureAtlas( UITheme::New( name, name ), tgl.getTextureAtlas() );

	mPreviewScene->getUIThemeManager()->setDefaultTheme( uitheme )->add( uitheme );
	mProjectThemes.emplace_back( std::move( uitheme ) );
}

void App::onLayoutSelected( const Event* event ) {
	if ( !event->getNode()->isType( UI_TYPE_MENUCHECKBOX ) )
		return;

	const String& txt = event->getNode()->asType<UIMenuItem>()->getText();

	UIPopUpMenu* uiLayoutsMenu;

	if ( ( uiLayoutsMenu = mUIMenuBar->getPopUpMenu( "Layouts" ) ) &&
		 uiLayoutsMenu->getCount() > 0 ) {
		for ( size_t i = 0; i < uiLayoutsMenu->getCount(); i++ )
			uiLayoutsMenu->getItem( i )->asType<UIMenuCheckBox>()->setActive( false );
	}

	UIMenuCheckBox* chk = static_cast<UIMenuCheckBox*>( event->getNode() );
	chk->setActive( true );

	UnorderedMap<std::string, std::string>::iterator it;

	if ( ( it = mLayouts.find( txt.toUtf8() ) ) != mLayouts.end() )
		loadLayout( it->second );
}

void App::refreshLayoutList() {
	if ( NULL == mUIMenuBar )
		return;

	auto context = mAppUISceneNode->makeCurrent();

	if ( mLayouts.size() > 0 ) {
		UIPopUpMenu* uiLayoutsMenu = NULL;

		if ( mUIMenuBar->getButton( "Layouts" ) == NULL ) {
			uiLayoutsMenu = UIPopUpMenu::New();

			mUIMenuBar->addMenuButton( "Layouts", uiLayoutsMenu );

			uiLayoutsMenu->on( Event::OnItemClicked,
							   [this]( const Event* event ) { onLayoutSelected( event ); } );
		} else {
			uiLayoutsMenu = mUIMenuBar->getPopUpMenu( "Layouts" );
		}

		uiLayoutsMenu->removeAll();

		for ( auto it = mLayouts.begin(); it != mLayouts.end(); ++it )
			uiLayoutsMenu->addCheckBox( it->first )->setActive( mCurrentLayout == it->second );
	} else if ( mUIMenuBar->getButton( "Layouts" ) != NULL ) {
		mUIMenuBar->removeMenuButton( "Layouts" );
	}
}

void App::loadProjectNodes( pugi::xml_node node ) {
	mPreviewScene->getUIThemeManager()->setDefaultTheme( mUseDefaultTheme ? mTheme : UIThemePtr{} );

	for ( pugi::xml_node resources = node; resources; resources = resources.next_sibling() ) {
		std::string name = String::toLower( std::string( resources.name() ) );

		if ( name == "uiproject" ) {
			pugi::xml_node basePathNode = resources.child( "basepath" );

			if ( !basePathNode.empty() ) {
				mBasePath = pathFix( basePathNode.text().as_string() );
				FileSystem::dirAddSlashAtEnd( mBasePath );
			}

			pugi::xml_node fontNode = resources.child( "font" );

			if ( !fontNode.empty() ) {
				for ( pugi::xml_node pathNode = fontNode.child( "path" ); pathNode;
					  pathNode = pathNode.next_sibling( "path" ) ) {
					std::string fontPath( pathFix( pathNode.text().as_string() ) );

					if ( FileSystem::isDirectory( fontPath ) ) {
						loadFontsFromFolder( fontPath );
					} else if ( isFont( fontPath ) ) {
						loadFont( fontPath );
					}
				}
			}

			pugi::xml_node drawableNode = resources.child( "drawable" );

			if ( !drawableNode.empty() ) {
				for ( pugi::xml_node pathNode = drawableNode.child( "path" ); pathNode;
					  pathNode = pathNode.next_sibling( "path" ) ) {
					std::string drawablePath( pathFix( pathNode.text().as_string() ) );

					if ( FileSystem::isDirectory( drawablePath ) ) {
						loadImagesFromFolder( drawablePath );
					} else if ( Image::isImageExtension( drawablePath ) ) {
						loadImage( drawablePath );
					}
				}
			}

			pugi::xml_node widgetNode = resources.child( "widget" );

			if ( !widgetNode.empty() ) {
				for ( pugi::xml_node cwNode = widgetNode.child( "customWidget" ); cwNode;
					  cwNode = cwNode.next_sibling( "customWidget" ) ) {
					std::string wname( cwNode.attribute( "name" ).as_string() );
					std::string replacement( cwNode.attribute( "replacement" ).as_string() );
					mWidgetRegistered[String::toLower( wname )] = std::move( replacement );
				}

				for ( auto it = mWidgetRegistered.begin(); it != mWidgetRegistered.end(); ++it ) {
					if ( !UIWidgetCreator::existsCustomWidgetCallback( it->first ) ) {
						UIWidgetCreator::addCustomWidgetCallback(
							it->first, [this]( const std::string& widgetName ) -> UIWidget* {
								return createWidget( widgetName );
							} );
						mRegisteredCallbacks.emplace_back( it->first );
					}
				}
			}

			pugi::xml_node uiNode = resources.child( "uitheme" );

			if ( !uiNode.empty() ) {
				for ( pugi::xml_node uiThemeNode = uiNode; uiThemeNode;
					  uiThemeNode = uiThemeNode.next_sibling( "uitheme" ) ) {
					std::string uiThemePath( pathFix( uiThemeNode.text().as_string() ) );

					loadUITheme( uiThemePath );
				}
			}

			pugi::xml_node styleSheetNode = resources.child( "stylesheet" );

			if ( !styleSheetNode.empty() ) {
				std::string cssPath = pathFix( styleSheetNode.attribute( "path" ).as_string() );

				if ( isCSS( cssPath ) && FileSystem::fileExists( cssPath ) )
					loadStyleSheet( cssPath );
			}

			pugi::xml_node layoutNode = resources.child( "layout" );

			if ( !layoutNode.empty() ) {
				bool loaded = false;

				Float width = layoutNode.attribute( "width" ).as_float();
				Float height = layoutNode.attribute( "height" ).as_float();

				mProjectScreenSize = { width, height };
				mUseProjectViewport = width > 0 && height > 0;

				resizeCb();

				mLayouts.clear();

				for ( pugi::xml_node layNode = layoutNode.child( "path" ); layNode;
					  layNode = layNode.next_sibling( "path" ) ) {
					std::string layoutPath( pathFix( layNode.text().as_string() ) );

					if ( FileSystem::isDirectory( layoutPath ) ) {
						loadLayoutsFromFolder( layoutPath );
					} else if ( FileSystem::fileExists( layoutPath ) && isXML( layoutPath ) ) {
						mLayouts[FileSystem::fileRemoveExtension(
							FileSystem::fileNameFromPath( layoutPath ) )] = layoutPath;

						if ( !loaded ) {
							loadLayout( layoutPath );
							loaded = true;
						}
					}
				}

				if ( mLayouts.size() > 0 && !loaded )
					loadLayout( mLayouts.begin()->second );
			}

			refreshLayoutList();
		}
	}
}

void App::loadLayoutFile( std::string layoutPath ) {
	layoutPath = absolutePath( std::move( layoutPath ) );
	if ( FileSystem::fileExists( layoutPath ) ) {
		loadLayout( layoutPath );

		for ( auto pathIt = mRecentFiles.begin(); pathIt != mRecentFiles.end(); pathIt++ ) {
			if ( *pathIt == layoutPath ) {
				mRecentFiles.erase( pathIt );
				break;
			}
		}

		mRecentFiles.insert( mRecentFiles.begin(), layoutPath );

		if ( mRecentFiles.size() > 10 )
			mRecentFiles.resize( 10 );

		updateRecentMenu( false );
	}
}

void App::loadProject( std::string projectPath ) {
	projectPath = absolutePath( std::move( projectPath ) );
	if ( !FileSystem::fileExists( projectPath ) )
		return;

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file( projectPath.c_str() );

	if ( result ) {
		closeProject();
		mBasePath = FileSystem::fileRemoveFileName( projectPath );
		loadProjectNodes( doc.first_child() );

		for ( auto pathIt = mRecentProjects.begin(); pathIt != mRecentProjects.end(); pathIt++ ) {
			if ( *pathIt == projectPath ) {
				mRecentProjects.erase( pathIt );
				break;
			}
		}

		mRecentProjects.insert( mRecentProjects.begin(), projectPath );

		if ( mRecentProjects.size() > 10 )
			mRecentProjects.resize( 10 );

		updateRecentMenu( true );
	} else {
		Log::error( "Couldn't load UI Layout: %s", projectPath.c_str() );
		Log::error( "Error description: %s", result.description() );
		Log::error( "Error offset: %d", result.offset );
	}
}

void App::closeEditors() {
	auto context = mAppUISceneNode->makeCurrent();
	std::vector<UICodeEditor*> editors = mSplitter->getAllEditors();
	while ( !editors.empty() ) {
		UICodeEditor* editor = editors[0];
		UITabWidget* tabWidget = mSplitter->tabWidgetFromEditor( editor );
		tabWidget->removeTab( (UITab*)editor->getData(), true, false );
		editors = mSplitter->getAllEditors();
		if ( editors.size() == 1 && editors[0]->getDocument().isEmpty() )
			break;
	};
	if ( !mSplitter->getTabWidgets().empty() && mSplitter->getTabWidgets()[0]->getTabCount() == 0 )
		mSplitter->createCodeEditorInTabWidget( mSplitter->getTabWidgets()[0] );
}

void App::closeProject() {
	mPreviewDirty = false;
	mCurrentLayout.clear();
	mCurrentStyleSheet.clear();
	mBasePath.clear();
	mLayouts.clear();
	mTmpDocs.clear();
	mProjectScreenSize = {};
	mUseProjectViewport = false;
	clearPreviewDocument();
	mPreviewScene->setURI( URI{} );
	for ( const auto& theme : mProjectThemes )
		mPreviewScene->getUIThemeManager()->remove( theme.get() );
	mProjectThemes.clear();
	mPreviewScene->getUIThemeManager()->setDefaultTheme( mUseDefaultTheme ? mTheme : UIThemePtr{} );
	// Scope ownership releases all project assets together, including loaded texture atlases.
	mPreviewScene->getResourceScope()->clearLocal();
	for ( const auto& name : mRegisteredCallbacks )
		UIWidgetCreator::removeCustomWidgetCallback( name );
	mRegisteredCallbacks.clear();
	mWidgetRegistered.clear();
	closeEditors();
	refreshLayoutList();
	updateWatches();
	loadBaseStyleSheet();
	resizeCb();
	invalidatePreview();
}

bool App::onCloseRequestCallback( EE::Window::Window* ) {
	if ( mMsgBox )
		return false;

	auto context = mAppUISceneNode->makeCurrent();

	mMsgBox = UIMessageBox::New(
		UIMessageBox::OK_CANCEL,
		"Do you really want to close the current file?\nAll changes will be lost.",
		UI_MESSAGE_BOX_DEFAULT_FLAGS | UI_WIN_SHADOW );
	mMsgBox->setTheme( mTheme.get() );
	mMsgBox->on( Event::OnConfirm, [this]( const Event* ) { mWindow->close(); } );
	mMsgBox->on( Event::OnWindowClose, [this]( const Event* ) { mMsgBox = NULL; } );
	mMsgBox->setTitle( "Close Editor?" );
	mMsgBox->center();
	mMsgBox->showWhenReady();

	return false;
}

void App::update() {
	if ( !mWindow->isOpen() )
		return;
	if ( mListener->takeChanges() )
		invalidatePreview();
	if ( mPreviewDirty && mReloadClock.getElapsedTime() >= Milliseconds( 350 ) )
		refreshPreview();
}

void App::imagePathOpen( const Event* event ) {
	loadImagesFromFolder( event->getNode()->asType<UIFileDialog>()->getFullPath() );
}

void App::fontPathOpen( const Event* event ) {
	loadFontsFromFolder( event->getNode()->asType<UIFileDialog>()->getFullPath() );
}

void App::styleSheetPathOpen( const Event* event ) {
	loadStyleSheet( event->getNode()->asType<UIFileDialog>()->getFullPath() );
}

void App::layoutOpen( const Event* event ) {
	loadLayoutFile( event->getNode()->asType<UIFileDialog>()->getFullPath() );
}

void App::projectOpen( const Event* event ) {
	loadProject( event->getNode()->asType<UIFileDialog>()->getFullPath() );
}

void App::showFileDialog( const String& title, const std::function<void( const Event* )>& cb,
						  const std::string& filePattern, const Uint32& dialogFlags ) {
	UIFileDialog* dialog = UIFileDialog::New( dialogFlags, filePattern );
	dialog->setTheme( mTheme.get() );
	dialog->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( title );
	dialog->on( Event::OpenFile, cb );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->center();
	dialog->show();
}

String App::i18n( const std::string& key, const String& def ) {
	return mAppUISceneNode->getTranslatorStringFromKey( key, def );
}

void App::updateEditorState() {
	if ( mSplitter->curEditorExistsAndFocused() ) {
		updateEditorTitle( mSplitter->getCurEditor() );
	}
}

UIFileDialog* App::saveFileDialog( UICodeEditor* editor, bool focusOnClose ) {
	auto context = mAppUISceneNode->makeCurrent();
	if ( !editor )
		return nullptr;
	UIFileDialog* dialog =
		UIFileDialog::New( UIFileDialog::DefaultFlags | UIFileDialog::SaveDialog, "*" );
	dialog->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( i18n( "save_file_as", "Save File As" ) );
	dialog->setCloseShortcut( KEY_ESCAPE );
	std::string filename( editor->getDocument().getFilename() );
	if ( FileSystem::fileExtension( editor->getDocument().getFilename() ).empty() )
		filename += editor->getSyntaxDefinition().getFileExtension();
	dialog->setFileName( filename );
	dialog->on( Event::SaveFile, [this, editor]( const Event* event ) {
		if ( editor ) {
			std::string path( event->getNode()->asType<UIFileDialog>()->getFullPath() );
			if ( !path.empty() && !FileSystem::isDirectory( path ) ) {
				std::string oldPath( editor->getDocument().getFilePath() );
				bool wasTemporary = editor->getDocument().isDeleteOnClose();
				if ( editor->getDocument().save( path ) ) {
					editor->getDocument().setDeleteOnClose( false );
					if ( wasTemporary && oldPath != path )
						FileSystem::fileRemove( oldPath );
					if ( mCurrentLayout == oldPath )
						mCurrentLayout = path;
					if ( mCurrentStyleSheet == oldPath )
						mCurrentStyleSheet = path;
					if ( mBaseStyleSheet == oldPath )
						mBaseStyleSheet = path;
					UITab* tab = mSplitter->isDocumentOpen( path );
					if ( tab )
						tab->setTooltipText( editor->getDocument().getFilePath() );
					updateWatches();
					invalidatePreview();
					updateEditorState();
				} else {
					UIMessageBox* msg =
						UIMessageBox::New( UIMessageBox::OK, i18n( "couldnt_write_the_file",
																   "Couldn't write the file." ) );
					msg->setTitle( "Error" );
					msg->show();
				}
			} else {
				UIMessageBox* msg = UIMessageBox::New(
					UIMessageBox::OK,
					i18n( "empty_file_name", "You must set a name to the file." ) );
				msg->setTitle( "Error" );
				msg->show();
			}
		}
	} );
	if ( focusOnClose ) {
		dialog->on( Event::OnWindowClose, [editor]( const Event* ) {
			if ( editor && !SceneManager::instance()->isShuttingDown() )
				editor->setFocus();
		} );
	}
	dialog->center();
	dialog->show();
	return dialog;
}

void App::createNewLayout() {
	std::string file;
	std::string tmpPath( Sys::getTempPath() + "untitled_%d.xml" );
	int i = 0;
	do {
		file = String::format( tmpPath.c_str(), ++i );
	} while ( FileSystem::fileExists( file ) );
	FileSystem::fileWrite( file, "<vbox>\n</vbox>" );
	std::pair<UITab*, UICodeEditor*> d = loadLayout( file );
	if ( !d.first )
		return;
	d.second->getDocument().setDeleteOnClose( true );
}

void App::fileMenuClick( const Event* event ) {
	if ( event->getNode()->isType( UI_TYPE_MENUITEM ) )
		executeCommand( event->getNode()->asType<UIMenuItem>()->getId() );
}

void App::executeCommand( const std::string& command ) {
	auto context = mAppUISceneNode->makeCurrent();
	if ( "new-layout" == command ) {
		createNewLayout();
	} else if ( "open-project" == command ) {
		showFileDialog(
			"Open project...", [this]( const Event* event ) { projectOpen( event ); }, "*.xml" );
	} else if ( "open-layout" == command ) {
		showFileDialog(
			"Open layout...", [this]( const Event* event ) { layoutOpen( event ); }, "*.xml" );
	} else if ( "close" == command ) {
		closeProject();
	} else if ( "quit" == command ) {
		onCloseRequestCallback( mWindow );
	} else if ( "load-images-from-path" == command ) {
		showFileDialog(
			"Open images from folder...", [this]( const Event* event ) { imagePathOpen( event ); },
			"*", UIFileDialog::DefaultFlags | UIFileDialog::AllowFolderSelect );
	} else if ( "load-fonts-from-path" == command ) {
		showFileDialog(
			"Open fonts from folder...", [this]( const Event* event ) { fontPathOpen( event ); },
			"*", UIFileDialog::DefaultFlags | UIFileDialog::AllowFolderSelect );
	} else if ( "load-css-from-path" == command ) {
		showFileDialog(
			"Open style sheet from path...",
			[this]( const Event* event ) { styleSheetPathOpen( event ); }, "*.css" );
	} else if ( "toggle-console" == command ) {
		mConsole->toggle();
	} else if ( "toggle-editor" == command ) {
		toggleEditor();
	} else if ( "highlight-focus" == command ) {
		mPreviewScene->setHighlightFocusRecursive( !mPreviewScene->getHighlightFocus() );
		mPreviewScene->setHighlightOverRecursive( !mPreviewScene->getHighlightOver() );
	} else if ( "debug-boxes" == command ) {
		mPreviewScene->setDrawBoxesRecursive( !mPreviewScene->getDrawBoxes() );
	} else if ( "debug-data" == command ) {
		mPreviewScene->setDrawDebugDataRecursive( !mPreviewScene->getDrawDebugData() );
	} else if ( "inspect-widgets" == command ) {
		createWidgetInspector();
	} else if ( "save-doc" == command ) {
		saveDoc();
	} else if ( "save-as-doc" == command ) {
		if ( mSplitter->curEditorExistsAndFocused() )
			saveFileDialog( mSplitter->getCurEditor() );
	} else if ( "save-all" == command ) {
		saveAll();
	} else if ( "use-app-theme" == command ) {
		toggleAppTheme();
		mUIMenuBar->getPopUpMenu( "View" )
			->getItemId( "use-app-theme" )
			->asType<UIMenuCheckBox>()
			->setActive( mUseDefaultTheme );
	} else if ( "project-viewport" == command ) {
		mUseProjectViewport = !mUseProjectViewport;
		resizeCb();
	} else if ( "resize-preview" == command ) {
		resizeWindowToLayout();
	} else if ( "reload-style-state" == command ) {
		mPreviewScene->getRoot()->reportStyleStateChangeRecursive();
	}
}

void App::createKeyBindings() {
	static constexpr struct {
		const char* command;
		const char* shortcut;
	} bindings[] = { { "quit", "ctrl+escape" },		 { "resize-preview", "f1" },
					 { "toggle-console", "f3" },	 { "highlight-focus", "f6" },
					 { "debug-boxes", "f7" },		 { "debug-data", "f8" },
					 { "toggle-editor", "f9" },		 { "inspect-widgets", "f11" },
					 { "reload-style-state", "f12" } };
	for ( const auto& binding : bindings ) {
		mAppUISceneNode->addKeyBindingString( binding.shortcut, binding.command );
		mAppUISceneNode->setKeyBindingCommand(
			binding.command, [this, command = binding.command] { executeCommand( command ); } );
	}
	mAppUISceneNode->addKeyBinding( { KEY_BACKSLASH, 0 }, "toggle-console" );
}

DrawablePtr App::findIcon( const std::string& icon ) {
	return mAppUISceneNode->findIconDrawable( icon, mMenuIconSize );
}

void App::createAppMenu() {
	auto context = mAppUISceneNode->makeCurrent();

	mUIMenuBar = mAppUISceneNode->find( "menubar" )->asType<UIMenuBar>();
	UIPopUpMenu* uiPopMenu = UIPopUpMenu::New();
	uiPopMenu->add( "New layout", findIcon( "file-add" ) )->setId( "new-layout" );
	uiPopMenu->add( "Open layout...", findIcon( "document-open" ) )->setId( "open-layout" );
	uiPopMenu->add( "Open project...", findIcon( "document-open" ) )->setId( "open-project" );
	uiPopMenu->addSeparator();
	uiPopMenu->addSubMenu( "Recent files", NULL, UIPopUpMenu::New() )->setId( "recent-files" );
	uiPopMenu->addSubMenu( "Recent projects", NULL, UIPopUpMenu::New() )
		->setId( "recent-projects" );
	uiPopMenu->addSeparator();
	uiPopMenu->add( i18n( "save", "Save" ), findIcon( "document-save" ) )->setId( "save-doc" );
	uiPopMenu->add( i18n( "save_as_ellipsis", "Save as..." ), findIcon( "document-save-as" ) )
		->setId( "save-as-doc" );
	uiPopMenu->add( i18n( "save_all", "Save All" ), findIcon( "document-save-as" ) )
		->setId( "save-all" );
	uiPopMenu->addSeparator();
	uiPopMenu->add( "Close", findIcon( "document-close" ) )->setId( "close" );
	uiPopMenu->addSeparator();
	uiPopMenu->add( "Quit", findIcon( "quit" ) )->setId( "quit" );

	mUIMenuBar->addMenuButton( "File", uiPopMenu );
	uiPopMenu->on( Event::OnItemClicked, [this]( const Event* event ) { fileMenuClick( event ); } );

	UIPopUpMenu* uiResourceMenu = UIPopUpMenu::New();
	uiResourceMenu->add( "Load images from path...", findIcon( "document-open" ) )
		->setId( "load-images-from-path" );
	uiResourceMenu->addSeparator();
	uiResourceMenu->add( "Load fonts from path...", findIcon( "document-open" ) )
		->setId( "load-fonts-from-path" );
	uiResourceMenu->addSeparator();
	uiResourceMenu->add( "Load style sheet from path...", findIcon( "document-open" ) )
		->setId( "load-css-from-path" );
	mUIMenuBar->addMenuButton( "Resources", uiResourceMenu );
	uiResourceMenu->on( Event::OnItemClicked,
						[this]( const Event* event ) { fileMenuClick( event ); } );

	UIPopUpMenu* colorsMenu = UIPopUpMenu::New();
	colorsMenu
		->addRadioButton( i18n( "system", "System" ),
						  mUIColorScheme == ColorSchemeExtPreference::System )
		->setId( "system" );
	colorsMenu
		->addRadioButton( i18n( "light", "Light" ),
						  mUIColorScheme == ColorSchemeExtPreference::Light )
		->setId( "light" );
	colorsMenu
		->addRadioButton( i18n( "dark", "Dark" ), mUIColorScheme == ColorSchemeExtPreference::Dark )
		->setId( "dark" );
	colorsMenu->on( Event::OnItemClicked, [this]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		mUIColorScheme = ColorSchemePreferences::fromStringExt( item->getId() );
		mPreviewScene->setColorSchemePreference( mUIColorScheme );
		invalidatePreview();
	} );

	UIPopUpMenu* viewMenu = UIPopUpMenu::New();
	viewMenu->addSubMenu( i18n( "ui_prefes_color_scheme", "UI Prefers Color Scheme" ),
						  findIcon( "color-scheme" ), colorsMenu );
	viewMenu->addSeparator();
	viewMenu->addCheckBox( "Use app theme (Breeze)", mUseDefaultTheme )->setId( "use-app-theme" );
	viewMenu->addCheckBox( "Use project viewport", mUseProjectViewport )
		->setId( "project-viewport" );
	viewMenu->addSeparator();
	viewMenu->add( "Highlight Focus & Hover", nullptr, "F6" )->setId( "highlight-focus" );
	viewMenu->add( "Draw debug boxes", nullptr, "F7" )->setId( "debug-boxes" );
	viewMenu->add( "Draw debug data (mouse hover boxes)", nullptr, "F8" )->setId( "debug-data" );
	viewMenu->addSeparator();
	viewMenu->add( "Inspect Widgets", findIcon( "package" ), "F11" )->setId( "inspect-widgets" );
	viewMenu->add( "Toggle Console", findIcon( "terminal" ), "F3" )->setId( "toggle-console" );
	viewMenu->add( "Toggle Editor", findIcon( "editor" ), "F9" )->setId( "toggle-editor" );
	mUIMenuBar->addMenuButton( "View", viewMenu );
	viewMenu->on( Event::OnItemClicked, [this]( const Event* event ) { fileMenuClick( event ); } );
	mConsole = UIConsole::New();
	mConsole->setQuakeMode( true );
	mConsole->setVisible( false );
}

App::App() {}

App::~App() {
	// Stop and join the producer before destroying the queue or anything referenced by callbacks.
	delete mFileWatcher;
	delete mListener;
	saveConfig();
	releaseEditor();
	mProjectThemes.clear();
	mTheme.reset();
	// Destroy scenes while this client's callback state is still alive.
	mUIApplication.reset();
}

void App::releaseEditor() {
	if ( mMsgBox )
		mMsgBox->clearEventListener();
	mMsgBox = nullptr;
	for ( const auto& name : mRegisteredCallbacks )
		UIWidgetCreator::removeCustomWidgetCallback( name );
	mRegisteredCallbacks.clear();
	eeSAFE_DELETE( mSplitter );
}

int App::init( const Float& pixelDensityConf, const bool& useAppTheme, std::string cssFile,
			   std::string xmlFile, std::string projectFile, const std::string& colorScheme ) {
	mUseDefaultTheme = useAppTheme;
	// UIApplication selects the resource working directory; CLI files belong to the caller's cwd.
	if ( !cssFile.empty() )
		cssFile = absolutePath( std::move( cssFile ) );
	if ( !xmlFile.empty() )
		xmlFile = absolutePath( std::move( xmlFile ) );
	if ( !projectFile.empty() )
		projectFile = absolutePath( std::move( projectFile ) );

	Log::instance()->setLiveWrite( true );
	Log::instance()->setLogToStdOut( !Runtime::isOffscreen() );

	std::string resourceBasePath = Sys::getProcessPath();
#if EE_PLATFORM == EE_PLATFORM_MACOS
	if ( String::contains( resourceBasePath, "eepp-UIEditor.app" ) )
		resourceBasePath = FileSystem::getCurrentWorkingDirectory();
#elif EE_PLATFORM == EE_PLATFORM_LINUX
	if ( String::contains( resourceBasePath, ".mount_" ) )
		resourceBasePath = FileSystem::getCurrentWorkingDirectory();
#endif
	FileSystem::dirAddSlashAtEnd( resourceBasePath );
	mResPath = resourceBasePath + "assets/";
	mUIApplication = std::make_unique<UIApplication>(
		WindowSettings( 1280, 720, "eepp - UI Editor", WindowStyle::Default, WindowBackend::Default,
						32, mResPath + "icon/ee.png", pixelDensityConf ),
		UIApplication::Settings( std::move( resourceBasePath ), pixelDensityConf ),
		ContextSettings( false, ContextSettings::FrameRateLimitScreenRefreshRate, 4 ) );
	mWindow = mUIApplication->getWindow();
	mAppUISceneNode = mUIApplication->getUI();
	if ( !mWindow || !mWindow->isOpen() || !mAppUISceneNode )
		return EXIT_FAILURE;

	mFileWatcher = new efsw::FileWatcher();
	mListener = new UpdateListener();
	mFileWatcher->watch();
	mWindow->setCloseRequestCallback(
		[this]( auto* window ) -> bool { return onCloseRequestCallback( window ); } );
	mWindow->setQuitCallback( [this]( EE::Window::Window* win ) {
		if ( mWindow->isOpen() )
			onCloseRequestCallback( win );
	} );
	mAppUISceneNode->setId( "appUiSceneNode" );
	// UIApplication may destroy the scene before run() returns when its window closes.
	mAppUISceneNode->on( Event::OnClose, [this]( const Event* ) { releaseEditor(); } );
	auto context = mAppUISceneNode->makeCurrent();
	mAppUISceneNode->enableDrawInvalidation();
	mDisplayDPI = mAppUISceneNode->getDPI();
	mBaseStyleSheet = mResPath + "ui/breeze.css";
	mTheme = mAppUISceneNode->getUIThemeManager()->getDefaultThemeHandle();
	mMenuIconSize =
		StyleSheetLength( 11, StyleSheetLength::Dp ).asPixels( 0, Sizef(), mDisplayDPI );
	mUIColorScheme = ColorSchemePreferences::fromStringExt( colorScheme );
	loadConfig();

	const auto baseUI = R"xml(
	<vbox id="main_layout" layout_width="match_parent" layout_height="match_parent">
		<MenuBar id="menubar" layout_width="match_parent" layout_height="wrap_content" />
		<Splitter id="project_splitter" layout_width="match_parent" layout_height="0dp" layout_weight="1">
			<vbox id="code_container" />
			<vbox id="preview_container" />
		</Splitter>
	</vbox>
	)xml";
	mAppUISceneNode->loadLayoutFromString( baseUI );

	createAppMenu();
	createKeyBindings();

	mConfigPath = Sys::getConfigPath( "eepp-uieditor" );
	FileSystem::dirAddSlashAtEnd( mConfigPath );
	mColorSchemesPath = mConfigPath + "colorschemes/";
	auto colorSchemes(
		SyntaxColorScheme::loadFromFile( mResPath + "colorschemes/colorschemes.conf" ) );
	if ( FileSystem::isDirectory( mColorSchemesPath ) ) {
		auto colorSchemesFiles = FileSystem::filesGetInPath( mColorSchemesPath );
		for ( auto& file : colorSchemesFiles ) {
			auto colorSchemesInFile = SyntaxColorScheme::loadFromFile( mColorSchemesPath + file );
			for ( auto& coloScheme : colorSchemesInFile )
				colorSchemes.emplace_back( std::move( coloScheme ) );
		}
	}
	mAppUISceneNode->bind( "code_container", mBaseLayout );
	mAppUISceneNode->bind( "preview_container", mPreviewLayout );
	mAppUISceneNode->bind( "project_splitter", mProjectSplitter );
	mSidePanel = mProjectSplitter->getFirstWidget();
	mSplitter = UICodeEditorSplitter::New( this, mAppUISceneNode, nullptr, colorSchemes, "eepp" );
	mSplitter->setHideTabBarOnSingleTab( false );
	mSplitter->createEditorWithTabWidget( mBaseLayout );
	mPreviewHost = eeNew( PreviewHost, ( *this ) );
	mPreviewHost->setParent( mPreviewLayout );
	mPreviewScene = static_cast<PreviewHost*>( mPreviewHost )->scene();
	mPreviewScene->setId( "previewScene" );
	mPreviewScene->getResourceScope()->importCatalog( defaultResourceScope().getLocalCatalog() );
	mPreviewScene->getUIIconThemeManager()->setFallbackTheme(
		mAppUISceneNode->getUIIconThemeManager()->getCurrentThemeHandle() );
	mPreviewScene->setColorSchemePreference( mUIColorScheme );
	mPreviewScene->enableDrawInvalidation();
	mPreviewScene->getUIThemeManager()->setDefaultTheme( mUseDefaultTheme ? mTheme : UIThemePtr{} );
	mPreviewHost->on( Event::OnSizeChange, [this]( const Event* ) { resizeCb(); } );
	mProjectSplitter->setSplitPartition( StyleSheetLength( 30, StyleSheetLength::Percentage ) );
	updateRecentMenu( true );
	updateRecentMenu( false );
	resizeCb();
	loadBaseStyleSheet();
	if ( !cssFile.empty() )
		loadStyleSheet( cssFile );

	if ( !xmlFile.empty() )
		loadLayoutFile( xmlFile );

	if ( !projectFile.empty() )
		loadProject( projectFile );

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	if ( xmlFile.empty() && cssFile.empty() ) {
		loadStyleSheet( "assets/layouts/test.css" );
		loadLayoutFile( "assets/layouts/test.xml" );
	}
#endif

	if ( xmlFile.empty() && projectFile.empty() ) {
		createNewLayout();
	}

	return mUIApplication->run();
}

std::string App::titleFromEditor( UICodeEditor* editor ) {
	std::string title( editor->getDocument().getFilename() );
	return editor->getDocument().isDirty() ? title + "*" : title;
}

void App::updateEditorTabTitle( UICodeEditor* editor ) {
	std::string title( titleFromEditor( editor ) );
	if ( editor->getData() ) {
		UITab* tab = (UITab*)editor->getData();
		tab->setText( title );
	}
}

void App::updateEditorTitle( UICodeEditor* editor ) {
	updateEditorTabTitle( editor );
}

void App::tryUpdateEditorTitle( UICodeEditor* editor ) {
	if ( !editor->getData() )
		return;
	bool isDirty = editor->getDocument().isDirty();
	bool tabDirty = ( (UITab*)editor->getData() )->getText().lastChar() == '*';

	if ( isDirty != tabDirty )
		updateEditorTitle( editor );
}

void App::onDocumentSelectionChange( UICodeEditor* editor, TextDocument& ) {
	tryUpdateEditorTitle( editor );
}

void App::onDocumentModified( UICodeEditor* editor, TextDocument& doc ) {
	tryUpdateEditorTitle( editor );

	if ( doc.getFilePath() == mCurrentLayout ) {
		invalidatePreview();
	} else if ( doc.getFilePath() == mCurrentStyleSheet ) {
		invalidatePreview();
	} else if ( mUseDefaultTheme && doc.getFilePath() == mBaseStyleSheet ) {
		invalidatePreview();
	}
}

void App::onDocumentUndoRedo( UICodeEditor* editor, TextDocument& doc ) {
	onDocumentModified( editor, doc );
}

void App::onDocumentLoaded( UICodeEditor* editor, const std::string& path ) {
	mSplitter->removeUnusedTab( mSplitter->tabWidgetFromEditor( editor ) );

	updateEditorTabTitle( editor );

	if ( !path.empty() ) {
		UITab* tab = reinterpret_cast<UITab*>( editor->getData() );
		tab->setTooltipText( path );
	}
}

void App::saveAllProcess() {
	if ( mTmpDocs.empty() )
		return;

	mSplitter->forEachEditorStoppable( [this]( UICodeEditor* editor ) {
		if ( editor->getDocument().isDirty() &&
			 mTmpDocs.find( &editor->getDocument() ) != mTmpDocs.end() ) {
			if ( editor->getDocument().hasFilepath() && !editor->getDocument().isDeleteOnClose() ) {
				editor->save();
				updateEditorTabTitle( editor );
				if ( mSplitter->getCurEditor() == editor )
					updateEditorTitle( editor );
				mTmpDocs.erase( &editor->getDocument() );
			} else {
				UIFileDialog* dialog = saveFileDialog( editor, false );
				dialog->on( Event::SaveFile, [this, editor]( const Event* ) {
					updateEditorTabTitle( editor );
					if ( mSplitter->getCurEditor() == editor )
						updateEditorTitle( editor );
				} );
				dialog->on( Event::OnWindowClose, [this, editor]( const Event* ) {
					mTmpDocs.erase( &editor->getDocument() );
					if ( !SceneManager::instance()->isShuttingDown() && !mTmpDocs.empty() )
						saveAllProcess();
				} );
				return true;
			}
		}
		return false;
	} );
}

void App::saveDoc() {
	if ( mSplitter->getCurEditor() ) {
		auto* editor = mSplitter->getCurEditor();
		if ( editor->getDocument().isDeleteOnClose() || !editor->getDocument().hasFilepath() )
			saveFileDialog( editor );
		else
			editor->save();
		updateEditorTabTitle( mSplitter->getCurEditor() );
	}
}

void App::saveAll() {
	mTmpDocs.clear();
	mSplitter->forEachEditor( [this]( UICodeEditor* editor ) {
		if ( editor->isDirty() )
			mTmpDocs.insert( &editor->getDocument() );
	} );
	saveAllProcess();
}

void App::onCodeEditorCreated( UICodeEditor* editor, TextDocument& doc ) {
	editor->setAutoCloseXMLTags( true );
	editor->setColorPreview( true );
	editor->setLineWrapMode( LineWrapMode::Word );
	doc.setCommand( "save-doc", [this] { saveDoc(); } );
	doc.setCommand( "save-as-doc", [this] {
		if ( mSplitter->curEditorExistsAndFocused() )
			saveFileDialog( mSplitter->getCurEditor() );
	} );
	doc.setCommand( "save-all", [this] { saveAll(); } );
	doc.setCommand( "create-new", [this] { createNewLayout(); } );
}

void App::onCodeEditorFocusChange( UICodeEditor* editor ) {
	std::string ext( FileSystem::fileExtension( editor->getDocument().getFilePath() ) );
	if ( ext == "xml" && !editor->getDocument().isEmpty() &&
		 editor->getDocument().getFilePath() != mCurrentLayout )
		loadLayout( editor->getDocument().getFilePath() );
}

} // namespace uieditor

using namespace uieditor;

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
	args::ArgumentParser parser( "eepp UIEditor" );
	args::HelpFlag help( parser, "help", "Display this help menu", { 'h', "help" } );
	args::ValueFlag<std::string> xmlFile( parser, "xml", "Loads XML file", { 'x', "xml" } );
	args::ValueFlag<std::string> cssFile( parser, "css", "Loads CSS file", { 'c', "css" } );
	args::ValueFlag<std::string> projectFile( parser, "project", "Loads project file",
											  { 'p', "project" } );
	args::ValueFlag<Float> pixelDensityConf( parser, "pixel-density",
											 "Set default application pixel density",
											 { 'd', "pixel-density" }, 0.f );
	args::Flag disableAppTheme( parser, "disable-app-theme",
								"Disable the default Breeze theme in the preview.",
								{ "disable-app-theme" } );
	args::ValueFlag<std::string> prefersColorScheme(
		parser, "prefers-color-scheme",
		"Set the preferred color scheme (\"light\", \"dark\" or \"system\")",
		{ "prefers-color-scheme" } );

	try {
		parser.ParseCLI( Sys::parseArguments( argc, argv ) );
	} catch ( const args::Help& ) {
		std::cout << parser;
		return EXIT_SUCCESS;
	} catch ( const args::ParseError& e ) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return EXIT_FAILURE;
	} catch ( args::ValidationError& e ) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return EXIT_FAILURE;
	}

	int result;
	{
		App app;
		result = app.init( pixelDensityConf.Get(), !disableAppTheme.Get(), cssFile.Get(),
						   xmlFile.Get(), projectFile.Get(), prefersColorScheme.Get() );
	}

	MemoryManager::showResults();

	return result;
}
