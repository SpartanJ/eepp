#ifndef EE_UICUITEXTBOXSIMPLE_H
#define EE_UICUITEXTBOXSIMPLE_H

#include "cuicontrol.hpp"
#include "cuitextbox.hpp"

namespace EE { namespace UI {

class EE_API cUITextBoxSimple : public cUIControl {
	public:
		cUITextBoxSimple( const cUITextBox::CreateParams& Params );

		~cUITextBoxSimple();

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

		eeFloat GetTextWidth();

		eeFloat GetTextHeight();

		const eeInt& GetNumLines() const;
	protected:
		std::wstring	mText;
		cFont *			mFont;
		eeFloat 		mCachedWidth;
		eeInt			mNumLines;

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
