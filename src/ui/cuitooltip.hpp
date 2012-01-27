#ifndef EE_UICUITOOLTIP_HPP
#define EE_UICUITOOLTIP_HPP

#include "cuicontrolanim.hpp"

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

				cFont * 	Font;
				eeColorA 	FontColor;
				eeColorA 	FontShadowColor;
		};

		cUITooltip( cUITooltip::CreateParams& Params, cUIControl * TooltipOf );

		virtual ~cUITooltip();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void SetTheme( cUITheme * Theme );

		void Show();

		void Hide();

		virtual void Draw();

		virtual void Alpha( const eeFloat& alpha );

		cFont * Font() const;

		void Font( cFont * font );

		virtual const String& Text();

		virtual void Text( const String& text );

		const eeColorA& Color() const;

		void Color( const eeColorA& color );

		const eeColorA& ShadowColor() const;

		void ShadowColor( const eeColorA& color );

		virtual void OnTextChanged();

		virtual void OnFontChanged();

		virtual void Padding( const eeRecti& padding );

		const eeRecti& Padding() const;

		cTextCache * GetTextCache();

		eeFloat GetTextWidth();

		eeFloat GetTextHeight();

		const eeInt& GetNumLines() const;

		const eeVector2f& AlignOffset() const;

		void TooltipTime( const eeFloat& Time );

		void TooltipTimeAdd( const eeFloat& Time );

		const eeFloat& TooltipTime() const;
	protected:
		cTextCache *	mTextCache;
		eeColorA 		mFontColor;
		eeColorA 		mFontShadowColor;
		eeVector2f 		mAlignOffset;
		eeRecti			mPadding;
		eeFloat			mTooltipTime;
		cUIControl *	mTooltipOf;

		virtual void OnSizeChange();

		virtual void AutoSize();

		virtual void AutoAlign();

		virtual void AutoPadding();
};

}}

#endif
