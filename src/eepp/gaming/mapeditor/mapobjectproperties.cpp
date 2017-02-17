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

	mUITheme		= UIThemeManager::instance()->defaultTheme();

	if ( NULL == mUITheme )
		return;

	mUIWindow	= mUITheme->createWindow( NULL, Sizei( 500, 500 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED, UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL, Sizei( 500, 500 ) );
	mUIWindow->addEventListener( UIEvent::EventOnWindowClose, cb::Make1( this, &MapObjectProperties::WindowClose ) );
	mUIWindow->title( "Object Properties" );

	Int32 InitialY		= 16;
	Int32 DistFromTitle	= 18;

	UITextBox * Txt = mUITheme->createTextBox( "Object name:", mUIWindow->getContainer(), Sizei(), Vector2i( 50, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	mUIInput = mUITheme->createTextInput( mUIWindow->getContainer(), Sizei( 120, 22 ), Vector2i( Txt->position().x + DistFromTitle, Txt->position().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE, true, 64 );
	mUIInput->text( mObj->Name() );
	mUIInput->addEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &MapObjectProperties::OKClick ) );

	UITextBox * Txt2 = mUITheme->createTextBox( "Object type:", mUIWindow->getContainer(), Sizei(), Vector2i( 50+192, InitialY ), UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_AUTO_SIZE );
	mUIInput2 = mUITheme->createTextInput( mUIWindow->getContainer(), Sizei( 120, 22 ), Vector2i( Txt2->position().x + DistFromTitle, Txt2->position().y + DistFromTitle ), UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE, true, 64 );
	mUIInput2->text( mObj->TypeName() );
	mUIInput2->addEventListener( UIEvent::EventOnPressEnter, cb::Make1( this, &MapObjectProperties::OKClick ) );

	Uint32 TxtBoxFlags = UI_CONTROL_DEFAULT_FLAGS | UI_DRAW_SHADOW | UI_HALIGN_CENTER | UI_VALIGN_CENTER;
	mUITheme->createTextBox( "Property Name", mUIWindow->getContainer(), Sizei(192, 24), Vector2i( 50, mUIInput->position().y + mUIInput->size().getHeight() + 12 ), TxtBoxFlags );
	UITextBox * TxtBox = mUITheme->createTextBox( "Property Value", mUIWindow->getContainer(), Sizei(192, 24), Vector2i( 50+192, mUIInput->position().y + mUIInput->size().getHeight() + 12 ), TxtBoxFlags );

	UIPushButton * OKButton = mUITheme->createPushButton( mUIWindow->getContainer(), Sizei( 80, 22 ), Vector2i(), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->getIconByName( "ok" ) );
	OKButton->position( mUIWindow->getContainer()->size().getWidth() - OKButton->size().getWidth() - 4, mUIWindow->getContainer()->size().getHeight() - OKButton->size().getHeight() - 4 );
	OKButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapObjectProperties::OKClick ) );

	OKButton->text( "OK" );

	UIPushButton * CancelButton = mUITheme->createPushButton( mUIWindow->getContainer(), OKButton->size(), Vector2i( OKButton->position().x - OKButton->size().getWidth() - 4, OKButton->position().y ), UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_SIZE, mUITheme->getIconByName( "cancel" ) );
	CancelButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapObjectProperties::CancelClick ) );
	CancelButton->text( "Cancel" );

	UIGenericGrid::CreateParams GridParams;
	GridParams.setParent( mUIWindow->getContainer() );
	GridParams.setPos( 50, TxtBox->position().y + TxtBox->size().getHeight() );
	GridParams.setSize( 400, 350 );
	GridParams.Flags = UI_AUTO_PADDING;
	GridParams.RowHeight = 24;
	GridParams.CollumnsCount = 5;
	mGenGrid = eeNew( UIGenericGrid, ( GridParams ) );
	mGenGrid->visible( true );
	mGenGrid->enabled( true );
	mGenGrid->collumnWidth( 0, 10 );
	mGenGrid->collumnWidth( 1, 175 );
	mGenGrid->collumnWidth( 2, 10 );
	mGenGrid->collumnWidth( 3, 175 );
	mGenGrid->collumnWidth( 4, 10 );

	Vector2i Pos( mGenGrid->position().x + mGenGrid->size().getWidth() + 10, mGenGrid->position().y );

	UIPushButton * AddButton = mUITheme->createPushButton( mUIWindow->getContainer(), Sizei(24,21), Pos, UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mUITheme->getIconByName( "add" ) );
	AddButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapObjectProperties::AddCellClick ) );

	if ( NULL == AddButton->icon()->subTexture() )
		AddButton->text( "+" );

	Pos.y += AddButton->size().getHeight() + 5;

	UIPushButton * RemoveButton = mUITheme->createPushButton( mUIWindow->getContainer(), Sizei(24,21), Pos, UI_CONTROL_ALIGN_CENTER | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP, mUITheme->getIconByName( "remove" )  );
	RemoveButton->addEventListener( UIEvent::EventMouseClick, cb::Make1( this, &MapObjectProperties::RemoveCellClick ) );

	if ( NULL == RemoveButton->icon()->subTexture() )
		RemoveButton->text( "-" );

	CreateGridElems();

	mUIWindow->center();
	mUIWindow->show();
}

MapObjectProperties::~MapObjectProperties() {
}

void MapObjectProperties::SaveProperties() {
	mObj->ClearProperties();

	for ( Uint32 i = 0; i < mGenGrid->getCount(); i++ ) {
		UIGridCell * Cell = mGenGrid->getCell( i );

		UITextInput * Input = reinterpret_cast<UITextInput*>( Cell->cell( 1 ) );
		UITextInput * Input2 = reinterpret_cast<UITextInput*>( Cell->cell( 3 ) );

		if ( NULL != Cell && Input->text().size() && Input2->text().size() ) {
			mObj->AddProperty(	Input->text(), Input2->text() );
		}
	}
}

void MapObjectProperties::LoadProperties() {
	GameObjectObject::PropertiesMap& Proper = mObj->GetProperties();

	for ( GameObjectObject::PropertiesMap::iterator it = Proper.begin(); it != Proper.end(); it++ ) {
		UIGridCell * Cell = CreateCell();

		UITextInput * Input = reinterpret_cast<UITextInput*>( Cell->cell( 1 ) );
		UITextInput * Input2 = reinterpret_cast<UITextInput*>( Cell->cell( 3 ) );

		Input->text( it->first );
		Input2->text( it->second );

		mGenGrid->add( Cell );
	}
}

void MapObjectProperties::OKClick( const UIEvent * Event ) {
	SaveProperties();

	mObj->Name( mUIInput->text().toUtf8() );
	mObj->TypeName( mUIInput2->text().toUtf8() );

	mUIWindow->CloseWindow();
}

void MapObjectProperties::CancelClick( const UIEvent * Event ) {
	mUIWindow->CloseWindow();
}

void MapObjectProperties::WindowClose( const UIEvent * Event ) {
	eeDelete( this );
}

void MapObjectProperties::AddCellClick( const UIEvent * Event ) {
	mGenGrid->add( CreateCell() );

	Uint32 Index = mGenGrid->getItemSelectedIndex();

	if ( eeINDEX_NOT_FOUND == Index ) {
		mGenGrid->getCell( 0 )->select();
	}
}

void MapObjectProperties::RemoveCellClick( const UIEvent * Event ) {
	Uint32 Index = mGenGrid->getItemSelectedIndex();

	if ( eeINDEX_NOT_FOUND != Index ) {
		mGenGrid->remove( Index );

		if ( Index < mGenGrid->getCount() ) {
			mGenGrid->getCell( Index )->select();
		} else {
			if ( mGenGrid->getCount() ) {
				if ( Index > 0 )
					mGenGrid->getCell( Index - 1 )->select();
			}
		}
	}
}

void MapObjectProperties::CreateGridElems() {
	LoadProperties();

	if ( 0 == mGenGrid->getCount() ) {
		AddCellClick( NULL );
	} else {
		mGenGrid->getCell( 0 )->select();
	}
}

UIGridCell * MapObjectProperties::CreateCell() {
	UIGridCell::CreateParams CellParams;
	CellParams.setParent( mGenGrid->getContainer() );

	UITextInput::CreateParams TxtInputParams;
	TxtInputParams.Flags = UI_CLIP_ENABLE | UI_VALIGN_CENTER | UI_AUTO_PADDING | UI_TEXT_SELECTION_ENABLED;
	TxtInputParams.MaxLength = LAYER_NAME_SIZE;

	UIComplexControl::CreateParams CControl;

	UIGridCell * Cell			= eeNew( UIGridCell, ( CellParams ) );
	UITextInput * TxtInput		= eeNew( UITextInput, ( TxtInputParams ) );
	UITextInput * TxtInput2	= eeNew( UITextInput, ( TxtInputParams ) );

	Cell->cell( 0, eeNew( UIComplexControl, ( CControl ) ) );

	Cell->cell( 1, TxtInput );

	Cell->cell( 2, eeNew( UIComplexControl, ( CControl ) ) );

	Cell->cell( 3, TxtInput2 );

	Cell->cell( 4, eeNew( UIComplexControl, ( CControl ) ) );

	return Cell;
}

}}}

