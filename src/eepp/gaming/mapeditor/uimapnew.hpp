#ifndef EE_GAMINGCUIMAPNEW_HPP
#define EE_GAMINGCUIMAPNEW_HPP

#include <eepp/gaming/base.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uislider.hpp>
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
		UIWidget *	mUIBaseColor;
		UISlider *			mUIRedSlider;
		UISlider *			mUIGreenSlider;
		UISlider *			mUIBlueSlider;
		UITextView *		mUIRedTxt;
		UITextView *		mUIGreenTxt;
		UITextView *		mUIBlueTxt;

		cb::Callback0<void> mNewMapCb;

		Sizei				mNewSize;
		bool				mResizeMap;

		void onWindowClose( const UIEvent * Event );

		void onCancelClick( const UIEvent * Event );

		void onOKClick( const UIEvent * Event );

		void onRedChange( const UIEvent * Event );

		void onGreenChange( const UIEvent * Event );

		void onBlueChange( const UIEvent * Event );
};

}}}

#endif
