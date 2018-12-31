#ifndef EE_UICUITOOLTIP_HPP
#define EE_UICUITOOLTIP_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace Graphics {
class Text;
}}

namespace EE { namespace UI {

class EE_API UITooltip : public UIWidget {
	public:
		static UITooltip * New();

		UITooltip();

		virtual ~UITooltip();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void setTheme( UITheme * Theme );

		void show();

		void hide();

		virtual void draw();

		Graphics::Font * getFont() const;

		void setFont( Graphics::Font * font );

		virtual const String& getText();

		virtual void setText( const String& text );

		const Color& getFontColor() const;

		void setFontColor( const Color& color );

		const Color& getFontShadowColor() const;

		void setFontShadowColor( const Color& color );

		virtual void onTextChanged();

		virtual void onFontChanged();

		virtual void setPadding( const Rectf& padding );

		const Rectf& getPadding() const;

		Text * getTextCache();

		Float getTextWidth();

		Float getTextHeight();

		const int& getNumLines() const;

		Vector2f getAlignOffset();

		void setTooltipTime( const Time& Time );

		void addTooltipTime( const Time & Time );

		const Time & getTooltipTime() const;

		UINode * getTooltipOf() const;

		void setTooltipOf(UINode * tooltipOf);

		UITooltipStyleConfig getStyleConfig() const;

		void setStyleConfig(const UITooltipStyleConfig & styleConfig);

		Uint32 getCharacterSize() const;

		UITooltip * setCharacterSize( const Uint32& characterSize );

		UITooltip * setFontStyle( const Uint32 & fontStyle );

		const Uint32& getFontStyle() const;

		const Float& getOutlineThickness() const;

		UITooltip * setOutlineThickness( const Float& outlineThickness );

		const Color& getOutlineColor() const;

		UITooltip * setOutlineColor( const Color& outlineColor );

		virtual bool setAttribute( const NodeAttribute& attribute, const Uint32& state = UIState::StateFlagNormal );
	protected:
		Text *	mTextCache;
		UITooltipStyleConfig mStyleConfig;
		Vector2f 	mAlignOffset;
		Rectf		mRealPadding;
		Time		mTooltipTime;
		UINode *	mTooltipOf;

		virtual void onAlphaChange();

		virtual void onSizeChange();

		virtual void onAutoSize();

		virtual void autoAlign();

		virtual void autoPadding();
};

}}

#endif
