#ifndef EE_UICUITEXTBOX_H
#define EE_UICUITEXTBOX_H

#include <eepp/ui/uiwidget.hpp>
#include <eepp/graphics/text.hpp>

namespace EE { namespace UI {

class EE_API UITextView : public UIWidget {
	public:
		static UITextView * New();

		UITextView();

		virtual ~UITextView();

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void draw();

		virtual void setAlpha( const Float& alpha );

		Graphics::Font * getFont() const;

		UITextView * setFont( Graphics::Font * font );

		Uint32 getCharacterSize();

		UITextView * setCharacterSize( const Uint32& characterSize );

		const Uint32& getFontStyle() const;

		UITextView * setFontStyle( const Uint32& fontStyle );

		const Float & getOutlineThickness() const;

		UITextView * setOutlineThickness( const Float& outlineThickness );

		const ColorA& getOutlineColor() const;

		UITextView * setOutlineColor( const ColorA& outlineColor );

		virtual const String& getText();

		virtual UITextView * setText( const String& text );

		const ColorA& getFontColor() const;

		UITextView * setFontColor( const ColorA& color );

		const ColorA& getFontShadowColor() const;

		UITextView * setFontShadowColor( const ColorA& color );

		const ColorA& getSelectionBackColor() const;

		UITextView * setSelectionBackColor( const ColorA& color );

		virtual void setTheme( UITheme * Theme );

		Float getTextWidth();

		Float getTextHeight();

		const int& getNumLines() const;

		const Vector2i & getAlignOffset() const;

		virtual void shrinkText( const Uint32& MaxWidth );

		bool isTextSelectionEnabled() const;

		virtual void setFontStyleConfig( const UITooltipStyleConfig& fontStyleConfig );

		UITooltipStyleConfig getFontStyleConfig() const;

		const Recti& getPadding() const;

		UITextView * setPadding(const Recti & padding);

		virtual void loadFromXmlNode( const pugi::xml_node& node );
	protected:
		Text *		mTextCache;
		String			mString;
		UITooltipStyleConfig mFontStyleConfig;
		Vector2i 		mAlignOffset;
		Vector2f 		mRealAlignOffset;
		Int32			mSelCurInit;
		Int32			mSelCurEnd;
		Recti		mPadding;
		Recti		mRealPadding;

		virtual void drawSelection(Text * textCache);

		virtual void onSizeChange();

		virtual void autoShrink();

		virtual void onAutoSize();

		virtual void autoAlign();

		virtual void onTextChanged();

		virtual void onFontChanged();

		virtual void onPaddingChange();

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
