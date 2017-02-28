#ifndef EE_UICUITEXTBOX_H
#define EE_UICUITEXTBOX_H

#include <eepp/ui/uicomplexcontrol.hpp>

namespace EE { namespace UI {

class EE_API UITextBox : public UIComplexControl {
	public:
		static UITextBox * New();

		class CreateParams : public UIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					UIComplexControl::CreateParams()
				{
					fontStyleConfig = UIThemeManager::instance()->getDefaultFontStyleConfig();
				}

				inline ~CreateParams() {}

				FontStyleConfig fontStyleConfig;
		};

		UITextBox( const UITextBox::CreateParams& Params );

		UITextBox();

		virtual ~UITextBox();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void draw();

		virtual void setAlpha( const Float& alpha );

		Graphics::Font * getFont() const;

		void setFont( Graphics::Font * font );

		virtual const String& getText();

		virtual void setText( const String& text );

		const ColorA& getFontColor() const;

		void setFontColor( const ColorA& color );

		const ColorA& getFontShadowColor() const;

		void setFontShadowColor( const ColorA& color );

		const ColorA& getSelectionBackColor() const;

		void setSelectionBackColor( const ColorA& color );

		virtual void setPadding( const Recti& padding );

		const Recti& getPadding() const;

		virtual void setTheme( UITheme * Theme );

		TextCache * getTextCache();

		Float getTextWidth();

		Float getTextHeight();

		const int& getNumLines() const;

		const Vector2i & getAlignOffset() const;

		virtual void shrinkText( const Uint32& MaxWidth );

		bool isTextSelectionEnabled() const;

		virtual void setFontStyleConfig( const FontStyleConfig& fontStyleConfig );

		FontStyleConfig getFontStyleConfig() const;
	protected:
		TextCache *		mTextCache;
		String			mString;
		FontStyleConfig mFontStyleConfig;
		Vector2i 		mAlignOffset;
		Vector2f 		mRealAlignOffset;
		Recti			mPadding;
		Recti			mRealPadding;
		Int32			mSelCurInit;
		Int32			mSelCurEnd;

		virtual void drawSelection(TextCache * textCache);

		virtual void onSizeChange();

		virtual void autoShrink();

		virtual void autoSize();

		virtual void autoAlign();

		virtual void onTextChanged();

		virtual void onFontChanged();

		virtual Uint32 onFocusLoss();

		virtual Uint32 onMouseDoubleClick( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onMouseClick( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onMouseDown( const Vector2i& position, const Uint32 flags );

		virtual void selCurInit( const Int32& init );

		virtual void selCurEnd( const Int32& end );

		virtual Int32 selCurInit();

		virtual Int32 selCurEnd();

		virtual void onAlignChange();

};

}}

#endif
