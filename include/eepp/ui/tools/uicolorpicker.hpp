#ifndef EE_UI_TOOLS_UICOLORPICKER_HPP
#define EE_UI_TOOLS_UICOLORPICKER_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uirelativelayout.hpp>
#include <eepp/ui/uislider.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uiwindow.hpp>

namespace EE { namespace UI { namespace Tools {

class EE_API UIColorPicker {
  public:
	typedef std::function<void( Color color )> ColorPickedCb;

	static UIColorPicker* NewModal( Node* nodeCreator,
									const ColorPickedCb& colorPickedCb = ColorPickedCb(),
									const Uint8& modalAlpha = 120,
									const Uint32& winFlags = UI_WIN_NO_DECORATION | UI_WIN_MODAL |
															 UI_WIN_DRAGABLE_CONTAINER |
															 UI_WIN_FRAME_BUFFER,
									const Sizef& winSize = Sizef( 320, 470 ) );

	static UIColorPicker*
	NewWindow( const ColorPickedCb& colorPickedCb = ColorPickedCb(),
			   const Uint32& winFlags = UI_WIN_DEFAULT_FLAGS | UI_WIN_SHADOW | UI_WIN_FRAME_BUFFER,
			   const Sizef& winSize = Sizef( 320, 478 ), const Uint8& modalAlpha = 0 );

	static UIColorPicker* New( UIWindow* attach = NULL,
							   const ColorPickedCb& colorPickedCb = ColorPickedCb(),
							   const Uint8& modalAlpha = 0 );

	UIColorPicker( UIWindow* attach = NULL, const ColorPickedCb& colorPickedCb = ColorPickedCb(),
				   const Uint8& modalAlpha = 0 );

	void setColor( const Color& color, bool hexColorNeedsUpdate = true );

	const Color& getColor() const;

	void setHsvColor( const Colorf& color );

	const Colorf& getHsvColor() const;

	UIWindow* getUIWindow() const;

	void closePicker();

  protected:
	UIWindow* mUIWindow{ nullptr };
	Node* mUIContainer{ nullptr };
	UIWidget* mRoot{ nullptr };
	ColorPickedCb mPickedCb;
	UIImage* mColorPicker{ nullptr };
	UIImage* mHuePicker{ nullptr };
	UIWidget* mVerticalLine{ nullptr };
	UIWidget* mHorizontalLine{ nullptr };
	UIWidget* mHueLine{ nullptr };
	UIWidget* mCurrentColor{ nullptr };
	UILinearLayout* mRedContainer{ nullptr };
	UILinearLayout* mGreenContainer{ nullptr };
	UILinearLayout* mBlueContainer{ nullptr };
	UILinearLayout* mAlphaContainer{ nullptr };
	UILinearLayout* mFooter{ nullptr };
	UITextInput* mTextInput{ nullptr };
	Colorf mHsv;
	Color mRgb;
	std::string mHexColor;
	Uint8 mModalAlpha;
	Uint8 mDefModalAlpha;
	bool mUpdating;

	void onKeyDown( const KeyEvent& event );

	void windowClose( const Event* Event );

	Texture* createHueTexture( const Sizef& size );

	Texture* createGridTexture();

	void updateColorPicker();

	void updateGuideLines();

	void updateChannelWidgets();

	void updateAll();

	void registerEvents();

	void onColorPickerEvent( const MouseEvent* mouseEvent );

	void onHuePickerEvent( const MouseEvent* mouseEvent );

	Uint8 getModalAlpha() const;

	void setModalAlpha( const Uint8& modalAlpha );

	void onColorPicked( const std::string& buffer );
};

}}} // namespace EE::UI::Tools

#endif
