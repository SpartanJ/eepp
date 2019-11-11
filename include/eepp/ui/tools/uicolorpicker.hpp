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

namespace EE { namespace UI { namespace Tools {

class EE_API UIColorPicker {
	public:
		typedef std::function<void( Color color )> ColorPickedCb;
		typedef std::function<void()> ColorPickerCloseCb;

		static UIColorPicker * NewWindow( const ColorPickedCb& colorPickedCb = ColorPickedCb(), const ColorPickerCloseCb& closeCb = ColorPickerCloseCb(), const Uint32& winFlags = UI_WIN_DEFAULT_FLAGS | UI_WIN_SHADOW | UI_WIN_FRAME_BUFFER, const Sizef& winSize = Sizef( 320, 478 ) );

		static UIColorPicker * New( UIWindow * attach = NULL, const ColorPickedCb& colorPickedCb = ColorPickedCb(), const ColorPickerCloseCb& closeCb = ColorPickerCloseCb() );

		UIColorPicker( UIWindow * attach = NULL, const ColorPickedCb& colorPickedCb = ColorPickedCb(), const ColorPickerCloseCb& closeCb = ColorPickerCloseCb() );

		void setColor( const Color& color );

		const Color& getColor() const;

		void setHsvColor( const Colorf& color );

		const Colorf& getHsvColor() const;

		UIWindow* getUIWindow() const;
	protected:
		UIWindow *			mUIWindow;
		Node *				mUIContainer;
		UIWidget *			mRoot;
		ColorPickedCb		mPickedCb;
		ColorPickerCloseCb	mCloseCb;
		UIImage *			mColorPicker;
		UIImage *			mHuePicker;
		UIWidget *			mVerticalLine;
		UIWidget *			mHorizontalLine;
		UIWidget *			mHueLine;
		UIWidget *			mCurrentColor;
		UILinearLayout *	mRedContainer;
		UILinearLayout *	mGreenContainer;
		UILinearLayout *	mBlueContainer;
		UILinearLayout *	mAlphaContainer;
		UILinearLayout *	mFooter;
		Colorf				mHsv;
		Color				mRgb;
		std::string			mHexColor;
		bool				mUpdating;

		void windowClose( const Event * Event );

		Texture * createHueTexture( const Sizef& size );

		Texture * createGridTexture();

		void updateColorPicker();

		void updateGuideLines();

		void updateChannelWidgets();

		void updateAll();

		void registerEvents();

		void onColorPickerEvent( const MouseEvent* mouseEvent );

		void onHuePickerEvent( const MouseEvent* mouseEvent );
};

}}}

#endif
