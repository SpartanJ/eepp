#ifndef EE_GAMINGCMAPEDITOR_HPP
#define EE_GAMINGCMAPEDITOR_HPP

#include "base.hpp"
#include "../../ui/cuiwindow.hpp"
#include "../../ui/cuimenucheckbox.hpp"
#include "cuimap.hpp"

using namespace EE::UI;

namespace EE { namespace Gaming { namespace MapEditor {

class cUILayerNew;

class EE_API cMapEditor {
	public:
		typedef cb::Callback0<void> MapEditorCloseCb;

		cMapEditor( cUIWindow * AttatchTo = NULL, const MapEditorCloseCb& callback = MapEditorCloseCb() );

		~cMapEditor();
	protected:
		cUIWindow *			mUIWindow;
		cUIControl *		mUIContainer;
		cUITheme *			mTheme;
		cUIMap *			mUIMap;
		MapEditorCloseCb	mCloseCb;
		cUIDropDownList *	mShapeGroupsList;
		cUIControl *		mWinContainer;
		cUIListBox *		mShapeList;
		cUIGfx *			mGfxPreview;
		cShapeGroup *		mCurSG;
		cUIScrollBar *		mMapHScroll;
		cUIScrollBar *		mMapVScroll;
		cUIDropDownList *	mGOTypeList;
		cUIDropDownList *	mLayerList;
		cUICheckBox *		mChkMirrored;
		cUICheckBox *		mChkFliped;
		cUICheckBox *		mChkBlocked;
		cUICheckBox *		mChkAnim;
		cLayer *			mCurLayer;
		cUIPushButton *		mBtnGOTypeAdd;
		Uint32				mCurGOType;
		Uint32				mCurGOFlags;
		cUIComplexControl * mRPCont;
		cUIComplexControl * mSGCont;
		cUIComplexControl * mDICont;
		cUICheckBox *		mChkDI;
		cUITextInput *		mDataIdInput;
		cUIMenuCheckBox	*	mLayerChkVisible;
		cUIMenuCheckBox *	mLayerChkLights;

		void WindowClose( const cUIEvent * Event );

		void TextureGroupOpen( const cUIEvent * Event );

		void CreateME();

		void CreateWinMenu();

		void CreateETGMenu();

		void CreateUIMap();

		void FillSGCombo();

		void FillShapeList();

		void CreateNewMap();

		void FileMenuClick( const cUIEvent * Event );

		void ViewMenuClick( const cUIEvent * Event );

		void MapMenuClick( const cUIEvent * Event );

		void LayerMenuClick( const cUIEvent * Event );

		void OnShapeGroupChange( const cUIEvent * Event );

		void MapOpen( const cUIEvent * Event );

		void MapSave( const cUIEvent * Event );

		void OnShapeChange( const cUIEvent * Event );

		void OnTypeChange( const cUIEvent * Event );

		void OnScrollMapH( const cUIEvent * Event );

		void OnScrollMapV( const cUIEvent * Event );

		void OnMapSizeChange( const cUIEvent * Event );

		void OnLayerSelect( const cUIEvent * Event );

		void MapCreated();

		void ChkClickMirrored( const cUIEvent * Event );

		void ChkClickFliped( const cUIEvent * Event );

		void ChkClickBlocked( const cUIEvent * Event );

		void ChkClickAnimated( const cUIEvent * Event );

		void ChkClickDI( const cUIEvent * Event );

		void OnMapMouseDown( const cUIEvent * Event );

		void OnMapMouseClick( const cUIEvent * Event );

		void OnLayerAdd( cUILayerNew * UILayer );

		void AddNewGOType( const cUIEvent * Event );

		void OnMapClose( const cUIEvent * Event );

		void OnNewGOTypeAdded( std::string name, Uint32 hash );

		void UpdateGfx();

		void UpdateFlags();

		void AddGameObjectToTile();

		void RemoveGameObjectFromTile();

		void AddGameObject();

		void RemoveGameObject();

		cGameObject * CreateGameObject();

		void MoveLayerUp();

		void MoveLayerDown();

		void RemoveLayer();

		void RefreshLayersList();

		void CreateNewEmptyMap();
};

}}}

#endif
