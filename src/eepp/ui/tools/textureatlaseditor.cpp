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
	if ( NULL == UIThemeManager::instance()->DefaultTheme() ) {
		eePRINTL( "TextureAtlasEditor needs a default theme assigned to work." );
		return;
	}

	mTheme = UIThemeManager::instance()->DefaultTheme();

	if ( NULL == mUIWindow ) {
		mUIWindow = UIManager::instance()->MainControl();
		mUIWindow->SetSkinFromTheme( mTheme, "winback" );
	}

	if ( UIManager::instance()->MainControl() == mUIWindow ) {
		mUIContainer = mUIWindow;
	} else {
		mUIContainer = mUIWindow->Container();
	}

	UITextBox * TxtBox;
	Uint32 Flags = UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_CLIP_ENABLE | UI_AUTO_SIZE | UI_TEXT_SELECTION_ENABLED;
	Uint32 InitY = 230;

	CreateTxtBox( Vector2i( mUIContainer->Size().width() - 205, 30 ), "SubTexture List:" );

	mSubTextureList = mTheme->CreateListBox( mUIContainer, Sizei( 200, 156 ), Vector2i( mUIContainer->Size().width() - 205, 50 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mSubTextureList->Size( mSubTextureList->Size().width(), mSubTextureList->RowHeight() * 9 + mSubTextureList->PaddingContainer().Top + mSubTextureList->PaddingContainer().Bottom );
	mSubTextureList->AddEventListener( UIEvent::EventOnItemSelected, cb::Make1( this, &TextureAtlasEditor::OnSubTextureChange ) );

	CreateTxtBox( Vector2i( mUIContainer->Size().width() - 205, InitY ), "Current SubTexture:" );

	InitY +=30;

	mSpinOffX = mTheme->CreateSpinBox( mUIContainer, Sizei( 100, 22 ), Vector2i(), Flags, 0, false );
	mSpinOffX->MinValue( -32000 );
	mSpinOffX->MaxValue( 32000 );
	mSpinOffX->Pos( mUIContainer->Size().width() - mSpinOffX->Size().width() - 10, InitY );
	mSpinOffX->AddEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TextureAtlasEditor::OnOffXChange ) );

	TxtBox = CreateTxtBox( Vector2i(), "Offset X:" );
	TxtBox->Pos( mSpinOffX->Pos().x - 10 - TxtBox->Size().width(), InitY );

	InitY +=30;

	mSpinOffY = mTheme->CreateSpinBox( mUIContainer, Sizei( 100, 22 ), Vector2i(), Flags, 0, false );
	mSpinOffY->MinValue( -32000 );
	mSpinOffY->MaxValue( 32000 );
	mSpinOffY->Pos( mUIContainer->Size().width() - mSpinOffY->Size().width() - 10, InitY );
	mSpinOffY->AddEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TextureAtlasEditor::OnOffYChange ) );
	TxtBox = CreateTxtBox( Vector2i(), "Offset Y:" );
	TxtBox->Pos( mSpinOffY->Pos().x - 10 - TxtBox->Size().width(), InitY );

	InitY +=30;

	mSpinDestW = mTheme->CreateSpinBox( mUIContainer, Sizei( 100, 22 ), Vector2i(), Flags, 0, false );
	mSpinDestW->MaxValue( 32000 );
	mSpinDestW->Pos( mUIContainer->Size().width() - mSpinDestW->Size().width() - 10, InitY );
	mSpinDestW->AddEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TextureAtlasEditor::OnDestWChange ) );
	TxtBox = CreateTxtBox( Vector2i(), "Dest. Width:" );
	TxtBox->Pos( mSpinDestW->Pos().x - 10 - TxtBox->Size().width(), InitY );

	InitY +=30;

	mSpinDestH = mTheme->CreateSpinBox( mUIContainer, Sizei( 100, 22 ), Vector2i(), Flags, 0, false );
	mSpinDestH->MaxValue( 32000 );
	mSpinDestH->Pos( mUIContainer->Size().width() - mSpinDestH->Size().width() - 10, InitY );
	mSpinDestH->AddEventListener( UIEvent::EventOnValueChange, cb::Make1( this, &TextureAtlasEditor::OnDestHChange ) );
	TxtBox = CreateTxtBox( Vector2i(), "Dest. Height:" );
	TxtBox->Pos( mSpinDestH->Pos().x - 10 - TxtBox->Size().width(), InitY );

	Uint32 ButFlags = UI_CONTROL_ALIGN_CENTER | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_AUTO_SIZE;

	UIPushButton * ResetButton = mTheme->CreatePushButton( mUIContainer, Sizei( 120, 22 ), Vector2i( mUIContainer->Size().width() - 120 - 5 , mSpinDestH->Pos().y + mSpinDestH->Size().height() + 8 ), ButFlags );
	ResetButton->Text( "Reset Dest. Size" );
	ResetButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TextureAtlasEditor::OnResetDestSize ) );

	UIPushButton * ResetOffsetButton = mTheme->CreatePushButton( mUIContainer, Sizei( 120, 22 ), Vector2i( ResetButton->Pos().x, ResetButton->Pos().y + ResetButton->Size().height() + 8 ), ButFlags );
	ResetOffsetButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TextureAtlasEditor::OnResetOffset ) );
	ResetOffsetButton->Text( "Reset Offset" );

	UIPushButton * CenterOffsetButton = mTheme->CreatePushButton( mUIContainer, Sizei( 120, 22 ), Vector2i( ResetOffsetButton->Pos().x, ResetOffsetButton->Pos().y + ResetOffsetButton->Size().height() + 8 ), ButFlags );
	CenterOffsetButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TextureAtlasEditor::OnCenterOffset ) );
	CenterOffsetButton->Text( "Centered Offset" );

	UIPushButton * HBOffsetButton = mTheme->CreatePushButton( mUIContainer, Sizei( 120, 22 ), Vector2i( CenterOffsetButton->Pos().x, CenterOffsetButton->Pos().y + CenterOffsetButton->Size().height() + 8 ), ButFlags );
	HBOffsetButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &TextureAtlasEditor::OnHBOffset ) );
	HBOffsetButton->Text( "Half-Bottom Offset" );

	mUIWindow->Title( "Texture Atlas Editor" );
	mUIWindow->AddEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &TextureAtlasEditor::WindowClose ) );

	CreateTGEditor();

	UIComplexControl::CreateParams Params;
	Params.Parent( mUIContainer );
	Params.PosSet( 0, mWinMenu->Size().height() );
	Params.SizeSet( 800, 600 );
	Params.Background.Color( ColorA( 0, 0, 0, 50 ) );
	Params.Flags |= UI_ANCHOR_BOTTOM | UI_ANCHOR_RIGHT | UI_CLIP_ENABLE | UI_BORDER | UI_FILL_BACKGROUND;
	mSubTextureEditor = eeNew( TextureAtlasSubTextureEditor, ( Params, this ) );
	mSubTextureEditor->Visible( true );
	mSubTextureEditor->Enabled( true );

	mTGEU = eeNew( UITGEUpdater, ( UITGEUpdater::CreateParams(), this ) );
}

TextureAtlasEditor::~TextureAtlasEditor() {
	eeSAFE_DELETE( mTexturePacker );
	eeSAFE_DELETE( mTextureAtlasLoader );

	if ( !UIManager::instance()->IsShootingDown() ) {
		mTGEU->Close();
	}
}

void TextureAtlasEditor::OnResetDestSize( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( NULL != mCurSubTexture && MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		Sizei RealSize = mCurSubTexture->RealSize();

		mCurSubTexture->ResetDestSize();

		mSpinDestW->Value( RealSize.width() );
		mSpinDestH->Value( RealSize.height() );
	}
}

void TextureAtlasEditor::OnResetOffset( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( NULL != mCurSubTexture && MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		mSpinOffX->Value( 0 );
		mSpinOffY->Value( 0 );
	}
}

void TextureAtlasEditor::OnCenterOffset( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( NULL != mCurSubTexture && MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		Sizei NSize( -( (Int32)mCurSubTexture->DestSize().x / 2 ), -( (Int32)mCurSubTexture->DestSize().y / 2 ) );

		mSpinOffX->Value( NSize.x );
		mSpinOffY->Value( NSize.y );
	}
}

void TextureAtlasEditor::OnHBOffset( const UIEvent * Event ) {
	const UIEventMouse * MouseEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( NULL != mCurSubTexture && MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		Sizei NSize( -( (Int32)mCurSubTexture->DestSize().x / 2 ), -(Int32)mCurSubTexture->DestSize().y );

		mSpinOffX->Value( NSize.x );
		mSpinOffY->Value( NSize.y );
	}
}

void TextureAtlasEditor::OnOffXChange( const UIEvent * Event ) {
	if ( NULL != mCurSubTexture ) {
		mCurSubTexture->Offset( Vector2i( (Int32)mSpinOffX->Value(), mCurSubTexture->Offset().y ) );
	}
}

void TextureAtlasEditor::OnOffYChange( const UIEvent * Event ) {
	if ( NULL != mCurSubTexture ) {
		mCurSubTexture->Offset( Vector2i( mCurSubTexture->Offset().x, (Int32)mSpinOffY->Value() ) );
	}
}

void TextureAtlasEditor::OnDestWChange( const UIEvent * Event ) {
	if ( NULL != mCurSubTexture ) {
		mCurSubTexture->DestSize( Sizef( (Int32)mSpinDestW->Value(), mCurSubTexture->DestSize().y ) );
	}
}

void TextureAtlasEditor::OnDestHChange( const UIEvent * Event ) {
	if ( NULL != mCurSubTexture ) {
		mCurSubTexture->DestSize( Sizef( mCurSubTexture->DestSize().x, (Int32)mSpinDestH->Value() ) );
	}
}

UITextBox * TextureAtlasEditor::CreateTxtBox( Vector2i Pos, const String& Text ) {
	return mTheme->CreateTextBox( Text, mUIContainer, Sizei(), Pos, UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_DRAW_SHADOW | UI_AUTO_SIZE );
}

void TextureAtlasEditor::WindowClose( const UIEvent * Event ) {
	if ( mCloseCb.IsSet() )
		mCloseCb();

	eeDelete( this );
}

void TextureAtlasEditor::CreateTGEditor() {
	CreateWinMenu();
}

void TextureAtlasEditor::CreateWinMenu() {
	mWinMenu = mTheme->CreateWinMenu( mUIContainer );

	UIPopUpMenu * PU = mTheme->CreatePopUpMenu();
	PU->Add( "New...", mTheme->GetIconByName( "document-new" ) );
	PU->Add( "Open...", mTheme->GetIconByName( "document-open" ) );
	PU->AddSeparator();
	PU->Add( "Save", mTheme->GetIconByName( "document-save" ) );
	PU->AddSeparator();
	PU->Add( "Close", mTheme->GetIconByName( "document-close" ) );
	PU->AddSeparator();
	PU->Add( "Quit", mTheme->GetIconByName( "quit" ) );

	PU->AddEventListener( UIEvent::EventOnItemClicked, cb::Make1( this, &TextureAtlasEditor::FileMenuClick ) );
	mWinMenu->AddMenuButton( "File", PU );
}

void TextureAtlasEditor::FileMenuClick( const UIEvent * Event ) {
	if ( !Event->Ctrl()->IsType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->Ctrl() )->Text();

	if ( "New..." == txt ) {
		eeNew( TextureAtlasNew, ( cb::Make1( this, &TextureAtlasEditor::OnTextureAtlasCreate ) ) );
	} else if ( "Open..." == txt ) {
		UICommonDialog * TGDialog = mTheme->CreateCommonDialog( NULL, Sizei(), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, Sizei(), 255, UI_CDL_DEFAULT_FLAGS, std::string( "*" ) + EE_TEXTURE_ATLAS_EXTENSION );

		TGDialog->Title( "Open Texture Atlas" );
		TGDialog->AddEventListener( UIEvent::EventOpenFile, cb::Make1( this, &TextureAtlasEditor::OpenTextureAtlas ) );
		TGDialog->Center();
		TGDialog->Show();
	} else if ( "Save" == txt ) {
		if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->IsLoaded() ) {
			mTextureAtlasLoader->UpdateTextureAtlas();
		}
	} else if ( "Close" == txt ) {
		if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->IsLoaded()  ) {
			UIMessageBox * MsgBox = mTheme->CreateMessageBox( MSGBOX_OKCANCEL, "Do you really want to close the current texture atlas?\nAll changes will be lost." );
			MsgBox->AddEventListener( UIEvent::EventMsgBoxConfirmClick, cb::Make1( this, &TextureAtlasEditor::OnTextureAtlasClose ) );
			MsgBox->Title( "Close Texture Atlas?" );
			MsgBox->Center();
			MsgBox->Show();
		} else {
			OnTextureAtlasClose( NULL );
		}
	} else if ( "Quit" == txt ) {
		if ( mUIWindow == UIManager::instance()->MainControl() ) {
			UIManager::instance()->GetWindow()->Close();
		} else {
			mUIWindow->CloseWindow();
		}
	}
}

void TextureAtlasEditor::OnTextureAtlasCreate( TexturePacker * TexPacker ) {
	eeSAFE_DELETE( mTexturePacker );
	mTexturePacker = TexPacker;

	eeSAFE_DELETE( mTextureAtlasLoader );

	std::string FPath( FileSystem::fileRemoveExtension( mTexturePacker->GetFilepath() + EE_TEXTURE_ATLAS_EXTENSION ) );

	mTextureAtlasLoader = eeNew( TextureAtlasLoader, ( FPath, true, cb::Make1( this, &TextureAtlasEditor::OnTextureAtlasLoaded ) ) );
}

void TextureAtlasEditor::UpdateControls() {
	if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->IsLoaded()  ) {
		FillSubTextureList();
	}
}

void TextureAtlasEditor::FillSubTextureList() {
	if ( NULL == mTextureAtlasLoader || NULL == mTextureAtlasLoader->GetTextureAtlas() || !mTextureAtlasLoader->IsLoaded()  )
		return;

	std::list<SubTexture*>& Res = mTextureAtlasLoader->GetTextureAtlas()->getResources();

	mSubTextureList->Clear();

	std::vector<String> items;

	for ( std::list<SubTexture*>::iterator it = Res.begin(); it != Res.end(); it++ ) {
			items.push_back( (*it)->Name() );
	}

	if ( items.size() ) {
		std::sort( items.begin(), items.end() );

		mSubTextureList->AddListBoxItems( items );
		mSubTextureList->SetSelected( 0 );
	}

	mSubTextureList->VerticalScrollBar()->ClickStep( 8.f / (Float)mSubTextureList->Count() );
}

void TextureAtlasEditor::OnSubTextureChange( const UIEvent * Event ) {
	if ( NULL != mTextureAtlasLoader && NULL != mTextureAtlasLoader->GetTextureAtlas() ) {
		mCurSubTexture = mTextureAtlasLoader->GetTextureAtlas()->getByName( mSubTextureList->GetItemSelectedText() );

		if ( NULL != mCurSubTexture ) {
			mSubTextureEditor->SubTexture( mCurSubTexture );
			mSpinOffX->Value( mCurSubTexture->Offset().x );
			mSpinOffY->Value( mCurSubTexture->Offset().y );
			mSpinDestW->Value( mCurSubTexture->DestSize().x );
			mSpinDestH->Value( mCurSubTexture->DestSize().y );
		}
	}
}

void TextureAtlasEditor::Update() {
	if ( NULL != mTextureAtlasLoader && !mTextureAtlasLoader->IsLoaded() ) {
		mTextureAtlasLoader->Update();
	}
}

void TextureAtlasEditor::OpenTextureAtlas( const UIEvent * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->Ctrl() );

	eeSAFE_DELETE( mTextureAtlasLoader );
	bool threaded = true;
	#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	threaded = false;
	#endif

	mTextureAtlasLoader = eeNew( TextureAtlasLoader, ( CDL->GetFullPath(), threaded, cb::Make1( this, &TextureAtlasEditor::OnTextureAtlasLoaded ) ) );
}

void TextureAtlasEditor::OnTextureAtlasLoaded( TextureAtlasLoader * TGLoader ) {
	if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->IsLoaded() ) {
		UpdateControls();
	}
}

void TextureAtlasEditor::SaveTextureAtlas( const UIEvent * Event ) {
	if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->IsLoaded() ) {
		mTextureAtlasLoader->UpdateTextureAtlas();
	}
}

void TextureAtlasEditor::OnTextureAtlasClose( const UIEvent * Event ) {
	eeSAFE_DELETE( mTextureAtlasLoader );
	mSubTextureList->Clear();
	mSpinOffX->Value( 0 );
	mSpinOffY->Value( 0 );
	mSpinDestW->Value( 0 );
	mSpinDestH->Value( 0 );
	mSubTextureEditor->SubTexture( NULL );
	mCurSubTexture = NULL;
}

}}}
