#ifndef EE_UICUITOOLTIP_HPP
#define EE_UICUITOOLTIP_HPP

#include <eepp/ui/cuicontrolanim.hpp>

namespace EE { namespace Graphics {
class TextCache;
}}

namespace EE { namespace UI {

class EE_API cUITooltip : public cUIControlAnim {
	public:
		class CreateParams : public cUIControlAnim::CreateParams {
			public:
				inline CreateParams() :
					cUIControlAnim::CreateParams(),
					Font( NULL ),
					FontColor( 0, 0, 0, 255 ),
					FontShadowColor( 255, 255, 255, 150 )
				{
					cUITheme * Theme = cUIThemeManager::instance()->DefaultTheme();

					if ( NULL != Theme ) {
						Font			= Theme->Font();
						FontColor		= Theme->FontColor();
						FontShadowColor	= Theme->FontShadowColor();
					}

					if ( NULL == Font )
						Font = cUIThemeManager::instance()->DefaultFont();
				}

				inline ~CreateParams() {}

				Graphics::Font * 	Font;
				ColorA				FontColor;
				ColorA				FontShadowColor;
				Recti				Padding;
		};

		cUITooltip( cUITooltip::CreateParams& Params, cUIControl * TooltipOf );

		virtual ~cUITooltip();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( cUITheme * Theme );

		void Show();

		void Hide();

		virtual void Draw();

		virtual void Alpha( const Float& alpha );

		Graphics::Font * Font() const;

		void Font( Graphics::Font * font );

		virtual const String& Text();

		virtual void Text( const String& text );

		const ColorA& Color() const;

		void Color( const ColorA& color );

		const ColorA& ShadowColor() const;

		void ShadowColor( const ColorA& color );

		virtual void OnTextChanged();

		virtual void OnFontChanged();

		virtual void Padding( const Recti& padding );

		const Recti& Padding() const;

		TextCache * GetTextCache();

		Float GetTextWidth();

		Float GetTextHeight();

		const int& GetNumLines() const;

		const Vector2f& AlignOffset() const;

		void TooltipTime( const Time& Time );

		void TooltipTimeAdd( const Time & Time );

		const Time & TooltipTime() const;
	protected:
		TextCache *	mTextCache;
		ColorA 		mFontColor;
		ColorA 		mFontShadowColor;
		Vector2f 		mAlignOffset;
		Recti			mPadding;
		Time			mTooltipTime;
		cUIControl *	mTooltipOf;

		virtual void OnSizeChange();

		virtual void AutoSize();

		virtual void AutoAlign();

		virtual void AutoPadding();
};

}}

#endif
