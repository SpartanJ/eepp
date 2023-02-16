#include "filelocator.hpp"
#include "ecode.hpp"

namespace ecode {

static int LOCATEBAR_MAX_VISIBLE_ITEMS = 18;
static int LOCATEBAR_MAX_RESULTS = 100;

FileLocator::FileLocator( UICodeEditorSplitter* editorSplitter, UISceneNode* sceneNode, App* app ) :
	mSplitter( editorSplitter ),
	mUISceneNode( sceneNode ),
	mApp( app ),
	mCommandPalette( mApp->getThreadPool() ) {}

void FileLocator::hideLocateBar() {
	mLocateBarLayout->setVisible( false );
	mLocateTable->setVisible( false );
}

void FileLocator::updateFilesTable() {
	if ( !mLocateInput->getText().empty() ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
		mApp->getDirTree()->asyncFuzzyMatchTree(
			mLocateInput->getText(), LOCATEBAR_MAX_RESULTS, [&]( auto res ) {
				mUISceneNode->runOnMainThread( [&, res] {
					mLocateTable->setModel( res );
					mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
					mLocateTable->scrollToTop();
				} );
			} );
#else
		mLocateTable->setModel(
			mApp->getDirTree()->fuzzyMatchTree( mLocateInput->getText(), LOCATEBAR_MAX_RESULTS ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
#endif
	} else {
		mLocateTable->setModel( mApp->getDirTree()->asModel( LOCATEBAR_MAX_RESULTS ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
	}
}

void FileLocator::updateCommandPaletteTable() {
	if ( !mCommandPalette.isSet() )
		mCommandPalette.setCommandPalette( mApp->getMainLayout()->getCommandList(),
										   mApp->getMainLayout()->getKeyBindings() );

	if ( !mCommandPalette.isEditorSet() && mSplitter->curEditorIsNotNull() )
		mCommandPalette.setEditorCommandPalette(
			mSplitter->getCurEditor()->getDocumentRef()->getCommandList(),
			mSplitter->getCurEditor()->getKeyBindings() );

	mCommandPalette.setCurModel( mSplitter->curEditorIsNotNull() ? mCommandPalette.getEditorModel()
																 : mCommandPalette.getBaseModel() );

	auto txt( mLocateInput->getText() );
	txt.trim();

	if ( txt.size() > 1 ) {

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
		mCommandPalette.asyncFuzzyMatch( txt.substr( 1 ), 1000, [&]( auto res ) {
			mUISceneNode->runOnMainThread( [&, res] {
				mLocateTable->setModel( res );
				mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
				mLocateTable->scrollToTop();
			} );
		} );
#else
		mLocateTable->setModel( mCommandPalette.fuzzyMatch( txt.substr(), 1000 ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
#endif
	} else if ( mCommandPalette.getCurModel() ) {
		mLocateTable->setModel( mCommandPalette.getCurModel() );

		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
	}

	mLocateTable->setColumnsVisible( { 0, 1 } );
}

void FileLocator::showLocateTable() {
	mLocateTable->setVisible( true );
	Vector2f pos( mLocateInput->convertToWorldSpace( { 0, 0 } ) );
	pos.y -= mLocateTable->getPixelsSize().getHeight();
	mLocateTable->setPixelsPosition( pos );
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
			if ( mouseEvent->getFlags() & EE_BUTTON_LMASK ) {
				mLocateBarLayout->execute( cmd );
			}
		} );
	};
	mLocateTable = UITableView::New();
	mLocateTable->setId( "locate_bar_table" );
	mLocateTable->setParent( mUISceneNode->getRoot() );
	mLocateTable->setHeadersVisible( false );
	mLocateTable->setVisible( false );
	mLocateInput->addEventListener( Event::OnTextChanged, [&]( const Event* ) {
		const String& txt = mLocateInput->getText();
		if ( mSplitter->curEditorExistsAndFocused() && String::startsWith( txt, String( "l " ) ) ) {
			String number( txt.substr( 2 ) );
			Int64 val;
			if ( String::fromString( val, number ) && val - 1 >= 0 ) {
				if ( mSplitter->curEditorExistsAndFocused() )
					mSplitter->getCurEditor()->goToLine( { val - 1, 0 } );
				mLocateTable->setVisible( false );
			}
		} else if ( !txt.empty() && mLocateInput->getText()[0] == '>' ) {
			showCommandPalette();
		} else {
			showLocateTable();
			if ( !mApp->isDirTreeReady() )
				return;
			updateFilesTable();
		}
	} );
	mLocateInput->addEventListener( Event::OnPressEnter, [&]( const Event* ) {
		KeyEvent keyEvent( mLocateTable, Event::KeyDown, KEY_RETURN, SCANCODE_UNKNOWN, 0, 0 );
		mLocateTable->forceKeyDown( keyEvent );
	} );
	mLocateInput->addEventListener( Event::KeyDown, [&]( const Event* event ) {
		const KeyEvent* keyEvent = static_cast<const KeyEvent*>( event );
		mLocateTable->forceKeyDown( *keyEvent );
	} );
	mLocateBarLayout->setCommand( "close-locatebar", [&] {
		hideLocateBar();
		if ( mSplitter->getCurWidget() )
			mSplitter->getCurWidget()->setFocus();
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
			// Keep it simple for now, command palette has 3 columns
			if ( modelEvent->getModel()->columnCount() == 3 ) {
				ModelIndex idx(
					modelEvent->getModel()->index( modelEvent->getModelIndex().row(), 2 ) );
				if ( idx.isValid() ) {
					auto cmd = modelEvent->getModel()->data( idx, ModelRole::Display ).toString();
					mApp->runCommand( cmd );
					if ( !mSplitter->getCurWidget()->isType( UI_TYPE_TERMINAL ) ) {
						if ( mSplitter->curEditorIsNotNull() &&
							 mSplitter->getCurEditor()->getDocument().hasCommand( cmd ) )
							mSplitter->getCurEditor()->setFocus();
					}
				}
				hideLocateBar();
			} else {
				Variant vPath( modelEvent->getModel()->data(
					modelEvent->getModel()->index( modelEvent->getModelIndex().row(), 1 ),
					ModelRole::Display ) );
				if ( vPath.isValid() && vPath.is( Variant::Type::cstr ) ) {
					std::string path( vPath.asCStr() );
					UITab* tab = mSplitter->isDocumentOpen( path, true );
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
		}
	} );
}

void FileLocator::updateLocateBar() {
	mLocateBarLayout->runOnMainThread( [&] {
		Float width = eeceil( mLocateInput->getPixelsSize().getWidth() );
		mLocateTable->setPixelsSize( width,
									 mLocateTable->getRowHeight() * LOCATEBAR_MAX_VISIBLE_ITEMS );
		width -= mLocateTable->getVerticalScrollBar()->getPixelsSize().getWidth();
		mLocateTable->setColumnsVisible( { 0, 1 } );
		mLocateTable->setColumnWidth( 0, eeceil( width * 0.5 ) );
		mLocateTable->setColumnWidth( 1, width - mLocateTable->getColumnWidth( 0 ) );
		Vector2f pos( mLocateInput->convertToWorldSpace( { 0, 0 } ) );
		pos.y -= mLocateTable->getPixelsSize().getHeight();
		mLocateTable->setPixelsPosition( pos );
	} );
}

void FileLocator::showBar() {
	mApp->hideGlobalSearchBar();
	mApp->hideSearchBar();

	mLocateBarLayout->setVisible( true );
	mLocateInput->setFocus();
	mLocateTable->setVisible( true );
	const String& text = mLocateInput->getText();

	if ( !text.empty() && text[0] == '>' ) {
		Int64 selectFrom = 1;
		if ( text.size() >= 2 && text[1] == ' ' )
			selectFrom = 2;

		mLocateInput->getDocument().setSelection(
			{ { 0, selectFrom },
			  { 0, mLocateInput->getDocument().endOfLine( { 0, 0 } ).column() } } );
	} else {
		mLocateInput->getDocument().selectAll();
	}

	mLocateInput->addEventListener( Event::OnSizeChange,
									[&]( const Event* ) { updateLocateBar(); } );
}

void FileLocator::showLocateBar() {
	showBar();

	if ( !mLocateInput->getText().empty() && mLocateInput->getText()[0] == '>' )
		mLocateInput->setText( "" );

	if ( mApp->getDirTree() && !mLocateTable->getModel() ) {
		mLocateTable->setModel( mApp->getDirTree()->asModel( LOCATEBAR_MAX_RESULTS ) );
		mLocateTable->getSelection().set( mLocateTable->getModel()->index( 0 ) );
	}

	updateLocateBar();
}

void FileLocator::showCommandPalette() {
	showBar();

	if ( mLocateInput->getText().empty() || mLocateInput->getText()[0] != '>' )
		mLocateInput->setText( "> " );

	updateCommandPaletteTable();
	updateLocateBar();
}

} // namespace ecode
