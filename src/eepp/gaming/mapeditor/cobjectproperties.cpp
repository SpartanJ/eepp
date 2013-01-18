#include <eepp/gaming/mapeditor/cobjectproperties.hpp>
#include <eepp/gaming/mapeditor/cmapeditor.hpp>

namespace EE { namespace Gaming { namespace MapEditor {

cObjectProperties::cObjectProperties( cGameObjectObject * Obj ) :
	mUITheme( NULL ),
	mUIWindow( NULL ),
	mGenGrid( NULL ),
	mObj( Obj )
{
	if ( NULL == mObj ) {
		eeDelete( this );
		return;
	}

	mUITheme		= cUIThemeManager::instance()->DefaultTheme();

	if ( NULL == mUITheme )
		return;

	mUIWindow	= mUITheme->CreateWindow( NULL, eeSize( 500, 500 ), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL, eeSize( 500, 500 ) );
	mUIWindow->AddEventListener( cUIEvent::EventOnWindowClose, cb::Make1( this, &cObjectProperties::WindowClose ) );
	mUIWindow->Title( "Object Properties" );

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	cUITextBox * Txt = mUITheme->CreateTextBox( "Object name:", mUIWindow->Container(), eeSize(), eeVector2i( 50, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	mUIInput = mUITheme->CreateTextInput( mUIWindow->Container(), eeSize( 120, 22 ), eeVector2i( Txt->Pos().x + DistFromTitle, Txt->Pos().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE, true, 64 );
	mUIInput->Text( mObj->Name() );
	mUIInput->AddEventListener( cUIEvent::EventOnPressEnter, cb::Make1( this, &cObjectProperties::OKClick ) );

	cUITextBox * Txt2 = mUITheme->CreateTextBox( "Object type:", mUIWindow->Container(), eeSize(), eeVector2i( 50+192, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	mUIInput2 = mUITheme->CreateTextInput( mUIWindow->Container(), eeSize( 120, 22 ), eeVector2i( Txt2->Pos().x + DistFromTitle, Txt2->Pos().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE, true, 64 );
	mUIInput2->Text( mObj->TypeName() );
	mUIInput2->AddEventListener( cUIEvent::EventOnPressEnter, cb::Make1( this, &cObjectProperties::OKClick ) );

	Uint32 TxtBoxFlags = UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_HALIGN_CENTER | UI_VALIGN_CENTER;
	mUITheme->CreateTextBox( "Property Name", mUIWindow->Container(), eeSize(192, 24), eeVector2i( 50, mUIInput->Pos().y + mUIInput->Size().Height() + 12 ), TxtBoxFlags );
	cUITextBox * TxtBox = mUITheme->CreateTextBox( "Property Value", mUIWindow->Container(), eeSize(192, 24), eeVector2i( 50+192, mUIInput->Pos().y + mUIInput->Size().Height() + 12 ), TxtBoxFlags );

	cUIPushButton * OKButton = mUITheme->CreatePushButton( mUIWindow->Container(), eeSize( 80, 22 ), eeVector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->GetIconByName( "ok" ) );
	OKButton->Pos( mUIWindow->Container()->Size().Width() - OKButton->Size().Width() - 4, mUIWindow->Container()->Size().Height() - OKButton->Size().Height() - 4 );
	OKButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cObjectProperties::OKClick ) );

	OKButton->Text( "OK" );

	cUIPushButton * CancelButton = mUITheme->CreatePushButton( mUIWindow->Container(), OKButton->Size(), eeVector2i( OKButton->Pos().x - OKButton->Size().Width() - 4, OKButton->Pos().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->GetIconByName( "cancel" ) );
	CancelButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cObjectProperties::CancelClick ) );
	CancelButton->Text( "Cancel" );

	cUIGenericGrid::CreateParams GridParams;
	GridParams.Parent( mUIWindow->Container() );
	GridParams.PosSet( 50, TxtBox->Pos().y + TxtBox->Size().Height() );
	GridParams.SizeSet( 400, 350 );
	GridParams.Flags = UI_AUTO_PADDING;
	GridParams.RowHeight = 24;
	GridParams.CollumnsCount = 5;
	mGenGrid = eeNew( cUIGenericGrid, ( GridParams ) );
	mGenGrid->Visible( true );
	mGenGrid->Enabled( true );
	mGenGrid->CollumnWidth( 0, 10 );
	mGenGrid->CollumnWidth( 1, 175 );
	mGenGrid->CollumnWidth( 2, 10 );
	mGenGrid->CollumnWidth( 3, 175 );
	mGenGrid->CollumnWidth( 4, 10 );

	eeVector2i Pos( mGenGrid->Pos().x + mGenGrid->Size().Width() + 10, mGenGrid->Pos().y );

	cUIPushButton * AddButton = mUITheme->CreatePushButton( mUIWindow->Container(), eeSize(24,21), Pos, UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mUITheme->GetIconByName( "add" ) );
	AddButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cObjectProperties::AddCellClick ) );

	if ( NULL == AddButton->Icon()->SubTexture() )
		AddButton->Text( "+" );

	Pos.y += AddButton->Size().Height() + 5;

	cUIPushButton * RemoveButton = mUITheme->CreatePushButton( mUIWindow->Container(), eeSize(24,21), Pos, UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mUITheme->GetIconByName( "remove" )  );
	RemoveButton->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cObjectProperties::RemoveCellClick ) );

	if ( NULL == RemoveButton->Icon()->SubTexture() )
		RemoveButton->Text( "-" );

	CreateGridElems();

	mUIWindow->Center();
	mUIWindow->Show();
}

cObjectProperties::~cObjectProperties() {
}

void cObjectProperties::SaveProperties() {
	mObj->ClearProperties();

	for ( Uint32 i = 0; i < mGenGrid->Count(); i++ ) {
		cUIGridCell * Cell = mGenGrid->GetCell( i );

		cUITextInput * Input = reinterpret_cast<cUITextInput*>( Cell->Cell( 1 ) );
		cUITextInput * Input2 = reinterpret_cast<cUITextInput*>( Cell->Cell( 3 ) );

		if ( NULL != Cell && Input->Text().size() && Input2->Text().size() ) {
			mObj->AddProperty(	Input->Text(), Input2->Text() );
		}
	}
}

void cObjectProperties::LoadProperties() {
	cGameObjectObject::PropertiesMap& Proper = mObj->GetProperties();

	for ( cGameObjectObject::PropertiesMap::iterator it = Proper.begin(); it != Proper.end(); it++ ) {
		cUIGridCell * Cell = CreateCell();

		cUITextInput * Input = reinterpret_cast<cUITextInput*>( Cell->Cell( 1 ) );
		cUITextInput * Input2 = reinterpret_cast<cUITextInput*>( Cell->Cell( 3 ) );

		Input->Text( it->first );
		Input2->Text( it->second );

		mGenGrid->Add( Cell );
	}
}

void cObjectProperties::OKClick( const cUIEvent * Event ) {
	SaveProperties();

	mObj->Name( mUIInput->Text().ToUtf8() );
	mObj->TypeName( mUIInput2->Text().ToUtf8() );

	mUIWindow->CloseWindow();
}

void cObjectProperties::CancelClick( const cUIEvent * Event ) {
	mUIWindow->CloseWindow();
}

void cObjectProperties::WindowClose( const cUIEvent * Event ) {
	eeDelete( this );
}

void cObjectProperties::AddCellClick( const cUIEvent * Event ) {
	mGenGrid->Add( CreateCell() );

	Uint32 Index = mGenGrid->GetItemSelectedIndex();

	if ( eeINDEX_NOT_FOUND == Index ) {
		mGenGrid->GetCell( 0 )->Select();
	}
}

void cObjectProperties::RemoveCellClick( const cUIEvent * Event ) {
	Uint32 Index = mGenGrid->GetItemSelectedIndex();

	if ( eeINDEX_NOT_FOUND != Index ) {
		mGenGrid->Remove( Index );

		if ( Index < mGenGrid->Count() ) {
			mGenGrid->GetCell( Index )->Select();
		} else {
			if ( mGenGrid->Count() ) {
				if ( Index > 0 )
					mGenGrid->GetCell( Index - 1 )->Select();
			}
		}
	}
}

void cObjectProperties::CreateGridElems() {
	LoadProperties();

	if ( 0 == mGenGrid->Count() ) {
		AddCellClick( NULL );
	} else {
		mGenGrid->GetCell( 0 )->Select();
	}
}

cUIGridCell * cObjectProperties::CreateCell() {
	cUIGridCell::CreateParams CellParams;
	CellParams.Parent( mGenGrid->Container() );

	cUITextInput::CreateParams TxtInputParams;
	TxtInputParams.Flags = UI_CLIP_ENABLE | UI_VALIGN_CENTER | UI_AUTO_PADDING;
	TxtInputParams.MaxLength = LAYER_NAME_SIZE;

	cUIComplexControl::CreateParams CControl;

	cUIGridCell * Cell			= eeNew( cUIGridCell, ( CellParams ) );
	cUITextInput * TxtInput		= eeNew( cUITextInput, ( TxtInputParams ) );
	cUITextInput * TxtInput2	= eeNew( cUITextInput, ( TxtInputParams ) );

	Cell->Cell( 0, eeNew( cUIComplexControl, ( CControl ) ) );

	Cell->Cell( 1, TxtInput );

	Cell->Cell( 2, eeNew( cUIComplexControl, ( CControl ) ) );

	Cell->Cell( 3, TxtInput2 );

	Cell->Cell( 4, eeNew( cUIComplexControl, ( CControl ) ) );

	return Cell;
}

}}}

