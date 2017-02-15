#include <eepp/gaming/mapeditor/mapobjectproperties.hpp>
#include <eepp/gaming/mapeditor/mapeditor.hpp>

namespace EE { namespace Gaming { namespace Private {

MapObjectProperties::MapObjectProperties( GameObjectObject * Obj ) :
	mUITheme( NULL ),
	mUIWindow( NULL ),
	mGenGrid( NULL ),
	mObj( Obj )
{
	if ( NULL == mObj ) {
		eeDelete( this );
		return;
	}

	mUITheme		= UIThemeManager::instance()->DefaultTheme();

	if ( NULL == mUITheme )
		return;

	mUIWindow	= mUITheme->CreateWindow( NULL, Sizei( 500, 500 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL, Sizei( 500, 500 ) );
	mUIWindow->AddEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &MapObjectProperties::WindowClose ) );
	mUIWindow->Title( "Object Properties" );

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	UITextBox * Txt = mUITheme->CreateTextBox( "Object name:", mUIWindow->Container(), Sizei(), Vector2i( 50, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	mUIInput = mUITheme->CreateTextInput( mUIWindow->Container(), Sizei( 120, 22 ), Vector2i( Txt->Pos().x + DistFromTitle, Txt->Pos().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE, true, 64 );
	mUIInput->Text( mObj->Name() );
	mUIInput->AddEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &MapObjectProperties::OKClick ) );

	UITextBox * Txt2 = mUITheme->CreateTextBox( "Object type:", mUIWindow->Container(), Sizei(), Vector2i( 50+192, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	mUIInput2 = mUITheme->CreateTextInput( mUIWindow->Container(), Sizei( 120, 22 ), Vector2i( Txt2->Pos().x + DistFromTitle, Txt2->Pos().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE, true, 64 );
	mUIInput2->Text( mObj->TypeName() );
	mUIInput2->AddEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &MapObjectProperties::OKClick ) );

	Uint32 TxtBoxFlags = UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_HALIGN_CENTER | UI_VALIGN_CENTER;
	mUITheme->CreateTextBox( "Property Name", mUIWindow->Container(), Sizei(192, 24), Vector2i( 50, mUIInput->Pos().y + mUIInput->Size().height() + 12 ), TxtBoxFlags );
	UITextBox * TxtBox = mUITheme->CreateTextBox( "Property Value", mUIWindow->Container(), Sizei(192, 24), Vector2i( 50+192, mUIInput->Pos().y + mUIInput->Size().height() + 12 ), TxtBoxFlags );

	UIPushButton * OKButton = mUITheme->CreatePushButton( mUIWindow->Container(), Sizei( 80, 22 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->GetIconByName( "ok" ) );
	OKButton->Pos( mUIWindow->Container()->Size().width() - OKButton->Size().width() - 4, mUIWindow->Container()->Size().height() - OKButton->Size().height() - 4 );
	OKButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapObjectProperties::OKClick ) );

	OKButton->Text( "OK" );

	UIPushButton * CancelButton = mUITheme->CreatePushButton( mUIWindow->Container(), OKButton->Size(), Vector2i( OKButton->Pos().x - OKButton->Size().width() - 4, OKButton->Pos().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->GetIconByName( "cancel" ) );
	CancelButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapObjectProperties::CancelClick ) );
	CancelButton->Text( "Cancel" );

	UIGenericGrid::CreateParams GridParams;
	GridParams.Parent( mUIWindow->Container() );
	GridParams.PosSet( 50, TxtBox->Pos().y + TxtBox->Size().height() );
	GridParams.SizeSet( 400, 350 );
	GridParams.Flags = UI_AUTO_PADDING;
	GridParams.RowHeight = 24;
	GridParams.CollumnsCount = 5;
	mGenGrid = eeNew( UIGenericGrid, ( GridParams ) );
	mGenGrid->Visible( true );
	mGenGrid->Enabled( true );
	mGenGrid->CollumnWidth( 0, 10 );
	mGenGrid->CollumnWidth( 1, 175 );
	mGenGrid->CollumnWidth( 2, 10 );
	mGenGrid->CollumnWidth( 3, 175 );
	mGenGrid->CollumnWidth( 4, 10 );

	Vector2i Pos( mGenGrid->Pos().x + mGenGrid->Size().width() + 10, mGenGrid->Pos().y );

	UIPushButton * AddButton = mUITheme->CreatePushButton( mUIWindow->Container(), Sizei(24,21), Pos, UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mUITheme->GetIconByName( "add" ) );
	AddButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapObjectProperties::AddCellClick ) );

	if ( NULL == AddButton->Icon()->SubTexture() )
		AddButton->Text( "+" );

	Pos.y += AddButton->Size().height() + 5;

	UIPushButton * RemoveButton = mUITheme->CreatePushButton( mUIWindow->Container(), Sizei(24,21), Pos, UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mUITheme->GetIconByName( "remove" )  );
	RemoveButton->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapObjectProperties::RemoveCellClick ) );

	if ( NULL == RemoveButton->Icon()->SubTexture() )
		RemoveButton->Text( "-" );

	CreateGridElems();

	mUIWindow->Center();
	mUIWindow->Show();
}

MapObjectProperties::~MapObjectProperties() {
}

void MapObjectProperties::SaveProperties() {
	mObj->ClearProperties();

	for ( Uint32 i = 0; i < mGenGrid->Count(); i++ ) {
		UIGridCell * Cell = mGenGrid->GetCell( i );

		UITextInput * Input = reinterpret_cast<UITextInput*>( Cell->Cell( 1 ) );
		UITextInput * Input2 = reinterpret_cast<UITextInput*>( Cell->Cell( 3 ) );

		if ( NULL != Cell && Input->Text().size() && Input2->Text().size() ) {
			mObj->AddProperty(	Input->Text(), Input2->Text() );
		}
	}
}

void MapObjectProperties::LoadProperties() {
	GameObjectObject::PropertiesMap& Proper = mObj->GetProperties();

	for ( GameObjectObject::PropertiesMap::iterator it = Proper.begin(); it != Proper.end(); it++ ) {
		UIGridCell * Cell = CreateCell();

		UITextInput * Input = reinterpret_cast<UITextInput*>( Cell->Cell( 1 ) );
		UITextInput * Input2 = reinterpret_cast<UITextInput*>( Cell->Cell( 3 ) );

		Input->Text( it->first );
		Input2->Text( it->second );

		mGenGrid->Add( Cell );
	}
}

void MapObjectProperties::OKClick( const UIEvent * Event ) {
	SaveProperties();

	mObj->Name( mUIInput->Text().toUtf8() );
	mObj->TypeName( mUIInput2->Text().toUtf8() );

	mUIWindow->CloseWindow();
}

void MapObjectProperties::CancelClick( const UIEvent * Event ) {
	mUIWindow->CloseWindow();
}

void MapObjectProperties::WindowClose( const UIEvent * Event ) {
	eeDelete( this );
}

void MapObjectProperties::AddCellClick( const UIEvent * Event ) {
	mGenGrid->Add( CreateCell() );

	Uint32 Index = mGenGrid->GetItemSelectedIndex();

	if ( eeINDEX_NOT_FOUND == Index ) {
		mGenGrid->GetCell( 0 )->Select();
	}
}

void MapObjectProperties::RemoveCellClick( const UIEvent * Event ) {
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

void MapObjectProperties::CreateGridElems() {
	LoadProperties();

	if ( 0 == mGenGrid->Count() ) {
		AddCellClick( NULL );
	} else {
		mGenGrid->GetCell( 0 )->Select();
	}
}

UIGridCell * MapObjectProperties::CreateCell() {
	UIGridCell::CreateParams CellParams;
	CellParams.Parent( mGenGrid->Container() );

	UITextInput::CreateParams TxtInputParams;
	TxtInputParams.Flags = UI_CLIP_ENABLE | UI_VALIGN_CENTER | UI_AUTO_PADDING | UI_TEXT_SELECTION_ENABLED;
	TxtInputParams.MaxLength = LAYER_NAME_SIZE;

	UIComplexControl::CreateParams CControl;

	UIGridCell * Cell			= eeNew( UIGridCell, ( CellParams ) );
	UITextInput * TxtInput		= eeNew( UITextInput, ( TxtInputParams ) );
	UITextInput * TxtInput2	= eeNew( UITextInput, ( TxtInputParams ) );

	Cell->Cell( 0, eeNew( UIComplexControl, ( CControl ) ) );

	Cell->Cell( 1, TxtInput );

	Cell->Cell( 2, eeNew( UIComplexControl, ( CControl ) ) );

	Cell->Cell( 3, TxtInput2 );

	Cell->Cell( 4, eeNew( UIComplexControl, ( CControl ) ) );

	return Cell;
}

}}}

