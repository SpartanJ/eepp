#ifndef EE_UI_UITEXTSPAN_HPP
#define EE_UI_UITEXTSPAN_HPP

#include <eepp/ui/uifontstyleconfig.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

using SpanHitBoxes = SmallVector<Rectf, 4>;

class EE_API UITextSpan : public UIHTMLWidget {
  public:
	static UITextSpan* New();

	static UITextSpan* NewWithTag( const std::string& tag );

	static UITextSpan* NewStrong() { return NewWithTag( "strong" ); }

	static UITextSpan* NewBold() { return NewWithTag( "b" ); }

	static UITextSpan* NewItalics() { return NewWithTag( "i" ); }

	static UITextSpan* NewEmphasis() { return NewWithTag( "em" ); }

	static UITextSpan* NewUnderline() { return NewWithTag( "u" ); }

	static UITextSpan* NewStrikethrough() { return NewWithTag( "s" ); }

	static UITextSpan* NewFont() { return NewWithTag( "font" ); }

	static UITextSpan* NewMark() { return NewWithTag( "mark" ); }

	static UITextSpan* NewCode() { return NewWithTag( "code" ); }

	static UITextSpan* NewSmall() { return NewWithTag( "small" ); }

	virtual ~UITextSpan();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void draw();

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	const String& getText() const;

	UITextSpan* setText( const String& text );

	const UIFontStyleConfig& getFontStyleConfig() const;

	virtual void loadFromXmlNode( const pugi::xml_node& node );

	void setFontStyleConfig( const UIFontStyleConfig& fontStyleConfig );

	Graphics::Font* getFont() const;

	UITextSpan* setFont( Graphics::Font* font );

	Uint32 getFontSize() const;

	UITextSpan* setFontSize( const Uint32& characterSize );

	const Uint32& getFontStyle() const;

	UITextSpan* setFontStyle( const Uint32& fontStyle );

	Uint32 getTextDecoration() const;

	UITextSpan* setTextDecoration( const Uint32& textDecoration );

	const Float& getOutlineThickness() const;

	UITextSpan* setOutlineThickness( const Float& outlineThickness );

	const Color& getOutlineColor() const;

	UITextSpan* setOutlineColor( const Color& outlineColor );

	const Color& getFontColor() const;

	UITextSpan* setFontColor( const Color& color );

	const Color& getFontBackgroundColor() const;

	UITextSpan* setFontBackgroundColor( const Color& color );

	const Color& getFontShadowColor() const;

	UITextSpan* setFontShadowColor( const Color& color );

	const Vector2f& getFontShadowOffset() const;

	UITextSpan* setFontShadowOffset( const Vector2f& offset );

	void setInheritedStyle( const UIFontStyleConfig& fontStyleConfig );

	enum StyleState {
		StyleStateNone = 0,
		StyleStateFont = 1 << 0,
		StyleStateFontSize = 1 << 1,
		StyleStateFontStyle = 1 << 2,
		StyleStateFontColor = 1 << 3,
		StyleStateOutlineThickness = 1 << 4,
		StyleStateOutlineColor = 1 << 5,
		StyleStateFontShadowColor = 1 << 6,
		StyleStateFontShadowOffset = 1 << 7,
		StyleStateFontBackgroundColor = 1 << 8,
		StyleStateAll = 0xFFFFFFFF
	};

	bool hasFont() const;
	bool hasFontSize() const;
	bool hasFontStyle() const;
	bool hasFontColor() const;
	bool hasOutlineThickness() const;
	bool hasOutlineColor() const;
	bool hasFontShadowColor() const;
	bool hasFontShadowOffset() const;
	bool hasFontBackgroundColor() const;

	SpanHitBoxes& getHitBoxes();

	const SpanHitBoxes& getHitBoxes() const;

	void setHitBoxes( SpanHitBoxes&& hitBoxes );

	virtual Node* overFind( const Vector2f& point );

  protected:
	Uint32 mStyleState{ StyleStateNone };
	String mText;
	UIFontStyleConfig mFontStyleConfig;
	SpanHitBoxes mHitBoxes;

	explicit UITextSpan( const std::string& tag = "span" );

	virtual void drawBorder();

	virtual void onTextChanged();

	virtual void onFontChanged();

	virtual void onFontStyleChanged();

	virtual void onAlphaChange();

	virtual void onChildCountChange( Node* child, const bool& removed );

	virtual Uint32 onMessage( const NodeMessage* Msg );
};

class EE_API UIAnchorSpan : public UITextSpan {
  public:
	static UIAnchorSpan* New();

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	void setHref( const std::string& href );

	const std::string& getHref() const;

  protected:
	UIAnchorSpan( const std::string& tag = "a" );

	std::string mHref;

	virtual Uint32 onKeyDown( const KeyEvent& event );

	virtual Uint32 onMessage( const NodeMessage* Msg );
};

}} // namespace EE::UI

#endif
