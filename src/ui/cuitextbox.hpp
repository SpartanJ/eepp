#ifndef EE_UICUITEXTBOX_H
#define EE_UICUITEXTBOX_H

#include "cuicomplexcontrol.hpp"

namespace EE { namespace UI {

class EE_API cUITextBox : public cUIComplexControl {
	public:
		class CreateParams : public cUIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					cUIComplexControl::CreateParams(),
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

		cUITextBox( const cUITextBox::CreateParams& Params );

		~cUITextBox();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

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

		virtual void SetTheme( cUITheme * Theme );

		cTextCache * GetTextCache();

		eeFloat GetTextWidth();

		eeFloat GetTextHeight();

		const eeInt& GetNumLines() const;

		const eeVector2f& AlignOffset() const;

		virtual void ShrinkText( const Uint32& MaxWidth );
	protected:
		cTextCache *	mTextCache;
		String			mString;
		eeColorA 		mFontColor;
		eeColorA 		mFontShadowColor;
		eeVector2f 		mAlignOffset;
		eeRecti			mPadding;

		virtual void OnSizeChange();

		virtual void AutoShrink();

		virtual void AutoSize();

		virtual void AutoAlign();
};

}}

#endif
