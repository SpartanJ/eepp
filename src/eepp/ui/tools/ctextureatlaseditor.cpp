#include <eepp/ui/tools/ctextureatlaseditor.hpp>
#include <eepp/ui/tools/ctextureatlassubtextureeditor.hpp>
#include <eepp/ui/tools/ctextureatlasnew.hpp>
#include <eepp/ui/cuimanager.hpp>
#include <eepp/ui/cuipopupmenu.hpp>
#include <eepp/ui/cuimenuitem.hpp>
#include <eepp/ui/cuicommondialog.hpp>
#include <eepp/ui/cuimessagebox.hpp>
#include <algorithm>

namespace EE { namespace UI { namespace Tools {

cTextureAtlasEditor::cTextureAtlasEditor( cUIWindow * AttatchTo, const TGEditorCloseCb& callback ) :
	mUIWindow( AttatchTo ),
	mCloseCb( callback ),
	mTexturePacker( NULL ),
	mTextureAtlasLoader( NULL ),
	mCurSubTexture( NULL )
{
	if ( NULL == cUIThemeManager::instance()->DefaultTheme() ) {
		eePRINTL( "cTextureAtlasEditor needs a default theme assigned to work." );
		return;
	}

	mTheme = cUIThemeManager::instance()->DefaultTheme();

	if ( NULL == mUIWindow ) {
		mUIWindow = cUIManager::instance()->MainControl();
		mUIWindow->SetSkinFromTheme( mTheme, "winback" );
	}

	if ( cUIManager::instance()->MainControl() == mUIWindow ) {
		mUIContainer = mUIWindow;
	} else {
		mUIContainer = mUIWindow->Container();
	}

	cUITextBox * TxtBox;
	Uint32 Flags = UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_CLIP_ENABLE | UI_AUTO_SIZE;
	Uint32 InitY = 230;

	CreateTxtBox( eeVector2i( mUIContainer->Size().Width() - 205, 30 ), "SubTexture List:" );

	mSubTextureList = mTheme->CreateListBox( mUIContainer, eeSize( 200, 156 ), eeVector2i( mUIContainer->Size().Width() - 205, 50 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mSubTextureList->Size( mSubTextureList->Size().Width(), mSubTextureList->RowHeight() * 9 + mSubTextureList->PaddingContainer().Top + mSubTextureList->PaddingContainer().Bottom );
	mSubTextureList->AddEventListener( cUIEvent::EventOnItemSelected, cb::Make1( this, &cTextureAtlasEditor::OnSubTextureChange ) );

	CreateTxtBox( eeVector2i( mUIContainer->Size().Width() - 205, InitY ), "Current SubTexture:" );

	InitY +=30;

	mSpinOffX = mTheme->CreateSpinBox( mUIContainer, eeSize( 100, 22 ), eeVector2i(), Flags, 0, false );
	mSpinOffX->MinValue( -32000 );
	mSpinOffX->MaxValue( 32000 );
	mSpinOffX->Pos( mUIContainer->Size().Width() - mSpinOffX->Size().Width() - 10, InitY );
	mSpinOffX->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cTextureAtlasEditor::OnOffXChange ) );

	TxtBox = CreateTxtBox( eeVector2i(), "Offset X:" );
	TxtBox->Pos( mSpinOffX->Pos().x - 10 - TxtBox->Size().Width(), InitY );

	InitY +=30;

	mSpinOffY = mTheme->CreateSpinBox( mUIContainer, eeSize( 100, 22 ), eeVector2i(), Flags, 0, false );
	mSpinOffY->MinValue( -32000 );
	mSpinOffY->MaxValue( 32000 );
	mSpinOffY->Pos( mUIContainer->Size().Width() - mSpinOffY->Size().Width() - 10, InitY );
	mSpinOffY->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cTextureAtlasEditor::OnOffYChange ) );
	TxtBox = CreateTxtBox( eeVector2i(), "Offset Y:" );
	TxtBox->Pos( mSpinOffY->Pos().x - 10 - TxtBox->Size().Width(), InitY );

	InitY +=30;

	mSpinDestW = mTheme->CreateSpinBox( mUIContainer, eeSize( 100, 22 ), eeVector2i(), Flags, 0, false );
	mSpinDestW->MaxValue( 32000 );
	mSpinDestW->Pos( mUIContainer->Size().Width() - mSpinDestW->Size().Width() - 10, InitY );
	mSpinDestW->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cTextureAtlasEditor::OnDestWChange ) );
	TxtBox = CreateTxtBox( eeVector2i(), "Dest. Width:" );
	TxtBox->Pos( mSpinDestW->Pos().x - 10 - TxtBox->Size().Width(), InitY );

	InitY +=30;

	mSpinDestH = mTheme->CreateSpinBox( mUIContainer, eeSize( 100, 22 ), eeVector2i(), Flags, 0, false );
	mSpinDestH->MaxValue( 32000 );
	mSpinDestH->Pos( mUIContainer->Size().Width() - mSpinDestH->Size().Width() - 10, InitY );
	mSpinDestH->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cTextureAtlasEditor::OnDestHChange ) );
	TxtBox = CreateTxtBox( eeVector2i(), "Dest. Height:" );
	TxtBox->Pos( mSpinDestH->Pos().x - 10 - TxtBox->Size().Width(), InitY );

	Uint32 ButFlags = UI_CONTROL_ALIGN_CENTER | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_AUTO_SIZE;

	cUIPushButton * ResetButton = mTheme->CreatePushButton( mUIContainer, eeSize( 120, 22 ), eeVector2i( mUIContainer->Size().Width() - 120 - 5 , mSpinDestH->Pos().y + mSpinDestH->Size().Height() + 8 ), ButFlags );
	ResetButton->Text( "Reset Dest. Size" );
	ResetButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cTextureAtlasEditor::OnResetDestSize ) );

	cUIPushButton * ResetOffsetButton = mTheme->CreatePushButton( mUIContainer, eeSize( 120, 22 ), eeVector2i( ResetButton->Pos().x, ResetButton->Pos().y + ResetButton->Size().Height() + 8 ), ButFlags );
	ResetOffsetButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cTextureAtlasEditor::OnResetOffset ) );
	ResetOffsetButton->Text( "Reset Offset" );

	cUIPushButton * CenterOffsetButton = mTheme->CreatePushButton( mUIContainer, eeSize( 120, 22 ), eeVector2i( ResetOffsetButton->Pos().x, ResetOffsetButton->Pos().y + ResetOffsetButton->Size().Height() + 8 ), ButFlags );
	CenterOffsetButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cTextureAtlasEditor::OnCenterOffset ) );
	CenterOffsetButton->Text( "Centered Offset" );

	cUIPushButton * HBOffsetButton = mTheme->CreatePushButton( mUIContainer, eeSize( 120, 22 ), eeVector2i( CenterOffsetButton->Pos().x, CenterOffsetButton->Pos().y + CenterOffsetButton->Size().Height() + 8 ), ButFlags );
	HBOffsetButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cTextureAtlasEditor::OnHBOffset ) );
	HBOffsetButton->Text( "Half-Bottom Offset" );

	mUIWindow->Title( "Texture Atlas Editor" );
	mUIWindow->AddEventListener( cUIEvent::EventOnWindowClose, cb::Make1( this, &cTextureAtlasEditor::WindowClose ) );

	CreateTGEditor();

	cUIComplexControl::CreateParams Params;
	Params.Parent( mUIContainer );
	Params.PosSet( 0, mWinMenu->Size().Height() );
	Params.SizeSet( 800, 600 );
	Params.Background.Color( eeColorA( 0, 0, 0, 50 ) );
	Params.Flags |= UI_ANCHOR_BOTTOM | UI_ANCHOR_RIGHT | UI_CLIP_ENABLE | UI_BORDER | UI_FILL_BACKGROUND;
	mSubTextureEditor = eeNew( cTextureAtlasSubTextureEditor, ( Params, this ) );
	mSubTextureEditor->Visible( true );
	mSubTextureEditor->Enabled( true );

	mTGEU = eeNew( cUITGEUpdater, ( cUITGEUpdater::CreateParams(), this ) );
}

cTextureAtlasEditor::~cTextureAtlasEditor() {
	eeSAFE_DELETE( mTexturePacker );
	eeSAFE_DELETE( mTextureAtlasLoader );

	if ( !cUIManager::instance()->IsShootingDown() ) {
		mTGEU->Close();
	}
}

void cTextureAtlasEditor::OnResetDestSize( const cUIEvent * Event ) {
	const cUIEventMouse * MouseEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( NULL != mCurSubTexture && MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		eeSize RealSize = mCurSubTexture->RealSize();

		mCurSubTexture->ResetDestSize();

		mSpinDestW->Value( RealSize.Width() );
		mSpinDestH->Value( RealSize.Height() );
	}
}

void cTextureAtlasEditor::OnResetOffset( const cUIEvent * Event ) {
	const cUIEventMouse * MouseEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( NULL != mCurSubTexture && MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		mSpinOffX->Value( 0 );
		mSpinOffY->Value( 0 );
	}
}

void cTextureAtlasEditor::OnCenterOffset( const cUIEvent * Event ) {
	const cUIEventMouse * MouseEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( NULL != mCurSubTexture && MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		eeSize NSize( -( (Int32)mCurSubTexture->DestSize().x / 2 ), -( (Int32)mCurSubTexture->DestSize().y / 2 ) );

		mSpinOffX->Value( NSize.x );
		mSpinOffY->Value( NSize.y );
	}
}

void cTextureAtlasEditor::OnHBOffset( const cUIEvent * Event ) {
	const cUIEventMouse * MouseEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( NULL != mCurSubTexture && MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		eeSize NSize( -( (Int32)mCurSubTexture->DestSize().x / 2 ), -(Int32)mCurSubTexture->DestSize().y );

		mSpinOffX->Value( NSize.x );
		mSpinOffY->Value( NSize.y );
	}
}

void cTextureAtlasEditor::OnOffXChange( const cUIEvent * Event ) {
	if ( NULL != mCurSubTexture ) {
		mCurSubTexture->Offset( eeVector2i( (Int32)mSpinOffX->Value(), mCurSubTexture->Offset().y ) );
	}
}

void cTextureAtlasEditor::OnOffYChange( const cUIEvent * Event ) {
	if ( NULL != mCurSubTexture ) {
		mCurSubTexture->Offset( eeVector2i( mCurSubTexture->Offset().x, (Int32)mSpinOffY->Value() ) );
	}
}

void cTextureAtlasEditor::OnDestWChange( const cUIEvent * Event ) {
	if ( NULL != mCurSubTexture ) {
		mCurSubTexture->DestSize( eeSizef( (Int32)mSpinDestW->Value(), mCurSubTexture->DestSize().y ) );
	}
}

void cTextureAtlasEditor::OnDestHChange( const cUIEvent * Event ) {
	if ( NULL != mCurSubTexture ) {
		mCurSubTexture->DestSize( eeSizef( mCurSubTexture->DestSize().x, (Int32)mSpinDestH->Value() ) );
	}
}

cUITextBox * cTextureAtlasEditor::CreateTxtBox( eeVector2i Pos, const String& Text ) {
	return mTheme->CreateTextBox( Text, mUIContainer, eeSize(), Pos, UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_DRAW_SHADOW | UI_AUTO_SIZE );
}

void cTextureAtlasEditor::WindowClose( const cUIEvent * Event ) {
	if ( mCloseCb.IsSet() )
		mCloseCb();

	eeDelete( this );
}

void cTextureAtlasEditor::CreateTGEditor() {
	CreateWinMenu();
}

void cTextureAtlasEditor::CreateWinMenu() {
	mWinMenu = mTheme->CreateWinMenu( mUIContainer );

	cUIPopUpMenu * PU = mTheme->CreatePopUpMenu();
	PU->Add( "New...", mTheme->GetIconByName( "document-new" ) );
	PU->Add( "Open...", mTheme->GetIconByName( "document-open" ) );
	PU->AddSeparator();
	PU->Add( "Save", mTheme->GetIconByName( "document-save" ) );
	PU->AddSeparator();
	PU->Add( "Close", mTheme->GetIconByName( "document-close" ) );
	PU->AddSeparator();
	PU->Add( "Quit", mTheme->GetIconByName( "quit" ) );

	PU->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &cTextureAtlasEditor::FileMenuClick ) );
	mWinMenu->AddMenuButton( "File", PU );
}

void cTextureAtlasEditor::FileMenuClick( const cUIEvent * Event ) {
	if ( !Event->Ctrl()->IsType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<cUIMenuItem*> ( Event->Ctrl() )->Text();

	if ( "New..." == txt ) {
		eeNew( cTextureAtlasNew, ( cb::Make1( this, &cTextureAtlasEditor::OnTextureAtlasCreate ) ) );
	} else if ( "Open..." == txt ) {
		cUICommonDialog * TGDialog = mTheme->CreateCommonDialog( NULL, eeSize(), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, eeSize(), 255, UI_CDL_DEFAULT_FLAGS, std::string( "*" ) + EE_TEXTURE_ATLAS_EXTENSION );

		TGDialog->Title( "Open Texture Atlas" );
		TGDialog->AddEventListener( cUIEvent::EventOpenFile, cb::Make1( this, &cTextureAtlasEditor::OpenTextureAtlas ) );
		TGDialog->Center();
		TGDialog->Show();
	} else if ( "Save" == txt ) {
		if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->IsLoaded() ) {
			mTextureAtlasLoader->UpdateTextureAtlas();
		}
	} else if ( "Close" == txt ) {
		if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->IsLoaded()  ) {
			cUIMessageBox * MsgBox = mTheme->CreateMessageBox( MSGBOX_OKCANCEL, "Do you really want to close the current texture atlas?\nAll changes will be lost." );
			MsgBox->AddEventListener( cUIEvent::EventMsgBoxConfirmClick, cb::Make1( this, &cTextureAtlasEditor::OnTextureAtlasClose ) );
			MsgBox->Title( "Close Texture Atlas?" );
			MsgBox->Center();
			MsgBox->Show();
		} else {
			OnTextureAtlasClose( NULL );
		}
	} else if ( "Quit" == txt ) {
		if ( mUIWindow == cUIManager::instance()->MainControl() ) {
			cUIManager::instance()->GetWindow()->Close();
		} else {
			mUIWindow->CloseWindow();
		}
	}
}

void cTextureAtlasEditor::OnTextureAtlasCreate( cTexturePacker * TexPacker ) {
	eeSAFE_DELETE( mTexturePacker );
	mTexturePacker = TexPacker;

	eeSAFE_DELETE( mTextureAtlasLoader );

	std::string FPath( FileSystem::FileRemoveExtension( mTexturePacker->GetFilepath() + EE_TEXTURE_ATLAS_EXTENSION ) );

	mTextureAtlasLoader = eeNew( cTextureAtlasLoader, ( FPath, true, cb::Make1( this, &cTextureAtlasEditor::OnTextureAtlasLoaded ) ) );
}

void cTextureAtlasEditor::UpdateControls() {
	if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->IsLoaded()  ) {
		FillSubTextureList();
	}
}

void cTextureAtlasEditor::FillSubTextureList() {
	if ( NULL == mTextureAtlasLoader || NULL == mTextureAtlasLoader->GetTextureAtlas() || !mTextureAtlasLoader->IsLoaded()  )
		return;

	std::list<cSubTexture*>& Res = mTextureAtlasLoader->GetTextureAtlas()->GetResources();

	mSubTextureList->Clear();

	std::vector<String> items;

	for ( std::list<cSubTexture*>::iterator it = Res.begin(); it != Res.end(); it++ ) {
			items.push_back( (*it)->Name() );
	}

	if ( items.size() ) {
		std::sort( items.begin(), items.end() );

		mSubTextureList->AddListBoxItems( items );
		mSubTextureList->SetSelected( 0 );
	}

	mSubTextureList->VerticalScrollBar()->ClickStep( 8.f / (eeFloat)mSubTextureList->Count() );
}

void cTextureAtlasEditor::OnSubTextureChange( const cUIEvent * Event ) {
	if ( NULL != mTextureAtlasLoader && NULL != mTextureAtlasLoader->GetTextureAtlas() ) {
		mCurSubTexture = mTextureAtlasLoader->GetTextureAtlas()->GetByName( mSubTextureList->GetItemSelectedText() );

		if ( NULL != mCurSubTexture ) {
			mSubTextureEditor->SubTexture( mCurSubTexture );
			mSpinOffX->Value( mCurSubTexture->Offset().x );
			mSpinOffY->Value( mCurSubTexture->Offset().y );
			mSpinDestW->Value( mCurSubTexture->DestSize().x );
			mSpinDestH->Value( mCurSubTexture->DestSize().y );
		}
	}
}

void cTextureAtlasEditor::Update() {
	if ( NULL != mTextureAtlasLoader && !mTextureAtlasLoader->IsLoaded() ) {
		mTextureAtlasLoader->Update();
	}
}

void cTextureAtlasEditor::OpenTextureAtlas( const cUIEvent * Event ) {
	cUICommonDialog * CDL = reinterpret_cast<cUICommonDialog*> ( Event->Ctrl() );

	eeSAFE_DELETE( mTextureAtlasLoader );
	bool threaded = true;
	#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	threaded = false;
	#endif

	mTextureAtlasLoader = eeNew( cTextureAtlasLoader, ( CDL->GetFullPath(), threaded, cb::Make1( this, &cTextureAtlasEditor::OnTextureAtlasLoaded ) ) );
}

void cTextureAtlasEditor::OnTextureAtlasLoaded( cTextureAtlasLoader * TGLoader ) {
	if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->IsLoaded() ) {
		UpdateControls();
	}
}

void cTextureAtlasEditor::SaveTextureAtlas( const cUIEvent * Event ) {
	if ( NULL != mTextureAtlasLoader && mTextureAtlasLoader->IsLoaded() ) {
		mTextureAtlasLoader->UpdateTextureAtlas();
	}
}

void cTextureAtlasEditor::OnTextureAtlasClose( const cUIEvent * Event ) {
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
