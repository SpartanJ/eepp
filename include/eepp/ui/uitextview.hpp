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

		const Color& getOutlineColor() const;

		UITextView * setOutlineColor( const Color& outlineColor );

		virtual const String& getText();

		virtual UITextView * setText( const String& text );

		const Color& getFontColor() const;

		UITextView * setFontColor( const Color& color );

		const Color& getFontShadowColor() const;

		UITextView * setFontShadowColor( const Color& color );

		const Color& getSelectionBackColor() const;

		UITextView * setSelectionBackColor( const Color& color );

		virtual void setTheme( UITheme * Theme );

		Float getTextWidth();

		Float getTextHeight();

		const int& getNumLines() const;

		const Vector2f & getAlignOffset() const;

		virtual void shrinkText( const Uint32& MaxWidth );

		bool isTextSelectionEnabled() const;

		virtual void setFontStyleConfig( const UITooltipStyleConfig& fontStyleConfig );

		UITooltipStyleConfig getFontStyleConfig() const;

		const Rectf& getPadding() const;

		UITextView * setPadding(const Rectf& padding);

		virtual void loadFromXmlNode( const pugi::xml_node& node );
	protected:
		Text *		mTextCache;
		String			mString;
		UITooltipStyleConfig mFontStyleConfig;
		Vector2f 		mAlignOffset;
		Vector2f 		mRealAlignOffset;
		Int32			mSelCurInit;
		Int32			mSelCurEnd;
		Rectf		mPadding;
		Rectf		mRealPadding;
		struct SelPosCache
		{
			SelPosCache( Vector2f ip, Vector2f ep ) :
				initPos( ip ),
				endPos( ep )
			{}

			Vector2f initPos;
			Vector2f endPos;
		};
		std::vector<SelPosCache> mSelPosCache;
		Int32		mLastSelCurInit;
		Int32		mLastSelCurEnd;
		bool		mSelecting;

		virtual void drawSelection(Text * textCache);

		virtual void onSizeChange();

		virtual void autoShrink();

		virtual void onAutoSize();

		virtual void alignFix();

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

		void recalculate();

		void resetSelCache();

};

}}

#endif
