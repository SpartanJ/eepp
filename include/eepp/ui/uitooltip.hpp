#ifndef EE_UICUITOOLTIP_HPP
#define EE_UICUITOOLTIP_HPP

#include <eepp/ui/uinode.hpp>

namespace EE { namespace Graphics {
class Text;
}}

namespace EE { namespace UI {

class EE_API UITooltip : public UINode {
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

		virtual void setAlpha( const Float& alpha );

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

		virtual void setPadding( const Rect& padding );

		const Rect& getPadding() const;

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
	protected:
		Text *	mTextCache;
		UITooltipStyleConfig mStyleConfig;
		Vector2f 	mAlignOffset;
		Rect		mRealPadding;
		Time		mTooltipTime;
		UINode *	mTooltipOf;

		virtual void onSizeChange();

		virtual void onAutoSize();

		virtual void autoAlign();

		virtual void autoPadding();
};

}}

#endif
