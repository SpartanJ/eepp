#ifndef EE_UICUITEXTBOX_H
#define EE_UICUITEXTBOX_H

#include <eepp/ui/cuicomplexcontrol.hpp>

namespace EE { namespace UI {

class EE_API cUITextBox : public cUIComplexControl {
	public:
		class CreateParams : public cUIComplexControl::CreateParams {
			public:
				inline CreateParams() :
					cUIComplexControl::CreateParams(),
					Font( NULL ),
					FontColor( 0, 0, 0, 255 ),
					FontShadowColor( 255, 255, 255, 150 ),
					FontSelectionBackColor( 150, 150, 150, 150 )
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
				ColorA				FontSelectionBackColor;
		};

		cUITextBox( const cUITextBox::CreateParams& Params );

		virtual ~cUITextBox();

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

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

		const ColorA& SelectionBackColor() const;

		void SelectionBackColor( const ColorA& color );

		virtual void OnTextChanged();

		virtual void OnFontChanged();

		virtual void Padding( const Recti& padding );

		const Recti& Padding() const;

		virtual void SetTheme( cUITheme * Theme );

		TextCache * GetTextCache();

		Float GetTextWidth();

		Float GetTextHeight();

		const int& GetNumLines() const;

		const Vector2f& AlignOffset() const;

		virtual void ShrinkText( const Uint32& MaxWidth );

		bool IsTextSelectionEnabled() const;
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

		virtual void DrawSelection();

		virtual void OnSizeChange();

		virtual void AutoShrink();

		virtual void AutoSize();

		virtual void AutoAlign();

		virtual Uint32 OnFocusLoss();

		virtual Uint32 OnMouseDoubleClick( const Vector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnMouseClick( const Vector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnMouseDown( const Vector2i& Pos, const Uint32 Flags );

		virtual void SelCurInit( const Int32& init );

		virtual void SelCurEnd( const Int32& end );

		virtual Int32 SelCurInit();

		virtual Int32 SelCurEnd();

};

}}

#endif
