#ifndef EE_GAMINGCMAPEDITOR_HPP
#define EE_GAMINGCMAPEDITOR_HPP

#include "base.hpp"
#include "../../ui/cuiwindow.hpp"
#include "cuimap.hpp"

using namespace EE::UI;

namespace EE { namespace Gaming { namespace MapEditor {

class cMapEditor {
	public:
		typedef cb::Callback0<void> MapEditorCloseCb;

		cMapEditor( cUIWindow * AttatchTo = NULL, const MapEditorCloseCb& callback = MapEditorCloseCb() );

		~cMapEditor();

		void CloseCallback( const MapEditorCloseCb& callback );
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
		cUIDropDownList *	mTypeAdd;
		cUIDropDownList *	mLayerList;

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

		void EditMenuClick( const cUIEvent * Event );

		void ViewMenuClick( const cUIEvent * Event );

		void MapMenuClick( const cUIEvent * Event );

		void LayerMenuClick( const cUIEvent * Event );

		void OnShapeGroupChange( const cUIEvent * Event );

		void OnShapeChange( const cUIEvent * Event );

		void OnScrollMapH( const cUIEvent * Event );

		void OnScrollMapV( const cUIEvent * Event );

		void OnMapSizeChange( const cUIEvent * Event );
};

}}}

#endif
