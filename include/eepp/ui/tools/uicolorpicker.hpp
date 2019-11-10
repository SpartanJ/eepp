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
class DrawableGroup;
}}

namespace EE { namespace UI { namespace Tools {

class EE_API UIColorPicker {
	public:
		typedef std::function<void( Color color )> ColorPickedCb;
		typedef std::function<void()> ColorPickerCloseCb;

		static UIColorPicker * New( UIWindow * attach = NULL, const ColorPickedCb& colorPickedCb = ColorPickedCb(), const ColorPickerCloseCb& closeCb = ColorPickerCloseCb() );

		UIColorPicker( UIWindow * attach = NULL, const ColorPickedCb& colorPickedCb = ColorPickedCb(), const ColorPickerCloseCb& closeCb = ColorPickerCloseCb() );

		virtual ~UIColorPicker();

		void setColor( const Color& color );

		const Color& getColor() const;

		void setHsvColor( const Colorf& color );

		const Colorf& getHsvColor() const;
	protected:
		UIWindow *			mUIWindow;
		Node *				mUIContainer;
		ColorPickedCb		mPickedCb;
		ColorPickerCloseCb	mCloseCb;
		Texture *			mHueTexture;
		DrawableGroup *		mColorRectangle;
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
		bool				mUpdating;
		std::map<UIWidget*, Uint32> mEventsIds;

		void windowClose( const Event * Event );

		Texture * createHueTexture( const Sizef& size );

		void updateColorPicker();

		void updateGuideLines();

		void updateChannelWidgets();

		void updateAll();

		void registerEvents();

		void unregisterEvents();

		void onColorPickerEvent( const MouseEvent* mouseEvent );

		void onHuePickerEvent( const MouseEvent* mouseEvent );
};

}}}

#endif
