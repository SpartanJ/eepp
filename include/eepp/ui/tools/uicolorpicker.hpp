#ifndef EE_UI_TOOLS_UICOLORPICKER_HPP
#define EE_UI_TOOLS_UICOLORPICKER_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uirelativelayout.hpp>
#include <eepp/ui/uislider.hpp>
#include <eepp/ui/uispinbox.hpp>

namespace EE { namespace Graphics {
class Texture;
}}

namespace EE { namespace UI { namespace Tools {

class EE_API UIColorPicker {
	public:
		typedef std::function<void( Color color )> ColorPickedCb;
		typedef std::function<void()> ColorPickerCloseCb;

		static UIColorPicker * New( UIWindow * attach = NULL, const ColorPickedCb& colorPickedCb = ColorPickedCb(), const ColorPickerCloseCb& closeCb = ColorPickerCloseCb() );

		UIColorPicker( UIWindow * attach = NULL, const ColorPickedCb& colorPickedCb = ColorPickedCb(), const ColorPickerCloseCb& closeCb = ColorPickerCloseCb() );

		virtual ~UIColorPicker();
	protected:
		UIWindow *			mUIWindow;
		Node *				mUIContainer;
		ColorPickedCb		mPickedCb;
		ColorPickerCloseCb	mCloseCb;
		Texture *			mHueTexture;
		RectangleDrawable * mColorRectangle;
		UIImage *			mColorPicker;
		UIImage *			mHuePicker;
		Colorf				mHsv;
		Color				mRgb;

		void windowClose( const Event * Event );

		Texture * createHueTexture( const Sizef& size );

		void updateColorPicker();
};

}}}

#endif
