#ifndef EE_UICUITEXTBOX_H
#define EE_UICUITEXTBOX_H

#include <eepp/ui/uicomplexcontrol.hpp>

namespace EE { namespace UI {

class EE_API UITextBox : public UIComplexControl {
	public:
		class CreateParams : public UIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					UIComplexControl::CreateParams(),
					Font( NULL ),
					FontColor( 0, 0, 0, 255 ),
					FontShadowColor( 255, 255, 255, 150 ),
					FontSelectionBackColor( 150, 150, 150, 150 )
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
				ColorA				FontSelectionBackColor;
		};

		UITextBox( const UITextBox::CreateParams& Params );

		virtual ~UITextBox();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

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

		const ColorA& selectionBackColor() const;

		void selectionBackColor( const ColorA& color );

		virtual void onTextChanged();

		virtual void onFontChanged();

		virtual void padding( const Recti& padding );

		const Recti& padding() const;

		virtual void setTheme( UITheme * Theme );

		TextCache * getTextCache();

		Float getTextWidth();

		Float getTextHeight();

		const int& getNumLines() const;

		const Vector2f& alignOffset() const;

		virtual void shrinkText( const Uint32& MaxWidth );

		bool isTextSelectionEnabled() const;
	protected:
		TextCache *	mTextCache;
		String			mString;
		ColorA 		mFontColor;
		ColorA 		mFontShadowColor;
		ColorA		mFontSelectionBackColor;
		Vector2f 		mAlignOffset;
		Recti			mPadding;
		Int32			mSelCurInit;
		Int32			mSelCurEnd;

		virtual void drawSelection();

		virtual void onSizeChange();

		virtual void autoShrink();

		virtual void autoSize();

		virtual void autoAlign();

		virtual Uint32 onFocusLoss();

		virtual Uint32 onMouseDoubleClick( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onMouseClick( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onMouseDown( const Vector2i& position, const Uint32 flags );

		virtual void selCurInit( const Int32& init );

		virtual void selCurEnd( const Int32& end );

		virtual Int32 selCurInit();

		virtual Int32 selCurEnd();

};

}}

#endif
