#include <eepp/ee.hpp>
#include <efsw/efsw.hpp>

EE::Window::Window * window = NULL;
UIMessageBox * MsgBox = NULL;
efsw::FileWatcher * fileWatcher = NULL;
UITheme * theme = NULL;
UIWidget * uiContainer = NULL;
UIWinMenu * uiWinMenu = NULL;
DirectoryPack * directoryPack = NULL;
UISceneNode * uiSceneNode = NULL;
std::string currentLayout;
bool updateLayout = false;
Clock waitClock;
efsw::WatchID watch = 0;

class UpdateListener : public efsw::FileWatchListener
{
public:
	UpdateListener() {}

	void handleFileAction( efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename = "" )
	{
		if ( action == efsw::Actions::Modified ) {
			std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Modified" << std::endl;

			if ( dir + filename == currentLayout )
			{
				updateLayout = true;
				waitClock.restart();
			}
		}
	}
};

UpdateListener * listener = NULL;

static bool isImage( const std::string& path ) {
	std::string mPath = path;

	if ( path.size() >= 4 )
	{
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

	if ( path.size() >= 4 )
	{
		std::string File = mPath.substr( mPath.find_last_of("/\\") + 1 );
		std::string Ext = File.substr( File.find_last_of(".") + 1 );
		String::toLowerInPlace( Ext );

		if ( Ext == "ttf" || Ext == "otf" || Ext == "wolff" )
			return true;
	}

	return false;
}

static void loadImage( std::string path )
{
	std::string filename( FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( path ) ) );

	GlobalTextureAtlas::instance()->add( TextureFactory::instance()->loadFromFile( path ), filename );
}

static void loadFont( std::string path )
{
	std::string filename( FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( path ) ) );
	FontTrueType * font = FontTrueType::New( filename );

	font->loadFromFile( path );
}

static void loadImagesFromFolder( std::string folderPath )
{
	std::vector<std::string> files = FileSystem::filesGetInPath( folderPath );

	FileSystem::dirPathAddSlashAtEnd( folderPath );

	for ( auto it = files.begin(); it != files.end(); ++it )
	{
		if ( isImage( *it ) )
		{
			loadImage( folderPath + (*it) );
		}
	}
}

static void loadFontsFromFolder( std::string folderPath )
{
	std::vector<std::string> files = FileSystem::filesGetInPath( folderPath );

	FileSystem::dirPathAddSlashAtEnd( folderPath );

	for ( auto it = files.begin(); it != files.end(); ++it )
	{
		if ( isFont( *it ) )
		{
			loadFont( folderPath + (*it) );
		}
	}
}

static void loadLayoutFromFile( std::string file )
{
	if ( watch != 0 )
	{
		fileWatcher->removeWatch( watch );
	}

	std::string folder( FileSystem::fileRemoveFileName( file ) );

	watch = fileWatcher->addWatch( folder, listener );

	Float pd = PixelDensity::getPixelDensity();
	uiContainer->childsCloseAll();
	PixelDensity::setPixelDensity(1);
	uiSceneNode->loadLayoutFromFile( file, uiContainer );
	PixelDensity::setPixelDensity(pd);

	currentLayout = file;
}

static void refreshLayout() {
	if ( !currentLayout.empty() && FileSystem::fileExists( currentLayout ) && uiContainer != NULL ) {
		loadLayoutFromFile( currentLayout );
	}

	updateLayout = false;
}

bool onCloseRequestCallback( EE::Window::Window * w ) {
	MsgBox = UIMessageBox::New( MSGBOX_OKCANCEL, "Do you really want to close the current file?\nAll changes will be lost." );
	MsgBox->addEventListener( Event::MsgBoxConfirmClick, cb::Make1<void, const Event*>( []( const Event * event ) { window->close(); } ) );
	MsgBox->addEventListener( Event::OnClose, cb::Make1<void, const Event*>( []( const Event * event ) { MsgBox = NULL; } ) );
	MsgBox->setTitle( "Close Editor?" );
	MsgBox->center();
	MsgBox->show();
	return false;
}

void mainLoop() {
	window->getInput()->update();

	if ( window->getInput()->isKeyUp( KEY_ESCAPE ) && NULL == MsgBox && onCloseRequestCallback( window ) ) {
		window->close();
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

void resizeCb(EE::Window::Window * window)
{
	Float scaleW = (Float)window->getSize().getWidth() / (Float)uiContainer->getSize().getWidth();
	Float scaleH = (Float)window->getSize().getHeight() / (Float)uiContainer->getSize().getHeight();

	uiContainer->setScale( scaleW < scaleH ? scaleW : scaleH );
	uiContainer->center();
}

void imagePathOpen( const Event * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->getNode() );

	loadImagesFromFolder( CDL->getFullPath() );
}

void fontPathOpen( const Event * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->getNode() );

	loadFontsFromFolder( CDL->getFullPath() );
}

void layoutOpen( const Event * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->getNode() );

	Float pd = PixelDensity::getPixelDensity();
	PixelDensity::setPixelDensity(1);
	uiSceneNode->loadLayoutFromFile( CDL->getFullPath(), uiContainer );
	PixelDensity::setPixelDensity(pd);
}

void fileMenuClick( const Event * Event ) {
	if ( !Event->getNode()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->getNode() )->getText();

	if ( "Open..." == txt ) {
		UICommonDialog * TGDialog = UICommonDialog::New( UI_CDL_DEFAULT_FLAGS, "*.xml" );
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
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Open images from folder..." );
		TGDialog->addEventListener( Event::OpenFile, cb::Make1( imagePathOpen ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Load fonts from path..." == txt ) {
		UICommonDialog * TGDialog = UICommonDialog::New( UI_CDL_DEFAULT_FLAGS | CDL_FLAG_ALLOW_FOLDER_SELECT );
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Open fonts from folder..." );
		TGDialog->addEventListener( Event::OpenFile, cb::Make1( fontPathOpen ) );
		TGDialog->center();
		TGDialog->show();
	}
}

EE_MAIN_FUNC int main (int argc, char * argv []) {
	directoryPack = eeNew( DirectoryPack, () );
	directoryPack->create( Sys::getProcessPath() );

	fileWatcher = new efsw::FileWatcher();
	listener = new UpdateListener();
	fileWatcher->watch();

	Display * currentDisplay = Engine::instance()->getDisplayManager()->getDisplayIndex(0);
	Float pixelDensity = 1;
	DisplayMode currentMode = currentDisplay->getCurrentMode();

	Uint32 width = eemin( currentMode.Width, (Uint32)( 1280 * pixelDensity ) );
	Uint32 height = eemin( currentMode.Height, (Uint32)( 720 * pixelDensity ) );

	window = Engine::instance()->createWindow( WindowSettings( width, height, "eepp - UI Editor", WindowStyle::Default, WindowBackend::Default, 32, "assets/icon/ee.png" ), ContextSettings( true, GLv_default, true, 24, 1, 0, false ) );

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
		uiContainer->setId( "appContainer" )->setSize( 1200, 1920 );
		uiContainer->clipDisable();

		uiWinMenu = UIWinMenu::New();

		UIPopUpMenu * uiPopMenu = UIPopUpMenu::New();
		uiPopMenu->add( "Open...", theme->getIconByName( "document-open" ) );
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

	eeSAFE_DELETE( directoryPack );

	Engine::destroySingleton();

	MemoryManager::showResults();

	delete fileWatcher;

	delete listener;

	return EXIT_SUCCESS;
}
