#ifndef EE_MAPS_CMAPEDITOR_HPP
#define EE_MAPS_CMAPEDITOR_HPP

#include <eepp/maps/base.hpp>
#include <eepp/maps/gameobject.hpp>
#include <eepp/maps/maplayer.hpp>
#include <eepp/maps/maplight.hpp>
#include <eepp/ui/tools/textureatlaseditor.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uifiledialog.hpp>
#include <eepp/ui/uimenubar.hpp>
#include <eepp/ui/uimenucheckbox.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uitextureregion.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwindow.hpp>

namespace EE { namespace UI {
class UIMessageBox;
class UITextView;
}} // namespace EE::UI

using namespace EE::UI;

namespace EE { namespace Maps {

namespace Private {
class UIMapLayerNew;
class UIMap;
} // namespace Private

class EE_MAPS_API MapEditor {
  public:
	typedef std::function<void()> MapEditorCloseCb;

	static MapEditor* New( UIWindow* AttatchTo = NULL,
						   const MapEditorCloseCb& callback = MapEditorCloseCb() );

	MapEditor( UIWindow* AttatchTo = NULL, const MapEditorCloseCb& callback = MapEditorCloseCb() );

	~MapEditor();

  protected:
	UIWindow* mUIWindow;
	Node* mUIContainer;
	UITheme* mTheme;
	Private::UIMap* mUIMap;
	MapEditorCloseCb mCloseCb;
	UIDropDownList* mTextureAtlasesList;
	UIWidget* mWinContainer;
	UIListBox* mTextureRegionList;
	UITextureRegion* mGfxPreview;
	TextureAtlas* mCurSG;
	UIScrollBar* mMapHScroll;
	UIScrollBar* mMapVScroll;
	UIDropDownList* mGOTypeList;
	UIDropDownList* mLayerList;
	UICheckBox* mChkMirrored;
	UICheckBox* mChkFliped;
	UICheckBox* mChkBlocked;
	UICheckBox* mChkAnim;
	UICheckBox* mChkRot90;
	UICheckBox* mChkAutoFix;
	UICheckBox* mChkBlendAdd;
	MapLayer* mCurLayer;
	UIPushButton* mBtnGOTypeAdd;
	Uint32 mCurGOType;
	Uint32 mCurGOFlags;
	UIWidget* mTextureRegionCont;
	UIWidget* mLightCont;
	UIWidget* mObjectCont;
	UIWidget* mSGCont;
	UIWidget* mDICont;
	UICheckBox* mChkDI;
	UITextInput* mDataIdInput;
	UIMenuCheckBox* mLayerChkVisible;
	UIMenuCheckBox* mLayerChkLights;
	UITabWidget* mTabWidget;
	UIMenuCheckBox* mChkShowGrid;
	UIMenuCheckBox* mChkMarkTileOver;
	UIMenuCheckBox* mChkShowBlocked;
	UICheckBox* mChkClampToTile;

	//! Light Color
	UIWidget* mUIBaseColor;
	UISlider* mUIRedSlider;
	UISlider* mUIGreenSlider;
	UISlider* mUIBlueSlider;
	UITextView* mUIRedTxt;
	UITextView* mUIGreenTxt;
	UITextView* mUIBlueTxt;
	UISpinBox* mLightRadius;
	UICheckBox* mLightTypeChk;
	UITextView* mTileBox;
	Int32 mLastSelButtonY;
	bool mMouseScrolling;

	std::vector<UISelectButton*> mObjContButton;

	void onRedChange( const Event* Event );

	void onGreenChange( const Event* Event );

	void onBlueChange( const Event* Event );

	void createLighContainer();

	UISelectButton* addObjContButton( String text, Uint32 mode );

	void createObjectsContainer();

	void createTextureRegionContainer( Int32 Width );

	void windowClose( const Event* Event );

	void cextureAtlasOpen( const Event* Event );

	void createME();

	void createMenuBar();

	void createETGMenu();

	void createUIMap();

	void fillSGCombo();

	void fillTextureRegionList();

	void createNewMap();

	void fileMenuClick( const Event* Event );

	void viewMenuClick( const Event* Event );

	void mapMenuClick( const Event* Event );

	void layerMenuClick( const Event* Event );

	void onTextureAtlasChange( const Event* Event );

	void mapOpen( const Event* Event );

	void mapSave( const Event* Event );

	void onTextureRegionChange( const Event* Event );

	void onTypeChange( const Event* Event );

	void onScrollMapH( const Event* Event );

	void onScrollMapV( const Event* Event );

	void onMapSizeChange( const Event* Event = NULL );

	void onLayerSelect( const Event* Event );

	void mapCreated();

	void chkClickMirrored( const Event* Event );

	void chkClickFlipped( const Event* Event );

	void chkClickBlocked( const Event* Event );

	void chkClickAnimated( const Event* Event );

	void chkClickRot90( const Event* Event );

	void chkClickAutoFix( const Event* Event );

	void chkClickDI( const Event* Event );

	void chkClickClampToTile( const Event* Event );

	void onMapMouseDown( const Event* Event );

	void onMapMouseClick( const Event* Event );

	void onLayerAdd( Private::UIMapLayerNew* UILayer );

	void addNewGOType( const Event* Event );

	void onMapClose( const Event* Event );

	void onNewLight( const Event* Event );

	void onLightRadiusChangeVal( const Event* Event );

	void onLightTypeChange( const Event* Event );

	void onLightSelect( MapLight* Light );

	void onLightRadiusChange( MapLight* Light );

	void onObjectModeSel( const Event* Event );

	void onNewGOTypeAdded( std::string name, String::HashType hash );

	void onAddObject( Uint32 Type, Polygon2f poly );

	void updateGfx();

	void updateFlags();

	void addGameObjectToTile();

	void removeGameObjectFromTile();

	void addGameObject();

	void removeGameObject();

	GameObject* createGameObject();

	void moveLayerUp();

	void moveLayerDown();

	void removeLayer();

	void refreshLayersList();

	void createNewEmptyMap();

	void fillGotyList();

	void refreshGotyList();

	void setViewOptions();

	GameObject* getCurrentGOOver();

	void zoomIn();

	void zoomOut();

	UIMessageBox* createAlert( const String& title, const String& text );

	UIMessageBox* createNoLayerAlert( const String title );

	void onTabSelected( const Event* Event );

	void createTabs();

	void onMapLoad();

	void updateScroll();

	void addShortcut( const KeyBindings::Shortcut& shortcut, const std::string& cmd,
					  std::function<void()> func );
};

}} // namespace EE::Maps

#endif
