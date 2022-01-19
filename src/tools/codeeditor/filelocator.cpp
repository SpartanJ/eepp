#include "filelocator.hpp"
#include "codeeditor.hpp"

static int LOCATEBAR_MAX_VISIBLE_ITEMS = 18;
static int LOCATEBAR_MAX_RESULTS = 100;

FileLocator::FileLocator( UICodeEditorSplitter* editorSplitter, UISceneNode* sceneNode, App* app ) :
	mEditorSplitter( editorSplitter ), mUISceneNode( sceneNode ), mApp( app ) {}

void FileLocator::hideLocateBar() {
	mLocateBarLayout->setVisible( false );
	mLocateTable->setVisible( false );
}

void FileLocator::updateLocateTable() {
	if ( !mLocateInput->getText().empty() ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
		mApp->getDirTree()->asyncFuzzyMatchTree(
			mLocateInput->getText(), LOCATEBAR_MAX_RESULTS, [&]( auto res ) {
				mUISceneNode->runOnMainThread( [&, res] {
					mLocateTable->setModel( res );
					mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
				} );
			} );
#else
		mLocateTable->setModel(
			mDirTree->fuzzyMatchTree( mLocateInput->getText(), LOCATEBAR_MAX_RESULTS ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
#endif
	} else {
		mLocateTable->setModel( mApp->getDirTree()->asModel( LOCATEBAR_MAX_RESULTS ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
	}
}

void FileLocator::goToLine() {
	showLocateBar();
	mLocateInput->setText( "l " );
}

void FileLocator::initLocateBar( UILocateBar* locateBar, UITextInput* locateInput ) {
	mLocateBarLayout = locateBar;
	mLocateInput = locateInput;
	auto addClickListener = [&]( UIWidget* widget, std::string cmd ) {
		widget->addEventListener( Event::MouseClick, [this, cmd]( const Event* event ) {
			const MouseEvent* mouseEvent = static_cast<const MouseEvent*>( event );
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK )
				mLocateBarLayout->execute( cmd );
		} );
	};
	mLocateTable = UITableView::New();
	mLocateTable->setId( "locate_bar_table" );
	mLocateTable->setParent( mUISceneNode->getRoot() );
	mLocateTable->setHeadersVisible( false );
	mLocateTable->setVisible( false );
	mLocateInput->addEventListener( Event::OnTextChanged, [&]( const Event* ) {
		if ( mEditorSplitter->getCurEditor() &&
			 String::startsWith( mLocateInput->getText(), String( "l " ) ) ) {
			String number( mLocateInput->getText().substr( 2 ) );
			Int64 val;
			if ( String::fromString( val, number ) && val - 1 >= 0 ) {
				mEditorSplitter->getCurEditor()->goToLine( { val - 1, 0 } );
				mLocateTable->setVisible( false );
			}
		} else {
			mLocateTable->setVisible( true );
			Vector2f pos( mLocateInput->convertToWorldSpace( { 0, 0 } ) );
			pos.y -= mLocateTable->getPixelsSize().getHeight();
			mLocateTable->setPixelsPosition( pos );
			if ( !mApp->isDirTreeReady() )
				return;
			updateLocateTable();
		}
	} );
	mLocateInput->addEventListener( Event::OnPressEnter, [&]( const Event* ) {
		KeyEvent keyEvent( mLocateTable, Event::KeyDown, KEY_RETURN, 0, 0 );
		mLocateTable->forceKeyDown( keyEvent );
	} );
	mLocateInput->addEventListener( Event::KeyDown, [&]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		mLocateTable->forceKeyDown( *keyEvent );
	} );
	mLocateBarLayout->addCommand( "close-locatebar", [&] {
		hideLocateBar();
		mEditorSplitter->getCurEditor()->setFocus();
	} );
	mLocateBarLayout->getKeyBindings().addKeybindsString( {
		{ "escape", "close-locatebar" },
	} );
	mLocateTable->addEventListener( Event::KeyDown, [&]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		if ( keyEvent->getKeyCode() == KEY_ESCAPE )
			mLocateBarLayout->execute( "close-locatebar" );
	} );
	addClickListener( mLocateBarLayout->find<UIWidget>( "locatebar_close" ), "close-locatebar" );
	mLocateTable->addEventListener( Event::OnModelEvent, [&]( const Event* event ) {
		const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
		if ( modelEvent->getModelEventType() == ModelEventType::Open ) {
			Variant vPath( modelEvent->getModel()->data(
				modelEvent->getModel()->index( modelEvent->getModelIndex().row(), 1 ),
				ModelRole::Display ) );
			if ( vPath.isValid() && vPath.is( Variant::Type::cstr ) ) {
				std::string path( vPath.asCStr() );
				UITab* tab = mEditorSplitter->isDocumentOpen( path );
				if ( !tab ) {
					FileInfo fileInfo( path );
					if ( fileInfo.exists() && fileInfo.isRegularFile() )
						mApp->loadFileFromPath( path );
				} else {
					tab->getTabWidget()->setTabSelected( tab );
				}
				mLocateBarLayout->execute( "close-locatebar" );
			}
		}
	} );
}

void FileLocator::updateLocateBar() {
	mLocateBarLayout->runOnMainThread( [&] {
		Float width = eeceil( mLocateInput->getPixelsSize().getWidth() );
		mLocateTable->setPixelsSize( width,
									 mLocateTable->getRowHeight() * LOCATEBAR_MAX_VISIBLE_ITEMS );
		width -= mLocateTable->getVerticalScrollBar()->getPixelsSize().getWidth();
		mLocateTable->setColumnWidth( 0, eeceil( width * 0.5 ) );
		mLocateTable->setColumnWidth( 1, width - mLocateTable->getColumnWidth( 0 ) );
		Vector2f pos( mLocateInput->convertToWorldSpace( { 0, 0 } ) );
		pos.y -= mLocateTable->getPixelsSize().getHeight();
		mLocateTable->setPixelsPosition( pos );
	} );
}

void FileLocator::showLocateBar() {
	mApp->hideGlobalSearchBar();
	mApp->hideSearchBar();

	mLocateBarLayout->setVisible( true );
	mLocateInput->setFocus();
	mLocateTable->setVisible( true );
	mLocateInput->getDocument().selectAll();
	mLocateInput->addEventListener( Event::OnSizeChange,
									[&]( const Event* ) { updateLocateBar(); } );
	if ( mApp->getDirTree() && !mLocateTable->getModel() ) {
		mLocateTable->setModel( mApp->getDirTree()->asModel( LOCATEBAR_MAX_RESULTS ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
	}
	updateLocateBar();
}
