#ifndef EE_GAMINGCMAPEDITOR_HPP
#define EE_GAMINGCMAPEDITOR_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uimenucheckbox.hpp>
#include <eepp/ui/uisubtexture.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwinmenu.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uicommondialog.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/tools/textureatlaseditor.hpp>
#include <eepp/gaming/maplayer.hpp>
#include <eepp/gaming/maplight.hpp>
#include <eepp/gaming/gameobject.hpp>

namespace EE { namespace UI {
class UIMessageBox;
class UITextView;
}}

using namespace EE::UI;

namespace EE { namespace Gaming {

namespace Private {
class UIMapLayerNew;
class UIMap;
}

class EE_API MapEditor {
	public:
		typedef cb::Callback0<void> MapEditorCloseCb;

		MapEditor( UIWindow * AttatchTo = NULL, const MapEditorCloseCb& callback = MapEditorCloseCb() );

		~MapEditor();
	protected:
		UIWindow *			mUIWindow;
		UIControl *			mUIContainer;
		UITheme *			mTheme;
		Private::UIMap *	mUIMap;
		MapEditorCloseCb	mCloseCb;
		UIDropDownList *	mTextureAtlasesList;
		UIWidget *	mWinContainer;
		UIListBox *			mSubTextureList;
		UISubTexture *				mGfxPreview;
		TextureAtlas *		mCurSG;
		UIScrollBar *		mMapHScroll;
		UIScrollBar *		mMapVScroll;
		UIDropDownList *	mGOTypeList;
		UIDropDownList *	mLayerList;
		UICheckBox *		mChkMirrored;
		UICheckBox *		mChkFliped;
		UICheckBox *		mChkBlocked;
		UICheckBox *		mChkAnim;
		UICheckBox *		mChkRot90;
		UICheckBox *		mChkAutoFix;
		MapLayer *			mCurLayer;
		UIPushButton *		mBtnGOTypeAdd;
		Uint32				mCurGOType;
		Uint32				mCurGOFlags;
		UIWidget *	mSubTextureCont;
		UIWidget *	mLightCont;
		UIWidget *	mObjectCont;
		UIWidget *	mSGCont;
		UIWidget *	mDICont;
		UICheckBox *		mChkDI;
		UITextInput *		mDataIdInput;
		UIMenuCheckBox	*	mLayerChkVisible;
		UIMenuCheckBox *	mLayerChkLights;
		UITabWidget *		mTabWidget;
		UIMenuCheckBox	*	mChkShowGrid;
		UIMenuCheckBox	*	mChkMarkTileOver;
		UIMenuCheckBox	*	mChkShowBlocked;
		UICheckBox	*		mChkClampToTile;

		//! Light Color
		UIWidget *	mUIBaseColor;
		UISlider *			mUIRedSlider;
		UISlider *			mUIGreenSlider;
		UISlider *			mUIBlueSlider;
		UITextView *			mUIRedTxt;
		UITextView *			mUIGreenTxt;
		UITextView *			mUIBlueTxt;
		UISpinBox *			mLightRadius;
		UICheckBox *		mLightTypeChk;
		UITextView *			mTileBox;
		Int32				mLastSelButtonY;
		bool				mMouseScrolling;

		std::list<UISelectButton*> mObjContButton;

		void onRedChange( const UIEvent * Event );

		void onGreenChange( const UIEvent * Event );

		void onBlueChange( const UIEvent * Event );

		void createLighContainer();

		UISelectButton * addObjContButton( String text , Uint32 mode );

		void createObjectsContainer();

		void createSubTextureContainer( Int32 Width );

		void windowClose( const UIEvent * Event );

		void cextureAtlasOpen( const UIEvent * Event );

		void createME();

		void createWinMenu();

		void createETGMenu();

		void createUIMap();

		void fillSGCombo();

		void fillSubTextureList();

		void createNewMap();

		void fileMenuClick( const UIEvent * Event );

		void viewMenuClick( const UIEvent * Event );

		void mapMenuClick( const UIEvent * Event );

		void layerMenuClick( const UIEvent * Event );

		void onTextureAtlasChange( const UIEvent * Event );

		void mapOpen( const UIEvent * Event );

		void mapSave( const UIEvent * Event );

		void onSubTextureChange( const UIEvent * Event );

		void onTypeChange( const UIEvent * Event );

		void onScrollMapH( const UIEvent * Event );

		void onScrollMapV( const UIEvent * Event );

		void onMapSizeChange( const UIEvent * Event = NULL );

		void onLayerSelect( const UIEvent * Event );

		void mapCreated();

		void chkClickMirrored( const UIEvent * Event );

		void chkClickFlipped( const UIEvent * Event );

		void chkClickBlocked( const UIEvent * Event );

		void chkClickAnimated( const UIEvent * Event );

		void chkClickRot90( const UIEvent * Event );

		void chkClickAutoFix( const UIEvent * Event );

		void chkClickDI( const UIEvent * Event );

		void chkClickClampToTile( const UIEvent * Event );

		void onMapMouseDown( const UIEvent * Event );

		void onMapMouseClick( const UIEvent * Event );

		void onLayerAdd( Private::UIMapLayerNew * UILayer );

		void addNewGOType( const UIEvent * Event );

		void onMapClose( const UIEvent * Event );

		void onNewLight( const UIEvent * Event );

		void onLightRadiusChangeVal( const UIEvent * Event );

		void onLightTypeChange( const UIEvent * Event );

		void onLightSelect( MapLight * Light );

		void onLightRadiusChange( MapLight * Light );

		void onObjectModeSel( const UIEvent * Event );

		void onNewGOTypeAdded( std::string name, Uint32 hash );

		void onAddObject( Uint32 Type, Polygon2f poly );

		void updateGfx();

		void updateFlags();

		void addGameObjectToTile();

		void removeGameObjectFromTile();

		void addGameObject();

		void removeGameObject();

		GameObject * createGameObject();

		void moveLayerUp();

		void moveLayerDown();

		void removeLayer();

		void refreshLayersList();

		void createNewEmptyMap();

		void fillGotyList();

		void refreshGotyList();

		void setViewOptions();

		GameObject * getCurrentGOOver();

		void zoomIn();

		void zoomOut();

		UIMessageBox * createAlert( const String& title, const String& text );

		UIMessageBox * createNoLayerAlert( const String title );

		void onTabSelected( const UIEvent * Event );

		void createTabs();

		void onMapLoad();

		void updateScroll();
};

}}

#endif
