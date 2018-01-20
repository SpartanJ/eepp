#include <eepp/maps/mapeditor/mapeditor.hpp>
#include <eepp/maps/mapeditor/uimapnew.hpp>
#include <eepp/maps/mapeditor/uimaplayernew.hpp>
#include <eepp/maps/mapeditor/uigotypenew.hpp>
#include <eepp/maps/mapeditor/tilemapproperties.hpp>
#include <eepp/maps/mapeditor/maplayerproperties.hpp>
#include <eepp/maps/mapeditor/uimap.hpp>

#include <eepp/maps/tilemaplayer.hpp>
#include <eepp/maps/mapobjectlayer.hpp>
#include <eepp/maps/gameobjectvirtual.hpp>
#include <eepp/maps/gameobjecttextureregion.hpp>
#include <eepp/maps/gameobjecttextureregionex.hpp>
#include <eepp/maps/gameobjectsprite.hpp>
#include <eepp/maps/gameobjectobject.hpp>
#include <eepp/maps/gameobjectpolygon.hpp>
#include <eepp/maps/gameobjectpolyline.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/graphics/globaltextureatlas.hpp>
#include <eepp/graphics/textureatlasloader.hpp>

#include <algorithm>

using namespace EE::Graphics;
using namespace EE::Maps::Private;

#define TAB_CONT_X_DIST 3

namespace EE { namespace Maps {

static UITextView * createTextBox( const String& Text = "", UINode * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, const Uint32& fontStyle = Text::Regular ) {
	UITextView * Ctrl = UITextView::New();
	Ctrl->setFontStyle( fontStyle );
	Ctrl->resetFlags( Flags )->setParent( Parent )->setPosition( Pos )->setSize( Size )->setVisible( true )->setEnabled( false );
	Ctrl->setText( Text );
	return Ctrl;
}

MapEditor::MapEditor( UIWindow * AttatchTo, const MapEditorCloseCb& callback ) :
	mUIWindow( AttatchTo ),
	mTheme( UIThemeManager::instance()->getDefaultTheme() ),
	mUIMap( NULL ),
	mCloseCb( callback ),
	mGOTypeList( NULL ),
	mChkAnim( NULL ),
	mCurLayer( NULL ),
	mLastSelButtonY(2),
	mMouseScrolling( false )
{
	if ( NULL == mTheme ) {
		eePRINTL( "MapEditor needs a default theme seted to work." );
		return;
	}

	if ( NULL == mUIWindow ) {
		mUIWindow = UIManager::instance()->getMainControl();
		mUIWindow->setThemeSkin( mTheme, "winback" );
	}

	if ( UIManager::instance()->getMainControl() == mUIWindow ) {
		mUIContainer = mUIWindow;
	} else {
		mUIContainer = mUIWindow->getContainer();
	}

	mUIWindow->setTitle( "Map Editor" );
	mUIWindow->addEventListener( UIEvent::OnWindowClose, cb::Make1( this, &MapEditor::windowClose ) );

	createME();
}

MapEditor::~MapEditor() {
}

void MapEditor::createME() {
	createWinMenu();

	createETGMenu();

	createUIMap();
}

void MapEditor::createWinMenu() {
	UIWinMenu * WinMenu = UIWinMenu::New();
	WinMenu->setParent( mUIContainer );

	mTileBox = createTextBox( "", mUIContainer, Sizei(), Vector2i(), UI_HALIGN_RIGHT | UI_VALIGN_CENTER | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT );
	mTileBox->setSize( 100, WinMenu->getSize().getHeight() );
	mTileBox->setPosition( Vector2i( mUIContainer->getSize().getWidth() - mTileBox->getSize().getWidth(), 0 ) );
	mTileBox->updateAnchorsDistances();

	UIPopUpMenu * PU1 = UIPopUpMenu::New();
	PU1->setParent( mUIContainer );
	PU1->add( "New...", mTheme->getIconByName( "document-new" ) );
	PU1->add( "Open...", mTheme->getIconByName( "document-open" ) );
	PU1->addSeparator();
	PU1->add( "Save", mTheme->getIconByName( "document-save" ) );
	PU1->add( "Save As...", mTheme->getIconByName( "document-save-as" ) );
	PU1->addSeparator();
	PU1->add( "Close", mTheme->getIconByName( "document-close" ) );
	PU1->addSeparator();
	PU1->add( "Quit", mTheme->getIconByName( "quit" ) );

	PU1->addEventListener( UIEvent::OnItemClicked, cb::Make1( this, &MapEditor::fileMenuClick ) );
	WinMenu->addMenuButton( "File", PU1 );

	UIPopUpMenu * PU3 = UIPopUpMenu::New();
	PU3->setParent( mUIContainer );
	mChkShowGrid = reinterpret_cast<UIMenuCheckBox*>( PU3->getItem( PU3->addCheckBox( "Show Grid" ) ) );

	mChkShowGrid->setActive( true );

	mChkMarkTileOver = reinterpret_cast<UIMenuCheckBox*>( PU3->getItem( PU3->addCheckBox( "Mark Tile Over" ) ) );

	mChkShowBlocked = reinterpret_cast<UIMenuCheckBox*>( PU3->getItem( PU3->addCheckBox( "Show Blocked" ) ) );

	PU3->addSeparator();
	mUIWindow->addShortcut( KEY_KP_PLUS	, KEYMOD_CTRL, reinterpret_cast<UIPushButton*> ( PU3->getItem( PU3->add( "Zoom In", mTheme->getIconByName( "zoom-in" ) ) ) ) );
	mUIWindow->addShortcut( KEY_KP_MINUS, KEYMOD_CTRL, reinterpret_cast<UIPushButton*> ( PU3->getItem( PU3->add( "Zoom Out", mTheme->getIconByName( "zoom-out" ) ) ) ) );
	mUIWindow->addShortcut( KEY_KP0		, KEYMOD_CTRL, reinterpret_cast<UIPushButton*> ( PU3->getItem( PU3->add( "Normal Size", mTheme->getIconByName( "zoom-original" ) ) ) ) );
	PU3->addSeparator();

	PU3->addEventListener( UIEvent::OnItemClicked, cb::Make1( this, &MapEditor::viewMenuClick ) );
	WinMenu->addMenuButton( "View", PU3 );

	UIPopUpMenu * PU4 = UIPopUpMenu::New();
	PU4->setParent( mUIContainer );
	PU4->add( "Properties..." );
	PU4->add( "Resize..." );

	PU4->addEventListener( UIEvent::OnItemClicked, cb::Make1( this, &MapEditor::mapMenuClick ) );
	WinMenu->addMenuButton( "Map", PU4 );

	UIPopUpMenu * PU5 = UIPopUpMenu::New();
	PU5->setParent( mUIContainer );
	PU5->add( "Add Tile Layer..." );
	PU5->add( "Add Object Layer..." );
	PU5->addSeparator();
	PU5->add( "Remove Layer" );
	PU5->addSeparator();
	PU5->add( "Move Layer Up" );
	PU5->add( "Move Layer Down" );
	PU5->addSeparator();
	PU5->add( "Layer Properties..." );
	PU5->addSeparator();

	Uint32 LayerChkBoxIndex = PU5->addCheckBox( "Lights Enabled" );
	mLayerChkLights = reinterpret_cast<UIMenuCheckBox*> ( PU5->getItem( LayerChkBoxIndex ) );

	PU5->addSeparator();

	LayerChkBoxIndex = PU5->addCheckBox( "Visible" );
	mLayerChkVisible = reinterpret_cast<UIMenuCheckBox*> ( PU5->getItem( LayerChkBoxIndex ) );

	PU5->addEventListener( UIEvent::OnItemClicked, cb::Make1( this, &MapEditor::layerMenuClick ) );
	WinMenu->addMenuButton( "Layer", PU5 );

	UIPopUpMenu * PU6 = UIPopUpMenu::New();
	PU6->setParent( mUIContainer );
	PU6->add( "New Texture Atlas..." );
	PU6->add( "Add External Texture Atlas..." );
	WinMenu->addMenuButton( "Atlases", PU6 );
	PU6->addEventListener( UIEvent::OnItemClicked, cb::Make1( this, &MapEditor::mapMenuClick ) );

	mWinContainer = UIWidget::New();
	mWinContainer->setFlags( UI_REPORT_SIZE_CHANGE_TO_CHILDS );
	mWinContainer->setParent( mUIContainer );
	mWinContainer->setPosition( 0, WinMenu->getSize().getHeight() );
	mWinContainer->setSize( mUIContainer->getSize().getWidth(), mUIContainer->getSize().getHeight() - WinMenu->getSize().getHeight() );
	mWinContainer->setAnchors(UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM | UI_ANCHOR_LEFT | UI_ANCHOR_RIGHT  );
}

void MapEditor::createETGMenu() {
	Int32 Width = 200;
	Int32 DistToBorder = 5;
	Int32 ContPosX = mWinContainer->getSize().getWidth() - Width - DistToBorder;
	Int32 DistFromTopMenu = 4;

	mTextureRegionCont = UIWidget::New();
	mTextureRegionCont->setParent( mWinContainer );
	mTextureRegionCont->setSize( Sizei( Width + DistToBorder, mWinContainer->getSize().getHeight() ) );
	mTextureRegionCont->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );

	mLightCont = UIWidget::New();
	mLightCont->setParent( mWinContainer );
	mLightCont->setSize( mTextureRegionCont->getSize() );
	mLightCont->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );

	mObjectCont = UIWidget::New();
	mObjectCont->setParent( mWinContainer );
	mObjectCont->setSize( mTextureRegionCont->getSize() );
	mObjectCont->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );

	mTabWidget = UITabWidget::New();

	mTabWidget->setParent( mWinContainer )->
				setSize( Width + DistToBorder, mWinContainer->getSize().getHeight() - DistFromTopMenu )->
				setPosition( ContPosX, DistFromTopMenu )->resetFlags()->
				setFlags( UI_HALIGN_CENTER | UI_VALIGN_BOTTOM | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM );

	mTabWidget->addEventListener( UIEvent::OnTabSelected, cb::Make1( this, &MapEditor::onTabSelected ) );

	createTabs();

	createLighContainer();

	createTextureRegionContainer( Width );

	createObjectsContainer();
}

void MapEditor::createTabs() {
	mTabWidget->removeAll();
	mTabWidget->add( "Sprites", mTextureRegionCont );

	if ( NULL != mUIMap && NULL != mUIMap->Map() ) {
		if ( mUIMap->Map()->getLightsEnabled() ) {
			mTabWidget->add( "Lights", mLightCont );
		}
	}

	mTabWidget->add( "Objects", mObjectCont );
}

void MapEditor::onTabSelected( const UIEvent * Event ) {
	if ( NULL != mUIMap ) {
		switch ( mTabWidget->getSelectedTabIndex() ) {
			case 0:
				mUIMap->editingDisable();
				break;
			case 1:
				mUIMap->setEditingLights( true );
				break;
			case 2:
				mUIMap->setEditingObjects( true );
				break;
		}
	}
}

void MapEditor::fillGotyList() {
	std::vector<String> items;
	items.push_back( "TextureRegion" );
	items.push_back( "TextureRegionEx" );
	items.push_back( "Sprite" );
	mGOTypeList->getListBox()->clear();
	mGOTypeList->getListBox()->addListBoxItems( items );
	mGOTypeList->getListBox()->setSelected(0);
}

void MapEditor::createTextureRegionContainer( Int32 Width ) {
	UITextView * Txt;
	Uint32 TxtFlags = UI_CONTROL_DEFAULT_ALIGN | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;

	Txt = createTextBox( "Add Game Object as...", mTextureRegionCont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, 4 ), TxtFlags, Text::Shadow );

	mGOTypeList = UIDropDownList::New();
	mGOTypeList->setFontStyle( Text::Shadow )->setParent( mTextureRegionCont )->setSize( Width - 26, 0 )->setPosition( TAB_CONT_X_DIST, Txt->getPosition().y + Txt->getSize().getHeight() + 4 );

	mGOTypeList->addEventListener( UIEvent::OnItemSelected, cb::Make1( this, &MapEditor::onTypeChange ) );
	fillGotyList();

	mBtnGOTypeAdd = UIPushButton::New();
	mBtnGOTypeAdd->setParent( mTextureRegionCont )->setSize( 24, mGOTypeList->getSize().getHeight() )
				 ->setPosition(  mGOTypeList->getPosition().x + mGOTypeList->getSize().getWidth() + 2, mGOTypeList->getPosition().y );
	mBtnGOTypeAdd->setIcon( mTheme->getIconByName( "add" ) )->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mBtnGOTypeAdd->setTooltipText( "Adds a new game object type\nunknown by the map editor." );
	mBtnGOTypeAdd->addEventListener( UIEvent::MouseClick, cb::Make1( this, &MapEditor::addNewGOType ) );

	if ( NULL == mBtnGOTypeAdd->getIcon()->getDrawable() )
		mBtnGOTypeAdd->setText( "..." );

	Txt = createTextBox( "Layers:", mTextureRegionCont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, mGOTypeList->getPosition().y + mGOTypeList->getSize().getHeight() + 4 ), TxtFlags, Text::Shadow );

	mLayerList = UIDropDownList::New();
	mLayerList->setFontStyle( Text::Shadow )->setParent( mTextureRegionCont )->setSize( Width, 0 )->setPosition( TAB_CONT_X_DIST, Txt->getPosition().y + Txt->getSize().getHeight() + 4 );

	mLayerList->addEventListener( UIEvent::OnItemSelected, cb::Make1( this, &MapEditor::onLayerSelect ) );

	Txt = createTextBox( "Game Object Flags:", mTextureRegionCont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, mLayerList->getPosition().y + mLayerList->getSize().getHeight() + 4 ), TxtFlags, Text::Shadow );

	Uint32 ChkFlags = UI_CONTROL_DEFAULT_ALIGN | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;

	mChkMirrored = UICheckBox::New();
	mChkMirrored->setFontStyle( Text::Shadow )->setParent( mTextureRegionCont )->setPosition( TAB_CONT_X_DIST, Txt->getPosition().y + Txt->getSize().getHeight() + 4 )->resetFlags( ChkFlags );
	mChkMirrored->setText( "Mirrored" );
	mChkMirrored->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &MapEditor::chkClickMirrored ) );

	mChkFliped = UICheckBox::New();
	mChkFliped->setFontStyle( Text::Shadow )->setParent( mTextureRegionCont )->setPosition( mChkMirrored->getPosition().x + mChkMirrored->getSize().getWidth() + 32, mChkMirrored->getPosition().y  )->resetFlags( ChkFlags );
	mChkFliped->setText( "Fliped" );
	mChkFliped->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &MapEditor::chkClickFlipped ) );

	mChkBlocked = UICheckBox::New();
	mChkBlocked->setFontStyle( Text::Shadow )->setParent( mTextureRegionCont )->setPosition( mChkMirrored->getPosition().x, mChkMirrored->getPosition().y + mChkMirrored->getSize().getHeight() + 4 )->resetFlags( ChkFlags );
	mChkBlocked->setText( "Blocked" );
	mChkBlocked->setTooltipText( "Blocks the tile occupied by the sprite." );
	mChkBlocked->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &MapEditor::chkClickBlocked ) );

	mChkAnim = UICheckBox::New();
	mChkAnim->setFontStyle( Text::Shadow )->setParent( mTextureRegionCont )->setPosition( mChkFliped->getPosition().x, mChkFliped->getPosition().y + mChkFliped->getSize().getHeight() + 4 )->resetFlags( ChkFlags );
	mChkAnim->setText( "Animated" );
	mChkAnim->setTooltipText( "Indicates if the Sprite is animated." );
	mChkAnim->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &MapEditor::chkClickAnimated ) );

	mChkRot90 = UICheckBox::New();
	mChkRot90->setFontStyle( Text::Shadow )->setParent( mTextureRegionCont )->setPosition( mChkBlocked->getPosition().x, mChkBlocked->getPosition().y + mChkBlocked->getSize().getHeight() + 4 )->resetFlags( ChkFlags );
	mChkRot90->setText( String::fromUtf8( "Rotate 90º" ) );
	mChkRot90->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &MapEditor::chkClickRot90 ) );

	mChkAutoFix = UICheckBox::New();
	mChkAutoFix->setFontStyle( Text::Shadow )->setParent( mTextureRegionCont )->setPosition( mChkAnim->getPosition().x, mChkAnim->getPosition().y + mChkAnim->getSize().getHeight() + 4 )->resetFlags( ChkFlags );
	mChkAutoFix->setText( "AutoFix TilePos" );
	mChkAutoFix->setTooltipText( "In a tiled layer if the sprite is moved,\nit will update the current tile position automatically." );
	mChkAutoFix->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &MapEditor::chkClickAutoFix ) );

	mChkBlendAdd = UICheckBox::New();
	mChkBlendAdd->setFontStyle( Text::Shadow )->setParent( mTextureRegionCont )->setPosition( mChkRot90->getPosition().x, mChkRot90->getPosition().y + mChkRot90->getSize().getHeight() + 4 )->resetFlags( ChkFlags );
	mChkBlendAdd->setText( "Additive Blend" );
	mChkBlendAdd->setTooltipText( "Use additive blend mode." );
	mChkBlendAdd->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &MapEditor::chkClickAutoFix ) );

	Txt = createTextBox( "Game Object Data:", mTextureRegionCont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, mChkBlendAdd->getPosition().y + mChkBlendAdd->getSize().getHeight() + 8 ), TxtFlags, Text::Shadow );

	mChkDI = UICheckBox::New();
	mChkDI->setFontStyle( Text::Shadow )->setParent( mTextureRegionCont )->setPosition( TAB_CONT_X_DIST, Txt->getPosition().y + Txt->getSize().getHeight() + 4 )->resetFlags( ChkFlags );
	mChkDI->setText( "Add as DataId" );
	mChkDI->setTooltipText( "If the resource it's not a sprite,\nyou can reference it with a data id" );
	mChkDI->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &MapEditor::chkClickDI ) );

	mSGCont = UIWidget::New();
	mSGCont->setParent( mTextureRegionCont )->setPosition( TAB_CONT_X_DIST, mChkDI->getPosition().y + mChkDI->getSize().getHeight() + 8 )->setSize( Width, 400 );
	mSGCont->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mSGCont->setEnabled( true );
	mSGCont->setVisible( true );

	Txt = createTextBox( "Texture Atlases:", mSGCont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, 0 ), TxtFlags, Text::Shadow );

	mTextureAtlasesList = UIDropDownList::New();
	mTextureAtlasesList->setFontStyle( Text::Shadow )->setParent( mSGCont )->setSize( Width, 0 )->setPosition( 0, Txt->getPosition().y +Txt->getSize().getHeight() + 4 );
	mTextureAtlasesList->addEventListener( UIEvent::OnItemSelected, cb::Make1( this, &MapEditor::onTextureAtlasChange ) );

	mTextureRegionList = UIListBox::New();
	mTextureRegionList->setParent( mSGCont )
			->setPosition( 0, mTextureAtlasesList->getPosition().y + mTextureAtlasesList->getSize().getHeight() + 4 )
			->setSize( Width, mTextureRegionList->getRowHeight() * 9 + mTextureRegionList->getContainerPadding().Top + mTextureRegionList->getContainerPadding().Bottom );
	mTextureRegionList->setAnchors(UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mTextureRegionList->addEventListener( UIEvent::OnItemSelected, cb::Make1( this, &MapEditor::onTextureRegionChange ) );

	mGfxPreview = UITextureRegion::New();
	mGfxPreview->setScaleType( UIScaleType::FitInside )
			   ->resetFlags( UI_VALIGN_CENTER | UI_HALIGN_CENTER | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP )
			   ->setParent( mSGCont )->setSize( Width, Width )
			   ->setPosition( 0, mTextureRegionList->getPosition().y + mTextureRegionList->getSize().getHeight() + 4 );

	mGfxPreview->setBorderEnabled( true )->setColor( Color( 0, 0, 0, 200 ) );

	mDICont = UIWidget::New();
	mDICont->setParent( mTextureRegionCont )->setPosition( TAB_CONT_X_DIST, mChkDI->getPosition().y + mChkDI->getSize().getHeight() + 8 );
	mDICont->setSize(  Width, 400 );
	mDICont->setAnchors( UI_ANCHOR_RIGHT | UI_ANCHOR_TOP );
	mDICont->setEnabled( false );
	mDICont->setVisible( false );

	Txt = createTextBox( "DataId String:", mDICont, Sizei( Width, 16 ), Vector2i( TAB_CONT_X_DIST, 0 ), TxtFlags, Text::Shadow );

	mDataIdInput = UITextInput::New();
	mDataIdInput->setParent( mDICont )->setSize( Width / 4 * 3, 0 )->setPosition( TAB_CONT_X_DIST + 8, Txt->getPosition().y + Txt->getSize().getHeight() + 8 );

	fillSGCombo();
}

void MapEditor::createLighContainer() {
	UIPushButton * NewLightBut = UIPushButton::New();
	NewLightBut->setParent( mLightCont )->setSize(  mLightCont->getSize().getWidth() - TAB_CONT_X_DIST * 2, 0 )->setPosition( TAB_CONT_X_DIST, 0 );
	NewLightBut->setText( "New Light" );
	NewLightBut->addEventListener( UIEvent::MouseClick, cb::Make1( this, &MapEditor::onNewLight ) );

	UITextView * Txt = createTextBox( "Light Color:", mLightCont, Sizei(), Vector2i( TAB_CONT_X_DIST, 32 ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE );
	Txt->setFontStyle( Text::Shadow );

	mUIBaseColor = UIWidget::New();
	mUIBaseColor->setFlags( UI_FILL_BACKGROUND | UI_BORDER );
	mUIBaseColor->setParent( mLightCont );
	mUIBaseColor->setPosition( Txt->getPosition().x, Txt->getPosition().y + Txt->getSize().getHeight() + 4 );
	mUIBaseColor->setSize( 58, 64 );
	mUIBaseColor->getBackground()->setColor( Color(255,255,255,255) );
	mUIBaseColor->getBorder()->setColor( Color( 100, 100, 100, 200 ) );

	Txt = createTextBox( "R:", mLightCont, Sizei(), Vector2i( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4, mUIBaseColor->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	mUIRedSlider = UISlider::New()->setOrientation( UI_HORIZONTAL );
	mUIRedSlider->setParent( mLightCont )->setSize( 100, 20 )->setPosition( Txt->getPosition().x + Txt->getSize().getWidth(), Txt->getPosition().y );
	mUIRedSlider->setMaxValue( 255 );
	mUIRedSlider->setValue( 255 );
	mUIRedSlider->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &MapEditor::onRedChange ) );

	mUIRedTxt = createTextBox( String::toStr( (Uint32)255 ), mLightCont, Sizei(), Vector2i( mUIRedSlider->getPosition().x + mUIRedSlider->getSize().getWidth() + 4, mUIRedSlider->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	Txt = createTextBox( "G:", mLightCont, Sizei(), Vector2i( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4, mUIRedSlider->getPosition().y + mUIRedSlider->getSize().getHeight() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );
	mUIGreenSlider = UISlider::New()->setOrientation( UI_HORIZONTAL );
	mUIGreenSlider->setParent( mLightCont )->setSize( 100, 20 )->setPosition( mUIRedSlider->getPosition().x, Txt->getPosition().y );
	mUIGreenSlider->setMaxValue( 255 );
	mUIGreenSlider->setValue( 255 );
	mUIGreenSlider->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &MapEditor::onGreenChange ) );

	mUIGreenTxt = createTextBox( String::toStr( (Uint32)255 ), mLightCont, Sizei(), Vector2i( mUIGreenSlider->getPosition().x + mUIGreenSlider->getSize().getWidth() + 4, mUIGreenSlider->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	Txt = createTextBox( "B:", mLightCont, Sizei(), Vector2i( mUIBaseColor->getPosition().x + mUIBaseColor->getSize().getWidth() + 4, mUIGreenSlider->getPosition().y + mUIGreenSlider->getSize().getHeight() + 4 ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );
	mUIBlueSlider = UISlider::New()->setOrientation( UI_HORIZONTAL );
	mUIBlueSlider->setParent( mLightCont )->setSize( 100, 20 )->setPosition( mUIRedSlider->getPosition().x, Txt->getPosition().y );
	mUIBlueSlider->setMaxValue( 255 );
	mUIBlueSlider->setValue( 255 );
	mUIBlueSlider->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &MapEditor::onBlueChange ) );

	mUIBlueTxt = createTextBox( String::toStr( (Uint32)255 ), mLightCont, Sizei(), Vector2i( mUIBlueSlider->getPosition().x + mUIBlueSlider->getSize().getWidth() + 4, mUIBlueSlider->getPosition().y ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	Txt = createTextBox( "Light Radius:", mLightCont, Sizei(), Vector2i( TAB_CONT_X_DIST, mUIBlueTxt->getPosition().y + mUIBlueTxt->getSize().getHeight() + 16 ), UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE, Text::Shadow );

	mLightRadius = UISpinBox::New()->setAllowOnlyNumbers( false )->setValue( 100 );
	mLightRadius->setParent( mLightCont )->setSize( 100, 0 )->setPosition( Txt->getPosition().x, Txt->getPosition().y + Txt->getSize().getHeight() + 8 );
	mLightRadius->setMaxValue( 2000 );
	mLightRadius->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &MapEditor::onLightRadiusChangeVal ) );

	mLightTypeChk = UICheckBox::New();
	mLightTypeChk->setParent( mLightCont )->setPosition( mLightRadius->getPosition().x, mLightRadius->getPosition().y + mLightRadius->getSize().getHeight() + 8 )->setFlags( UI_AUTO_SIZE );
	mLightTypeChk->setText( "Isometric Light" );
	mLightTypeChk->setActive( false );
	mLightTypeChk->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &MapEditor::onLightTypeChange ) );
}

UISelectButton * MapEditor::addObjContButton( String text, Uint32 mode ) {
	UISelectButton * Button = UISelectButton::New();
	Button->setParent( mObjectCont );
	Button->setSize( mObjectCont->getSize().getWidth() - TAB_CONT_X_DIST * 2, 0 )->setPosition( TAB_CONT_X_DIST, mLastSelButtonY )->setFlags( UI_AUTO_SIZE );
	Button->setText( text );
	Button->setData( mode );

	Button->addEventListener( UIEvent::MouseClick, cb::Make1( this, &MapEditor::onObjectModeSel ) );

	mLastSelButtonY += Button->getSize().getHeight() + 4;

	mObjContButton.push_back( Button );

	return Button;
}

void MapEditor::createObjectsContainer() {
	addObjContButton( "Select Objects", UIMap::SELECT_OBJECTS )->select();
	addObjContButton( "Edit Polygons", UIMap::EDIT_POLYGONS );
	addObjContButton( "Insert Object", UIMap::INSERT_OBJECT );
	addObjContButton( "Insert Polygon", UIMap::INSERT_POLYGON );
	UISelectButton * Button = addObjContButton( "Insert Polyline", UIMap::INSERT_POLYLINE );

	Int32 nextY = Button->getPosition().y + Button->getSize().getHeight() + 4;

	Uint32 ChkFlags = UI_CONTROL_DEFAULT_ALIGN | UI_AUTO_SIZE | UI_ANCHOR_RIGHT | UI_ANCHOR_TOP;

	mChkClampToTile = UICheckBox::New();
	mChkClampToTile->setParent( mObjectCont )->setPosition( 12, nextY )->resetFlags( ChkFlags );
	mChkClampToTile->setText( "Clamp Position to Tile" );
	mChkClampToTile->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &MapEditor::chkClickClampToTile ) );
	mChkClampToTile->setActive( true );
}

void MapEditor::onObjectModeSel( const UIEvent * Event ) {
	UISelectButton * Button = static_cast<UISelectButton*>( Event->getControl() );
	UISelectButton * ButtonT = NULL;

	for ( std::list<UISelectButton*>::iterator it = mObjContButton.begin(); it != mObjContButton.end(); ++it ) {
		ButtonT = *it;

		ButtonT->unselect();
	}

	Button->select();

	mUIMap->setEditingObjMode( (UIMap::EDITING_OBJ_MODE)Button->getData() );
}

void MapEditor::createUIMap() {
	UISkin * HScrollSkin = mTheme->getSkin( "hscrollbar_bg" );
	UISkin * VScrollSkin = mTheme->getSkin( "vscrollbar_bg" );

	Int32 ScrollH = 16;
	Int32 ScrollV = 16;

	if ( NULL != HScrollSkin ) {
		ScrollH = HScrollSkin->getSize().getHeight();
	}

	if ( NULL != VScrollSkin ) {
		ScrollV = VScrollSkin->getSize().getHeight();
	}

	mUIMap = UIMap::New( mTheme );
	mUIMap->setParent( mWinContainer )->setPosition( 0, 0 );
	mUIMap->setSize( mWinContainer->getSize().getWidth() - 225 - ScrollV, mWinContainer->getSize().getHeight() - ScrollH );
	mUIMap->setAnchors( UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM | UI_ANCHOR_RIGHT );

	mUIMap->setVisible( true );
	mUIMap->setEnabled( true );
	createNewEmptyMap();
	mUIMap->addEventListener( UIEvent::OnSizeChange, cb::Make1( this, &MapEditor::onMapSizeChange ) );
	mUIMap->addEventListener( UIEvent::MouseDown, cb::Make1( this, &MapEditor::onMapMouseDown ) );
	mUIMap->addEventListener( UIEvent::MouseClick, cb::Make1( this, &MapEditor::onMapMouseClick ) );
	mUIMap->setLightSelectCb( cb::Make1( this, &MapEditor::onLightSelect ) );
	mUIMap->setLightRadiusChangeCb( cb::Make1( this, &MapEditor::onLightRadiusChange ) );
	mUIMap->setAddObjectCallback( cb::Make2( this, &MapEditor::onAddObject ) );
	mUIMap->setAlertCb( cb::Make2( this, &MapEditor::createAlert ) );
	mUIMap->setUpdateScrollCb( cb::Make0( this, &MapEditor::updateScroll ) );
	mUIMap->setTileBox( mTileBox );

	mMapHScroll = UIScrollBar::New();
	mMapHScroll->setOrientation( UI_HORIZONTAL )
			   ->setParent( mWinContainer )
			   ->setSize( mWinContainer->getSize().getWidth() - 225 - ScrollV, ScrollH )
			   ->setPosition( 0, mWinContainer->getSize().getHeight() - ScrollH );
	mMapHScroll->setAnchors( UI_ANCHOR_LEFT | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM );
	mMapHScroll->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &MapEditor::onScrollMapH ) );

	mMapVScroll = UIScrollBar::New();
	mMapVScroll->setParent( mWinContainer )->setSize( ScrollV, mWinContainer->getSize().getHeight() - ScrollH )
			   ->setPosition( mWinContainer->getSize().getWidth() - 225 - ScrollV, 0 );
	mMapVScroll->setAnchors( UI_ANCHOR_TOP | UI_ANCHOR_BOTTOM );
	mMapVScroll->addEventListener( UIEvent::OnValueChange, cb::Make1( this, &MapEditor::onScrollMapV ) );

	mapCreated();
}

void MapEditor::onAddObject( Uint32 Type, Polygon2f poly ) {
	if ( NULL == mCurLayer ) {
		createNoLayerAlert( "No layers found" )->setFocus();
		return;
	}

	if ( mCurLayer->getType() != MAP_LAYER_OBJECT ) {
		createAlert( "Wrong Layer", "Objects only can be added to an Object Layer" )->setFocus();
		return;
	}

	if ( poly.getSize() < 3 ) {
		return;
	}

	MapObjectLayer * OL = static_cast<MapObjectLayer*> ( mCurLayer );

	if ( GAMEOBJECT_TYPE_OBJECT == Type ) {
		OL->addGameObject( eeNew( GameObjectObject, ( mUIMap->Map()->getNewObjectId(), poly.getBounds(), mCurLayer ) ) );
	} else if ( GAMEOBJECT_TYPE_POLYGON == Type ) {
		OL->addGameObject( eeNew( GameObjectPolygon, ( mUIMap->Map()->getNewObjectId(), poly, mCurLayer ) ) );
	} else if ( GAMEOBJECT_TYPE_POLYLINE == Type ) {
		OL->addGameObject( eeNew( GameObjectPolyline, ( mUIMap->Map()->getNewObjectId(), poly, mCurLayer ) ) );
	}
}

void MapEditor::onLightTypeChange( const UIEvent * Event ) {
	if ( NULL != mUIMap->getSelectedLight() ) {
		mUIMap->getSelectedLight()->setType( mLightTypeChk->isActive() ? LIGHT_ISOMETRIC : LIGHT_NORMAL );
	}
}

void MapEditor::onLightRadiusChangeVal( const UIEvent * Event ) {
	if ( NULL != mUIMap->getSelectedLight() ) {
		mUIMap->getSelectedLight()->setRadius( mLightRadius->getValue() );
	}
}

void MapEditor::onLightRadiusChange( MapLight * Light ) {
	mLightRadius->setValue( Light->getRadius() );
}

void MapEditor::onLightSelect( MapLight * Light ) {
	Color Col( Light->getColor() );

	mUIRedSlider->setValue( Col.r );
	mUIGreenSlider->setValue( Col.g );
	mUIBlueSlider->setValue( Col.b );
	mLightRadius->setValue( Light->getRadius() );
	mLightTypeChk->setActive( Light->getType() == LIGHT_ISOMETRIC ? true : false );
}

void MapEditor::onNewLight( const UIEvent * Event ) {
	const UIEventMouse * MEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( MEvent->getFlags() & EE_BUTTON_LMASK ) {
		Vector2i Pos = mUIMap->Map()->getMouseMapPos();
		mUIMap->addLight( eeNew( MapLight, ( mLightRadius->getValue(), Pos.x, Pos.y, mUIBaseColor->getBackground()->getColor().toRGB(), mLightTypeChk->isActive() ? LIGHT_ISOMETRIC : LIGHT_NORMAL ) ) );
	}
}

void MapEditor::onRedChange( const UIEvent * Event ) {
	Color Col = mUIBaseColor->getBackground()->getColor();
	Col.r = (Uint8)mUIRedSlider->getValue();
	mUIBaseColor->getBackground()->setColor( Col );
	mUIRedTxt->setText( String::toStr( (Int32)mUIRedSlider->getValue() ) );

	if ( NULL != mUIMap->getSelectedLight() ) {
		RGB lCol( mUIMap->getSelectedLight()->getColor() );
		lCol.r = Col.r;
		mUIMap->getSelectedLight()->setColor( lCol );
	}
}

void MapEditor::onGreenChange( const UIEvent * Event ) {
	Color Col = mUIBaseColor->getBackground()->getColor();
	Col.g = (Uint8)mUIGreenSlider->getValue();
	mUIBaseColor->getBackground()->setColor( Col );
	mUIGreenTxt->setText( String::toStr( (Uint32)mUIGreenSlider->getValue() ) );

	if ( NULL != mUIMap->getSelectedLight() ) {
		RGB lCol( mUIMap->getSelectedLight()->getColor() );
		lCol.g = Col.g;
		mUIMap->getSelectedLight()->setColor( lCol );
	}
}

void MapEditor::onBlueChange( const UIEvent * Event ) {
	Color Col = mUIBaseColor->getBackground()->getColor();
	Col.b = (Uint8)mUIBlueSlider->getValue();
	mUIBaseColor->getBackground()->setColor( Col );
	mUIBlueTxt->setText( String::toStr( (Uint32)mUIBlueSlider->getValue() ) );

	if ( NULL != mUIMap->getSelectedLight() ) {
		RGB lCol( mUIMap->getSelectedLight()->getColor() );
		lCol.b = Col.b;
		mUIMap->getSelectedLight()->setColor( lCol );
	}
}

void MapEditor::chkClickDI( const UIEvent * Event ) {
	if ( mChkDI->isActive() ) {
		mSGCont->setEnabled( false );
		mSGCont->setVisible( false );
		mDICont->setEnabled( true );
		mDICont->setVisible( true );
	} else {
		mSGCont->setEnabled( true );
		mSGCont->setVisible( true );
		mDICont->setEnabled( false );
		mDICont->setVisible( false );
	}
}

void MapEditor::chkClickClampToTile( const UIEvent * Event ) {
	if ( NULL != mUIMap )
		mUIMap->setClampToTile( mChkClampToTile->isActive() );
}

void MapEditor::updateGfx() {
	if ( mChkMirrored->isActive() && mChkFliped->isActive() )
		mGfxPreview->setRenderMode( RENDER_FLIPPED_MIRRORED );
	else if( mChkMirrored->isActive() )
		mGfxPreview->setRenderMode( RENDER_MIRROR );
	else if ( mChkFliped->isActive() )
		mGfxPreview->setRenderMode( RENDER_FLIPPED );
	else
		mGfxPreview->setRenderMode( RENDER_NORMAL );

	if ( mChkRot90->isActive() )
		mGfxPreview->setRotation( 90 );
	else
		mGfxPreview->setRotation( 0 );
}

void MapEditor::updateFlags() {
	mCurGOFlags = 0;

	if ( mChkMirrored->isActive() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_MIRRORED;

	if ( mChkFliped->isActive() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_FLIPED;

	if ( mChkBlocked->isActive() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_BLOCKED;

	if ( mChkAnim->isActive() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_ANIMATED;

	if ( mChkRot90->isActive() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_ROTATE_90DEG;

	if ( mChkAutoFix->isActive() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_AUTO_FIX_TILE_POS;

	if ( mChkBlendAdd->isActive() )
		mCurGOFlags |= GObjFlags::GAMEOBJECT_BLEND_ADD;
}

void MapEditor::onTypeChange( const UIEvent * Event ) {
	if ( mGOTypeList->getText() == "TextureRegion" )
		mCurGOType = GAMEOBJECT_TYPE_TEXTUREREGION;
	else if ( mGOTypeList->getText() == "TextureRegionEx" )
		mCurGOType = GAMEOBJECT_TYPE_TEXTUREREGIONEX;
	else if ( mGOTypeList->getText() == "Sprite" )
		mCurGOType = GAMEOBJECT_TYPE_SPRITE;
	else
		mCurGOType = String::hash( mGOTypeList->getText().toUtf8() );

	if ( NULL != mChkAnim && NULL != mGOTypeList && mChkAnim->isActive() && mGOTypeList->getText() != "Sprite" ) {
		if ( mGOTypeList->getText() == "TextureRegion" || mGOTypeList->getText() == "TextureRegionEx" ) {
			mChkAnim->setActive( false );
		}
	}
}

void MapEditor::chkClickMirrored( const UIEvent * Event ) {
	updateGfx();
	updateFlags();
}

void MapEditor::chkClickFlipped( const UIEvent * Event ) {
	updateGfx();
	updateFlags();
}

void MapEditor::chkClickRot90( const UIEvent * Event ) {
	updateGfx();
	updateFlags();
}

void MapEditor::chkClickBlocked( const UIEvent * Event ) {
	updateFlags();
}

void MapEditor::chkClickAutoFix( const UIEvent * Event ) {
	updateFlags();
}

void MapEditor::chkClickAnimated( const UIEvent * Event ) {
	updateFlags();

	if ( mChkAnim->isActive() && ( mGOTypeList->getText() == "TextureRegion" || mGOTypeList->getText() == "TextureRegionEx" ) ) {
		mGOTypeList->getListBox()->setSelected( "Sprite" );
	}
}

void MapEditor::addNewGOType( const UIEvent * Event ) {
	eeNew( UIGOTypeNew, ( cb::Make2( this, &MapEditor::onNewGOTypeAdded ) ) );
}

void MapEditor::onNewGOTypeAdded( std::string name, Uint32 hash ) {
	if ( "" != name ) {
		for ( Uint32 i = 0; i < mGOTypeList->getListBox()->getCount(); i++ ) {
			UIListBoxItem * Item = mGOTypeList->getListBox()->getItem(i);

			if ( Item->getText() == name )
				return;
		}

		mGOTypeList->getListBox()->addListBoxItem( name );
		mUIMap->Map()->addVirtualObjectType( name );
	}
}

void MapEditor::fillSGCombo() {
	TextureAtlasManager * SGM = TextureAtlasManager::instance();
	std::list<TextureAtlas*>& Res = SGM->getResources();

	mTextureAtlasesList->getListBox()->clear();

	std::vector<String> items;

	Uint32 Restricted1 = String::hash( std::string( "global" ) );
	Uint32 Restricted2 = String::hash( mTheme->getTextureAtlas()->getName() );

	for ( std::list<TextureAtlas*>::iterator it = Res.begin(); it != Res.end(); ++it ) {
		if ( (*it)->getId() != Restricted1 && (*it)->getId() != Restricted2 )
			items.push_back( (*it)->getName() );
	}

	if ( items.size() ) {
		mTextureAtlasesList->getListBox()->addListBoxItems( items );
	}

	if ( mTextureAtlasesList->getListBox()->getCount() && NULL == mTextureAtlasesList->getListBox()->getItemSelected() ) {
		mTextureAtlasesList->getListBox()->setSelected( 0 );
	}
}

void MapEditor::fillTextureRegionList() {
	TextureAtlasManager * SGM = TextureAtlasManager::instance();
	mCurSG = SGM->getByName( mTextureAtlasesList->getText() );
	std::list<TextureRegion*>& Res = mCurSG->getResources();

	mTextureRegionList->clear();

	if ( NULL != mCurSG ) {
		std::vector<String> items;

		for ( std::list<TextureRegion*>::iterator it = Res.begin(); it != Res.end(); ++it ) {
				items.push_back( (*it)->getName() );
		}

		if ( items.size() ) {
			std::sort( items.begin(), items.end() );

			mTextureRegionList->addListBoxItems( items );
			mTextureRegionList->setSelected( 0 );
		}
	}

	mTextureRegionList->getVerticalScrollBar()->setClickStep( 8.f / (Float)mTextureRegionList->getCount() );
}

void MapEditor::onTextureRegionChange( const UIEvent * Event ) {
	if ( NULL != mCurSG ) {
		TextureRegion * tTextureRegion = mCurSG->getByName( mTextureRegionList->getItemSelectedText() );

		if ( NULL != tTextureRegion ) {
			mGfxPreview->setTextureRegion( tTextureRegion );
		}
	}
}

void MapEditor::onTextureAtlasChange( const UIEvent * Event ) {
	fillTextureRegionList();
}

void MapEditor::createNewMap() {
	eeNew( UIMapNew, ( mUIMap, cb::Make0( this, &MapEditor::mapCreated ) ) );
}

void MapEditor::createNewEmptyMap() {
	mUIMap->Map()->create( Sizei( 100, 100 ), 16, Sizei( 32, 32 ), MAP_EDITOR_DEFAULT_FLAGS | MAP_FLAG_LIGHTS_ENABLED, mUIMap->getRealSize() );
}

void MapEditor::mapCreated() {
	mCurLayer = NULL;
	mLayerList->getListBox()->clear();
	updateFlags();
	setViewOptions();
	fillSGCombo();
	fillGotyList();

	mMapHScroll->setValue( 0 );
	mMapVScroll->setValue( 0 );
	onMapSizeChange( NULL );

	mUIMap->clearLights();

	createTabs();
}

void MapEditor::onMapSizeChange( const UIEvent *Event ) {
	if ( mMouseScrolling )
		return;

	Vector2f t( mUIMap->Map()->getTileSize().getWidth() * mUIMap->Map()->getScale(),
				mUIMap->Map()->getTileSize().getHeight() * mUIMap->Map()->getScale() );

	Vector2f v( (Float)mUIMap->Map()->getViewSize().getWidth(), (Float)mUIMap->Map()->getViewSize().getHeight() );

	Vector2f s( t.x * mUIMap->Map()->getSize().getWidth(), t.y * mUIMap->Map()->getSize().getHeight() );

	Vector2f m( s.x - v.x, s.y - v.y );

	mMapHScroll->setMinValue( 0 );
	mMapHScroll->setMaxValue( m.x );
	mMapHScroll->setClickStep( t.y );
	mMapVScroll->setMinValue( 0 );
	mMapVScroll->setMaxValue( m.y );
	mMapVScroll->setClickStep( t.x );
	mMapHScroll->setPageStep( v.x / s.x * m.x );
	mMapVScroll->setPageStep( v.y / s.y * m.y );
}

void MapEditor::onScrollMapH( const UIEvent * Event ) {
	if ( mMouseScrolling )
		return;

	mUIMap->Map()->setOffset( Vector2f( -mMapHScroll->getValue(), -mMapVScroll->getValue() ) ) ;
}

void MapEditor::onScrollMapV( const UIEvent * Event ) {
	mUIMap->Map()->setOffset( Vector2f( -mMapHScroll->getValue(), -mMapVScroll->getValue() ) ) ;
}

void MapEditor::updateScroll() {
	mMouseScrolling = true;
	mMapHScroll->setValue( -mUIMap->Map()->getOffset().x );
	mMapVScroll->setValue( -mUIMap->Map()->getOffset().y );
	mMouseScrolling = false;
}

void MapEditor::mapOpen( const UIEvent * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->getControl() );

	if ( mUIMap->Map()->loadFromFile( CDL->getFullPath() ) ) {
		onMapLoad();
	}
}

void MapEditor::onMapLoad() {
	mCurLayer = NULL;

	mUIMap->Map()->setViewSize( mUIMap->getRealSize() );

	mapCreated();

	refreshLayersList();

	refreshGotyList();
}

void MapEditor::mapSave( const UIEvent * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->getControl() );

	std::string path( CDL->getFullPath() );

	if ( path.substr( path.size() - 4 ) != ".eem" ) {
		path += ".eem";
	}

	mUIMap->Map()->saveToFile( path );
}

void MapEditor::fileMenuClick( const UIEvent * Event ) {
	if ( !Event->getControl()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->getControl() )->getText();

	if ( "New..." == txt ) {
		createNewMap();
	} else if ( "Open..." == txt ) {
		UICommonDialog * TGDialog = UICommonDialog::New( UI_CDL_DEFAULT_FLAGS, "*.eem" );
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Open Map" );
		TGDialog->addEventListener( UIEvent::OpenFile, cb::Make1( this, &MapEditor::mapOpen ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Save As..." == txt ) {
		UICommonDialog * TGDialog = UICommonDialog::New( UI_CDL_DEFAULT_FLAGS | CDL_FLAG_SAVE_DIALOG, "*.eem" );
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Save Map" );
		TGDialog->addEventListener( UIEvent::SaveFile, cb::Make1( this, &MapEditor::mapSave ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Save" == txt ) {
		if ( mUIMap->Map()->getPath().size() ) {
			mUIMap->Map()->saveToFile( mUIMap->Map()->getPath() );
		}
	} else if ( "Close" == txt ) {
		UIMessageBox * MsgBox = UIMessageBox::New( MSGBOX_OKCANCEL, "Do you really want to close the current map?\nAll changes will be lost." );
		MsgBox->addEventListener( UIEvent::MsgBoxConfirmClick, cb::Make1( this, &MapEditor::onMapClose ) );
		MsgBox->setTitle( "Close Map?" );
		MsgBox->center();
		MsgBox->show();
	} else if ( "Quit" == txt ) {
		if ( mUIWindow == UIManager::instance()->getMainControl() ) {
			UIManager::instance()->getWindow()->close();
		} else {
			mUIWindow->closeWindow();
		}
	}
}

void MapEditor::onMapClose( const UIEvent * Event ) {
	createNewEmptyMap();

	mapCreated();

	mCurLayer = NULL;

	refreshLayersList();
}

void MapEditor::viewMenuClick( const UIEvent * Event ) {
	if ( !Event->getControl()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->getControl() )->getText();

	if ( "Show Grid" == txt ) {
		mUIMap->Map()->setDrawGrid( reinterpret_cast<UIMenuCheckBox*> ( Event->getControl() )->isActive() );
	} else if ( "Mark Tile Over" == txt ) {
		mUIMap->Map()->setDrawTileOver( reinterpret_cast<UIMenuCheckBox*> ( Event->getControl() )->isActive() );
	} else if ( "Show Blocked" == txt ) {
		mUIMap->Map()->setShowBlocked( reinterpret_cast<UIMenuCheckBox*> ( Event->getControl() )->isActive() );
	} else if ( "Zoom In" == txt ) {
		zoomIn();
	} else if ( "Zoom Out" == txt ) {
		zoomOut();
	} else if ( "Normal Size" == txt ) {
		mUIMap->Map()->setScale( 1 );
	}
}

void MapEditor::zoomIn() {
	TileMap * Map = mUIMap->Map();
	Float S = mUIMap->Map()->getScale();

	if ( S < 4 ) {
		if ( 0.0625f == S ) {
			Map->setScale( 0.125f );
		} else if ( 0.125f == S ) {
			Map->setScale( 0.25f );
		} else if ( 0.25f == S ) {
			Map->setScale( 0.5f );
		} else if ( 0.5f == S ) {
			Map->setScale( 0.75f );
		} else if ( 0.75f == S ) {
			Map->setScale( 1.0f );
		} else if ( 1.0f == S ) {
			Map->setScale( 1.5f );
		} else if ( 1.5f == S ) {
			Map->setScale( 2.0f );
		} else if ( 2.0f == S ) {
			Map->setScale( 3.0f );
		} else if ( 3.0f == S ) {
			Map->setScale( 4.0f );
		}
	}

	onMapSizeChange();
}

void MapEditor::zoomOut() {
	TileMap * Map = mUIMap->Map();
	Float S = mUIMap->Map()->getScale();

	if ( S > 0.0625f ) {
		if ( 0.125f == S ) {
			Map->setScale( 0.0625f );
		} else if ( 0.25f == S ) {
			Map->setScale( 0.125f );
		} else if ( 0.5f == S ) {
			Map->setScale( 0.25f );
		} else if ( 0.75f == S ) {
			Map->setScale( 0.5f );
		} else if ( 1.0f == S ) {
			Map->setScale( 0.75f );
		} else if ( 1.5f == S ) {
			Map->setScale( 1.0f );
		} else if ( 2.0f == S ) {
			Map->setScale( 1.5f );
		} else if ( 3.0f == S ) {
			Map->setScale( 2.0f );
		} else if ( 4.0f == S ) {
			Map->setScale( 3.0f );
		}
	}

	onMapSizeChange();
}

void MapEditor::mapMenuClick( const UIEvent * Event ) {
	if ( !Event->getControl()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->getControl() )->getText();

	if ( "New Texture Atlas..." == txt ) {
		UIWindow * tWin = UIWindow::New();
		tWin->setSizeWithDecoration( 1024, 768 );
		tWin->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_DRAGABLE_CONTAINER );
		tWin->setMinWindowSize( 1024, 768 );

		eeNew ( Tools::TextureAtlasEditor, ( tWin ) );
		tWin->center();
		tWin->show();
	} else if ( "Add External Texture Atlas..." == txt ) {
		UICommonDialog * TGDialog = UICommonDialog::New( UI_CDL_DEFAULT_FLAGS, std::string( "*" ) + EE_TEXTURE_ATLAS_EXTENSION );
		TGDialog->setWinFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		TGDialog->setTitle( "Load Texture Atlas..." );
		TGDialog->addEventListener( UIEvent::OpenFile, cb::Make1( this, &MapEditor::cextureAtlasOpen ) );
		TGDialog->center();
		TGDialog->show();
	} else if ( "Properties..." == txt ) {
		eeNew( TileMapProperties, ( mUIMap->Map() ) );
	} else if ( "Resize..." ) {
		eeNew( UIMapNew, ( mUIMap, cb::Make0( this, &MapEditor::onMapLoad ), true ) );
	}
}

void MapEditor::layerMenuClick( const UIEvent * Event ) {
	if ( !Event->getControl()->isType( UI_TYPE_MENUITEM ) )
		return;

	const String& txt = reinterpret_cast<UIMenuItem*> ( Event->getControl() )->getText();

	if ( "Add Tile Layer..." == txt ) {
		eeNew( UIMapLayerNew, ( mUIMap, MAP_LAYER_TILED, cb::Make1( this, &MapEditor::onLayerAdd ) ) );
	} else if ( "Add Object Layer..." == txt ) {
		eeNew( UIMapLayerNew, ( mUIMap, MAP_LAYER_OBJECT, cb::Make1( this, &MapEditor::onLayerAdd ) ) );
	} else if ( "Remove Layer" == txt ) {
		removeLayer();
	} else if ( "Move Layer Up" == txt ) {
		moveLayerUp();
	} else if ( "Move Layer Down" == txt ) {
		moveLayerDown();
	} else if ( "Layer Properties..." == txt ) {
		if ( NULL != mCurLayer) {
			eeNew( MapLayerProperties, ( mCurLayer, cb::Make0( this, &MapEditor::refreshLayersList ) ) );
		} else {
			createNoLayerAlert( "Error retrieving layer properties" );
		}
	} else if ( "Lights Enabled" == txt ) {
		if ( NULL != mCurLayer ) {
			mCurLayer->setLightsEnabled( !mCurLayer->getLightsEnabled() );
		}
	} else if ( "Visible" == txt ) {
		if ( NULL != mCurLayer ) {
			mCurLayer->setVisible( !mCurLayer->isVisible() );
		}
	}
}

UIMessageBox * MapEditor::createAlert( const String& title, const String& text ) {
	UIMessageBox * MsgBox = UIMessageBox::New( MSGBOX_OK, text );
	MsgBox->setTitle( title );
	MsgBox->center();
	MsgBox->show();
	return MsgBox;
}

UIMessageBox * MapEditor::createNoLayerAlert( const String title ) {
	return createAlert( title, "First select and add a new layer." );
}

void MapEditor::moveLayerUp() {
	if ( mUIMap->Map()->moveLayerUp( mCurLayer ) ) {
		refreshLayersList();
	}
}

void MapEditor::moveLayerDown() {
	if ( mUIMap->Map()->moveLayerDown( mCurLayer ) ) {
		refreshLayersList();
	}
}

void MapEditor::removeLayer() {
	if ( mUIMap->Map()->removeLayer( mCurLayer ) ) {
		mCurLayer = NULL;

		refreshLayersList();
	}
}

void MapEditor::refreshGotyList() {
	TileMap::GOTypesList& GOList = mUIMap->Map()->getVirtualObjectTypes();

	for ( TileMap::GOTypesList::iterator it = GOList.begin(); it != GOList.end(); ++it ) {
		mGOTypeList->getListBox()->addListBoxItem( (*it) );
	}
}

void MapEditor::refreshLayersList() {
	mLayerList->getListBox()->clear();

	if ( mUIMap->Map()->getLayerCount() ) {
		std::vector<String> layers;

		for ( Uint32 i = 0; i < mUIMap->Map()->getLayerCount(); i++ ) {
			layers.push_back( mUIMap->Map()->getLayer(i)->getName() );
		}

		mLayerList->getListBox()->addListBoxItems( layers );
	}

	if ( NULL != mCurLayer ) {
		mLayerList->getListBox()->setSelected( mCurLayer->getName() );
	} else {
		if ( mUIMap->Map()->getLayerCount() ) {
			mLayerList->getListBox()->setSelected(0);
		}
	}
}

void MapEditor::cextureAtlasOpen( const UIEvent * Event ) {
	UICommonDialog * CDL = reinterpret_cast<UICommonDialog*> ( Event->getControl() );

	std::string sgname = FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( CDL->getFullPath() ) );

	TextureAtlas * SG = TextureAtlasManager::instance()->getByName( sgname );

	if ( NULL == SG ) {
		TextureAtlasLoader tgl( CDL->getFullPath() );

		if ( tgl.isLoaded() ) {
			mTextureAtlasesList->getListBox()->addListBoxItem( sgname );
		}
	} else {
		if ( eeINDEX_NOT_FOUND == mTextureAtlasesList->getListBox()->getItemIndex( sgname ) ) {
			mTextureAtlasesList->getListBox()->addListBoxItem( sgname );
		}
	}
}

void MapEditor::onLayerAdd( UIMapLayerNew * UILayer ) {
	bool SetSelected = ( 0 == mLayerList->getListBox()->getCount() ) ? true : false;

	mLayerList->getListBox()->addListBoxItem( UILayer->getName() );

	if ( SetSelected ) {
		mCurLayer = UILayer->getLayer();

		mUIMap->setCurLayer( mCurLayer );

		mLayerList->getListBox()->setSelected(0);
	}
}

void MapEditor::onLayerSelect( const UIEvent * Event ) {
	MapLayer * tLayer = mUIMap->Map()->getLayer( mLayerList->getText() );

	if ( NULL != tLayer ) {
		mCurLayer = tLayer;

		mUIMap->setCurLayer( mCurLayer );

		mLayerChkVisible->setActive( mCurLayer->isVisible() );

		mLayerChkLights->setActive( mCurLayer->getLightsEnabled() );
	}
}

void MapEditor::windowClose( const UIEvent * Event ) {
	if ( mCloseCb.IsSet() )
		mCloseCb();

	eeDelete( this );
}

GameObject * MapEditor::createGameObject() {
	GameObject * tObj	= NULL;

	if ( GAMEOBJECT_TYPE_TEXTUREREGION == mCurGOType ) {

		tObj = eeNew( GameObjectTextureRegion, ( mCurGOFlags, mCurLayer, mGfxPreview->getTextureRegion() ) );

	} else if ( GAMEOBJECT_TYPE_TEXTUREREGIONEX == mCurGOType ) {

		tObj = eeNew( GameObjectTextureRegionEx, ( mCurGOFlags, mCurLayer, mGfxPreview->getTextureRegion() ) );

	} else if ( GAMEOBJECT_TYPE_SPRITE == mCurGOType ) {

		if ( mChkAnim->isActive() ) {

			Sprite * tAnimSprite = eeNew( Sprite, ( String::removeNumbersAtEnd( mGfxPreview->getTextureRegion()->getName() ) ) );
			tObj = eeNew( GameObjectSprite, ( mCurGOFlags, mCurLayer, tAnimSprite ) );
			tAnimSprite->setAutoAnimate( false );

		} else {

			Sprite * tStatiSprite = eeNew( Sprite, ( mGfxPreview->getTextureRegion() ) );
			tObj = eeNew( GameObjectSprite, ( mCurGOFlags, mCurLayer, tStatiSprite ) );

		}
	} else {
		//! Creates an empty game object. The client will interpret the GameObject Type, and instanciate the corresponding class.

		if ( mChkDI->isActive() )
			tObj = eeNew( GameObjectVirtual, ( String::hash( mDataIdInput->getText().toUtf8() ), mCurLayer, mCurGOFlags, mCurGOType ) );
		else
			tObj = eeNew( GameObjectVirtual, ( mGfxPreview->getTextureRegion(), mCurLayer, mCurGOFlags, mCurGOType ) );
	}

	return tObj;
}

void MapEditor::addGameObjectToTile() {
	TileMapLayer * tLayer	= reinterpret_cast<TileMapLayer*> ( mCurLayer );
	TileMap * tMap			= mUIMap->Map();
	GameObject * tObj	= createGameObject();

	if ( NULL != tObj ) {
		if ( tObj->getType() == GAMEOBJECT_TYPE_VIRTUAL )
			reinterpret_cast<GameObjectVirtual*> ( tObj )->setLayer( tLayer );

		tLayer->addGameObject( tObj, tMap->getMouseTilePos() );
	}
}

void MapEditor::removeGameObjectFromTile() {
	TileMapLayer * tLayer = reinterpret_cast<TileMapLayer*> ( mCurLayer );
	TileMap * tMap			= mUIMap->Map();

	tLayer->removeGameObject( tMap->getMouseTilePos() );
}

void MapEditor::addGameObject() {
	MapObjectLayer * tLayer	= reinterpret_cast<MapObjectLayer*> ( mCurLayer );
	TileMap * tMap			= mUIMap->Map();
	GameObject * tObj	= createGameObject();

	if ( NULL != tObj ) {
		if ( tObj->getType() == GAMEOBJECT_TYPE_VIRTUAL )
			reinterpret_cast<GameObjectVirtual*> ( tObj )->setLayer( tLayer );

		Vector2i p( tMap->getMouseMapPos() );

		if ( UIManager::instance()->getInput()->isKeyDown( KEY_LCTRL ) ) {
			p = tMap->getMouseTilePosCoords();
		}

		tObj->setPosition( Vector2f( p.x, p.y ) );
		tLayer->addGameObject( tObj );
	}
}

void MapEditor::removeGameObject() {
	MapObjectLayer * tLayer = reinterpret_cast<MapObjectLayer*> ( mCurLayer );
	TileMap * tMap			= mUIMap->Map();

	tLayer->removeGameObject( tMap->getMouseMapPos() );
}

GameObject * MapEditor::getCurrentGOOver() {
	return reinterpret_cast<TileMapLayer*>( mCurLayer )->getGameObject( mUIMap->Map()->getMouseTilePos() );
}

void MapEditor::onMapMouseClick( const UIEvent * Event ) {
	const UIEventMouse * MEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( mTextureRegionCont->isVisible() ) {
		if ( NULL == mCurLayer || NULL == mGfxPreview->getTextureRegion() || UIManager::instance()->getDownControl() != mUIMap ) {
			if ( NULL == mCurLayer )
				createNoLayerAlert( "No layers found" );

			return;
		}

		if ( MEvent->getFlags() & EE_BUTTON_LMASK ) {
			if ( mCurLayer->getType() == MAP_LAYER_OBJECT )
				addGameObject();
		} else if ( MEvent->getFlags() & EE_BUTTON_RMASK ) {
			if ( mCurLayer->getType() == MAP_LAYER_OBJECT )
				removeGameObject();
		} else if ( MEvent->getFlags() & EE_BUTTON_MMASK ) {
			if ( mCurLayer->getType() == MAP_LAYER_TILED ) {
				GameObject * tObj = getCurrentGOOver();

				if ( NULL != tObj ) {
					tObj->setBlocked( !tObj->isBlocked() );
				}
			}
		} else if ( MEvent->getFlags() & EE_BUTTON_WUMASK ) {
			if ( mCurLayer->getType() == MAP_LAYER_TILED ) {
				GameObject * tObj = getCurrentGOOver();

				if ( NULL != tObj ) {
					tObj->setMirrored( !tObj->isMirrored() );
				}
			}
		} else if ( MEvent->getFlags() & EE_BUTTON_WDMASK ) {
			if ( mCurLayer->getType() == MAP_LAYER_TILED ) {
				GameObject * tObj = getCurrentGOOver();

				if ( NULL != tObj ) {
					tObj->setRotated( !tObj->isRotated() );
				}
			}
		}
	}
}

void MapEditor::onMapMouseDown( const UIEvent * Event ) {
	const UIEventMouse * MEvent = reinterpret_cast<const UIEventMouse*> ( Event );

	if ( mTextureRegionCont->isVisible() ) {
		if ( NULL == mCurLayer || NULL == mGfxPreview->getTextureRegion() || UIManager::instance()->getDownControl() != mUIMap )
			return;


		if ( MEvent->getFlags() & EE_BUTTON_LMASK ) {
			if ( mCurLayer->getType() == MAP_LAYER_TILED )
				addGameObjectToTile();
		} else if ( MEvent->getFlags() & EE_BUTTON_RMASK ) {
			if ( mCurLayer->getType() == MAP_LAYER_TILED )
				removeGameObjectFromTile();
		}
	}
}

void MapEditor::setViewOptions() {
	mChkShowGrid->setActive( mUIMap->Map()->getDrawGrid() ? true : false );
	mChkMarkTileOver->setActive( mUIMap->Map()->getDrawTileOver() ? true : false  );
	mChkShowBlocked->setActive( mUIMap->Map()->getShowBlocked() ? true : false );
}

}}
