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
					UITheme * Theme = UIThemeManager::instance()->defaultTheme();

					if ( NULL != Theme ) {
						Font			= Theme->font();
						FontColor		= Theme->fontColor();
						FontShadowColor	= Theme->fontShadowColor();
					}

					if ( NULL == Font )
						Font = UIThemeManager::instance()->defaultFont();
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

		virtual void alpha( const Float& alpha );

		Graphics::Font * font() const;

		void font( Graphics::Font * font );

		virtual const String& text();

		virtual void text( const String& text );

		const ColorA& color() const;

		void color( const ColorA& color );

		const ColorA& shadowColor() const;

		void shadowColor( const ColorA& color );

		virtual void onTextChanged();

		virtual void onFontChanged();

		virtual void padding( const Recti& padding );

		const Recti& padding() const;

		TextCache * getTextCache();

		Float getTextWidth();

		Float getTextHeight();

		const int& getNumLines() const;

		const Vector2f& alignOffset() const;

		void tooltipTime( const Time& Time );

		void tooltipTimeAdd( const Time & Time );

		const Time & tooltipTime() const;
	protected:
		TextCache *	mTextCache;
		ColorA 		mFontColor;
		ColorA 		mFontShadowColor;
		Vector2f 		mAlignOffset;
		Recti			mPadding;
		Time			mTooltipTime;
		UIControl *	mTooltipOf;

		virtual void onSizeChange();

		virtual void autoSize();

		virtual void autoAlign();

		virtual void autoPadding();
};

}}

#endif
