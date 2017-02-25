#ifndef EE_UICUITOOLTIP_HPP
#define EE_UICUITOOLTIP_HPP

#include <eepp/ui/uicontrolanim.hpp>

namespace EE { namespace Graphics {
class TextCache;
}}

namespace EE { namespace UI {

class EE_API UITooltip : public UIControlAnim {
	public:
		class CreateParams : public UIControlAnim::CreateParams {
			public:
				inline CreateParams() :
					UIControlAnim::CreateParams(),
					Font( NULL ),
					FontColor( 0, 0, 0, 255 ),
					FontShadowColor( 255, 255, 255, 150 )
				{
					UITheme * Theme = UIThemeManager::instance()->getDefaultTheme();

					if ( NULL != Theme ) {
						Font			= Theme->getFont();
						FontColor		= Theme->getFontColor();
						FontShadowColor	= Theme->getFontShadowColor();
					}

					if ( NULL == Font )
						Font = UIThemeManager::instance()->getDefaultFont();
				}

				inline ~CreateParams() {}

				Graphics::Font * 	Font;
				ColorA				FontColor;
				ColorA				FontShadowColor;
				Recti				Padding;
		};

		UITooltip( UITooltip::CreateParams& Params, UIControl * TooltipOf );

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

		const ColorA& getColor() const;

		void setColor( const ColorA& color );

		const ColorA& getShadowColor() const;

		void setShadowColor( const ColorA& color );

		virtual void onTextChanged();

		virtual void onFontChanged();

		virtual void setPadding( const Recti& padding );

		const Recti& getPadding() const;

		TextCache * getTextCache();

		Float getTextWidth();

		Float getTextHeight();

		const int& getNumLines() const;

		Vector2f getAlignOffset();

		void setTooltipTime( const Time& Time );

		void addTooltipTime( const Time & Time );

		const Time & getTooltipTime() const;
	protected:
		TextCache *	mTextCache;
		ColorA 		mFontColor;
		ColorA 		mFontShadowColor;
		Vector2f 	mAlignOffset;
		Recti		mPadding;
		Recti		mRealPadding;
		Time		mTooltipTime;
		UIControl *	mTooltipOf;

		virtual void onSizeChange();

		virtual void autoSize();

		virtual void autoAlign();

		virtual void autoPadding();
};

}}

#endif
