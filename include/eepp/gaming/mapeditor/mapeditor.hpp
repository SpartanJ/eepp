#ifndef EE_GAMINGCMAPEDITOR_HPP
#define EE_GAMINGCMAPEDITOR_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uimenucheckbox.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/gaming/maplayer.hpp>
#include <eepp/gaming/maplight.hpp>
#include <eepp/gaming/gameobject.hpp>

namespace EE { namespace UI {
class UIMessageBox;
class UITextBox;
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
		UIControl *		mUIContainer;
		UITheme *			mTheme;
		Private::UIMap *	mUIMap;
		MapEditorCloseCb	mCloseCb;
		UIDropDownList *	mTextureAtlasesList;
		UIControl *		mWinContainer;
		UIListBox *		mSubTextureList;
		UIGfx *			mGfxPreview;
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
		UIComplexControl * mSubTextureCont;
		UIComplexControl * mLightCont;
		UIComplexControl * mObjectCont;
		UIComplexControl * mSGCont;
		UIComplexControl * mDICont;
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
		UIComplexControl *	mUIBaseColor;
		UISlider *			mUIRedSlider;
		UISlider *			mUIGreenSlider;
		UISlider *			mUIBlueSlider;
		UITextBox *		mUIRedTxt;
		UITextBox *		mUIGreenTxt;
		UITextBox *		mUIBlueTxt;
		UISpinBox *		mLightRadius;
		UICheckBox *		mLightTypeChk;
		UITextBox *		mTileBox;
		Int32				mLastSelButtonY;
		bool				mMouseScrolling;

		std::list<UISelectButton*> mObjContButton;

		void OnRedChange( const UIEvent * Event );

		void OnGreenChange( const UIEvent * Event );

		void OnBlueChange( const UIEvent * Event );

		void CreateLighContainer();

		UISelectButton * AddObjContButton( String text , Uint32 mode );

		void CreateObjectsContainer();

		void CreateSubTextureContainer( Int32 Width );

		void WindowClose( const UIEvent * Event );

		void TextureAtlasOpen( const UIEvent * Event );

		void CreateME();

		void CreateWinMenu();

		void CreateETGMenu();

		void CreateUIMap();

		void FillSGCombo();

		void FillSubTextureList();

		void CreateNewMap();

		void FileMenuClick( const UIEvent * Event );

		void ViewMenuClick( const UIEvent * Event );

		void MapMenuClick( const UIEvent * Event );

		void LayerMenuClick( const UIEvent * Event );

		void OnTextureAtlasChange( const UIEvent * Event );

		void MapOpen( const UIEvent * Event );

		void MapSave( const UIEvent * Event );

		void OnSubTextureChange( const UIEvent * Event );

		void OnTypeChange( const UIEvent * Event );

		void OnScrollMapH( const UIEvent * Event );

		void OnScrollMapV( const UIEvent * Event );

		void OnMapSizeChange( const UIEvent * Event = NULL );

		void OnLayerSelect( const UIEvent * Event );

		void MapCreated();

		void ChkClickMirrored( const UIEvent * Event );

		void ChkClickFliped( const UIEvent * Event );

		void ChkClickBlocked( const UIEvent * Event );

		void ChkClickAnimated( const UIEvent * Event );

		void ChkClickRot90( const UIEvent * Event );

		void ChkClickAutoFix( const UIEvent * Event );

		void ChkClickDI( const UIEvent * Event );

		void ChkClickClampToTile( const UIEvent * Event );

		void OnMapMouseDown( const UIEvent * Event );

		void OnMapMouseClick( const UIEvent * Event );

		void OnLayerAdd( Private::UIMapLayerNew * UILayer );

		void AddNewGOType( const UIEvent * Event );

		void OnMapClose( const UIEvent * Event );

		void OnNewLight( const UIEvent * Event );

		void OnLightRadiusChangeVal( const UIEvent * Event );

		void OnLightTypeChange( const UIEvent * Event );

		void OnLightSelect( MapLight * Light );

		void OnLightRadiusChange( MapLight * Light );

		void OnObjectModeSel( const UIEvent * Event );

		void OnNewGOTypeAdded( std::string name, Uint32 hash );

		void OnAddObject( Uint32 Type, Polygon2f poly );

		void UpdateGfx();

		void UpdateFlags();

		void AddGameObjectToTile();

		void RemoveGameObjectFromTile();

		void AddGameObject();

		void RemoveGameObject();

		GameObject * CreateGameObject();

		void MoveLayerUp();

		void MoveLayerDown();

		void RemoveLayer();

		void RefreshLayersList();

		void CreateNewEmptyMap();

		void FillGotyList();

		void RefreshGotyList();

		void SetViewOptions();

		GameObject * GetCurrentGOOver();

		void ZoomIn();

		void ZoomOut();

		UIMessageBox * CreateAlert( const String& title, const String& text );

		UIMessageBox * CreateNoLayerAlert( const String title );

		void OnTabSelected( const UIEvent * Event );

		void CreateTabs();

		void OnMapLoad();

		void UpdateScroll();
};

}}

#endif
