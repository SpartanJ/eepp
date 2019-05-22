#ifndef EE_GAMINGCUIMAPNEW_HPP
#define EE_GAMINGCUIMAPNEW_HPP

#include <eepp/maps/base.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uislider.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/maps/mapeditor/uimap.hpp>

using namespace EE::UI;

namespace EE { namespace Maps { namespace Private {

class EE_API UIMapNew {
	public:
		UIMapNew( UIMap * Map, std::function<void()> NewMapCb = cb::Callback0<void>(), bool ResizeMap = false );

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

		std::function<void()> mNewMapCb;

		Sizei				mNewSize;
		bool				mResizeMap;

		void onWindowClose( const Event * Event );

		void onCancelClick( const Event * Event );

		void onOKClick( const Event * Event );

		void onRedChange( const Event * Event );

		void onGreenChange( const Event * Event );

		void onBlueChange( const Event * Event );
};

}}}

#endif
