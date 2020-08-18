#ifndef EE_UICUITEXTBOX_H
#define EE_UICUITEXTBOX_H

#include <eepp/graphics/text.hpp>
#include <eepp/ui/uifontstyleconfig.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UITextView : public UIWidget {
  public:
	static UITextView* New();

	static UITextView* NewWithTag( const std::string& tag );

	UITextView();

	explicit UITextView( const std::string& tag );

	virtual ~UITextView();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void draw();

	Graphics::Font* getFont() const;

	UITextView* setFont( Graphics::Font* font );

	Uint32 getCharacterSize() const;

	UITextView* setFontSize( const Uint32& characterSize );

	const Uint32& getFontStyle() const;

	UITextView* setFontStyle( const Uint32& fontStyle );

	const Float& getOutlineThickness() const;

	UITextView* setOutlineThickness( const Float& outlineThickness );

	const Color& getOutlineColor() const;

	UITextView* setOutlineColor( const Color& outlineColor );

	virtual const String& getText();

	virtual UITextView* setText( const String& text );

	const Color& getFontColor() const;

	UITextView* setFontColor( const Color& color );

	const Color& getFontShadowColor() const;

	UITextView* setFontShadowColor( const Color& color );

	const Color& getSelectionBackColor() const;

	UITextView* setSelectionBackColor( const Color& color );

	virtual void setTheme( UITheme* Theme );

	Float getTextWidth();

	Float getTextHeight();

	const int& getNumLines() const;

	Vector2f getAlignOffset() const;

	virtual void shrinkText( const Uint32& MaxWidth );

	bool isTextSelectionEnabled() const;

	void setTextSelection( const bool& active );

	const UIFontStyleConfig& getFontStyleConfig() const;

	void setFontStyleConfig( const UIFontStyleConfig& fontStyleConfig );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 );

	void setTextAlign( const Uint32& align );

  protected:
	Text* mTextCache;
	String mString;
	UIFontStyleConfig mFontStyleConfig;
	Vector2f mRealAlignOffset;
	Int32 mSelCurInit;
	Int32 mSelCurEnd;
	struct SelPosCache {
		SelPosCache( Vector2f ip, Vector2f ep ) : initPos( ip ), endPos( ep ) {}

		Vector2f initPos;
		Vector2f endPos;
	};
	std::vector<SelPosCache> mSelPosCache;
	Int32 mLastSelCurInit;
	Int32 mLastSelCurEnd;
	Int32 mFontLineCenter;
	bool mSelecting;

	virtual void drawSelection( Text* textCache );

	virtual void onSizeChange();

	virtual void autoShrink();

	virtual void onAutoSize();

	virtual void alignFix();

	virtual void onTextChanged();

	virtual void onFontChanged();

	virtual void onFontStyleChanged();

	virtual void onAlphaChange();

	virtual Uint32 onFocusLoss();

	virtual Uint32 onMouseDoubleClick( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseClick( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseDown( const Vector2i& position, const Uint32& flags );

	virtual void selCurInit( const Int32& init );

	virtual void selCurEnd( const Int32& end );

	virtual Int32 selCurInit();

	virtual Int32 selCurEnd();

	virtual void onAlignChange();

	virtual void onSelectionChange();

	const Int32& getFontLineCenter();

	void recalculate();

	void resetSelCache();
};

}} // namespace EE::UI

#endif
