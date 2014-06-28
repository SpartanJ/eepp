#ifndef EE_GAMINGCUIMAPNEW_HPP
#define EE_GAMINGCUIMAPNEW_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/ui/cuiwindow.hpp>
#include <eepp/gaming/mapeditor/uimap.hpp>

using namespace EE::UI;

namespace EE { namespace Gaming { namespace Private {

class EE_API UIMapNew {
	public:
		UIMapNew( UIMap * Map, cb::Callback0<void> NewMapCb = cb::Callback0<void>(), bool ResizeMap = false );

		virtual ~UIMapNew();
	protected:
		cUITheme *			mTheme;
		cUIWindow *			mUIWindow;
		UIMap *			mUIMap;
		cUISpinBox *		mUIMapWidth;
		cUISpinBox *		mUIMapHeight;
		cUISpinBox *		mUIMapTWidth;
		cUISpinBox *		mUIMapTHeight;
		cUISpinBox *		mUIMapMaxLayers;
		cUICheckBox *		mUILightsEnabled;
		cUICheckBox *		mUILightsByVertex;
		cUICheckBox *		mUIClampBorders;
		cUICheckBox *		mUIClipArea;
		cUIComplexControl *	mUIBaseColor;
		cUISlider *			mUIRedSlider;
		cUISlider *			mUIGreenSlider;
		cUISlider *			mUIBlueSlider;
		cUITextBox *		mUIRedTxt;
		cUITextBox *		mUIGreenTxt;
		cUITextBox *		mUIBlueTxt;

		cb::Callback0<void> mNewMapCb;

		Sizei				mNewSize;
		bool				mResizeMap;

		void WindowClose( const cUIEvent * Event );

		void CancelClick( const cUIEvent * Event );

		void OKClick( const cUIEvent * Event );

		void OnRedChange( const cUIEvent * Event );

		void OnGreenChange( const cUIEvent * Event );

		void OnBlueChange( const cUIEvent * Event );
};

}}}

#endif
