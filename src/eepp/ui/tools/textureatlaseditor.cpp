#include <eepp/ui/tools/textureatlaseditor.hpp>
#include <eepp/ui/tools/textureatlassubtextureeditor.hpp>
#include <eepp/ui/tools/textureatlasnew.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uicommondialog.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <algorithm>

namespace EE { namespace UI { namespace Tools {

TextureAtlasEditor::TextureAtlasEditor( UIWindow * AttatchTo, const TGEditorCloseCb& callback ) :
	mUIWindow( AttatchTo ),
	mCloseCb( callback ),
	mTexturePacker( NULL ),
	mTextureAtlasLoader( NULL ),
	mCurSubTexture( NULL )
{
	if ( NULL == UIThemeManager::instance()->getDefaultTheme() ) {
		eePRINTL( "TextureAtlasEditor needs a default theme assigned to work." );
		return;
	}

	mTheme = UIThemeManager::instance()->getDefaultTheme();

	if ( NULL == mUIWindow ) {
		mUIWindow = UIManager::instance()->getMainControl();
		mUIWindow->setThemeControl( mTheme, "winback" );
	}

	if ( UIManager::instance()->getMainControl() == mUIWindow ) {
		mUIContainer = mUIWindow;
	} else {
		mUIContainer = mUIWindow->getContainer();
	}

	UITextBox * TxtBox;
	Uint32 InitY = 230;

	createTextBox( Vector2i( mUIContainer->getSize().getWidth() - 205, 30 ), "SubTexture List:" );

	mSubTextureList = UIListBox::New();
	mSubTextureList->setParent( mUIContainer )
			->setPosition( mUIContainer->getSize().getWidth() - 205, 50 )
			->setSize( 200, mSubTextureList->getRowHeight() * 9 + mSubTextureList->getContainerPadding().Top + mSubTextureList->getContainerPadding().Bottom );
	mSubTextureList->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mSubTextureList->addEventListener( UIEvent::EventOnItemSelected, cb::Make1( this, &TextureAtlasEditor::onSubTextureChange ) );

	createTextBox( Vector2i( mUIContainer->getSize().getWidth() - 205, InitY ), "Current SubTexture:" );

	InitY +=30;

	mSpinOffX = UISpinBox::New();
	mSpinOffX->setParent( mUIContainer )->setSize( 100, 0 )->setVisible( true )->setEnabled( true );
	mSpinOffX->setMinValue( -32000 );
	mSpinOffX->setMaxValue( 32000 );
	mSpinOffX->setPosition( mUIContainer->getSize().getWidth() - mSpinOffX->getSize().getWidth() - 10, InitY );
	mSpinOffX->setAnchors( UI_ANCHOR_TOP | UI_ANCHOR_RIGHT );
	mSpinOffX->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TextureAtlasEditor::onOffXChange ) );

	TxtBox = createTextBox( Vector2i(), "Offset X:" );
	TxtBox->setPosition( mSpinOffX->getPosition().x - 10 - TxtBox->getSize().getWidth(), InitY );

	InitY +=30;

	mSpinOffY = UISpinBox::New();
	mSpinOffY->setParent( mUIContainer )->setSize( 100, 0 )->setVisible( true )->setEnabled( true );
	mSpinOffY->setMinValue( -32000 );
	mSpinOffY->setMaxValue( 32000 );
	mSpinOffY->setPosition( mUIContainer->getSize().getWidth() - mSpinOffY->getSize().getWidth() - 10, InitY );
	mSpinOffY->setAnchors( UI_ANCHOR_TOP | UI_ANCHOR_RIGHT );
	mSpinOffY->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TextureAtlasEditor::onOffYChange ) );
	TxtBox = createTextBox( Vector2i(), "Offset Y:" );
	TxtBox->setPosition( mSpinOffY->getPosition().x - 10 - TxtBox->getSize().getWidth(), InitY );

	InitY +=30;

	mSpinDestW = UISpinBox::New();
	mSpinDestW->setParent( mUIContainer )->setSize( 100, 0 )->setVisible( true )->setEnabled( true );
	mSpinDestW->setMaxValue( 32000 );
	mSpinDestW->setPosition( mUIContainer->getSize().getWidth() - mSpinDestW->getSize().getWidth() - 10, InitY );
	mSpinDestW->setAnchors( UI_ANCHOR_TOP | UI_ANCHOR_RIGHT );
	mSpinDestW->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TextureAtlasEditor::onDestWChange ) );
	TxtBox = createTextBox( Vector2i(), "Dest. Width:" );
	TxtBox->setPosition( mSpinDestW->getPosition().x - 10 - TxtBox->getSize().getWidth(), InitY );

	InitY +=30;

	mSpinDestH = UISpinBox::New();
	mSpinDestH->setParent( mUIContainer )->setSize( 100, 0 )->setVisible( true )->setEnabled( true );
	mSpinDestH->setMaxValue( 32000 );
	mSpinDestH->setPosition( mUIContainer->getSize().getWidth() - mSpinDestH->getSize().getWidth() - 10, InitY );
	mSpinDestH->setAnchors( UI_ANCHOR_TOP | UI_ANCHOR_RIGHT );
	mSpinDestH->addEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TextureAtlasEditor::onDestHChange ) );
	TxtBox = createTextBox( Vector2i(), "Dest. Height:" );
	TxtBox->setPosition( mSpinDestH->getPosition().x - 10 - TxtBox->getSize().getWidth(), InitY );

	UIPushButton * ResetButton = UIPushButton::New();
	ResetButton->setParent( mUIContainer )->setSize( 200, 0 )->setPosition( mUIContainer->getSize().getWidth() - 200 - 5 , mSpinDestH->getPosition().y + mSpinDestH->getSize().getHeight() + 8 );
	ResetButton->setAnchors( UI_ANCHOR_TOP | UI_ANCHOR_RIGHT );
	ResetButton->setText( "Reset Dest. Size" );
	ResetButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TextureAtlasEditor::onResetDestSize ) );

	UIPushButton * ResetOffsetButton = UIPushButton::New();
	ResetOffsetButton->setParent( mUIContainer )->setSize( 200, 0 )->setPosition( ResetButton->getPosition().x, ResetButton->getPosition().y + ResetButton->getSize().getHeight() + 8 );
	ResetOffsetButton->setAnchors( UI_ANCHOR_TOP | UI_ANCHOR_RIGHT );
	ResetOffsetButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TextureAtlasEditor::onResetOffset ) );
	ResetOffsetButton->setText( "Reset Offset" );

	UIPushButton * CenterOffsetButton = UIPushButton::New();
	CenterOffsetButton->setParent( mUIContainer )->setSize( 200, 0 )->setPosition( ResetOffsetButton->getPosition().x, ResetOffsetButton->getPosition().y + ResetOffsetButton->getSize().getHeight() + 8 );
	CenterOffsetButton->setAnchors( UI_ANCHOR_TOP | UI_ANCHOR_RIGHT );
	CenterOffsetButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TextureAtlasEditor::onCenterOffset ) );
	CenterOffsetButton->setText( "Centered Offset" );

	UIPushButton * HBOffsetButton = UIPushButton::New();
	HBOffsetButton->setParent( mUIContainer )->setSize( 200, 0 )->setPosition( CenterOffsetButton->getPosition().x, CenterOffsetButton->getPosition().y + CenterOffsetButton->getSize().getHeight() + 8 );
	HBOffsetButton->setAnchors( UI_ANCHOR_TOP | UI_ANCHOR_RIGHT );
	HBOffsetButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TextureAtlasEditor::onHBOffset ) );
	HBOffsetButton->setText( "Half-Bottom Offset" );

	mUIWindow->setTitle( "Texture Atlas Editor" );
	mUIWindow->addEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &TextureAtlasEditor::windowClose ) );

	createTGEditor();

	mSubTextureEditor = eeNew( TextureAtlasSubTextureEditor, ( this ) );
	mSubTextureEditor->setFlags( UI_CLIP_ENABLE | UI_BORDER | UI_FILL_BACKGROUND );
	mSubTextureEditor->setParent( mUIContainer );
	mSubTextureEditor->setPosition( 0, mWinMenu->getSize().getHeight() );
	mSubTextureEditor->setSize( 800, 600 );
	mSubTextureEditor->setAnchors( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM | UI_ANCHOR_RIGHT );
	mSubTextureEditor->getBackground()->setColor( ColorA( 0, 0, 0, 50 ) );

	mTGEU = eeNew( UITGEUpdater, ( this ) );
}

TextureAtlasEditor::~TextureAtlasEditor() {
	eeSAFE_DELETE( mTexturePacker );
	eeSAFE_DELETE( mTextureAtlasLoader );

	if ( !UIManager::instance()->isShootingDown() ) {
		mTGEU->close();
	}
}

void TextureAtlasEditor::onResetDestSize( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( NULL != mCurSubTexture && MouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		Sizei RealSize = mCurSubTexture->getDpSize();

		mCurSubTexture->resetDestSize();

		mSpinDestW->setValue( RealSize.getWidth() );
		mSpinDestH->setValue( RealSize.getHeight() );
	}
}

void TextureAtlasEditor::onResetOffset( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( NULL != mCurSubTexture && MouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		mSpinOffX->setValue( 0 );
		mSpinOffY->setValue( 0 );
	}
}

void TextureAtlasEditor::onCenterOffset( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( NULL != mCurSubTexture && MouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		Sizei NSize( -( (Int32)mCurSubTexture->getDpSize().x / 2 ), -( (Int32)mCurSubTexture->getDpSize().y / 2 ) );

		mSpinOffX->setValue( NSize.x );
		mSpinOffY->setValue( NSize.y );
	}
}

void TextureAtlasEditor::onHBOffset( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( NULL != mCurSubTexture && MouseEvent->getFlags() & EE_BUTTON_LMASK ) {
		Sizei NSize( -( (Int32)mCurSubTexture->getDpSize().x / 2 ), -(Int32)mCurSubTexture->getDpSize().y );

		mSpinOffX->setValue( NSize.x );
		mSpinOffY->setValue( NSize.y );
	}
}

void TextureAtlasEditor::onOffXChange( const UIEvent * Event ) {
	if ( NULL != mCurSubTexture ) {
		mCurSubTexture->setOffset( Vector2i( (Int32)mSpinOffX->getValue(), mCurSubTexture->getOffset().y ) );
	}
}

void TextureAtlasEditor::onOffYChange( const UIEvent * Event ) {
	if ( NULL != mCurSubTexture ) {
		mCurSubTexture->setOffset( Vector2i( mCurSubTexture->getOffset().x, (Int32)mSpinOffY->getValue() ) );
	}
}

void TextureAtlasEditor::onDestWChange( const UIEvent * Event ) {
	if ( NULL != mCurSubTexture ) {
		mCurSubTexture->setOriDestSize( Sizef( (Int32)mSpinDestW->getValue(), mCurSubTexture->getDpSize().y ) );
		mSubTextureEditor->getGfx()->setSize( (Int32)mSpinDestW->getValue(), mSubTextureEditor->getGfx()->getSize().getHeight() );
	}
}

void TextureAtlasEditor::onDestHChange( const UIEvent * Event ) {
	if ( NULL != mCurSubTexture ) {
		mCurSubTexture->setOriDestSize( Sizef( mCurSubTexture->getDpSize().x, (Int32)mSpinDestH->getValue() ) );
		mSubTextureEditor->getGfx()->setSize( mSubTextureEditor->getGfx()->getSize().getWidth(), (Int32)mSpinDestH->getValue() );
	}
}

UITextBox * TextureAtlasEditor::createTextBox( Vector2i Pos, const String& Text ) {
	UITextBox * txtBox = UITextBox::New();
	txtBox->resetFlags( UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	txtBox->setParent( mUIContainer );
	txtBox->setPosition( Pos );
	txtBox->setText( Text );
	return txtBox;
}

void TextureAtlasEditor::windowClose( const UIEvent * Event ) {
	if ( mCloseCb.IsSet() )
		mCloseCb();

	eeDelete( this );
}

void TextureAtlasEditor::createTGEditor() {
	createWinMenu();
}

void TextureAtlasEditor::createWinMenu() {
	mWinMenu = UIWinMenu::New();
	mWinMenu->setParent( mUIContainer );

	UIPopUpMenu * PU = UIPopUpMenu::New();
	PU->add( "New...", mTheme->getIconByName( "document-new" ) );
	PU->add( "Open...", mTheme->getIconByName( "document-open" ) );
	PU->addSeparator();
	PU->add( "Save", mTheme->getIconByName( "document-save" ) );
	PU->addSeparator();
	PU->add( "Close", mTheme->getIconByName( "document-close" ) );
	PU->addSeparator();
	PU->add( "Quit", mTheme->getIconByName( "quit" ) );

	PU->addEventListener( UIEvent::EventOnItemClicked, cb::Make1( this, &TextureAtlasEditor::fileMenuClick ) );
	mWinMenu->addMenuButton( "File", PU );
}

void TextureAtlasEditor::fileMenuClick( const UIEvent * Event ) {
	if ( !Event->getControl()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->getControl() )->getText();

	if ( "New..." == txt ) {
		eeNew( TextureAtlasNew, ( cb::Make1( this, &TextureAtlasEditor::onTextureAtlasCreate ) ) );
	} else if ( "Open..." == txt ) {
		UICommonDialog * TGDialog = UICommonDialog::New( UI_CDL_DEFAULT_FLAGS, std::string( "*" ) + EE_TEXTURE_ATLAS_EXTENSION );
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Open Texture Atlas" );
		TGDialog->addEventListener( UIEvent::EventOpenFile, cb::Make1( this, &TextureAtlasEditor::openTextureAtlas ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Save" == txt ) {
		if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->isLoaded() ) {
			mTextureAtlasLoader->updateTextureAtlas();
		}
	} else if ( "Close" == txt ) {
		if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->isLoaded()  ) {
			UIMessageBox * MsgBox = UIMessageBox::New( MSGBOX_OKCANCEL, "Do you really want to close the current texture atlas?\nAll changes will be lost." );
			MsgBox->addEventListener( UIEvent::EventMsgBoxConfirmClick, cb::Make1( this, &TextureAtlasEditor::onTextureAtlasClose ) );
			MsgBox->setTitle( "Close Texture Atlas?" );
			MsgBox->center();
			MsgBox->show();
		} else {
			onTextureAtlasClose( NULL );
		}
	} else if ( "Quit" == txt ) {
		if ( mUIWindow == UIManager::instance()->getMainControl() ) {
			UIManager::instance()->getWindow()->close();
		} else {
			mUIWindow->closeWindow();
		}
	}
}

void TextureAtlasEditor::onTextureAtlasCreate( TexturePacker * TexPacker ) {
	eeSAFE_DELETE( mTexturePacker );
	mTexturePacker = TexPacker;

	eeSAFE_DELETE( mTextureAtlasLoader );

	std::string FPath( FileSystem::fileRemoveExtension( mTexturePacker->getFilepath() + EE_TEXTURE_ATLAS_EXTENSION ) );

	mTextureAtlasLoader = eeNew( TextureAtlasLoader, ( FPath, true, cb::Make1( this, &TextureAtlasEditor::onTextureAtlasLoaded ) ) );
}

void TextureAtlasEditor::updateControls() {
	if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->isLoaded()  ) {
		fillSubTextureList();
	}
}

void TextureAtlasEditor::fillSubTextureList() {
	if ( NULL == mTextureAtlasLoader || NULL == mTextureAtlasLoader->getTextureAtlas() || !mTextureAtlasLoader->isLoaded()  )
		return;

	std::list<SubTexture*>& Res = mTextureAtlasLoader->getTextureAtlas()->getResources();

	mSubTextureList->clear();

	std::vector<String> items;

	for ( std::list<SubTexture*>::iterator it = Res.begin(); it != Res.end(); it++ ) {
			items.push_back( (*it)->getName() );
	}

	if ( items.size() ) {
		std::sort( items.begin(), items.end() );

		mSubTextureList->addListBoxItems( items );
		mSubTextureList->setSelected( 0 );
	}

	mSubTextureList->getVerticalScrollBar()->setClickStep( 8.f / (Float)mSubTextureList->getCount() );
}

void TextureAtlasEditor::onSubTextureChange( const UIEvent * Event ) {
	if ( NULL != mTextureAtlasLoader && NULL != mTextureAtlasLoader->getTextureAtlas() ) {
		mCurSubTexture = mTextureAtlasLoader->getTextureAtlas()->getByName( mSubTextureList->getItemSelectedText() );

		if ( NULL != mCurSubTexture ) {
			mSubTextureEditor->setSubTexture( mCurSubTexture );
			mSpinOffX->setValue( PixelDensity::pxToDp( mCurSubTexture->getOffset().x ) );
			mSpinOffY->setValue( PixelDensity::pxToDp( mCurSubTexture->getOffset().y ) );
			mSpinDestW->setValue( mCurSubTexture->getDpSize().x );
			mSpinDestH->setValue( mCurSubTexture->getDpSize().y );
		}
	}
}

void TextureAtlasEditor::update() {
	if ( NULL != mTextureAtlasLoader && !mTextureAtlasLoader->isLoaded() ) {
		mTextureAtlasLoader->update();
	}
}

void TextureAtlasEditor::openTextureAtlas( const UIEvent * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->getControl() );

	eeSAFE_DELETE( mTextureAtlasLoader );
	bool threaded = true;
	#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	threaded = false;
	#endif

	mTextureAtlasLoader = eeNew( TextureAtlasLoader, ( CDL->getFullPath(), threaded, cb::Make1( this, &TextureAtlasEditor::onTextureAtlasLoaded ) ) );
}

void TextureAtlasEditor::onTextureAtlasLoaded( TextureAtlasLoader * TGLoader ) {
	if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->isLoaded() ) {
		updateControls();
	}
}

void TextureAtlasEditor::saveTextureAtlas( const UIEvent * Event ) {
	if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->isLoaded() ) {
		mTextureAtlasLoader->updateTextureAtlas();
	}
}

void TextureAtlasEditor::onTextureAtlasClose( const UIEvent * Event ) {
	eeSAFE_DELETE( mTextureAtlasLoader );
	mSubTextureList->clear();
	mSpinOffX->setValue( 0 );
	mSpinOffY->setValue( 0 );
	mSpinDestW->setValue( 0 );
	mSpinDestH->setValue( 0 );
	mSubTextureEditor->setSubTexture( NULL );
	mCurSubTexture = NULL;
}

}}}
