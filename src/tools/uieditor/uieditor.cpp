#include <eepp/ee.hpp>
#include <efsw/efsw.hpp>
#include <pugixml/pugixml.hpp>

EE::Window::Window * window = NULL;
UIMessageBox * MsgBox = NULL;
efsw::FileWatcher * fileWatcher = NULL;
UITheme * theme = NULL;
UIWidget * uiContainer = NULL;
UIWinMenu * uiWinMenu = NULL;
UISceneNode * uiSceneNode = NULL;
std::string currentLayout;
bool updateLayout = false;
Clock waitClock;
efsw::WatchID watch = 0;
std::map<std::string, std::string> widgetRegistered;
std::string basePath;

Vector2i mousePos;
Clock mouseClock;

class UpdateListener : public efsw::FileWatchListener
{
public:
	UpdateListener() {}

	void handleFileAction( efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename = "" ) {
		if ( action == efsw::Actions::Modified ) {
			std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Modified" << std::endl;

			if ( dir + filename == currentLayout ) {
				updateLayout = true;
				waitClock.restart();
			}
		}
	}
};

UpdateListener * listener = NULL;

static bool isImage( const std::string& path ) {
	std::string mPath = path;

	if ( path.size() >= 4 ) {
		std::string File = mPath.substr( mPath.find_last_of("/\\") + 1 );
		std::string Ext = File.substr( File.find_last_of(".") + 1 );
		String::toLowerInPlace( Ext );

		if ( Ext == "png" || Ext == "tga" || Ext == "bmp" || Ext == "jpg" || Ext == "gif" || Ext == "jpeg" || Ext == "dds" || Ext == "psd" || Ext == "hdr" || Ext == "pic" || Ext == "pvr" || Ext == "pkm" )
			return true;
	}

	return false;
}

static bool isFont( const std::string& path ) {
	std::string mPath = path;

	if ( path.size() >= 4 ) {
		std::string File = mPath.substr( mPath.find_last_of("/\\") + 1 );
		std::string Ext = File.substr( File.find_last_of(".") + 1 );
		String::toLowerInPlace( Ext );

		if ( Ext == "ttf" || Ext == "otf" || Ext == "wolff" )
			return true;
	}

	return false;
}

static void loadImage( std::string path ) {
	std::string filename( FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( path ) ) );

	GlobalTextureAtlas::instance()->add( TextureFactory::instance()->loadFromFile( path ), filename );
}

static void loadFont( std::string path ) {
	std::string filename( FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( path ) ) );
	FontTrueType * font = FontTrueType::New( filename );

	font->loadFromFile( path );
}

static void loadImagesFromFolder( std::string folderPath ) {
	std::vector<std::string> files = FileSystem::filesGetInPath( folderPath );

	FileSystem::dirPathAddSlashAtEnd( folderPath );

	for ( auto it = files.begin(); it != files.end(); ++it ) {
		if ( isImage( *it ) ) {
			loadImage( folderPath + (*it) );
		}
	}
}

static void loadFontsFromFolder( std::string folderPath ) {
	std::vector<std::string> files = FileSystem::filesGetInPath( folderPath );

	FileSystem::dirPathAddSlashAtEnd( folderPath );

	for ( auto it = files.begin(); it != files.end(); ++it ) {
		if ( isFont( *it ) ) {
			loadFont( folderPath + (*it) );
		}
	}
}

static void loadLayoutFromFile( std::string file ) {
	if ( watch != 0 ) {
		fileWatcher->removeWatch( watch );
	}

	std::string folder( FileSystem::fileRemoveFileName( file ) );

	watch = fileWatcher->addWatch( folder, listener );

	uiContainer->childsCloseAll();

	uiSceneNode->loadLayoutFromFile( file, uiContainer );

	currentLayout = file;
}

static void refreshLayout() {
	if ( !currentLayout.empty() && FileSystem::fileExists( currentLayout ) && uiContainer != NULL ) {
		loadLayoutFromFile( currentLayout );
	}

	updateLayout = false;
}

void resizeCb(EE::Window::Window * window) {
	Float scaleW = (Float)uiSceneNode->getSize().getWidth() / (Float)uiContainer->getSize().getWidth();
	Float scaleH = (Float)uiSceneNode->getSize().getHeight() / (Float)uiContainer->getSize().getHeight();

	uiContainer->setScale( scaleW < scaleH ? scaleW : scaleH );
	uiContainer->center();
}

static UIWidget * createWidget( std::string widgetName ) {
	return UIWidgetCreator::createFromName( widgetRegistered[ widgetName ] );
}

static std::string pathFix( std::string path ) {
	if ( !path.empty() && path.at(0) != FileSystem::getOSSlash().at(0) ) {
		return basePath + path;
	}

	return path;
}

static void loadUITheme( std::string themePath ) {
	TextureAtlasLoader tgl( themePath );

	std::string name( FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( themePath ) ) );

	UITheme * theme = UITheme::loadFromTextureAtlas( UIThemeDefault::New( name, name ), TextureAtlasManager::instance()->getByName( name ) );

	UIThemeManager::instance()->setDefaultTheme( theme )->add( theme );
}

static void loadProjectNodes( pugi::xml_node node ) {
	for ( pugi::xml_node resources = node; resources; resources = resources.next_sibling() ) {
		std::string name = String::toLower( resources.name() );

		if ( name == "uiproject" ) {
			pugi::xml_node basePathNode = resources.child( "basepath" );

			if ( !basePathNode.empty() ) {
				basePath = basePathNode.text().as_string();
			}

			pugi::xml_node fontNode = resources.child( "font" );

			if ( !fontNode.empty() ) {
				for ( pugi::xml_node pathNode = fontNode.child("path"); pathNode; pathNode= pathNode.next_sibling("path") ) {
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
				for ( pugi::xml_node pathNode = drawableNode.child("path"); pathNode; pathNode= pathNode.next_sibling("path") ) {
					std::string drawablePath( pathFix( pathNode.text().as_string() ) );

					if ( FileSystem::isDirectory( drawablePath ) ) {
						loadImagesFromFolder( drawablePath );
					} else if ( isImage( drawablePath ) ) {
						loadFont( drawablePath );
					}
				}
			}

			pugi::xml_node widgetNode = resources.child( "widget" );

			if ( !widgetNode.empty() ) {
				for ( pugi::xml_node cwNode = widgetNode.child("customWidget"); cwNode; cwNode= cwNode.next_sibling("customWidget") ) {
					std::string name( cwNode.attribute( "name" ).as_string() );
					std::string replacement( cwNode.attribute( "replacement" ).as_string() );
					widgetRegistered[ String::toLower( name ) ] = replacement;
				}

				for ( auto it = widgetRegistered.begin(); it != widgetRegistered.end(); ++it ) {
					if ( !UIWidgetCreator::existsCustomWidgetCallback( it->first ) ) {
						UIWidgetCreator::addCustomWidgetCallback( it->first, cb::Make1( createWidget ) );
					}
				}

			}

			pugi::xml_node uiNode = resources.child( "uitheme" );

			if ( !uiNode.empty() ) {
				for ( pugi::xml_node uiThemeNode = uiNode; uiThemeNode; uiThemeNode = uiThemeNode.next_sibling("uitheme") ) {
					std::string uiThemePath( pathFix( uiThemeNode.text().as_string() ) );

					loadUITheme( uiThemePath );
				}
			}

			pugi::xml_node layoutNode = resources.child( "layout" );

			if ( !layoutNode.empty() ) {
				Float width = layoutNode.attribute( "width" ).as_float();
				Float height = layoutNode.attribute( "height" ).as_float();
				std::string layoutPath( pathFix( layoutNode.text().as_string() ) );

				if ( 0.f != width && 0.f != height ) {
					uiContainer->setSize( width, height );
					resizeCb( window );
				}

				if ( FileSystem::fileExists( layoutPath ) )
					loadLayoutFromFile( layoutPath );
			}
		}
	}
}

static void loadProject( std::string projectPath ) {
	if ( FileSystem::fileExists( projectPath ) ) {
		basePath = FileSystem::fileRemoveFileName( projectPath );

		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file( projectPath.c_str() );

		if ( result ) {
			return loadProjectNodes( doc.first_child() );
		} else {
			eePRINTL( "Error: Couldn't load UI Layout: %s", projectPath.c_str() );
			eePRINTL( "Error description: %s", result.description() );
			eePRINTL( "Error offset: %d", result.offset );
		}
	}
}

bool onCloseRequestCallback( EE::Window::Window * w ) {
	UITheme * prevTheme = UIThemeManager::instance()->getDefaultTheme();
	UIThemeManager::instance()->setDefaultTheme( theme );

	MsgBox = UIMessageBox::New( MSGBOX_OKCANCEL, "Do you really want to close the current file?\nAll changes will be lost." );
	MsgBox->setTheme( theme );
	MsgBox->addEventListener( Event::MsgBoxConfirmClick, cb::Make1<void, const Event*>( []( const Event * event ) { window->close(); } ) );
	MsgBox->addEventListener( Event::OnClose, cb::Make1<void, const Event*>( []( const Event * event ) { MsgBox = NULL; } ) );
	MsgBox->setTitle( "Close Editor?" );
	MsgBox->center();
	MsgBox->show();

	UIThemeManager::instance()->setDefaultTheme( prevTheme );

	return false;
}

void mainLoop() {
	window->getInput()->update();

	if ( window->getInput()->isKeyUp( KEY_ESCAPE ) && NULL == MsgBox && onCloseRequestCallback( window ) ) {
		window->close();
	}

	if ( NULL != uiContainer && window->getInput()->isKeyUp( KEY_F1 ) ) {
		Sizef size( uiContainer->getSize() );
		DisplayMode displayMode = Engine::instance()->getDisplayManager()->getDisplayIndex(0)->getCurrentMode();

		Float scaleW = size.getWidth() > displayMode.Width ? displayMode.Width / size.getWidth() : 1.f;
		Float scaleH = size.getHeight() > displayMode.Height * 0.9f ? displayMode.Height * 0.9f / size.getHeight() : 1.f;
		Float scale = scaleW < scaleH ? scaleW : scaleH;

		window->setSize( (Uint32)( uiContainer->getSize().getWidth() * scale ), (Uint32)( uiContainer->getSize().getHeight() * scale ) );
		window->centerToDisplay();
	}

	if ( mousePos != window->getInput()->getMousePos() ) {
		mousePos = window->getInput()->getMousePos();
		mouseClock.restart();

		if ( uiWinMenu->getAlpha() != 255 && uiWinMenu->getActionManager()->isEmpty() ) {
			uiWinMenu->runAction( Actions::Fade::New( uiWinMenu->getAlpha(), 255, Milliseconds(250) ) );
		}
	} else if ( mouseClock.getElapsedTime() > Seconds(1) ) {
		if ( uiWinMenu->getAlpha() == 255 && uiWinMenu->getActionManager()->isEmpty() ) {
			uiWinMenu->runAction( Actions::Fade::New( 255, 0, Milliseconds(250) ) );
		}
	}

	if ( updateLayout && waitClock.getElapsedTime().asMilliseconds() > 250.f ) {
		refreshLayout();
	}

	SceneManager::instance()->update();

	if ( SceneManager::instance()->getUISceneNode()->invalidated() ) {
		window->clear();

		SceneManager::instance()->draw();

		window->display();
	} else {
		Sys::sleep( Milliseconds(8) );
	}
}

void imagePathOpen( const Event * event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( event->getNode() );

	loadImagesFromFolder( CDL->getFullPath() );
}

void fontPathOpen( const Event * event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( event->getNode() );

	loadFontsFromFolder( CDL->getFullPath() );
}

void layoutOpen( const Event * event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( event->getNode() );

	uiSceneNode->loadLayoutFromFile( CDL->getFullPath(), uiContainer );
}

void projectOpen( const Event * event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( event->getNode() );

	loadProject( CDL->getFullPath() );
}

void fileMenuClick( const Event * Event ) {
	if ( !Event->getNode()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->getNode() )->getText();

	UITheme * prevTheme = UIThemeManager::instance()->getDefaultTheme();
	UIThemeManager::instance()->setDefaultTheme( theme );

	if ( "Open project..." == txt ) {
		UICommonDialog * TGDialog = UICommonDialog::New( UI_CDL_DEFAULT_FLAGS, "*.xml" );
		TGDialog->setTheme( theme );
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Open layout..." );
		TGDialog->addEventListener( Event::OpenFile, cb::Make1( projectOpen ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Open layout..." == txt ) {
		UICommonDialog * TGDialog = UICommonDialog::New( UI_CDL_DEFAULT_FLAGS, "*.xml" );
		TGDialog->setTheme( theme );
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Open layout..." );
		TGDialog->addEventListener( Event::OpenFile, cb::Make1( layoutOpen ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Close" == txt ) {
		currentLayout = "";
		uiContainer->childsCloseAll();
	} else if ( "Quit" == txt ) {
		onCloseRequestCallback( window );
	} else if ( "Load images from path..." == txt ) {
		UICommonDialog * TGDialog = UICommonDialog::New( UI_CDL_DEFAULT_FLAGS | CDL_FLAG_ALLOW_FOLDER_SELECT );
		TGDialog->setTheme( theme );
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Open images from folder..." );
		TGDialog->addEventListener( Event::OpenFile, cb::Make1( imagePathOpen ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Load fonts from path..." == txt ) {
		UICommonDialog * TGDialog = UICommonDialog::New( UI_CDL_DEFAULT_FLAGS | CDL_FLAG_ALLOW_FOLDER_SELECT );
		TGDialog->setTheme( theme );
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Open fonts from folder..." );
		TGDialog->addEventListener( Event::OpenFile, cb::Make1( fontPathOpen ) );
		TGDialog->center();
		TGDialog->show();
	}

	UIThemeManager::instance()->setDefaultTheme( prevTheme );
}

EE_MAIN_FUNC int main (int argc, char * argv []) {
	fileWatcher = new efsw::FileWatcher();
	listener = new UpdateListener();
	fileWatcher->watch();

	Display * currentDisplay = Engine::instance()->getDisplayManager()->getDisplayIndex(0);
	Float pixelDensity = PixelDensity::toFloat( currentDisplay->getPixelDensity() );
	DisplayMode currentMode = currentDisplay->getCurrentMode();

	Uint32 width = eemin( currentMode.Width, (Uint32)( 1280 * pixelDensity ) );
	Uint32 height = eemin( currentMode.Height, (Uint32)( 720 * pixelDensity ) );

	window = Engine::instance()->createWindow( WindowSettings( width, height, "eepp - UI Editor", WindowStyle::Default, WindowBackend::Default, 32, "assets/icon/ee.png", pixelDensity ), ContextSettings( true, GLv_default, true, 24, 1, 0, false ) );

	if ( window->isOpen() ) {
		window->setCloseRequestCallback( cb::Make1( onCloseRequestCallback ) );

		uiSceneNode = UISceneNode::New();
		SceneManager::instance()->add( uiSceneNode );

		uiSceneNode->enableDrawInvalidation();

		{
			std::string pd;
			if ( PixelDensity::getPixelDensity() >= 1.5f ) pd = "1.5x";
			else if ( PixelDensity::getPixelDensity() >= 2.f ) pd = "2x";

			TextureAtlasLoader tgl( "assets/ui/uitheme" + pd + ".eta" );

			theme = UITheme::loadFromTextureAtlas( UIThemeDefault::New( "uitheme" + pd, "uitheme" + pd ), TextureAtlasManager::instance()->getByName( "uitheme" + pd ) );

			FontTrueType * font = FontTrueType::New( "NotoSans-Regular", "assets/fonts/NotoSans-Regular.ttf" );

			UIThemeManager::instance()->setDefaultEffectsEnabled( true )->setDefaultTheme( theme )->setDefaultFont( font )->add( theme );
		}

		uiContainer = UIWidget::New();
		uiContainer->setId( "appContainer" )->setSize( uiSceneNode->getSize() );
		uiContainer->clipDisable();

		uiWinMenu = UIWinMenu::New();

		UIPopUpMenu * uiPopMenu = UIPopUpMenu::New();
		uiPopMenu->add( "Open project...", theme->getIconByName( "document-open" ) );
		uiPopMenu->addSeparator();
		uiPopMenu->add( "Open layout...", theme->getIconByName( "document-open" ) );
		uiPopMenu->addSeparator();
		uiPopMenu->add( "Close", theme->getIconByName( "document-close" ) );
		uiPopMenu->addSeparator();
		uiPopMenu->add( "Quit", theme->getIconByName( "quit" ) );
		uiWinMenu->addMenuButton( "File", uiPopMenu );
		uiPopMenu->addEventListener( Event::OnItemClicked, cb::Make1( fileMenuClick ) );

		UIPopUpMenu * uiResourceMenu = UIPopUpMenu::New();
		uiResourceMenu->add( "Load images from path...", theme->getIconByName( "document-open" ) );
		uiResourceMenu->addSeparator();
		uiResourceMenu->add( "Load fonts from path...", theme->getIconByName( "document-open" ) );
		uiWinMenu->addMenuButton( "Resources", uiResourceMenu );
		uiResourceMenu->addEventListener( Event::OnItemClicked, cb::Make1( fileMenuClick ) );

		resizeCb( window );

		window->pushResizeCallback( cb::Make1( resizeCb ) );

		window->runMainLoop( &mainLoop );
	}

	Engine::destroySingleton();

	MemoryManager::showResults();

	delete fileWatcher;

	delete listener;

	return EXIT_SUCCESS;
}
