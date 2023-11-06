#include "uieditor.hpp"
#include <args/args.hxx>
#define PUGIXML_HEADER_ONLY
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

void App::updateLayoutFunc( const InvalidationType& invalidator ) {
	mUpdateLayout = true;
	mWaitClock.restart();
	mInvalidationLayout = invalidator;
}

void App::updateStyleSheetFunc( const InvalidationType& invalidator ) {
	mUpdateStyleSheet = true;
	mCssWaitClock.restart();
	mInvalidationStyleSheet = invalidator;
}
void App::updateBaseStyleSheetFunc( const InvalidationType& invalidator ) {
	mUpdateBaseStyleSheet = true;
	mCssBaseWaitClock.restart();
	mInvalidationBaseStyleSheet = invalidator;
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
				mApp->updateLayoutFunc( InvalidationType::FileSystem );
			} else if ( dir + filename == mApp->getCurrentStyleSheet() ) {
				mApp->updateStyleSheetFunc( InvalidationType::FileSystem );
			} else if ( dir + filename == mApp->getBaseStyleSheet() ) {
				mApp->updateBaseStyleSheetFunc( InvalidationType::FileSystem );
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
	Texture* tex = TextureFactory::instance()->loadFromFile( path );
	if ( tex ) {
		Uint32 texId = tex->getTextureId();
		TextureRegion* texRegion = GlobalTextureAtlas::instance()->add( texId, filename );
		mImagesLoaded[texId] = texRegion;
	}
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

void App::createWidgetInspector() {
	SceneManager::instance()->setCurrentUISceneNode( mAppUISceneNode );
	UIWidgetInspector::create( mUISceneNode, mMenuIconSize );
	SceneManager::instance()->setCurrentUISceneNode( mUISceneNode );
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

void App::loadBaseStyleSheet() {
	if ( !mUseDefaultTheme )
		return;

	if ( mBaseStyleSheetWatch == 0 ) {
		std::string baseFolder( FileSystem::fileRemoveFileName( mBaseStyleSheet ) );
		mBaseStyleSheetWatch = mFileWatcher->addWatch( baseFolder, mListener );
	}

	setUserDefaultTheme();

	if ( !mSplitter->isDocumentOpen( mBaseStyleSheet ) && mUseDefaultTheme ) {
		SceneManager::instance()->setCurrentUISceneNode( mAppUISceneNode );
		mSplitter->loadFileFromPathInNewTab( mBaseStyleSheet );
		SceneManager::instance()->setCurrentUISceneNode( mUISceneNode );
	}
}

void App::loadStyleSheet( std::string cssPath, bool updateCurrentStyleSheet ) {
	if ( NULL == mUISceneNode )
		return;
	CSS::StyleSheetParser parser;

	loadBaseStyleSheet();

	if ( NULL != mUISceneNode && !cssPath.empty() && parser.loadFromFile( cssPath ) ) {
		if ( mUseDefaultTheme ) {
			mUISceneNode->combineStyleSheet( parser.getStyleSheet() );
		} else {
			mUISceneNode->setStyleSheet( parser.getStyleSheet() );
		}

		if ( updateCurrentStyleSheet ) {
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

			if ( !mSplitter->isDocumentOpen( cssPath ) ) {
				SceneManager::instance()->setCurrentUISceneNode( mAppUISceneNode );
				mSplitter->loadFileFromPathInNewTab( cssPath );
				SceneManager::instance()->setCurrentUISceneNode( mUISceneNode );
			}
		}
	}
}

void App::tryUpdateWatch( const std::string& file ) {
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
}

std::pair<UITab*, UICodeEditor*> App::loadLayout( std::string file, bool updateCurrentLayout ) {
	mUIContainer->getContainer()->childsCloseAll();
	mUISceneNode->update( Time::Zero );

	Uint32 marker = String::hash( updateCurrentLayout ? file : mCurrentLayout );

	mUISceneNode->getStyleSheet().removeAllWithMarker( marker );
	mUISceneNode->loadLayoutFromFile( file, mUIContainer, marker );

	if ( updateCurrentLayout ) {
		tryUpdateWatch( file );

		mCurrentLayout = file;

		if ( !mSplitter->isDocumentOpen( file ) ) {
			SceneManager::instance()->setCurrentUISceneNode( mAppUISceneNode );
			auto d = mSplitter->loadFileFromPathInNewTab( file );
			SceneManager::instance()->setCurrentUISceneNode( mUISceneNode );
			return d;
		}
	}

	return std::make_pair( nullptr, nullptr );
}

void App::saveTmpDocument( TextDocument& doc,
						   std::function<void( const std::string& tmpPath )> action ) {
	std::string tmpPath = Sys::getTempPath() + doc.getFilename();
	if ( FileSystem::fileExists( tmpPath ) ) {
		tmpPath = Sys::getTempPath() + ".eepp-uieditor-" + doc.getFilename() + "." +
				  String::randString( 8 );
	}
	IOStreamString fileString;
	doc.save( fileString, true );
	FileSystem::fileWrite( tmpPath, (Uint8*)fileString.getStreamPointer(), fileString.getSize() );
	FileSystem::fileHide( tmpPath );
	action( tmpPath );
	FileSystem::fileRemove( tmpPath );
}

void App::reloadStyleSheet() {
	switch ( mInvalidationStyleSheet ) {
		case InvalidationType::Memory: {
			UICodeEditor* editor = mSplitter->findEditorFromPath( mCurrentStyleSheet );
			if ( !editor )
				return;
			saveTmpDocument( editor->getDocument(), [this]( const std::string& tmpPath ) {
				loadStyleSheet( tmpPath, false );
			} );
			break;
		}
		case InvalidationType::FileSystem:
		default:
			loadStyleSheet( mCurrentStyleSheet );
			break;
	}
}

void App::reloadBaseStyleSheet() {
	switch ( mInvalidationBaseStyleSheet ) {
		case InvalidationType::Memory: {
			std::string realStyleSheetPath( mTheme->getStyleSheetPath() );
			UICodeEditor* editor = mSplitter->findEditorFromPath( mTheme->getStyleSheetPath() );
			if ( !editor )
				return;
			saveTmpDocument( editor->getDocument(),
							 [&, realStyleSheetPath]( const std::string& tmpPath ) {
								 mTheme->setStyleSheetPath( tmpPath );
								 mTheme->reloadStyleSheet();
								 mTheme->setStyleSheetPath( realStyleSheetPath );
							 } );
			break;
		}
		case InvalidationType::FileSystem:
		default:
			mTheme->reloadStyleSheet();
			break;
	}
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
		mSidePanel->setParent( mUISceneNode->getRoot() );
	}
}

void App::toggleEditor() {
	showEditor( !mSidePanel->isVisible() );
}

void App::reloadLayout() {
	switch ( mInvalidationLayout ) {
		case InvalidationType::Memory: {
			UICodeEditor* editor = mSplitter->findEditorFromPath( mCurrentLayout );
			if ( !editor )
				return;
			saveTmpDocument( editor->getDocument(), [this]( const std::string& tmpPath ) {
				loadLayout( tmpPath, false );
			} );
			break;
		}
		case InvalidationType::FileSystem:
		default:
			loadLayout( mCurrentLayout );
			break;
	}
}

void App::refreshLayout() {
	if ( !mCurrentLayout.empty() && FileSystem::fileExists( mCurrentLayout ) &&
		 mUIContainer != NULL ) {
		if ( !mCurrentStyleSheet.empty() && FileSystem::fileExists( mCurrentStyleSheet ) &&
			 mUIContainer != NULL )
			reloadStyleSheet();
		reloadLayout();
	}

	mUpdateLayout = false;
	mInvalidationLayout = InvalidationType::None;
}

void App::refreshStyleSheet() {
	if ( mUpdateBaseStyleSheet )
		reloadBaseStyleSheet();

	if ( !mCurrentStyleSheet.empty() && FileSystem::fileExists( mCurrentStyleSheet ) &&
		 mUIContainer != NULL ) {
		reloadStyleSheet();
	} else if ( mUpdateBaseStyleSheet ) {
		setUserDefaultTheme();
	}

	if ( !mCurrentLayout.empty() && FileSystem::fileExists( mCurrentLayout ) &&
		 mUIContainer != NULL ) {
		loadLayout( mCurrentLayout );
	}

	mUpdateStyleSheet = false;
	mUpdateBaseStyleSheet = false;
	mInvalidationStyleSheet = InvalidationType::None;
	mInvalidationBaseStyleSheet = InvalidationType::None;
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
	SceneManager::instance()->setCurrentUISceneNode( mAppUISceneNode );

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
			Event::OnItemClicked, [this]( const Event* event ) { onRecentFilesClick( event ); } );
	}

	SceneManager::instance()->setCurrentUISceneNode( mUISceneNode );
}

void App::updateRecentProjects() {
	SceneManager::instance()->setCurrentUISceneNode( mAppUISceneNode );

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
			Event::OnItemClicked, [this]( const Event* event ) { onRecentProjectClick( event ); } );
	}

	SceneManager::instance()->setCurrentUISceneNode( mUISceneNode );
}

void App::resizeCb() {
	if ( mLayoutExpanded ) {
		mUIContainer->setSize( mUISceneNode->getSize() );
	} else {
		mUIContainer->setPixelsSize( mProjectScreenSize );

		Float scaleW =
			(Float)mUISceneNode->getPixelsSize().getWidth() / mProjectScreenSize.getWidth();
		Float scaleH =
			(Float)mUISceneNode->getPixelsSize().getHeight() / mProjectScreenSize.getHeight();
		Float scale = scaleW < scaleH ? scaleW : scaleH;

		if ( scale < 1 ) {
			mUIContainer->setScale( scale );
			mUIContainer->center();
		}
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

	SceneManager::instance()->setCurrentUISceneNode( mAppUISceneNode );

	if ( mLayouts.size() > 0 ) {
		UIPopUpMenu* uiLayoutsMenu = NULL;

		if ( mUIMenuBar->getButton( "Layouts" ) == NULL ) {
			uiLayoutsMenu = UIPopUpMenu::New();

			mUIMenuBar->addMenuButton( "Layouts", uiLayoutsMenu );

			uiLayoutsMenu->addEventListener(
				Event::OnItemClicked, [this]( const Event* event ) { onLayoutSelected( event ); } );
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
							it->first, [this]( std::string widgetName ) -> UIWidget* {
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

				Float width = layoutNode.attribute( "width" ).as_float();
				Float height = layoutNode.attribute( "height" ).as_float();

				mLayoutExpanded = ( 0.f == width && 0.f == height );
				mProjectScreenSize = { width, height };

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
	if ( !FileSystem::fileExists( projectPath ) )
		return;

	closeProject();

	mBasePath = FileSystem::fileRemoveFileName( projectPath );

	FileSystem::changeWorkingDirectory( mBasePath );

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file( projectPath.c_str() );

	if ( result ) {
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

		updateRecentProjects();
	} else {
		Log::error( "Couldn't load UI Layout: %s", projectPath.c_str() );
		Log::error( "Error description: %s", result.description() );
		Log::error( "Error offset: %d", result.offset );
	}
}

void App::closeEditors() {
	UISceneNode* prevUISceneNode = SceneManager::instance()->getUISceneNode();
	SceneManager::instance()->setCurrentUISceneNode( mSplitter->getUISceneNode() );
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
	SceneManager::instance()->setCurrentUISceneNode( prevUISceneNode );
}

void App::closeProject() {
	SceneManager::instance()->setCurrentUISceneNode( mUISceneNode );

	mCurrentLayout = "";
	mCurrentStyleSheet = "";
	mUIContainer->getContainer()->childsCloseAll();
	mUISceneNode->update( Time::Zero );
	mUISceneNode->setStyleSheet( CSS::StyleSheet() );

	mLayouts.clear();

	closeEditors();

	refreshLayoutList();

	unloadFonts();
	unloadImages();
}

bool App::onCloseRequestCallback( EE::Window::Window* ) {
	SceneManager::instance()->setCurrentUISceneNode( mAppUISceneNode );

	mMsgBox = UIMessageBox::New(
		UIMessageBox::OK_CANCEL,
		"Do you really want to close the current file?\nAll changes will be lost." );
	mMsgBox->setTheme( mTheme );
	mMsgBox->addEventListener( Event::OnConfirm, [this]( const Event* ) { mWindow->close(); } );
	mMsgBox->addEventListener( Event::OnWindowClose, [this]( const Event* ) { mMsgBox = NULL; } );
	mMsgBox->setTitle( "Close Editor?" );
	mMsgBox->center();
	mMsgBox->show();

	SceneManager::instance()->setCurrentUISceneNode( mUISceneNode );
	return false;
}

void App::mainLoop() {
	mWindow->getInput()->update();

	if ( mWindow->getInput()->isControlPressed() && mWindow->getInput()->isKeyUp( KEY_ESCAPE ) &&
		 NULL == mMsgBox && onCloseRequestCallback( mWindow ) )
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
		toggleEditor();
	}

	if ( mWindow->getInput()->isKeyUp( KEY_F11 ) )
		createWidgetInspector();

	if ( mWindow->getInput()->isKeyUp( KEY_F12 ) ) {
		Clock clock;
		mUISceneNode->getRoot()->reportStyleStateChangeRecursive();
		Log::info( "Applied style state changes in: %.2fms",
				   clock.getElapsedTime().asMilliseconds() );
	}

	if ( mUpdateLayout && mWaitClock.getElapsedTime().asMilliseconds() > 350.f )
		refreshLayout();

	if ( ( mUpdateStyleSheet && mCssWaitClock.getElapsedTime().asMilliseconds() > 350.f ) ||
		 ( mUpdateBaseStyleSheet && mCssBaseWaitClock.getElapsedTime().asMilliseconds() > 350.f ) )
		refreshStyleSheet();

	SceneManager::instance()->update();

	if ( mAppUISceneNode->invalidated() || mUISceneNode->invalidated() ) {
		mWindow->clear();

		SceneManager::instance()->draw();

		mWindow->display();
	} else {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		mWindow->getInput()->waitEvent( Milliseconds( mWindow->hasFocus() ? 16 : 100 ) );
#endif
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
	dialog->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( title );
	dialog->addEventListener( Event::OpenFile, cb );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->center();
	dialog->show();
}

String App::i18n( const std::string& key, const String& def ) {
	return mUISceneNode->getTranslatorStringFromKey( key, def );
}

void App::updateEditorState() {
	if ( mSplitter->curEditorExistsAndFocused() ) {
		updateEditorTitle( mSplitter->getCurEditor() );
	}
}

UIFileDialog* App::saveFileDialog( UICodeEditor* editor, bool focusOnClose ) {
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
	dialog->addEventListener( Event::SaveFile, [&, editor]( const Event* event ) {
		if ( editor ) {
			std::string path( event->getNode()->asType<UIFileDialog>()->getFullPath() );
			if ( !path.empty() && !FileSystem::isDirectory( path ) &&
				 FileSystem::fileWrite( path, "" ) ) {
				std::string oldPath( editor->getDocument().getFilePath() );
				if ( editor->getDocument().save( path ) ) {
					editor->getDocument().setDeleteOnClose( false );
					FileSystem::fileRemove( oldPath );
					if ( mCurrentLayout == oldPath )
						mCurrentLayout = path;
					UITab* tab = mSplitter->isDocumentOpen( path );
					if ( tab )
						tab->setTooltipText( editor->getDocument().getFilePath() );
					tryUpdateWatch( path );
					updateEditorState();
				} else {
					UIMessageBox* msg =
						UIMessageBox::New( UIMessageBox::OK, i18n( "coudlnt_write_the_file",
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
		dialog->addEventListener( Event::OnWindowClose, [&, editor]( const Event* ) {
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
	if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& id = event->getNode()->asType<UIMenuItem>()->getId();

	SceneManager::instance()->setCurrentUISceneNode( mAppUISceneNode );

	if ( "new-layout" == id ) {
		createNewLayout();
	} else if ( "open-project" == id ) {
		showFileDialog(
			"Open project...", [this]( const Event* event ) { projectOpen( event ); }, "*.xml" );
	} else if ( "open-layout" == id ) {
		showFileDialog(
			"Open layout...", [this]( const Event* event ) { layoutOpen( event ); }, "*.xml" );
	} else if ( "close" == id ) {
		closeProject();
	} else if ( "quit" == id ) {
		onCloseRequestCallback( mWindow );
	} else if ( "load-images-from-path" == id ) {
		showFileDialog(
			"Open images from folder...", [this]( const Event* event ) { imagePathOpen( event ); },
			"*", UIFileDialog::DefaultFlags | UIFileDialog::AllowFolderSelect );
	} else if ( "load-fonts-from-path" == id ) {
		showFileDialog(
			"Open fonts from folder...", [this]( const Event* event ) { fontPathOpen( event ); },
			"*", UIFileDialog::DefaultFlags | UIFileDialog::AllowFolderSelect );
	} else if ( "load-css-from-path" == id ) {
		showFileDialog(
			"Open style sheet from path...",
			[this]( const Event* event ) { styleSheetPathOpen( event ); }, "*.css" );
	} else if ( "toggle-console" == id ) {
		mConsole->toggle();
	} else if ( "toggle-editor" == id ) {
		toggleEditor();
	} else if ( "highlight-focus" == id ) {
		mUISceneNode->setHighlightFocus( !mUISceneNode->getHighlightFocus() );
		mUISceneNode->setHighlightOver( !mUISceneNode->getHighlightOver() );
	} else if ( "debug-boxes" == id ) {
		mUISceneNode->setDrawBoxes( !mUISceneNode->getDrawBoxes() );
	} else if ( "debug-data" == id ) {
		mUISceneNode->setDrawDebugData( !mUISceneNode->getDrawDebugData() );
	} else if ( "inspect-widgets" == id ) {
		createWidgetInspector();
	} else if ( "save-doc" == id ) {
		saveDoc();
	} else if ( "save-as-doc" == id ) {
		if ( mSplitter->curEditorExistsAndFocused() )
			saveFileDialog( mSplitter->getCurEditor() );
	} else if ( "save-all" == id ) {
		saveAll();
	}

	SceneManager::instance()->setCurrentUISceneNode( mUISceneNode );
}

Drawable* App::findIcon( const std::string& icon ) {
	return mAppUISceneNode->findIconDrawable( icon, mMenuIconSize );
}

void App::createAppMenu() {
	SceneManager::instance()->setCurrentUISceneNode( mAppUISceneNode );

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
	uiPopMenu->add( i18n( "save_as", "Save as..." ), findIcon( "document-save-as" ) )
		->setId( "save-as-doc" );
	uiPopMenu->add( i18n( "save_all", "Save All" ), findIcon( "document-save-as" ) )
		->setId( "save-all" );
	uiPopMenu->addSeparator();
	uiPopMenu->add( "Close", findIcon( "document-close" ) )->setId( "close" );
	uiPopMenu->addSeparator();
	uiPopMenu->add( "Quit", findIcon( "quit" ) )->setId( "quit" );

	mUIMenuBar->addMenuButton( "File", uiPopMenu );
	uiPopMenu->addEventListener( Event::OnItemClicked,
								 [this]( const Event* event ) { fileMenuClick( event ); } );

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
	uiResourceMenu->addEventListener( Event::OnItemClicked,
									  [this]( const Event* event ) { fileMenuClick( event ); } );

	UIPopUpMenu* viewMenu = UIPopUpMenu::New();
	viewMenu->add( "Highlight Focus & Hover", nullptr, "F6" )->setId( "highlight-focus" );
	viewMenu->add( "Draw debug boxes", nullptr, "F7" )->setId( "debug-boxes" );
	viewMenu->add( "Draw debug data (mouse hover boxes)", nullptr, "F8" )->setId( "debug-data" );
	viewMenu->addSeparator();
	viewMenu->add( "Inspect Widgets", findIcon( "package" ), "F11" )->setId( "inspect-widgets" );
	viewMenu->add( "Toggle Console", findIcon( "terminal" ), "F3" )->setId( "toggle-console" );
	viewMenu->add( "Toggle Editor", findIcon( "editor" ), "F9" )->setId( "toggle-editor" );
	mUIMenuBar->addMenuButton( "View", viewMenu );
	viewMenu->addEventListener( Event::OnItemClicked,
								[this]( const Event* event ) { fileMenuClick( event ); } );
	mConsole = UIConsole::New();
	mConsole->setQuakeMode( true );
	mConsole->setVisible( false );

	SceneManager::instance()->setCurrentUISceneNode( mUISceneNode );
}

App::App() {}

App::~App() {
	saveConfig();

	if ( mMsgBox )
		mMsgBox->clearEventListener();

	eeSAFE_DELETE( mSplitter );

	delete mFileWatcher;

	delete mListener;
}

void App::init( const Float& pixelDensityConf, const bool& useAppTheme, const std::string& cssFile,
				const std::string& xmlFile, const std::string& projectFile ) {
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

	Log::instance()->setLiveWrite( true );
	Log::instance()->setLogToStdOut( true );

	mResPath = Sys::getProcessPath();

	mWindow = Engine::instance()->createWindow(
		WindowSettings( 1280, 720, "eepp - UI Editor", WindowStyle::Default, WindowBackend::Default,
						32, mResPath + "assets/icon/ee.png", pixelDensity ),
		ContextSettings( false, GLv_default, true, 24, 1, 4, true ) );

	if ( mWindow->isOpen() ) {
		mWindow->setFrameRateLimit( displayManager->getDisplayIndex( 0 )->getRefreshRate() );

		PixelDensity::setPixelDensity( eemax( mWindow->getScale(), pixelDensity ) );

		mWindow->setCloseRequestCallback(
			[this]( auto* window ) -> bool { return onCloseRequestCallback( window ); } );

		mWindow->setQuitCallback( [this]( EE::Window::Window* win ) {
			if ( mWindow->isOpen() )
				onCloseRequestCallback( win );
		} );

		mResPath = Sys::getProcessPath();
#if EE_PLATFORM == EE_PLATFORM_MACOS
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

		mAppUISceneNode = UISceneNode::New();
		mAppUISceneNode->setId( "appUiSceneNode" );
		SceneManager::instance()->add( mAppUISceneNode );

		mAppUISceneNode->enableDrawInvalidation();
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
			{ "arrow-down", 0xea4c },
			{ "arrow-up", 0xea76 },
			{ "arrow-down-s", 0xea4e },
			{ "arrow-right-s", 0xea6e },
			{ "match-case", 0xed8d },
			{ "cursor-pointer", 0xec09 },
		};
		for ( const auto& icon : icons ) {
			iconTheme->add( UIGlyphIcon::New( icon.first, iconFont, icon.second ) );
			iconTheme2->add( UIGlyphIcon::New( icon.first, iconFont, icon.second ) );
		}

		mAppUISceneNode->setStyleSheet( mTheme->getStyleSheet() );
		mAppUISceneNode->getUIThemeManager()
			->setDefaultEffectsEnabled( true )
			->setDefaultTheme( mTheme )
			->setDefaultFont( font )
			->add( mTheme );

		mUISceneNode->getUIThemeManager()->setDefaultFont( font )->setDefaultEffectsEnabled( true );

		mAppUISceneNode->getUIIconThemeManager()->setCurrentTheme( iconTheme );

		mUISceneNode->getUIIconThemeManager()->setCurrentTheme( iconTheme2 );

		loadConfig();

		UIWindow::StyleConfig winStyle( UI_NODE_DEFAULT_FLAGS | UI_WIN_NO_DECORATION );
		mUIContainer = UIWindow::NewOpt( UIWindow::SIMPLE_LAYOUT, winStyle );
		mUIContainer->setId( "appContainer" )->setSize( mUISceneNode->getSize() );
		mUIContainer->setParent( mUISceneNode->getRoot() );
		mUISceneNode->addEventListener( Event::OnSizeChange, [this]( const Event* ) {
			mUIContainer->setPixelsSize( mUISceneNode->getPixelsSize() );
		} );

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
		mAppUISceneNode->getRoot()->addClass( "appbackground" );

		createAppMenu();

		mConfigPath = Sys::getConfigPath( "ecode" );
		mColorSchemesPath = mConfigPath + "colorschemes";
		auto colorSchemes(
			SyntaxColorScheme::loadFromFile( mResPath + "colorschemes/colorschemes.conf" ) );
		if ( FileSystem::isDirectory( mColorSchemesPath ) ) {
			auto colorSchemesFiles = FileSystem::filesGetInPath( mColorSchemesPath );
			for ( auto& file : colorSchemesFiles ) {
				auto colorSchemesInFile = SyntaxColorScheme::loadFromFile( file );
				for ( auto& coloScheme : colorSchemesInFile )
					colorSchemes.emplace_back( coloScheme );
			}
		}
		mAppUISceneNode->bind( "code_container", mBaseLayout );
		mAppUISceneNode->bind( "preview_container", mPreviewLayout );
		mAppUISceneNode->bind( "project_splitter", mProjectSplitter );
		mSidePanel = mProjectSplitter->getFirstWidget();
		SceneManager::instance()->setCurrentUISceneNode( mAppUISceneNode );
		mSplitter =
			UICodeEditorSplitter::New( this, mAppUISceneNode, nullptr, colorSchemes, "eepp" );
		mSplitter->setHideTabBarOnSingleTab( false );
		mSplitter->createEditorWithTabWidget( mBaseLayout );
		SceneManager::instance()->setCurrentUISceneNode( mUISceneNode );
		mUISceneNode->setParent( mProjectSplitter->getLastWidget() );
		mProjectSplitter->setSplitPartition( StyleSheetLength( 30, StyleSheetLength::Percentage ) );

		updateRecentProjects();
		updateRecentFiles();

		resizeCb();

		mUISceneNode->addEventListener( Event::OnSizeChange,
										[this]( const Event* ) { resizeCb(); } );

		mUseDefaultTheme = useAppTheme;

		if ( !cssFile.empty() ) {
			loadStyleSheet( cssFile );
		} else if ( mUseDefaultTheme ) {
			loadBaseStyleSheet();
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

		if ( xmlFile.empty() && projectFile.empty() ) {
			createNewLayout();
		}

		mWindow->runMainLoop( &appLoop );
	}
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
	std::string title( titleFromEditor( editor ) );
	updateEditorTabTitle( editor );
}

void App::tryUpdateEditorTitle( UICodeEditor* editor ) {
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

	if ( doc.getFilePath() == getCurrentLayout() ) {
		updateLayoutFunc( InvalidationType::Memory );
	} else if ( doc.getFilePath() == getCurrentStyleSheet() ) {
		updateStyleSheetFunc( InvalidationType::Memory );
	} else if ( doc.getFilePath() == getBaseStyleSheet() ) {
		updateBaseStyleSheetFunc( InvalidationType::Memory );
	}
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
			 std::find( mTmpDocs.begin(), mTmpDocs.end(), &editor->getDocument() ) !=
				 mTmpDocs.end() ) {
			if ( editor->getDocument().hasFilepath() ) {
				editor->save();
				updateEditorTabTitle( editor );
				if ( mSplitter->getCurEditor() == editor )
					updateEditorTitle( editor );
				mTmpDocs.erase( &editor->getDocument() );
			} else {
				UIFileDialog* dialog = saveFileDialog( editor, false );
				dialog->addEventListener( Event::SaveFile, [&, editor]( const Event* ) {
					updateEditorTabTitle( editor );
					if ( mSplitter->getCurEditor() == editor )
						updateEditorTitle( editor );
				} );
				dialog->addEventListener( Event::OnWindowClose, [&, editor]( const Event* ) {
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
		mSplitter->getCurEditor()->save();
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
	if ( ext == "xml" && editor->getDocument().getFilePath() != mCurrentLayout )
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
	args::Flag useAppTheme( parser, "use-app-theme",
							"Use the default application theme in the editor.",
							{ 'u', "use-app-theme" } );

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

	appInstance = eeNew( App, () );
	appInstance->init( pixelDensityConf.Get(), useAppTheme.Get(), cssFile.Get(), xmlFile.Get(),
					   projectFile.Get() );
	eeSAFE_DELETE( appInstance );

	Engine::destroySingleton();

	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
