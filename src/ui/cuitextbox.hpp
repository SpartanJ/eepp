#ifndef EE_UICUITEXTBOX_H
#define EE_UICUITEXTBOX_H

#include "cuicontrolanim.hpp"

namespace EE { namespace UI {

class EE_API cUITextBox : public cUIControlAnim {
	public:
		class CreateParams : public cUIControl::CreateParams {
			public:
				inline CreateParams() :
					cUIControl::CreateParams(),
					Font( NULL ),
					FontColor( 0, 0, 0, 255 ),
					FontShadowColor( 255, 255, 255, 150 )
				{
				}

				inline ~CreateParams() {}

				cFont * 	Font;
				eeColorA 	FontColor;
				eeColorA 	FontShadowColor;
		};

		cUITextBox( const cUITextBox::CreateParams& Params );

		~cUITextBox();

		virtual void Draw();

		virtual void Alpha( const eeFloat& alpha );

		cFont * Font() const;

		void Font( cFont * font );

		virtual const std::wstring& Text();

		virtual void Text( const std::wstring& text );

		virtual void Text( const std::string& text );

		const eeColorA& Color() const;

		void Color( const eeColorA& color );

		const eeColorA& ShadowColor() const;

		void ShadowColor( const eeColorA& color );

		virtual void OnTextChanged();

		virtual void OnFontChanged();

		virtual void Padding( const eeRecti& padding );

		const eeRecti& Padding() const;

		virtual void SetTheme( cUITheme * Theme );

		cTextCache& GetTextCache();
	protected:
		cTextCache 		mTextCache;
		eeColorA 		mFontColor;
		eeColorA 		mFontShadowColor;
		eeVector2f 		mAlignOffset;
		eeRecti			mPadding;

		virtual void OnSizeChange();

		void AutoShrink();

		void AutoSize();

		void AutoAlign();
};

}}

#endif
