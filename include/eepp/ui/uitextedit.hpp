#ifndef EE_UICUITEXTEDIT_HPP
#define EE_UICUITEXTEDIT_HPP

#include <eepp/graphics/text.hpp>
#include <eepp/ui/uicodeeditor.hpp>

namespace EE { namespace UI {

class EE_API UITextEdit : public UICodeEditor {
  public:
	static UITextEdit* New();

	static UITextEdit* NewWithTag( const std::string& tag );

	virtual ~UITextEdit();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	String getText() const;

	void setText( const String& text );

	void setWordWrap( bool enabled );

	const String& getHint() const;

	UITextEdit* setHint( const String& hint );

	const Color& getHintColor() const;

	UITextEdit* setHintColor( const Color& hintColor );

	const Color& getHintShadowColor() const;

	UITextEdit* setHintShadowColor( const Color& shadowColor );

	const Vector2f& getHintShadowOffset() const;

	UITextEdit* setHintShadowOffset( const Vector2f& shadowOffset );

	Font* getHintFont() const;

	UITextEdit* setHintFont( Font* font );

	Uint32 getHintFontSize() const;

	UITextEdit* setHintFontSize( const Uint32& characterSize );

	const Uint32& getHintFontStyle() const;

	UITextEdit* setHintFontStyle( const Uint32& fontStyle );

	const Float& getHintOutlineThickness() const;

	UITextEdit* setHintOutlineThickness( const Float& outlineThickness );

	const Color& getHintOutlineColor() const;

	UITextEdit* setHintOutlineColor( const Color& outlineColor );

	void setHintDisplay( HintDisplay display );

	HintDisplay getHintDisplay() const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

  protected:
	UITextEdit( const std::string& tag );

	Text mHintCache;
	FontStyleConfig mHintStyleConfig;
	HintDisplay mHintDisplay{ HintDisplay::Always };

	virtual void drawLineText( const Int64& line, Vector2f position, const Float& fontSize,
							   const Float& lineHeight,
							   const DocumentViewLineRange& visibleLineRange );

	virtual void drawCursor( const Vector2f& startScroll, const Float& lineHeight,
							 const TextPosition& cursor );

	virtual void onTextHintsChanged();
};

}} // namespace EE::UI

#endif
