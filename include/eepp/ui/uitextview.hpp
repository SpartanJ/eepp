#ifndef EE_UI_UITEXTVIEW_HPP
#define EE_UI_UITEXTVIEW_HPP

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

	Uint32 getFontSize() const;

	UITextView* setFontSize( const Uint32& characterSize );

	const Uint32& getFontStyle() const;

	UITextView* setFontStyle( const Uint32& fontStyle );

	const Float& getOutlineThickness() const;

	UITextView* setOutlineThickness( const Float& outlineThickness );

	const Color& getOutlineColor() const;

	UITextView* setOutlineColor( const Color& outlineColor );

	virtual const String& getText() const;

	virtual UITextView* setText( const String& text );

	const Color& getFontColor() const;

	UITextView* setFontColor( const Color& color );

	const Color& getFontShadowColor() const;

	UITextView* setFontShadowColor( const Color& color );

	const Vector2f& getFontShadowOffset() const;

	UITextView* setFontShadowOffset( const Vector2f& offset );

	const Color& getSelectionBackColor() const;

	UITextView* setSelectionBackColor( const Color& color );

	virtual void setTheme( UITheme* Theme );

	Float getTextWidth();

	Float getTextHeight();

	Uint32 getNumLines();

	Vector2f getAlignOffset() const;

	virtual void wrapText( const Uint32& maxWidth );

	bool isTextSelectionEnabled() const;

	void setTextSelection( const bool& active );

	const UIFontStyleConfig& getFontStyleConfig() const;

	void setFontStyleConfig( const UIFontStyleConfig& fontStyleConfig );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	void setTextAlign( const Uint32& align );

	UITextView* setFontFillColor( const Color& color, Uint32 from, Uint32 to );

	const Text* getTextCache() const;

	Text* getTextCache();

	const Vector2f& getRealAlignOffset() const;

	const TextTransform::Value& getTextTransform() const;

	void setTextTransform( const TextTransform::Value& textTransform );

	virtual void loadFromXmlNode( const pugi::xml_node& node );

	UITextView* setTextOverflow( const std::string_view& textOverflow );

	const std::string& getTextOverflow() const;

	bool hasTextOverflow() const;

	bool getUsingCustomStyling() const;

	void setUsingCustomStyling( bool usingCustomStyling );

	void setWordWrap( bool set );

	bool isWordWrap() const;

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
	bool mSelecting;
	bool mUsingCustomStyling{ false };
	std::string mTextOverflow;
	Float mTextOverflowWidth{ 0 };
	TextTransform::Value mTextTransform{ TextTransform::None };

	virtual void drawSelection( Text* textCache );

	virtual void onSizeChange();

	virtual void autoWrap();

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

	virtual Text* getVisibleTextCache() const;

	void transformText();

	void recalculate();

	void resetSelCache();

	void updateTextOverflow();
};

class EE_API UIAnchor : public UITextView {
  public:
	static UIAnchor* New();

	UIAnchor();

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	void setHref( const std::string& href );

	const std::string& getHref() const;

  protected:
	std::string mHref;

	virtual Uint32 onKeyDown( const KeyEvent& event );
};

}} // namespace EE::UI

#endif
