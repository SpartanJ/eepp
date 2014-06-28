#ifndef EE_GAMINGCMAPEDITOR_HPP
#define EE_GAMINGCMAPEDITOR_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/ui/cuiwindow.hpp>
#include <eepp/ui/cuimenucheckbox.hpp>
#include <eepp/ui/cuiselectbutton.hpp>
#include <eepp/gaming/maplayer.hpp>
#include <eepp/gaming/maplight.hpp>
#include <eepp/gaming/gameobject.hpp>

namespace EE { namespace UI {
class cUIMessageBox;
class cUITextBox;
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

		MapEditor( cUIWindow * AttatchTo = NULL, const MapEditorCloseCb& callback = MapEditorCloseCb() );

		~MapEditor();
	protected:
		cUIWindow *			mUIWindow;
		cUIControl *		mUIContainer;
		cUITheme *			mTheme;
		Private::UIMap *	mUIMap;
		MapEditorCloseCb	mCloseCb;
		cUIDropDownList *	mTextureAtlasesList;
		cUIControl *		mWinContainer;
		cUIListBox *		mSubTextureList;
		cUIGfx *			mGfxPreview;
		TextureAtlas *		mCurSG;
		cUIScrollBar *		mMapHScroll;
		cUIScrollBar *		mMapVScroll;
		cUIDropDownList *	mGOTypeList;
		cUIDropDownList *	mLayerList;
		cUICheckBox *		mChkMirrored;
		cUICheckBox *		mChkFliped;
		cUICheckBox *		mChkBlocked;
		cUICheckBox *		mChkAnim;
		cUICheckBox *		mChkRot90;
		cUICheckBox *		mChkAutoFix;
		MapLayer *			mCurLayer;
		cUIPushButton *		mBtnGOTypeAdd;
		Uint32				mCurGOType;
		Uint32				mCurGOFlags;
		cUIComplexControl * mSubTextureCont;
		cUIComplexControl * mLightCont;
		cUIComplexControl * mObjectCont;
		cUIComplexControl * mSGCont;
		cUIComplexControl * mDICont;
		cUICheckBox *		mChkDI;
		cUITextInput *		mDataIdInput;
		cUIMenuCheckBox	*	mLayerChkVisible;
		cUIMenuCheckBox *	mLayerChkLights;
		cUITabWidget *		mTabWidget;
		cUIMenuCheckBox	*	mChkShowGrid;
		cUIMenuCheckBox	*	mChkMarkTileOver;
		cUIMenuCheckBox	*	mChkShowBlocked;
		cUICheckBox	*		mChkClampToTile;

		//! Light Color
		cUIComplexControl *	mUIBaseColor;
		cUISlider *			mUIRedSlider;
		cUISlider *			mUIGreenSlider;
		cUISlider *			mUIBlueSlider;
		cUITextBox *		mUIRedTxt;
		cUITextBox *		mUIGreenTxt;
		cUITextBox *		mUIBlueTxt;
		cUISpinBox *		mLightRadius;
		cUICheckBox *		mLightTypeChk;
		cUITextBox *		mTileBox;
		Int32				mLastSelButtonY;
		bool				mMouseScrolling;

		std::list<cUISelectButton*> mObjContButton;

		void OnRedChange( const cUIEvent * Event );

		void OnGreenChange( const cUIEvent * Event );

		void OnBlueChange( const cUIEvent * Event );

		void CreateLighContainer();

		cUISelectButton * AddObjContButton( String text , Uint32 mode );

		void CreateObjectsContainer();

		void CreateSubTextureContainer( Int32 Width );

		void WindowClose( const cUIEvent * Event );

		void TextureAtlasOpen( const cUIEvent * Event );

		void CreateME();

		void CreateWinMenu();

		void CreateETGMenu();

		void CreateUIMap();

		void FillSGCombo();

		void FillSubTextureList();

		void CreateNewMap();

		void FileMenuClick( const cUIEvent * Event );

		void ViewMenuClick( const cUIEvent * Event );

		void MapMenuClick( const cUIEvent * Event );

		void LayerMenuClick( const cUIEvent * Event );

		void OnTextureAtlasChange( const cUIEvent * Event );

		void MapOpen( const cUIEvent * Event );

		void MapSave( const cUIEvent * Event );

		void OnSubTextureChange( const cUIEvent * Event );

		void OnTypeChange( const cUIEvent * Event );

		void OnScrollMapH( const cUIEvent * Event );

		void OnScrollMapV( const cUIEvent * Event );

		void OnMapSizeChange( const cUIEvent * Event = NULL );

		void OnLayerSelect( const cUIEvent * Event );

		void MapCreated();

		void ChkClickMirrored( const cUIEvent * Event );

		void ChkClickFliped( const cUIEvent * Event );

		void ChkClickBlocked( const cUIEvent * Event );

		void ChkClickAnimated( const cUIEvent * Event );

		void ChkClickRot90( const cUIEvent * Event );

		void ChkClickAutoFix( const cUIEvent * Event );

		void ChkClickDI( const cUIEvent * Event );

		void ChkClickClampToTile( const cUIEvent * Event );

		void OnMapMouseDown( const cUIEvent * Event );

		void OnMapMouseClick( const cUIEvent * Event );

		void OnLayerAdd( Private::UIMapLayerNew * UILayer );

		void AddNewGOType( const cUIEvent * Event );

		void OnMapClose( const cUIEvent * Event );

		void OnNewLight( const cUIEvent * Event );

		void OnLightRadiusChangeVal( const cUIEvent * Event );

		void OnLightTypeChange( const cUIEvent * Event );

		void OnLightSelect( MapLight * Light );

		void OnLightRadiusChange( MapLight * Light );

		void OnObjectModeSel( const cUIEvent * Event );

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

		cUIMessageBox * CreateAlert( const String& title, const String& text );

		cUIMessageBox * CreateNoLayerAlert( const String title );

		void OnTabSelected( const cUIEvent * Event );

		void CreateTabs();

		void OnMapLoad();

		void UpdateScroll();
};

}}

#endif
