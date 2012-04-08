#include "ctexturegroupeditor.hpp"
#include "ctexturegroupnew.hpp"
#include "../cuimanager.hpp"
#include "../cuipopupmenu.hpp"
#include "../cuimenuitem.hpp"
#include "../cuicommondialog.hpp"
#include "../cuimessagebox.hpp"

namespace EE { namespace UI { namespace Tools {

cTextureGroupEditor::cTextureGroupEditor( cUIWindow * AttatchTo, const TGEditorCloseCb& callback ) :
	mUIWindow( AttatchTo ),
	mCloseCb( callback ),
	mTexturePacker( NULL ),
	mTextureGroupLoader( NULL ),
	mCurShape( NULL )
{
	if ( NULL == cUIThemeManager::instance()->DefaultTheme() ) {
		eePRINT( "cTextureGroupEditor needs a default theme seted to work." );
		return;
	}

	mTheme = cUIThemeManager::instance()->DefaultTheme();

	if ( NULL == mUIWindow ) {
		mUIWindow = cUIManager::instance()->MainControl();
	}

	if ( cUIManager::instance()->MainControl() == mUIWindow ) {
		mUIContainer = mUIWindow;
	} else {
		mUIContainer = mUIWindow->Container();
	}

	cUITextBox * TxtBox;
	Uint32 Flags = UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_CLIP_ENABLE | UI_AUTO_SIZE;
	Uint32 InitY = 230;

	CreateTxtBox( eeVector2i( mUIContainer->Size().Width() - 205, 30 ), "Shape List:" );

	mShapeList = mTheme->CreateListBox( mUIContainer, eeSize( 200, 156 ), eeVector2i( mUIContainer->Size().Width() - 205, 50 ), UI_CONTROL_DEFAULT_ALIGN | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mShapeList->Size( mShapeList->Size().Width(), mShapeList->RowHeight() * 9 + mShapeList->PaddingContainer().Top + mShapeList->PaddingContainer().Bottom );
	mShapeList->AddEventListener( cUIEvent::EventOnItemSelected, cb::Make1( this, &cTextureGroupEditor::OnShapeChange ) );

	CreateTxtBox( eeVector2i( mUIContainer->Size().Width() - 205, InitY ), "Current Shape:" );

	InitY +=30;

	mSpinOffX = mTheme->CreateSpinBox( mUIContainer, eeSize( 100, 22 ), eeVector2i(), Flags, 0, false );
	mSpinOffX->MinValue( -32000 );
	mSpinOffX->MaxValue( 32000 );
	mSpinOffX->Pos( mUIContainer->Size().Width() - mSpinOffX->Size().Width() - 10, InitY );
	mSpinOffX->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cTextureGroupEditor::OnOffXChange ) );

	TxtBox = CreateTxtBox( eeVector2i(), "Offset X:" );
	TxtBox->Pos( mSpinOffX->Pos().x - 10 - TxtBox->Size().Width(), InitY );

	InitY +=30;

	mSpinOffY = mTheme->CreateSpinBox( mUIContainer, eeSize( 100, 22 ), eeVector2i(), Flags, 0, false );
	mSpinOffY->MinValue( -32000 );
	mSpinOffY->MaxValue( 32000 );
	mSpinOffY->Pos( mUIContainer->Size().Width() - mSpinOffY->Size().Width() - 10, InitY );
	mSpinOffY->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cTextureGroupEditor::OnOffYChange ) );
	TxtBox = CreateTxtBox( eeVector2i(), "Offset Y:" );
	TxtBox->Pos( mSpinOffY->Pos().x - 10 - TxtBox->Size().Width(), InitY );

	InitY +=30;

	mSpinDestW = mTheme->CreateSpinBox( mUIContainer, eeSize( 100, 22 ), eeVector2i(), Flags, 0, false );
	mSpinDestW->MaxValue( 32000 );
	mSpinDestW->Pos( mUIContainer->Size().Width() - mSpinDestW->Size().Width() - 10, InitY );
	mSpinDestW->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cTextureGroupEditor::OnDestWChange ) );
	TxtBox = CreateTxtBox( eeVector2i(), "Dest. Width:" );
	TxtBox->Pos( mSpinDestW->Pos().x - 10 - TxtBox->Size().Width(), InitY );

	InitY +=30;

	mSpinDestH = mTheme->CreateSpinBox( mUIContainer, eeSize( 100, 22 ), eeVector2i(), Flags, 0, false );
	mSpinDestH->MaxValue( 32000 );
	mSpinDestH->Pos( mUIContainer->Size().Width() - mSpinDestH->Size().Width() - 10, InitY );
	mSpinDestH->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cTextureGroupEditor::OnDestHChange ) );
	TxtBox = CreateTxtBox( eeVector2i(), "Dest. Height:" );
	TxtBox->Pos( mSpinDestH->Pos().x - 10 - TxtBox->Size().Width(), InitY );

	Uint32 ButFlags = UI_CONTROL_ALIGN_CENTER | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_AUTO_SIZE;

	cUIPushButton * ResetButton = mTheme->CreatePushButton( mUIContainer, eeSize( 120, 22 ), eeVector2i( mUIContainer->Size().Width() - 120 - 5 , mSpinDestH->Pos().y + mSpinDestH->Size().Height() + 8 ), ButFlags );
	ResetButton->Text( "Reset Dest. Size" );
	ResetButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cTextureGroupEditor::OnResetDestSize ) );

	cUIPushButton * ResetOffsetButton = mTheme->CreatePushButton( mUIContainer, eeSize( 120, 22 ), eeVector2i( ResetButton->Pos().x, ResetButton->Pos().y + ResetButton->Size().Height() + 8 ), ButFlags );
	ResetOffsetButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cTextureGroupEditor::OnResetOffset ) );
	ResetOffsetButton->Text( "Reset Offset" );

	cUIPushButton * CenterOffsetButton = mTheme->CreatePushButton( mUIContainer, eeSize( 120, 22 ), eeVector2i( ResetOffsetButton->Pos().x, ResetOffsetButton->Pos().y + ResetOffsetButton->Size().Height() + 8 ), ButFlags );
	CenterOffsetButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cTextureGroupEditor::OnCenterOffset ) );
	CenterOffsetButton->Text( "Centered Offset" );

	cUIPushButton * HBOffsetButton = mTheme->CreatePushButton( mUIContainer, eeSize( 120, 22 ), eeVector2i( CenterOffsetButton->Pos().x, CenterOffsetButton->Pos().y + CenterOffsetButton->Size().Height() + 8 ), ButFlags );
	HBOffsetButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cTextureGroupEditor::OnHBOffset ) );
	HBOffsetButton->Text( "Half-Bottom Offset" );

	mUIWindow->Title( "Texture Group Editor" );
	mUIWindow->AddEventListener( cUIEvent::EventOnWindowClose, cb::Make1( this, &cTextureGroupEditor::WindowClose ) );

	CreateTGEditor();

	cUIComplexControl::CreateParams Params;
	Params.Parent( mUIContainer );
	Params.PosSet( 0, mWinMenu->Size().Height() );
	Params.SizeSet( 800, 600 );
	Params.Background.Color( eeColorA( 0, 0, 0, 50 ) );
	Params.Flags |= UI_ANCHOR_BOTTOM | UI_ANCHOR_RIGHT | UI_CLIP_ENABLE | UI_BORDER | UI_FILL_BACKGROUND;
	mShapeEditor = eeNew( cTextureGroupShapeEditor, ( Params, this ) );
	mShapeEditor->Visible( true );
	mShapeEditor->Enabled( true );

	mTGEU = eeNew( cUITGEUpdater, ( cUITGEUpdater::CreateParams(), this ) );
}

cTextureGroupEditor::~cTextureGroupEditor() {
	eeSAFE_DELETE( mTexturePacker );
	eeSAFE_DELETE( mTextureGroupLoader );

	if ( !cUIManager::instance()->IsShootingDown() ) {
		mTGEU->Close();
	}
}

void cTextureGroupEditor::OnResetDestSize( const cUIEvent * Event ) {
	const cUIEventMouse * MouseEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( NULL != mCurShape && MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		eeSize RealSize = mCurShape->RealSize();

		mCurShape->DestWidth( RealSize.Width() );
		mCurShape->DestHeight( RealSize.Height() );
		mSpinDestW->Value( RealSize.Width() );
		mSpinDestH->Value( RealSize.Height() );
	}
}

void cTextureGroupEditor::OnResetOffset( const cUIEvent * Event ) {
	const cUIEventMouse * MouseEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( NULL != mCurShape && MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		mSpinOffX->Value( 0 );
		mSpinOffY->Value( 0 );
	}
}

void cTextureGroupEditor::OnCenterOffset( const cUIEvent * Event ) {
	const cUIEventMouse * MouseEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( NULL != mCurShape && MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		eeSize NSize( -( (Int32)mCurShape->DestWidth() / 2 ), -( (Int32)mCurShape->DestHeight() / 2 ) );

		mSpinOffX->Value( NSize.x );
		mSpinOffY->Value( NSize.y );
	}
}

void cTextureGroupEditor::OnHBOffset( const cUIEvent * Event ) {
	const cUIEventMouse * MouseEvent = reinterpret_cast<const cUIEventMouse*> ( Event );

	if ( NULL != mCurShape && MouseEvent->Flags() & EE_BUTTON_LMASK ) {
		eeSize NSize( -( (Int32)mCurShape->DestWidth() / 2 ), -(Int32)mCurShape->DestHeight() );

		mSpinOffX->Value( NSize.x );
		mSpinOffY->Value( NSize.y );
	}
}

void cTextureGroupEditor::OnOffXChange( const cUIEvent * Event ) {
	if ( NULL != mCurShape ) {
		mCurShape->OffsetX( (Int32)mSpinOffX->Value() );
	}
}

void cTextureGroupEditor::OnOffYChange( const cUIEvent * Event ) {
	if ( NULL != mCurShape ) {
		mCurShape->OffsetY( (Int32)mSpinOffY->Value() );
	}
}

void cTextureGroupEditor::OnDestWChange( const cUIEvent * Event ) {
	if ( NULL != mCurShape ) {
		mCurShape->DestWidth( (Int32)mSpinDestW->Value() );
	}
}

void cTextureGroupEditor::OnDestHChange( const cUIEvent * Event ) {
	if ( NULL != mCurShape ) {
		mCurShape->DestHeight( (Int32)mSpinDestH->Value() );
	}
}

cUITextBox * cTextureGroupEditor::CreateTxtBox( eeVector2i Pos, const String& Text ) {
	return mTheme->CreateTextBox( Text, mUIContainer, eeSize(), Pos, UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_DRAW_SHADOW | UI_AUTO_SIZE );
}

void cTextureGroupEditor::WindowClose( const cUIEvent * Event ) {
	if ( mCloseCb.IsSet() )
		mCloseCb();

	eeDelete( this );
}

void cTextureGroupEditor::CreateTGEditor() {
	CreateWinMenu();
}

void cTextureGroupEditor::CreateWinMenu() {
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

	PU->AddEventListener( cUIEvent::EventOnItemClicked, cb::Make1( this, &cTextureGroupEditor::FileMenuClick ) );
	mWinMenu->AddMenuButton( "File", PU );
}

void cTextureGroupEditor::FileMenuClick( const cUIEvent * Event ) {
	if ( !Event->Ctrl()->IsType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<cUIMenuItem*> ( Event->Ctrl() )->Text();

	if ( "New..." == txt ) {
		eeNew( cTextureGroupNew, ( cb::Make1( this, &cTextureGroupEditor::OnTextureGroupCreate ) ) );
	} else if ( "Open..." == txt ) {
		cUICommonDialog * TGDialog = mTheme->CreateCommonDialog( NULL, eeSize(), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL, eeSize(), 255, UI_CDL_DEFAULT_FLAGS, "*.etg" );

		TGDialog->Title( "Open Texture Group" );
		TGDialog->AddEventListener( cUIEvent::EventOpenFile, cb::Make1( this, &cTextureGroupEditor::OpenTextureGroup ) );
		TGDialog->Center();
		TGDialog->Show();
	} else if ( "Save" == txt ) {
		if ( NULL != mTextureGroupLoader && mTextureGroupLoader->IsLoaded() ) {
			mTextureGroupLoader->UpdateTextureAtlas();
		}
	} else if ( "Close" == txt ) {
		if ( NULL != mTextureGroupLoader && mTextureGroupLoader->IsLoaded()  ) {
			cUIMessageBox * MsgBox = mTheme->CreateMessageBox( MSGBOX_OKCANCEL, "Do you really want to close the current texture group?\nAll changes will be lost." );
			MsgBox->AddEventListener( cUIEvent::EventMsgBoxConfirmClick, cb::Make1( this, &cTextureGroupEditor::OnTextureGroupClose ) );
			MsgBox->Title( "Close Texture Group?" );
			MsgBox->Center();
			MsgBox->Show();
		} else {
			OnTextureGroupClose( NULL );
		}
	} else if ( "Quit" == txt ) {
		if ( mUIWindow == cUIManager::instance()->MainControl() ) {
			cUIManager::instance()->GetWindow()->Close();
		} else {
			mUIWindow->CloseWindow();
		}
	}
}

void cTextureGroupEditor::OnTextureGroupCreate( cTexturePacker * TexPacker ) {
	eeSAFE_DELETE( mTexturePacker );
	mTexturePacker = TexPacker;

	eeSAFE_DELETE( mTextureGroupLoader );

	std::string FPath( FileRemoveExtension( mTexturePacker->GetFilepath() ) + ".etg" );

	mTextureGroupLoader = eeNew( cTextureGroupLoader, ( FPath, true, cb::Make1( this, &cTextureGroupEditor::OnTextureGroupLoaded ) ) );
}

void cTextureGroupEditor::UpdateControls() {
	if ( NULL != mTextureGroupLoader && mTextureGroupLoader->IsLoaded()  ) {
		FillShapeList();
	}
}

void cTextureGroupEditor::FillShapeList() {
	if ( NULL == mTextureGroupLoader || NULL == mTextureGroupLoader->GetShapeGroup() || !mTextureGroupLoader->IsLoaded()  )
		return;

	std::list<cShape*>& Res = mTextureGroupLoader->GetShapeGroup()->GetResources();

	mShapeList->Clear();

	std::vector<String> items;

	for ( std::list<cShape*>::iterator it = Res.begin(); it != Res.end(); it++ ) {
			items.push_back( (*it)->Name() );
	}

	if ( items.size() ) {
		std::sort( items.begin(), items.end() );

		mShapeList->AddListBoxItems( items );
		mShapeList->SetSelected( 0 );
	}

	mShapeList->VerticalScrollBar()->ClickStep( 8.f / (eeFloat)mShapeList->Count() );
}

void cTextureGroupEditor::OnShapeChange( const cUIEvent * Event ) {
	if ( NULL != mTextureGroupLoader && NULL != mTextureGroupLoader->GetShapeGroup() ) {
		mCurShape = mTextureGroupLoader->GetShapeGroup()->GetByName( mShapeList->GetItemSelectedText() );

		if ( NULL != mCurShape ) {
			mShapeEditor->Shape( mCurShape );
			mSpinOffX->Value( mCurShape->OffsetX() );
			mSpinOffY->Value( mCurShape->OffsetY() );
			mSpinDestW->Value( mCurShape->DestWidth() );
			mSpinDestH->Value( mCurShape->DestHeight() );
		}
	}
}

void cTextureGroupEditor::Update() {
	if ( NULL != mTextureGroupLoader && !mTextureGroupLoader->IsLoaded() ) {
		mTextureGroupLoader->Update();
	}
}

void cTextureGroupEditor::OpenTextureGroup( const cUIEvent * Event ) {
	cUICommonDialog * CDL = reinterpret_cast<cUICommonDialog*> ( Event->Ctrl() );

	eeSAFE_DELETE( mTextureGroupLoader );
	mTextureGroupLoader = eeNew( cTextureGroupLoader, ( CDL->GetFullPath(), true, cb::Make1( this, &cTextureGroupEditor::OnTextureGroupLoaded ) ) );
}

void cTextureGroupEditor::OnTextureGroupLoaded( cTextureGroupLoader * TGLoader ) {
	if ( NULL != mTextureGroupLoader && mTextureGroupLoader->IsLoaded() ) {
		UpdateControls();
	}
}

void cTextureGroupEditor::SaveTextureGroup( const cUIEvent * Event ) {
	if ( NULL != mTextureGroupLoader && mTextureGroupLoader->IsLoaded() ) {
		mTextureGroupLoader->UpdateTextureAtlas();
	}
}

void cTextureGroupEditor::OnTextureGroupClose( const cUIEvent * Event ) {
	eeSAFE_DELETE( mTextureGroupLoader );
	mShapeList->Clear();
	mSpinOffX->Value( 0 );
	mSpinOffY->Value( 0 );
	mSpinDestW->Value( 0 );
	mSpinDestH->Value( 0 );
	mShapeEditor->Shape( NULL );
	mCurShape = NULL;
}

}}}
