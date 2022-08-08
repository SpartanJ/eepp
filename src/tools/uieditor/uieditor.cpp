#include "uieditor.hpp"
#include <args/args.hxx>
#include <pugixml/pugixml.hpp>

namespace uieditor {
/**
This is a real time visual editor for the UI module.
The layout files can be edited with any editor, and the layout changes can be seen live with this
editor. So this is a layout preview app. The layout is updated every time the layout file is
modified by the user. You'll need to save the file in your editor to see the changes. This was
done in a rush for a personal project ( hence the horrendous code ), but it's quite useful and
functional. Project files are created by hand for the moment, and they shuld look like this one:

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

App* appInstance = nullptr;

void appLoop() {
	appInstance->mainLoop();
}

void App::updateLayoutFunc() {
	mUpdateLayout = true;
	mWaitClock.restart();
}

void App::updateStyleSheetFunc() {
	mUpdateStyleSheet = true;
	mCssWaitClock.restart();
}
void App::updateBaseStyleSheetFunc() {
	mUpdateBaseStyleSheet = true;
	mCssBaseWaitClock.restart();
}

const std::string& App::getCurrentLayout() const {
	return mCurrentLayout;
}

const std::string& App::getCurrentStyleSheet() const {
	return mCurrentStyleSheet;
}

const std::string& App::getBaseStyleSheet() const {
	return mBaseStyleSheet;
}

class UpdateListener : public efsw::FileWatchListener {
  public:
	UpdateListener( App* app ) : mApp( app ) {}

	virtual ~UpdateListener() {}

	void handleFileAction( efsw::WatchID, const std::string& dir, const std::string& filename,
						   efsw::Action action, std::string ) {
		if ( action == efsw::Actions::Modified ) {
			if ( dir + filename == mApp->getCurrentLayout() ) {
				mApp->updateLayoutFunc();
			} else if ( dir + filename == mApp->getCurrentStyleSheet() ) {
				mApp->updateStyleSheetFunc();
			} else if ( dir + filename == mApp->getBaseStyleSheet() ) {
				mApp->updateBaseStyleSheetFunc();
			}
		}
	}

  protected:
	App* mApp;
};

static bool isFont( const std::string& path ) {
	std::string ext = FileSystem::fileExtension( path );
	return ext == "ttf" || ext == "otf" || ext == "wolff";
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

void App::unloadImages() {
	for ( auto it = mImagesLoaded.begin(); it != mImagesLoaded.end(); ++it ) {
		GlobalTextureAtlas::instance()->remove( it->second );
		TextureFactory::instance()->remove( it->first );
	}
	mImagesLoaded.clear();
}

void App::unloadFonts() {
	for ( auto it = mFontsLoaded.begin(); it != mFontsLoaded.end(); ++it )
		FontManager::instance()->remove( it->first );
	mFontsLoaded.clear();
}

void App::loadImage( std::string path ) {
	std::string filename( FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( path ) ) );
	Uint32 texId = TextureFactory::instance()->loadFromFile( path );
	TextureRegion* texRegion = GlobalTextureAtlas::instance()->add( texId, filename );
	mImagesLoaded[texId] = texRegion;
}

FontTrueType* App::loadFont( const std::string& name, std::string fontPath,
							 const std::string& fallback ) {
	if ( FileSystem::isRelativePath( fontPath ) )
		fontPath = mResPath + fontPath;
	if ( fontPath.empty() || !FileSystem::fileExists( fontPath ) ) {
		fontPath = fallback;
		if ( !fontPath.empty() && FileSystem::isRelativePath( fontPath ) )
			fontPath = mResPath + fontPath;
	}
	if ( fontPath.empty() )
		return nullptr;
	return FontTrueType::New( name, fontPath );
}

void App::loadFont( std::string path ) {
	std::string filename( FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( path ) ) );
	FontTrueType* font = FontTrueType::New( filename );
	font->loadFromFile( path );
	mFontsLoaded[font] = filename;
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

void App::setUserDefaultTheme() {
	mUseDefaultTheme = true;
	mUISceneNode->getUIThemeManager()->setDefaultTheme( mTheme );
	mUISceneNode->setStyleSheet( mTheme->getStyleSheet() );
}

void App::loadStyleSheet( std::string cssPath ) {
	CSS::StyleSheetParser parser;

	if ( NULL != mUISceneNode && parser.loadFromFile( cssPath ) ) {
		if ( mUseDefaultTheme ) {
			setUserDefaultTheme();
			mUISceneNode->combineStyleSheet( parser.getStyleSheet() );
		} else {
			mUISceneNode->setStyleSheet( parser.getStyleSheet() );
		}

		mCurrentStyleSheet = cssPath;

		std::string folder( FileSystem::fileRemoveFileName( cssPath ) );

		bool keepWatch = false;

		for ( auto& directory : mFileWatcher->directories() ) {
			if ( directory == folder )
				keepWatch = true;
		}

		if ( !keepWatch ) {
			if ( mStyleSheetWatch != 0 )
				mFileWatcher->removeWatch( mStyleSheetWatch );

			mStyleSheetWatch = mFileWatcher->addWatch( folder, mListener );
		}

		if ( mBaseStyleSheetWatch == 0 && mUseDefaultTheme ) {
			std::string baseFolder( FileSystem::fileRemoveFileName( mBaseStyleSheet ) );
			mBaseStyleSheetWatch = mFileWatcher->addWatch( baseFolder, mListener );
		}
	}
}

void App::loadLayout( std::string file ) {
	std::string folder( FileSystem::fileRemoveFileName( file ) );

	bool keepWatch = false;

	for ( auto& directory : mFileWatcher->directories() ) {
		if ( directory == folder )
			keepWatch = true;
	}

	if ( !keepWatch ) {
		if ( mWatch != 0 )
			mFileWatcher->removeWatch( mWatch );
		mWatch = mFileWatcher->addWatch( folder, mListener );
	}

	mUIContainer->getContainer()->childsCloseAll();
	mUISceneNode->update( Time::Zero );

	mUISceneNode->loadLayoutFromFile( file, mUIContainer );

	mCurrentLayout = file;
}

void App::refreshLayout() {
	if ( !mCurrentLayout.empty() && FileSystem::fileExists( mCurrentLayout ) &&
		 mUIContainer != NULL ) {
		if ( !mCurrentStyleSheet.empty() && FileSystem::fileExists( mCurrentStyleSheet ) &&
			 mUIContainer != NULL )
			loadStyleSheet( mCurrentStyleSheet );

		loadLayout( mCurrentLayout );
	}

	mUpdateLayout = false;
}

void App::refreshStyleSheet() {
	if ( mUpdateBaseStyleSheet )
		mTheme->reloadStyleSheet();

	if ( !mCurrentStyleSheet.empty() && FileSystem::fileExists( mCurrentStyleSheet ) &&
		 mUIContainer != NULL ) {
		loadStyleSheet( mCurrentStyleSheet );
	} else if ( mUpdateBaseStyleSheet ) {
		setUserDefaultTheme();
	}

	if ( !mCurrentLayout.empty() && FileSystem::fileExists( mCurrentLayout ) &&
		 mUIContainer != NULL ) {
		loadLayout( mCurrentLayout );
	}

	mUpdateStyleSheet = false;
	mUpdateBaseStyleSheet = false;
}

void App::onRecentProjectClick( const Event* event ) {
	if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = event->getNode()->asType<UIMenuItem>()->getText();
	std::string path( txt.toUtf8() );

	if ( FileSystem::fileExists( path ) && !FileSystem::isDirectory( path ) ) {
		loadProject( path );
	}
}

void App::onRecentFilesClick( const Event* event ) {
	if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = event->getNode()->asType<UIMenuItem>()->getText();
	std::string path( txt.toUtf8() );

	if ( FileSystem::fileExists( path ) && !FileSystem::isDirectory( path ) )
		loadLayoutFile( path );
}

void App::updateRecentFiles() {
	SceneManager::instance()->setCurrentUISceneNode( mAppUiSceneNode );

	if ( NULL == mUIMenuBar )
		return;

	UIPopUpMenu* fileMenu = mUIMenuBar->getPopUpMenu( "File" );

	UINode* node = NULL;

	if ( NULL != fileMenu && ( node = fileMenu->getItem( "Recent files" ) ) ) {
		UIMenuSubMenu* uiMenuSubMenu = static_cast<UIMenuSubMenu*>( node );
		UIMenu* menu = uiMenuSubMenu->getSubMenu();

		menu->removeAll();

		for ( size_t i = 0; i < mRecentFiles.size(); i++ )
			menu->add( mRecentFiles[i] );

		if ( 0xFFFFFFFF != mRecentFilesEventClickId )
			menu->removeEventListener( mRecentFilesEventClickId );

		mRecentFilesEventClickId = menu->addEventListener(
			Event::OnItemClicked, [&]( const Event* event ) { onRecentFilesClick( event ); } );
	}

	SceneManager::instance()->setCurrentUISceneNode( mUISceneNode );
}

void App::updateRecentProjects() {
	SceneManager::instance()->setCurrentUISceneNode( mAppUiSceneNode );

	if ( NULL == mUIMenuBar )
		return;

	UIPopUpMenu* fileMenu = mUIMenuBar->getPopUpMenu( "File" );

	UINode* node = NULL;

	if ( NULL != fileMenu && ( node = fileMenu->getItem( "Recent projects" ) ) ) {
		UIMenuSubMenu* uiMenuSubMenu = static_cast<UIMenuSubMenu*>( node );
		UIMenu* menu = uiMenuSubMenu->getSubMenu();

		menu->removeAll();

		for ( size_t i = 0; i < mRecentProjects.size(); i++ )
			menu->add( mRecentProjects[i] );

		if ( 0xFFFFFFFF != mRecentProjectEventClickId )
			menu->removeEventListener( mRecentProjectEventClickId );

		mRecentProjectEventClickId = menu->addEventListener(
			Event::OnItemClicked, [&]( const Event* event ) { onRecentProjectClick( event ); } );
	}

	SceneManager::instance()->setCurrentUISceneNode( mUISceneNode );
}

void App::resizeCb( EE::Window::Window* window ) {
	mUIContainer->setPadding(
		Rectf( 0, mUIMenuBar ? mUIMenuBar->getSize().getHeight() : 0.f, 0, 0 ) );
	if ( mLayoutExpanded ) {
		mUIContainer->setSize( mUISceneNode->getSize() );
	} else if ( !mPreserveContainerSize ) {
		Float scaleW =
			(Float)mUISceneNode->getSize().getWidth() / (Float)mUIContainer->getSize().getWidth();
		Float scaleH = (Float)mUISceneNode->getSize().getHeight() /
					   ( (Float)mUIContainer->getSize().getHeight() );

		mUIContainer->setScale( scaleW < scaleH ? scaleW : scaleH );
		mUIContainer->center();
	} else {
		window->setSize( mUIContainer->getPixelsSize().getWidth(),
						 mUIContainer->getPixelsSize().getHeight() );
	}
}

void App::resizeWindowToLayout() {
	if ( mLayoutExpanded )
		return;

	Sizef size( mUIContainer->getSize() );
	Rect borderSize( mWindow->getBorderSize() );
	Sizei displayMode = Engine::instance()
							->getDisplayManager()
							->getDisplayIndex( mWindow->getCurrentDisplayIndex() )
							->getUsableBounds()
							.getSize();
	displayMode.x = displayMode.x - borderSize.Left - borderSize.Right;
	displayMode.y = displayMode.y - borderSize.Top - borderSize.Bottom;

	Float scaleW =
		size.getWidth() > displayMode.getWidth() ? displayMode.getWidth() / size.getWidth() : 1.f;
	Float scaleH = size.getHeight() > displayMode.getHeight()
					   ? displayMode.getHeight() / size.getHeight()
					   : 1.f;
	Float scale = scaleW < scaleH ? scaleW : scaleH;

	mWindow->setSize( (Uint32)( size.getWidth() * scale ), (Uint32)( size.getHeight() * scale ) );
	mWindow->centerToDisplay();
}

UIWidget* App::createWidget( std::string widgetName ) {
	return UIWidgetCreator::createFromName( mWidgetRegistered[widgetName] );
}

std::string App::pathFix( std::string path ) {
	if ( !path.empty() && ( path.at( 0 ) != '/' || !( Sys::getPlatform() == "Windows" &&
													  path.size() > 3 && path.at( 1 ) != ':' ) ) ) {
		return mBasePath + path;
	}

	return path;
}

void App::loadUITheme( std::string themePath ) {
	TextureAtlasLoader tgl( themePath );

	std::string name(
		FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( themePath ) ) );

	UITheme* uitheme = UITheme::loadFromTextureAtlas(
		UITheme::New( name, name ), TextureAtlasManager::instance()->getByName( name ) );

	mUISceneNode->getUIThemeManager()->setDefaultTheme( uitheme )->add( uitheme );
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

	std::map<std::string, std::string>::iterator it;

	if ( ( it = mLayouts.find( txt.toUtf8() ) ) != mLayouts.end() )
		loadLayout( it->second );
}

void App::refreshLayoutList() {
	if ( NULL == mUIMenuBar )
		return;

	SceneManager::instance()->setCurrentUISceneNode( mAppUiSceneNode );

	if ( mLayouts.size() > 0 ) {
		UIPopUpMenu* uiLayoutsMenu = NULL;

		if ( mUIMenuBar->getButton( "Layouts" ) == NULL ) {
			uiLayoutsMenu = UIPopUpMenu::New();

			mUIMenuBar->addMenuButton( "Layouts", uiLayoutsMenu );

			uiLayoutsMenu->addEventListener(
				Event::OnItemClicked, [&]( const Event* event ) { onLayoutSelected( event ); } );
		} else {
			uiLayoutsMenu = mUIMenuBar->getPopUpMenu( "Layouts" );
		}

		uiLayoutsMenu->removeAll();

		for ( auto it = mLayouts.begin(); it != mLayouts.end(); ++it )
			uiLayoutsMenu->addCheckBox( it->first )->setActive( mCurrentLayout == it->second );
	} else if ( mUIMenuBar->getButton( "Layouts" ) != NULL ) {
		mUIMenuBar->removeMenuButton( "Layouts" );
	}

	SceneManager::instance()->setCurrentUISceneNode( mUISceneNode );
}

void App::loadProjectNodes( pugi::xml_node node ) {
	mUISceneNode->getUIThemeManager()->setDefaultTheme( mUseDefaultTheme ? mTheme : NULL );

	for ( pugi::xml_node resources = node; resources; resources = resources.next_sibling() ) {
		std::string name = String::toLower( std::string( resources.name() ) );

		if ( name == "uiproject" ) {
			pugi::xml_node basePathNode = resources.child( "basepath" );

			if ( !basePathNode.empty() )
				mBasePath = basePathNode.text().as_string();

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
					mWidgetRegistered[String::toLower( wname )] = replacement;
				}

				for ( auto it = mWidgetRegistered.begin(); it != mWidgetRegistered.end(); ++it ) {
					if ( !UIWidgetCreator::existsCustomWidgetCallback( it->first ) )
						UIWidgetCreator::addCustomWidgetCallback(
							it->first, [&]( std::string widgetName ) -> UIWidget* {
								return createWidget( widgetName );
							} );
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
				std::string cssPath( styleSheetNode.attribute( "path" ).as_string() );

				if ( isCSS( cssPath ) && FileSystem::fileExists( mBasePath + cssPath ) )
					loadStyleSheet( mBasePath + cssPath );
			}

			pugi::xml_node layoutNode = resources.child( "layout" );

			if ( !layoutNode.empty() ) {
				bool loaded = false;
				bool loadedSizedLayout = false;

				Float width = layoutNode.attribute( "width" ).as_float();
				Float height = layoutNode.attribute( "height" ).as_float();

				if ( 0.f != width && 0.f != height ) {
					mLayoutExpanded = false;
					mUIContainer->setSize( width, height );
				} else {
					mLayoutExpanded = true;
				}

				resizeCb( mWindow );

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

						if ( width != 0 && height != 0 )
							loadedSizedLayout = true;
					}
				}

				if ( mLayouts.size() > 0 && !loaded ) {
					loadLayout( mLayouts.begin()->second );

					if ( width != 0 && height != 0 )
						loadedSizedLayout = true;
				}

				if ( loadedSizedLayout )
					resizeWindowToLayout();
			}

			refreshLayoutList();
		}
	}
}

void App::loadLayoutFile( std::string layoutPath ) {
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

		updateRecentFiles();
	}
}

void App::loadProject( std::string projectPath ) {
	if ( FileSystem::fileExists( projectPath ) ) {
		closeProject();

		mBasePath = FileSystem::fileRemoveFileName( projectPath );

		FileSystem::changeWorkingDirectory( mBasePath );

		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file( projectPath.c_str() );

		if ( result ) {
			loadProjectNodes( doc.first_child() );

			for ( auto pathIt = mRecentProjects.begin(); pathIt != mRecentProjects.end();
				  pathIt++ ) {
				if ( *pathIt == projectPath ) {
					mRecentProjects.erase( pathIt );
					break;
				}
			}

			mRecentProjects.insert( mRecentProjects.begin(), projectPath );

			if ( mRecentProjects.size() > 10 )
				mRecentProjects.resize( 10 );

			updateRecentProjects();
		} else {
			Log::error( "Couldn't load UI Layout: %s", projectPath.c_str() );
			Log::error( "Error description: %s", result.description() );
			Log::error( "Error offset: %d", result.offset );
		}
	}
}

void App::closeProject() {
	mCurrentLayout = "";
	mCurrentStyleSheet = "";
	mUIContainer->getContainer()->childsCloseAll();
	mUISceneNode->update( Time::Zero );
	mUISceneNode->setStyleSheet( CSS::StyleSheet() );

	mLayouts.clear();

	refreshLayoutList();

	unloadFonts();
	unloadImages();
}

bool App::onCloseRequestCallback( EE::Window::Window* ) {
	SceneManager::instance()->setCurrentUISceneNode( mAppUiSceneNode );

	mMsgBox = UIMessageBox::New(
		UIMessageBox::OK_CANCEL,
		"Do you really want to close the current file?\nAll changes will be lost." );
	mMsgBox->setTheme( mTheme );
	mMsgBox->addEventListener( Event::MsgBoxConfirmClick,
							   [&]( const Event* ) { mWindow->close(); } );
	mMsgBox->addEventListener( Event::OnClose, [&]( const Event* ) { mMsgBox = NULL; } );
	mMsgBox->setTitle( "Close Editor?" );
	mMsgBox->center();
	mMsgBox->show();

	SceneManager::instance()->setCurrentUISceneNode( mUISceneNode );
	return false;
}

void App::mainLoop() {
	mWindow->getInput()->update();

	if ( mWindow->getInput()->isKeyUp( KEY_ESCAPE ) && NULL == mMsgBox &&
		 onCloseRequestCallback( mWindow ) )
		mWindow->close();

	if ( mWindow->getInput()->isKeyUp( KEY_F3 ) || mWindow->getInput()->isKeyUp( KEY_BACKSLASH ) )
		mConsole->toggle();

	if ( NULL != mUIContainer && mWindow->getInput()->isKeyUp( KEY_F1 ) )
		resizeWindowToLayout();

	if ( mWindow->getInput()->isKeyUp( KEY_F6 ) ) {
		mUISceneNode->setHighlightFocus( !mUISceneNode->getHighlightFocus() );
		mUISceneNode->setHighlightOver( !mUISceneNode->getHighlightOver() );
	}

	if ( mWindow->getInput()->isKeyUp( KEY_F7 ) )
		mUISceneNode->setDrawBoxes( !mUISceneNode->getDrawBoxes() );

	if ( mWindow->getInput()->isKeyUp( KEY_F8 ) )
		mUISceneNode->setDrawDebugData( !mUISceneNode->getDrawDebugData() );

	if ( mWindow->getInput()->isKeyUp( KEY_F9 ) ) {
		Clock clock;
		mUISceneNode->getRoot()->reportStyleStateChangeRecursive();
		Log::info( "Applied style state changes in: %.2fms",
				   clock.getElapsedTime().asMilliseconds() );
	}

	if ( mUpdateLayout && mWaitClock.getElapsedTime().asMilliseconds() > 250.f )
		refreshLayout();

	if ( ( mUpdateStyleSheet && mCssWaitClock.getElapsedTime().asMilliseconds() > 250.f ) ||
		 ( mUpdateBaseStyleSheet && mCssBaseWaitClock.getElapsedTime().asMilliseconds() > 250.f ) )
		refreshStyleSheet();

	SceneManager::instance()->update();

	if ( mAppUiSceneNode->invalidated() || mUISceneNode->invalidated() ) {
		mWindow->clear();

		SceneManager::instance()->draw();

		mWindow->display();
	} else {
		Sys::sleep( Milliseconds( 8 ) );
	}
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
	dialog->setTheme( mTheme );
	dialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( title );
	dialog->addEventListener( Event::OpenFile, cb );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->center();
	dialog->show();
}

void App::fileMenuClick( const Event* event ) {
	if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& id = event->getNode()->asType<UIMenuItem>()->getId();

	SceneManager::instance()->setCurrentUISceneNode( mAppUiSceneNode );

	if ( "open-project" == id ) {
		showFileDialog(
			"Open project...", [&]( const Event* event ) { projectOpen( event ); }, "*.xml" );
	} else if ( "open-layout" == id ) {
		showFileDialog(
			"Open layout...", [&]( const Event* event ) { layoutOpen( event ); }, "*.xml" );
	} else if ( "close" == id ) {
		closeProject();
	} else if ( "quit" == id ) {
		onCloseRequestCallback( mWindow );
	} else if ( "load-images-from-path" == id ) {
		showFileDialog(
			"Open images from folder...", [&]( const Event* event ) { imagePathOpen( event ); },
			"*", UIFileDialog::DefaultFlags | UIFileDialog::AllowFolderSelect );
	} else if ( "load-fonts-from-path" == id ) {
		showFileDialog(
			"Open fonts from folder...", [&]( const Event* event ) { fontPathOpen( event ); }, "*",
			UIFileDialog::DefaultFlags | UIFileDialog::AllowFolderSelect );
	} else if ( "load-css-from-path" == id ) {
		showFileDialog(
			"Open style sheet from path...",
			[&]( const Event* event ) { styleSheetPathOpen( event ); }, "*.css" );
	}

	SceneManager::instance()->setCurrentUISceneNode( mUISceneNode );
}

void App::createAppMenu() {
	SceneManager::instance()->setCurrentUISceneNode( mAppUiSceneNode );

	mUIMenuBar = UIMenuBar::New();

	size_t iconSize = mMenuIconSize;
	UIPopUpMenu* uiPopMenu = UIPopUpMenu::New();
	uiPopMenu
		->add( "Open project...", mAppUiSceneNode->findIconDrawable( "document-open", iconSize ) )
		->setId( "open-project" );
	uiPopMenu->addSeparator();
	uiPopMenu
		->add( "Open layout...", mAppUiSceneNode->findIconDrawable( "document-open", iconSize ) )
		->setId( "open-layout" );
	uiPopMenu->addSeparator();
	uiPopMenu->addSubMenu( "Recent files", NULL, UIPopUpMenu::New() )->setId( "recent-files" );
	uiPopMenu->addSubMenu( "Recent projects", NULL, UIPopUpMenu::New() )
		->setId( "recent-projects" );
	uiPopMenu->addSeparator();
	uiPopMenu->add( "Close", mAppUiSceneNode->findIconDrawable( "document-close", iconSize ) )
		->setId( "close" );
	uiPopMenu->addSeparator();
	uiPopMenu->add( "Quit", mAppUiSceneNode->findIconDrawable( "quit", iconSize ) )
		->setId( "quit" );
	mUIMenuBar->addMenuButton( "File", uiPopMenu );
	uiPopMenu->addEventListener( Event::OnItemClicked,
								 [&]( const Event* event ) { fileMenuClick( event ); } );

	UIPopUpMenu* uiResourceMenu = UIPopUpMenu::New();
	uiResourceMenu
		->add( "Load images from path...",
			   mAppUiSceneNode->findIconDrawable( "document-open", iconSize ) )
		->setId( "load-images-from-path" );
	uiResourceMenu->addSeparator();
	uiResourceMenu
		->add( "Load fonts from path...",
			   mAppUiSceneNode->findIconDrawable( "document-open", iconSize ) )
		->setId( "load-fonts-from-path" );
	uiResourceMenu->addSeparator();
	uiResourceMenu
		->add( "Load style sheet from path...",
			   mAppUiSceneNode->findIconDrawable( "document-open", iconSize ) )
		->setId( "load-css-from-path" );
	mUIMenuBar->addMenuButton( "Resources", uiResourceMenu );
	uiResourceMenu->addEventListener( Event::OnItemClicked,
									  [&]( const Event* event ) { fileMenuClick( event ); } );

	mConsole = UIConsole::New();
	mConsole->setQuakeMode( true );
	mConsole->setVisible( false );

	SceneManager::instance()->setCurrentUISceneNode( mUISceneNode );
}

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
std::vector<std::string> parseEmscriptenArgs( int argc, char* argv[] ) {
	if ( argc < 1 )
		return {};
	std::vector<std::string> args;
	args.emplace_back( argv[0] );
	for ( int i = 1; i < argc; i++ ) {
		Log::debug( "argv %d %s", i, argv[i] );
		auto split = String::split( std::string( argv[i] ), '=' );
		if ( split.size() == 2 ) {
			std::string arg( split[0] + "=" + URI::decode( split[1] ) );
			args.emplace_back( !String::startsWith( arg, "--" ) ? ( std::string( "--" ) + arg )
																: arg );
		}
	}
	return args;
}
#endif

App::App() {}

App::~App() {
	saveConfig();

	delete mFileWatcher;

	delete mListener;
}

void App::init( const Float& pixelDensityConf, const bool& preserveContainerSizeFlag,
				const bool& useAppTheme, const std::string& cssFile, const std::string& xmlFile,
				const std::string& projectFile ) {
	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	displayManager->enableScreenSaver();
	displayManager->enableMouseFocusClickThrough();
	displayManager->disableBypassCompositor();

	mFileWatcher = new efsw::FileWatcher();
	mListener = new UpdateListener( this );
	mFileWatcher->watch();

	Display* currentDisplay = displayManager->getDisplayIndex( 0 );
	mDisplayDPI = currentDisplay->getDPI();
	Float pixelDensity = currentDisplay->getPixelDensity();

	if ( pixelDensityConf != 0 )
		pixelDensity = pixelDensityConf;

	if ( preserveContainerSizeFlag )
		mPreserveContainerSize = true;

	Log::instance()->setLiveWrite( true );
	Log::instance()->setConsoleOutput( true );

	mResPath = Sys::getProcessPath();

	mWindow = Engine::instance()->createWindow(
		WindowSettings( 1280, 720, "eepp - UI Editor", WindowStyle::Default, WindowBackend::Default,
						32, mResPath + "assets/icon/ee.png", pixelDensity ),
		ContextSettings( true, GLv_default, true, 24, 1, 0, true ) );

	if ( mWindow->isOpen() ) {
		PixelDensity::setPixelDensity( eemax( mWindow->getScale(), pixelDensity ) );

		mWindow->setCloseRequestCallback(
			[&]( auto* window ) -> bool { return onCloseRequestCallback( window ); } );

		mResPath = Sys::getProcessPath();
#if EE_PLATFORM == EE_PLATFORM_MACOSX
		if ( String::contains( mResPath, "eepp-UIEditor.app" ) ) {
			mResPath = FileSystem::getCurrentWorkingDirectory();
			FileSystem::dirAddSlashAtEnd( mResPath );
			mIsBundledApp = true;
		}
#elif EE_PLATFORM == EE_PLATFORM_LINUX
		if ( String::contains( mResPath, ".mount_" ) ) {
			mResPath = FileSystem::getCurrentWorkingDirectory();
			FileSystem::dirAddSlashAtEnd( mResPath );
			mIsBundledApp = true;
		}
#endif
		mResPath += "assets";
		FileSystem::dirAddSlashAtEnd( mResPath );

		FontTrueType* font =
			FontTrueType::New( "NotoSans-Regular", mResPath + "fonts/NotoSans-Regular.ttf" );
		FontTrueType::New( "monospace", mResPath + "fonts/DejaVuSansMono.ttf" );

		mBaseStyleSheet = mResPath + "ui/breeze.css";
		mTheme = UITheme::load( "uitheme", "uitheme", "", font, mBaseStyleSheet );

		mUISceneNode = UISceneNode::New();
		mUISceneNode->setId( "uiSceneNode" );
		mUISceneNode->setVerbose( true );
		SceneManager::instance()->add( mUISceneNode );

		mAppUiSceneNode = UISceneNode::New();
		mAppUiSceneNode->setId( "appUiSceneNode" );
		SceneManager::instance()->add( mAppUiSceneNode );

		mAppUiSceneNode->enableDrawInvalidation();
		mUISceneNode->enableDrawInvalidation();

		FontTrueType* iconFont = loadFont( "icon", "fonts/remixicon.ttf" );
		UIIconTheme* iconTheme = UIIconTheme::New( "ecode" );
		UIIconTheme* iconTheme2 = UIIconTheme::New( "ecode" );
		StyleSheetLength fontSize{ 11, StyleSheetLength::Dp };
		mMenuIconSize = fontSize.asPixels( 0, Sizef(), mDisplayDPI );
		std::unordered_map<std::string, Uint32> icons = {
			{ "document-new", 0xecc3 },
			{ "document-open", 0xed70 },
			{ "document-save", 0xf0b3 },
			{ "document-save-as", 0xf0b3 },
			{ "document-close", 0xeb99 },
			{ "quit", 0xeb97 },
			{ "undo", 0xea58 },
			{ "redo", 0xea5a },
			{ "cut", 0xf0c1 },
			{ "copy", 0xecd5 },
			{ "paste", 0xeb91 },
			{ "edit", 0xec86 },
			{ "split-horizontal", 0xf17a },
			{ "split-vertical", 0xf17b },
			{ "find-replace", 0xed2b },
			{ "folder", 0xed54 },
			{ "folder-open", 0xed70 },
			{ "folder-add", 0xed5a },
			{ "file", 0xecc3 },
			{ "file-add", 0xecc9 },
			{ "file-copy", 0xecd3 },
			{ "file-code", 0xecd1 },
			{ "file-edit", 0xecdb },
			{ "font-size", 0xed8d },
			{ "delete-bin", 0xec1e },
			{ "delete-text", 0xec1e },
			{ "zoom-in", 0xf2db },
			{ "zoom-out", 0xf2dd },
			{ "zoom-reset", 0xeb47 },
			{ "fullscreen", 0xed9c },
			{ "keybindings", 0xee75 },
			{ "tree-expanded", 0xea50 },
			{ "tree-contracted", 0xea54 },
			{ "search", 0xf0d1 },
			{ "go-up", 0xea78 },
			{ "ok", 0xeb7a },
			{ "cancel", 0xeb98 },
			{ "color-picker", 0xf13d },
			{ "pixel-density", 0xed8c },
			{ "go-to-line", 0xf1f8 },
			{ "table-view", 0xf1de },
			{ "list-view", 0xecf1 },
			{ "menu-unfold", 0xef40 },
			{ "menu-fold", 0xef3d },
			{ "download-cloud", 0xec58 },
			{ "layout-left", 0xee94 },
			{ "layout-right", 0xee9b },
			{ "color-scheme", 0xebd4 },
			{ "global-settings", 0xedcf },
			{ "folder-user", 0xed84 },
			{ "help", 0xf045 },
			{ "terminal", 0xf1f6 },
			{ "earth", 0xec7a },
		};
		for ( const auto& icon : icons ) {
			iconTheme->add( UIGlyphIcon::New( icon.first, iconFont, icon.second ) );
			iconTheme2->add( UIGlyphIcon::New( icon.first, iconFont, icon.second ) );
		}

		mAppUiSceneNode->setStyleSheet( mTheme->getStyleSheet() );
		mAppUiSceneNode->getUIThemeManager()
			->setDefaultEffectsEnabled( true )
			->setDefaultTheme( mTheme )
			->setDefaultFont( font )
			->add( mTheme );

		mUISceneNode->getUIThemeManager()->setDefaultFont( font )->setDefaultEffectsEnabled( true );

		mAppUiSceneNode->getUIIconThemeManager()->setCurrentTheme( iconTheme );

		mUISceneNode->getUIIconThemeManager()->setCurrentTheme( iconTheme2 );

		loadConfig();

		createAppMenu();

		UIWindow::StyleConfig winStyle( UI_NODE_DEFAULT_FLAGS | UI_WIN_NO_DECORATION );
		mUIContainer = UIWindow::NewOpt( UIWindow::SIMPLE_LAYOUT, winStyle );

		mUIContainer->setId( "appContainer" )->setSize( mUISceneNode->getSize() );

		updateRecentProjects();
		updateRecentFiles();

		resizeCb( mWindow );

		mUIMenuBar->addEventListener( Event::OnSizeChange,
									  [&]( const Event* ) { resizeCb( mWindow ); } );

		mWindow->pushResizeCallback( [&]( auto* window ) { resizeCb( window ); } );

		mUseDefaultTheme = useAppTheme;

		if ( !cssFile.empty() ) {
			loadStyleSheet( cssFile );
		} else if ( mUseDefaultTheme ) {
			setUserDefaultTheme();
		}

		if ( !xmlFile.empty() )
			loadLayoutFile( xmlFile );

		if ( !projectFile.empty() )
			loadProject( projectFile );

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
		if ( xmlFile.empty() && cssFile.empty() ) {
			mUseDefaultTheme = true;
			loadStyleSheet( "assets/layouts/test.css" );
			loadLayoutFile( "assets/layouts/test.xml" );
		}
#endif

		mWindow->runMainLoop( &appLoop );
	}
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
	args::Flag useAppTheme( parser, "use-app-theme",
							"Use the default application theme in the editor.",
							{ 'u', "use-app-theme" } );
	args::Flag preserveContainerSizeFlag( parser, "preserve-container-size",
										  "Using this flag the application will respect the window "
										  "size set in the project and won't scale the window.",
										  { 's', "preserve-container-size" } );

	try {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		parser.ParseCLI( argc, argv );
#else
		parser.ParseCLI( parseEmscriptenArgs( argc, argv ) );
#endif
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

	appInstance = eeNew( App, () );
	appInstance->init( pixelDensityConf.Get(), preserveContainerSizeFlag.Get(), useAppTheme.Get(),
					   cssFile.Get(), xmlFile.Get(), projectFile.Get() );
	eeSAFE_DELETE( appInstance );

	Engine::destroySingleton();

	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
