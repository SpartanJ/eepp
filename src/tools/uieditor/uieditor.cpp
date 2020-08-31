#include <args/args.hxx>
#include <eepp/ee.hpp>
#include <efsw/efsw.hpp>
#include <pugixml/pugixml.hpp>

/**
This is a real time visual editor for the UI module.
The layout files can be edited with any editor, and the layout changes can be seen live with this
editor. So this is a layout preview app. The layout is updated every time the layout file is
modified by the user. So you'll need to save the file in your editor to see the changes. This was
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

basepath is optional, otherwise it will take the base path from the xml file itself ( it xml is in
/home/project.xml, basepath it going to be /home )

Layout width and height are the default/target window size for the project.

"path"s could be explicit files or directories.

customWidget are defined in the case you use special widgets in your application, you'll need to
indicate a valid replacement to be able to edit the file.
*/

EE::Window::Window* window = NULL;
UIMessageBox* MsgBox = NULL;
efsw::FileWatcher* fileWatcher = NULL;
UITheme* theme = NULL;
UIWindow* uiContainer = NULL;
UIMenuBar* uiMenuBar = NULL;
UISceneNode* uiSceneNode = NULL;
UISceneNode* appUiSceneNode = NULL;
std::string currentLayout;
std::string currentStyleSheet;
bool layoutExpanded = true;
bool updateLayout = false;
bool updateStyleSheet = false;
bool useDefaultTheme = false;
bool preserveContainerSize = false;
Clock waitClock;
Clock cssWaitClock;
efsw::WatchID watch = 0;
efsw::WatchID styleSheetWatch = 0;
std::map<std::string, std::string> widgetRegistered;
std::string basePath;

Vector2i mousePos;
Clock mouseClock;
Console* console = NULL;

std::map<std::string, std::string> layouts;
std::vector<std::string> recentProjects;
std::vector<std::string> recentFiles;
IniFile ini;
Uint32 recentProjectEventClickId = 0xFFFFFFFF;
Uint32 recentFilesEventClickId = 0xFFFFFFFF;

std::map<Uint32, TextureRegion*> imagesLoaded;
std::map<Font*, std::string> fontsLoaded;

void closeProject();
void updateRecentProjects();
void updateRecentFiles();
void loadProject( std::string projectPath );
void loadLayoutFile( std::string layoutPath );

void loadConfig() {
	std::string path( Sys::getConfigPath( "eepp-uieditor" ) );
	if ( !FileSystem::fileExists( path ) )
		FileSystem::makeDir( path );
	FileSystem::dirAddSlashAtEnd( path );
	path += "config.ini";
	ini.loadFromFile( path );
	std::string recent = ini.getValue( "UIEDITOR", "recentprojects", "" );
	recentProjects = String::split( recent, ';' );
	recent = ini.getValue( "UIEDITOR", "recentfiles", "" );
	recentFiles = String::split( recent, ';' );
}

void saveConfig() {
	ini.setValue( "UIEDITOR", "recentprojects", String::join( recentProjects, ';' ) );
	ini.setValue( "UIEDITOR", "recentfiles", String::join( recentFiles, ';' ) );
	ini.writeFile();
}

class UpdateListener : public efsw::FileWatchListener {
  public:
	UpdateListener() {}

	virtual ~UpdateListener() {}

	void handleFileAction( efsw::WatchID, const std::string& dir, const std::string& filename,
						   efsw::Action action, std::string ) {
		if ( action == efsw::Actions::Modified ) {
			if ( dir + filename == currentLayout ) {
				updateLayout = true;
				waitClock.restart();
			} else if ( dir + filename == currentStyleSheet ) {
				updateStyleSheet = true;
				cssWaitClock.restart();
			}
		}
	}
};

UpdateListener* listener = NULL;

void unloadImages() {
	for ( auto it = imagesLoaded.begin(); it != imagesLoaded.end(); ++it ) {
		GlobalTextureAtlas::instance()->remove( it->second );
		TextureFactory::instance()->remove( it->first );
	}

	imagesLoaded.clear();
}

void unloadFonts() {
	for ( auto it = fontsLoaded.begin(); it != fontsLoaded.end(); ++it ) {
		FontManager::instance()->remove( it->first );
	}

	fontsLoaded.clear();
}

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

static void loadImage( std::string path ) {
	std::string filename( FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( path ) ) );

	Uint32 texId = TextureFactory::instance()->loadFromFile( path );

	TextureRegion* texRegion = GlobalTextureAtlas::instance()->add( texId, filename );

	imagesLoaded[texId] = texRegion;
}

static void loadFont( std::string path ) {
	std::string filename( FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( path ) ) );
	FontTrueType* font = FontTrueType::New( filename );

	font->loadFromFile( path );

	fontsLoaded[font] = filename;
}

static void loadImagesFromFolder( std::string folderPath ) {
	std::vector<std::string> files = FileSystem::filesGetInPath( folderPath );

	FileSystem::dirAddSlashAtEnd( folderPath );

	for ( auto it = files.begin(); it != files.end(); ++it ) {
		if ( Image::isImageExtension( *it ) ) {
			loadImage( folderPath + ( *it ) );
		}
	}
}

static void loadFontsFromFolder( std::string folderPath ) {
	std::vector<std::string> files = FileSystem::filesGetInPath( folderPath );

	FileSystem::dirAddSlashAtEnd( folderPath );

	for ( auto it = files.begin(); it != files.end(); ++it ) {
		if ( isFont( *it ) ) {
			loadFont( folderPath + ( *it ) );
		}
	}
}

static void loadLayoutsFromFolder( std::string folderPath ) {
	std::vector<std::string> files = FileSystem::filesGetInPath( folderPath );

	FileSystem::dirAddSlashAtEnd( folderPath );

	for ( auto it = files.begin(); it != files.end(); ++it ) {
		if ( isXML( *it ) ) {
			layouts[FileSystem::fileRemoveExtension( ( *it ) )] = ( folderPath + ( *it ) );
		}
	}
}

static void loadStyleSheet( std::string cssPath ) {
	CSS::StyleSheetParser parser;

	if ( NULL != uiSceneNode && parser.loadFromFile( cssPath ) ) {
		uiSceneNode->setStyleSheet( parser.getStyleSheet() );

		currentStyleSheet = cssPath;

		std::string folder( FileSystem::fileRemoveFileName( cssPath ) );

		bool keepWatch = false;

		for ( auto& directory : fileWatcher->directories() ) {
			if ( directory == folder ) {
				keepWatch = true;
			}
		}

		if ( !keepWatch ) {
			if ( styleSheetWatch != 0 ) {
				fileWatcher->removeWatch( styleSheetWatch );
			}

			styleSheetWatch = fileWatcher->addWatch( folder, listener );
		}
	}
}

static void loadLayout( std::string file ) {
	std::string folder( FileSystem::fileRemoveFileName( file ) );

	bool keepWatch = false;

	for ( auto& directory : fileWatcher->directories() ) {
		if ( directory == folder ) {
			keepWatch = true;
		}
	}

	if ( !keepWatch ) {
		if ( watch != 0 ) {
			fileWatcher->removeWatch( watch );
		}

		watch = fileWatcher->addWatch( folder, listener );
	}

	uiContainer->getContainer()->childsCloseAll();
	uiSceneNode->update( Time::Zero );

	uiSceneNode->loadLayoutFromFile( file, uiContainer );

	currentLayout = file;
}

static void refreshLayout() {
	if ( !currentLayout.empty() && FileSystem::fileExists( currentLayout ) &&
		 uiContainer != NULL ) {
		if ( !currentStyleSheet.empty() && FileSystem::fileExists( currentStyleSheet ) &&
			 uiContainer != NULL ) {
			loadStyleSheet( currentStyleSheet );
		}

		loadLayout( currentLayout );
	}

	updateLayout = false;
}

static void refreshStyleSheet() {
	if ( !currentStyleSheet.empty() && FileSystem::fileExists( currentStyleSheet ) &&
		 uiContainer != NULL ) {
		loadStyleSheet( currentStyleSheet );

		if ( !currentLayout.empty() && FileSystem::fileExists( currentLayout ) &&
			 uiContainer != NULL ) {
			loadLayout( currentLayout );
		}
	}

	updateStyleSheet = false;
}

void onRecentProjectClick( const Event* event ) {
	if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = event->getNode()->asType<UIMenuItem>()->getText();
	std::string path( txt.toUtf8() );

	if ( FileSystem::fileExists( path ) && !FileSystem::isDirectory( path ) ) {
		loadProject( path );
	}
}

void onRecentFilesClick( const Event* event ) {
	if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = event->getNode()->asType<UIMenuItem>()->getText();
	std::string path( txt.toUtf8() );

	if ( FileSystem::fileExists( path ) && !FileSystem::isDirectory( path ) ) {
		loadLayoutFile( path );
	}
}

void updateRecentFiles() {
	SceneManager::instance()->setCurrentUISceneNode( appUiSceneNode );

	if ( NULL == uiMenuBar )
		return;

	UIPopUpMenu* fileMenu = uiMenuBar->getPopUpMenu( "File" );

	UINode* node = NULL;

	if ( NULL != fileMenu && ( node = fileMenu->getItem( "Recent files" ) ) ) {
		UIMenuSubMenu* uiMenuSubMenu = static_cast<UIMenuSubMenu*>( node );
		UIMenu* menu = uiMenuSubMenu->getSubMenu();

		menu->removeAll();

		for ( size_t i = 0; i < recentFiles.size(); i++ ) {
			menu->add( recentFiles[i] );
		}

		if ( 0xFFFFFFFF != recentFilesEventClickId ) {
			menu->removeEventListener( recentFilesEventClickId );
		}

		recentFilesEventClickId =
			menu->addEventListener( Event::OnItemClicked, cb::Make1( &onRecentFilesClick ) );
	}

	SceneManager::instance()->setCurrentUISceneNode( uiSceneNode );
}

void updateRecentProjects() {
	SceneManager::instance()->setCurrentUISceneNode( appUiSceneNode );

	if ( NULL == uiMenuBar )
		return;

	UIPopUpMenu* fileMenu = uiMenuBar->getPopUpMenu( "File" );

	UINode* node = NULL;

	if ( NULL != fileMenu && ( node = fileMenu->getItem( "Recent projects" ) ) ) {
		UIMenuSubMenu* uiMenuSubMenu = static_cast<UIMenuSubMenu*>( node );
		UIMenu* menu = uiMenuSubMenu->getSubMenu();

		menu->removeAll();

		for ( size_t i = 0; i < recentProjects.size(); i++ ) {
			menu->add( recentProjects[i] );
		}

		if ( 0xFFFFFFFF != recentProjectEventClickId ) {
			menu->removeEventListener( recentProjectEventClickId );
		}

		recentProjectEventClickId =
			menu->addEventListener( Event::OnItemClicked, cb::Make1( &onRecentProjectClick ) );
	}

	SceneManager::instance()->setCurrentUISceneNode( uiSceneNode );
}

void resizeCb( EE::Window::Window* window ) {
	if ( layoutExpanded ) {
		uiContainer->setSize( uiSceneNode->getSize() );
	} else if ( !preserveContainerSize ) {
		Float scaleW =
			(Float)uiSceneNode->getSize().getWidth() / (Float)uiContainer->getSize().getWidth();
		Float scaleH =
			(Float)uiSceneNode->getSize().getHeight() / (Float)uiContainer->getSize().getHeight();

		uiContainer->setScale( scaleW < scaleH ? scaleW : scaleH );
		uiContainer->center();
	} else {
		window->setSize( uiContainer->getPixelsSize().getWidth(),
						 uiContainer->getPixelsSize().getHeight() );
	}
}

void resizeWindowToLayout() {
	if ( layoutExpanded )
		return;

	Sizef size( uiContainer->getSize() );
	Rect borderSize( window->getBorderSize() );
	Sizei displayMode = Engine::instance()
							->getDisplayManager()
							->getDisplayIndex( window->getCurrentDisplayIndex() )
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

	window->setSize( ( Uint32 )( size.getWidth() * scale ),
					 ( Uint32 )( size.getHeight() * scale ) );
	window->centerToDisplay();
}

static UIWidget* createWidget( std::string widgetName ) {
	return UIWidgetCreator::createFromName( widgetRegistered[widgetName] );
}

static std::string pathFix( std::string path ) {
	if ( !path.empty() && ( path.at( 0 ) != '/' || !( Sys::getPlatform() == "Windows" &&
													  path.size() > 3 && path.at( 1 ) != ':' ) ) ) {
		return basePath + path;
	}

	return path;
}

static void loadUITheme( std::string themePath ) {
	TextureAtlasLoader tgl( themePath );

	std::string name(
		FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( themePath ) ) );

	UITheme* uitheme = UITheme::loadFromTextureAtlas(
		UITheme::New( name, name ), TextureAtlasManager::instance()->getByName( name ) );

	uiSceneNode->getUIThemeManager()->setDefaultTheme( uitheme )->add( uitheme );
}

void onLayoutSelected( const Event* event ) {
	if ( !event->getNode()->isType( UI_TYPE_MENUCHECKBOX ) )
		return;

	const String& txt = event->getNode()->asType<UIMenuItem>()->getText();

	UIPopUpMenu* uiLayoutsMenu;

	if ( ( uiLayoutsMenu = uiMenuBar->getPopUpMenu( "Layouts" ) ) &&
		 uiLayoutsMenu->getCount() > 0 ) {
		for ( size_t i = 0; i < uiLayoutsMenu->getCount(); i++ ) {
			UIMenuCheckBox* menuItem = static_cast<UIMenuCheckBox*>( uiLayoutsMenu->getItem( i ) );
			menuItem->setActive( false );
		}
	}

	UIMenuCheckBox* chk = static_cast<UIMenuCheckBox*>( event->getNode() );
	chk->setActive( true );

	std::map<std::string, std::string>::iterator it;

	if ( ( it = layouts.find( txt.toUtf8() ) ) != layouts.end() ) {
		loadLayout( it->second );
	}
}

void refreshLayoutList() {
	if ( NULL == uiMenuBar )
		return;

	SceneManager::instance()->setCurrentUISceneNode( appUiSceneNode );

	if ( layouts.size() > 0 ) {
		UIPopUpMenu* uiLayoutsMenu = NULL;

		if ( uiMenuBar->getButton( "Layouts" ) == NULL ) {
			uiLayoutsMenu = UIPopUpMenu::New();

			uiMenuBar->addMenuButton( "Layouts", uiLayoutsMenu );

			uiLayoutsMenu->addEventListener( Event::OnItemClicked, cb::Make1( &onLayoutSelected ) );
		} else {
			uiLayoutsMenu = uiMenuBar->getPopUpMenu( "Layouts" );
		}

		uiLayoutsMenu->removeAll();

		for ( auto it = layouts.begin(); it != layouts.end(); ++it ) {
			uiLayoutsMenu->addCheckBox( it->first )->setActive( currentLayout == it->second );
		}
	} else if ( uiMenuBar->getButton( "Layouts" ) != NULL ) {
		uiMenuBar->removeMenuButton( "Layouts" );
	}

	SceneManager::instance()->setCurrentUISceneNode( uiSceneNode );
}

static void loadProjectNodes( pugi::xml_node node ) {
	uiSceneNode->getUIThemeManager()->setDefaultTheme( useDefaultTheme ? theme : NULL );

	for ( pugi::xml_node resources = node; resources; resources = resources.next_sibling() ) {
		std::string name = String::toLower( std::string( resources.name() ) );

		if ( name == "uiproject" ) {
			pugi::xml_node basePathNode = resources.child( "basepath" );

			if ( !basePathNode.empty() ) {
				basePath = basePathNode.text().as_string();
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
					widgetRegistered[String::toLower( wname )] = replacement;
				}

				for ( auto it = widgetRegistered.begin(); it != widgetRegistered.end(); ++it ) {
					if ( !UIWidgetCreator::existsCustomWidgetCallback( it->first ) ) {
						UIWidgetCreator::addCustomWidgetCallback( it->first,
																  cb::Make1( createWidget ) );
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
				std::string cssPath( styleSheetNode.attribute( "path" ).as_string() );

				if ( isCSS( cssPath ) && FileSystem::fileExists( basePath + cssPath ) ) {
					loadStyleSheet( basePath + cssPath );
				}
			}

			pugi::xml_node layoutNode = resources.child( "layout" );

			if ( !layoutNode.empty() ) {
				bool loaded = false;
				bool loadedSizedLayout = false;

				Float width = layoutNode.attribute( "width" ).as_float();
				Float height = layoutNode.attribute( "height" ).as_float();

				if ( 0.f != width && 0.f != height ) {
					layoutExpanded = false;
					uiContainer->setSize( width, height );
				} else {
					layoutExpanded = true;
				}

				resizeCb( window );

				layouts.clear();

				for ( pugi::xml_node layNode = layoutNode.child( "path" ); layNode;
					  layNode = layNode.next_sibling( "path" ) ) {
					std::string layoutPath( pathFix( layNode.text().as_string() ) );

					if ( FileSystem::isDirectory( layoutPath ) ) {
						loadLayoutsFromFolder( layoutPath );
					} else if ( FileSystem::fileExists( layoutPath ) && isXML( layoutPath ) ) {
						layouts[FileSystem::fileRemoveExtension(
							FileSystem::fileNameFromPath( layoutPath ) )] = layoutPath;

						if ( !loaded ) {
							loadLayout( layoutPath );
							loaded = true;
						}

						if ( width != 0 && height != 0 ) {
							loadedSizedLayout = true;
						}
					}
				}

				if ( layouts.size() > 0 && !loaded ) {
					loadLayout( layouts.begin()->second );

					if ( width != 0 && height != 0 ) {
						loadedSizedLayout = true;
					}
				}

				if ( loadedSizedLayout ) {
					resizeWindowToLayout();
				}
			}

			refreshLayoutList();
		}
	}
}

void loadLayoutFile( std::string layoutPath ) {
	if ( FileSystem::fileExists( layoutPath ) ) {
		loadLayout( layoutPath );

		for ( auto pathIt = recentFiles.begin(); pathIt != recentFiles.end(); pathIt++ ) {
			if ( *pathIt == layoutPath ) {
				recentFiles.erase( pathIt );
				break;
			}
		}

		recentFiles.insert( recentFiles.begin(), layoutPath );

		if ( recentFiles.size() > 10 ) {
			recentFiles.resize( 10 );
		}

		updateRecentFiles();
	}
}

void loadProject( std::string projectPath ) {
	if ( FileSystem::fileExists( projectPath ) ) {
		closeProject();

		basePath = FileSystem::fileRemoveFileName( projectPath );

		FileSystem::changeWorkingDirectory( basePath );

		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file( projectPath.c_str() );

		if ( result ) {
			loadProjectNodes( doc.first_child() );

			for ( auto pathIt = recentProjects.begin(); pathIt != recentProjects.end(); pathIt++ ) {
				if ( *pathIt == projectPath ) {
					recentProjects.erase( pathIt );
					break;
				}
			}

			recentProjects.insert( recentProjects.begin(), projectPath );

			if ( recentProjects.size() > 10 ) {
				recentProjects.resize( 10 );
			}

			updateRecentProjects();
		} else {
			Log::error( "Couldn't load UI Layout: %s", projectPath.c_str() );
			Log::error( "Error description: %s", result.description() );
			Log::error( "Error offset: %d", result.offset );
		}
	}
}

void closeProject() {
	currentLayout = "";
	currentStyleSheet = "";
	uiContainer->getContainer()->childsCloseAll();
	uiSceneNode->update( Time::Zero );
	uiSceneNode->setStyleSheet( CSS::StyleSheet() );

	layouts.clear();

	refreshLayoutList();

	unloadFonts();
	unloadImages();
}

bool onCloseRequestCallback( EE::Window::Window* ) {
	SceneManager::instance()->setCurrentUISceneNode( appUiSceneNode );

	MsgBox = UIMessageBox::New(
		UIMessageBox::OK_CANCEL,
		"Do you really want to close the current file?\nAll changes will be lost." );
	MsgBox->setTheme( theme );
	MsgBox->addEventListener( Event::MsgBoxConfirmClick, []( const Event* ) { window->close(); } );
	MsgBox->addEventListener( Event::OnClose, []( const Event* ) { MsgBox = NULL; } );
	MsgBox->setTitle( "Close Editor?" );
	MsgBox->center();
	MsgBox->show();

	SceneManager::instance()->setCurrentUISceneNode( uiSceneNode );
	return false;
}

void mainLoop() {
	window->getInput()->update();

	if ( window->getInput()->isKeyUp( KEY_ESCAPE ) && NULL == MsgBox &&
		 onCloseRequestCallback( window ) ) {
		window->close();
	}

	if ( window->getInput()->isKeyUp( KEY_F3 ) || window->getInput()->isKeyUp( KEY_BACKSLASH ) ) {
		console->toggle();
	}

	if ( NULL != uiContainer && window->getInput()->isKeyUp( KEY_F1 ) ) {
		resizeWindowToLayout();
	}

	if ( window->getInput()->isKeyUp( KEY_F6 ) ) {
		uiSceneNode->setHighlightFocus( !uiSceneNode->getHighlightFocus() );
		uiSceneNode->setHighlightOver( !uiSceneNode->getHighlightOver() );
	}

	if ( window->getInput()->isKeyUp( KEY_F7 ) ) {
		uiSceneNode->setDrawBoxes( !uiSceneNode->getDrawBoxes() );
	}

	if ( window->getInput()->isKeyUp( KEY_F8 ) ) {
		uiSceneNode->setDrawDebugData( !uiSceneNode->getDrawDebugData() );
	}

	if ( window->getInput()->isKeyUp( KEY_F9 ) ) {
		Clock clock;
		uiSceneNode->getRoot()->reportStyleStateChangeRecursive();
		Log::info( "Applied style state changes in: %.2fms",
				   clock.getElapsedTime().asMilliseconds() );
	}

	if ( mousePos != window->getInput()->getMousePos() ) {
		mousePos = window->getInput()->getMousePos();
		mouseClock.restart();

		if ( uiMenuBar->getAlpha() != 255 && uiMenuBar->getActionManager()->isEmpty() ) {
			uiMenuBar->runAction(
				Actions::Fade::New( uiMenuBar->getAlpha(), 255, Milliseconds( 250 ) ) );
		}
	} else if ( mouseClock.getElapsedTime() > Seconds( 1 ) ) {
		if ( uiMenuBar->getAlpha() == 255 && uiMenuBar->getActionManager()->isEmpty() ) {
			uiMenuBar->runAction( Actions::Fade::New( 255, 0, Milliseconds( 250 ) ) );
		}
	}

	if ( updateLayout && waitClock.getElapsedTime().asMilliseconds() > 250.f ) {
		refreshLayout();
	}

	if ( updateStyleSheet && cssWaitClock.getElapsedTime().asMilliseconds() > 250.f ) {
		refreshStyleSheet();
	}

	Time elapsed = SceneManager::instance()->getElapsed();
	SceneManager::instance()->update();

	if ( appUiSceneNode->invalidated() || uiSceneNode->invalidated() || console->isActive() ||
		 console->isFading() ) {
		window->clear();

		SceneManager::instance()->draw();

		console->draw( elapsed );

		window->display();
	} else {
		Sys::sleep( Milliseconds( 8 ) );
	}
}

void imagePathOpen( const Event* event ) {
	loadImagesFromFolder( event->getNode()->asType<UIFileDialog>()->getFullPath() );
}

void fontPathOpen( const Event* event ) {
	loadFontsFromFolder( event->getNode()->asType<UIFileDialog>()->getFullPath() );
}

void styleSheetPathOpen( const Event* event ) {
	loadStyleSheet( event->getNode()->asType<UIFileDialog>()->getFullPath() );
}

void layoutOpen( const Event* event ) {
	loadLayoutFile( event->getNode()->asType<UIFileDialog>()->getFullPath() );
}

void projectOpen( const Event* event ) {
	loadProject( event->getNode()->asType<UIFileDialog>()->getFullPath() );
}

void fileMenuClick( const Event* event ) {
	if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = event->getNode()->asType<UIMenuItem>()->getText();

	SceneManager::instance()->setCurrentUISceneNode( appUiSceneNode );

	if ( "Open project..." == txt ) {
		UIFileDialog* TGDialog = UIFileDialog::New( UIFileDialog::DefaultFlags, "*.xml" );
		TGDialog->setTheme( theme );
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Open layout..." );
		TGDialog->addEventListener( Event::OpenFile, cb::Make1( projectOpen ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Open layout..." == txt ) {
		UIFileDialog* TGDialog = UIFileDialog::New( UIFileDialog::DefaultFlags, "*.xml" );
		TGDialog->setTheme( theme );
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Open layout..." );
		TGDialog->addEventListener( Event::OpenFile, cb::Make1( layoutOpen ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Close" == txt ) {
		closeProject();
	} else if ( "Quit" == txt ) {
		onCloseRequestCallback( window );
	} else if ( "Load images from path..." == txt ) {
		UIFileDialog* TGDialog =
			UIFileDialog::New( UIFileDialog::DefaultFlags | UIFileDialog::AllowFolderSelect );
		TGDialog->setTheme( theme );
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Open images from folder..." );
		TGDialog->addEventListener( Event::OpenFile, cb::Make1( imagePathOpen ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Load fonts from path..." == txt ) {
		UIFileDialog* TGDialog =
			UIFileDialog::New( UIFileDialog::DefaultFlags | UIFileDialog::AllowFolderSelect );
		TGDialog->setTheme( theme );
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Open fonts from folder..." );
		TGDialog->addEventListener( Event::OpenFile, cb::Make1( fontPathOpen ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Load style sheet from path..." == txt ) {
		UIFileDialog* TGDialog = UIFileDialog::New( UIFileDialog::DefaultFlags, "*.css" );
		TGDialog->setTheme( theme );
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Open style sheet from path..." );
		TGDialog->addEventListener( Event::OpenFile, cb::Make1( styleSheetPathOpen ) );
		TGDialog->center();
		TGDialog->show();
	}

	SceneManager::instance()->setCurrentUISceneNode( uiSceneNode );
}

void createAppMenu() {
	SceneManager::instance()->setCurrentUISceneNode( appUiSceneNode );

	uiMenuBar = UIMenuBar::New();

	size_t iconSize = PixelDensity::dpToPxI( 16 );
	UIPopUpMenu* uiPopMenu = UIPopUpMenu::New();
	uiPopMenu->add( "Open project...",
					appUiSceneNode->findIconDrawable( "document-open", iconSize ) );
	uiPopMenu->addSeparator();
	uiPopMenu->add( "Open layout...",
					appUiSceneNode->findIconDrawable( "document-open", iconSize ) );
	uiPopMenu->addSeparator();
	uiPopMenu->addSubMenu( "Recent files", NULL, UIPopUpMenu::New() );
	uiPopMenu->addSubMenu( "Recent projects", NULL, UIPopUpMenu::New() );
	uiPopMenu->addSeparator();
	uiPopMenu->add( "Close", appUiSceneNode->findIconDrawable( "document-close", iconSize ) );
	uiPopMenu->addSeparator();
	uiPopMenu->add( "Quit", appUiSceneNode->findIconDrawable( "quit", iconSize ) );
	uiMenuBar->addMenuButton( "File", uiPopMenu );
	uiPopMenu->addEventListener( Event::OnItemClicked, cb::Make1( fileMenuClick ) );

	UIPopUpMenu* uiResourceMenu = UIPopUpMenu::New();
	uiResourceMenu->add( "Load images from path...",
						 appUiSceneNode->findIconDrawable( "document-open", iconSize ) );
	uiResourceMenu->addSeparator();
	uiResourceMenu->add( "Load fonts from path...",
						 appUiSceneNode->findIconDrawable( "document-open", iconSize ) );
	uiResourceMenu->addSeparator();
	uiResourceMenu->add( "Load style sheet from path...",
						 appUiSceneNode->findIconDrawable( "document-open", iconSize ) );
	uiMenuBar->addMenuButton( "Resources", uiResourceMenu );
	uiResourceMenu->addEventListener( Event::OnItemClicked, cb::Make1( fileMenuClick ) );

	SceneManager::instance()->setCurrentUISceneNode( uiSceneNode );
}

void setUserDefaultTheme() {
	useDefaultTheme = true;
	uiSceneNode->getUIThemeManager()->setDefaultTheme( theme );
	uiSceneNode->combineStyleSheet( theme->getStyleSheet() );
}

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
	args::ArgumentParser parser( "eepp UIEditor" );
	args::HelpFlag help( parser, "help", "Display this help menu", {'h', "help"} );
	args::ValueFlag<std::string> xmlFile( parser, "xml", "Loads XML file", {'x', "xml"} );
	args::ValueFlag<std::string> cssFile( parser, "css", "Loads CSS file", {'c', "css"} );
	args::ValueFlag<std::string> projectFile( parser, "project", "Loads project file",
											  {'p', "project"} );
	args::ValueFlag<Float> pixelDenstiyConf(
		parser, "pixel-density", "Set default application pixel density", {'d', "pixel-density"} );
	args::Flag useAppTheme( parser, "use-app-theme",
							"Use the default application theme in the editor.",
							{'u', "use-app-theme"} );
	args::Flag preserveContainerSizeFlag( parser, "preserve-container-size",
										  "Using this flag the application will respect the window "
										  "size set in the project and won't scale the window.",
										  {'s', "preserve-container-size"} );

	try {
		parser.ParseCLI( argc, argv );
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

	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	displayManager->enableScreenSaver();
	displayManager->enableMouseFocusClickThrough();
	displayManager->disableBypassCompositor();

	fileWatcher = new efsw::FileWatcher();
	listener = new UpdateListener();
	fileWatcher->watch();

	Display* currentDisplay = displayManager->getDisplayIndex( 0 );
	Float pixelDensity = currentDisplay->getPixelDensity();

	if ( pixelDenstiyConf ) {
		pixelDensity = pixelDenstiyConf.Get();
	}

	if ( preserveContainerSizeFlag ) {
		preserveContainerSize = true;
	}

	Log::instance()->setLiveWrite( true );
	Log::instance()->setConsoleOutput( true );

	std::string resPath( Sys::getProcessPath() );

	window = Engine::instance()->createWindow(
		WindowSettings( 1280, 720, "eepp - UI Editor", WindowStyle::Default, WindowBackend::Default,
						32, resPath + "assets/icon/ee.png", pixelDensity ),
		ContextSettings( true, GLv_default, true, 24, 1, 0, true ) );

	if ( window->isOpen() ) {
		PixelDensity::setPixelDensity( eemax( window->getScale(), pixelDensity ) );

		window->setCloseRequestCallback( cb::Make1( onCloseRequestCallback ) );

		std::string pd;
		if ( PixelDensity::getPixelDensity() >= 1.5f )
			pd = "1.5x";
		else if ( PixelDensity::getPixelDensity() >= 2.f )
			pd = "2x";

		FontTrueType* font =
			FontTrueType::New( "NotoSans-Regular", resPath + "assets/fonts/NotoSans-Regular.ttf" );
		FontTrueType* fontMono =
			FontTrueType::New( "monospace", resPath + "assets/fonts/DejaVuSansMono.ttf" );

		console = eeNew( Console, ( fontMono, true, true, 1024 * 1000, 0, window ) );

		theme = UITheme::load( "uitheme" + pd, "uitheme" + pd,
							   resPath + "assets/ui/uitheme" + pd + ".eta", font,
							   resPath + "assets/ui/uitheme.css" );
		/*theme = UITheme::load( "uitheme", "uitheme", "", font, resPath + "assets/ui/breeze.css"
		 * );*/

		uiSceneNode = UISceneNode::New();
		uiSceneNode->setId( "uiSceneNode" );
		uiSceneNode->setVerbose( true );
		SceneManager::instance()->add( uiSceneNode );

		appUiSceneNode = UISceneNode::New();
		appUiSceneNode->setId( "appUiSceneNode" );
		SceneManager::instance()->add( appUiSceneNode );

		appUiSceneNode->enableDrawInvalidation();
		uiSceneNode->enableDrawInvalidation();

		appUiSceneNode->setStyleSheet( theme->getStyleSheet() );
		appUiSceneNode->getUIThemeManager()
			->setDefaultEffectsEnabled( true )
			->setDefaultTheme( theme )
			->setDefaultFont( font )
			->add( theme );

		uiSceneNode->getUIThemeManager()->setDefaultFont( font )->setDefaultEffectsEnabled( true );

		if ( useAppTheme.Get() ) {
			setUserDefaultTheme();
		}

		loadConfig();

		createAppMenu();

		UIWindow::StyleConfig winStyle( UI_NODE_DEFAULT_FLAGS | UI_WIN_NO_DECORATION );
		uiContainer = UIWindow::NewOpt( UIWindow::SIMPLE_LAYOUT, winStyle );
		uiContainer->setId( "appContainer" )->setSize( uiSceneNode->getSize() );

		updateRecentProjects();
		updateRecentFiles();

		resizeCb( window );

		window->pushResizeCallback( cb::Make1( resizeCb ) );

		if ( cssFile ) {
			loadStyleSheet( cssFile.Get() );
		}

		if ( xmlFile ) {
			loadLayoutFile( xmlFile.Get() );
		}

		if ( projectFile ) {
			loadProject( projectFile.Get() );
		}

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
		if ( !xmlFile && !cssFile )
			loadStyleSheet( "assets/ui/breeze.css" );
		loadLayoutFile( "assets/layouts/test_widgets.xml" );
#endif

		window->runMainLoop( &mainLoop );
	}

	saveConfig();

	eeSAFE_DELETE( console );

	Engine::destroySingleton();

	MemoryManager::showResults();

	delete fileWatcher;

	delete listener;

	return EXIT_SUCCESS;
}
