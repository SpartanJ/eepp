#ifndef EE_GAMINGCUIMAPNEW_HPP
#define EE_GAMINGCUIMAPNEW_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/gaming/mapeditor/uimap.hpp>

using namespace EE::UI;

namespace EE { namespace Gaming { namespace Private {

class EE_API UIMapNew {
	public:
		UIMapNew( UIMap * Map, cb::Callback0<void> NewMapCb = cb::Callback0<void>(), bool ResizeMap = false );

		virtual ~UIMapNew();
	protected:
		UITheme *			mTheme;
		UIWindow *			mUIWindow;
		UIMap *			mUIMap;
		UISpinBox *		mUIMapWidth;
		UISpinBox *		mUIMapHeight;
		UISpinBox *		mUIMapTWidth;
		UISpinBox *		mUIMapTHeight;
		UISpinBox *		mUIMapMaxLayers;
		UICheckBox *		mUILightsEnabled;
		UICheckBox *		mUILightsByVertex;
		UICheckBox *		mUIClampBorders;
		UICheckBox *		mUIClipArea;
		UIComplexControl *	mUIBaseColor;
		UISlider *			mUIRedSlider;
		UISlider *			mUIGreenSlider;
		UISlider *			mUIBlueSlider;
		UITextBox *		mUIRedTxt;
		UITextBox *		mUIGreenTxt;
		UITextBox *		mUIBlueTxt;

		cb::Callback0<void> mNewMapCb;

		Sizei				mNewSize;
		bool				mResizeMap;

		void WindowClose( const UIEvent * Event );

		void CancelClick( const UIEvent * Event );

		void OKClick( const UIEvent * Event );

		void OnRedChange( const UIEvent * Event );

		void OnGreenChange( const UIEvent * Event );

		void OnBlueChange( const UIEvent * Event );
};

}}}

#endif
