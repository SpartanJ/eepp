#ifndef EE_UI_UIRICHTEXT_HPP
#define EE_UI_UIRICHTEXT_HPP

#include <eepp/graphics/richtext.hpp>
#include <eepp/ui/uilayout.hpp>

namespace EE { namespace UI {

class EE_API UIRichText : public UILayout {
  public:
	static UIRichText* New();

	static UIRichText* NewWithTag( const std::string& tag );

	explicit UIRichText( const std::string& tag = "richtext" );

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void draw();

	virtual void loadFromXmlNode( const pugi::xml_node& node );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	const Graphics::RichText& getRichText();

	Graphics::Font* getFont() const;

	UIRichText* setFont( Graphics::Font* font );

	Uint32 getFontSize() const;

	UIRichText* setFontSize( const Uint32& characterSize );

	const Uint32& getFontStyle() const;

	UIRichText* setFontStyle( const Uint32& fontStyle );

	const Color& getFontColor() const;

	UIRichText* setFontColor( const Color& color );

	const Color& getFontShadowColor() const;

	UIRichText* setFontShadowColor( const Color& color );

	const Vector2f& getFontShadowOffset() const;

	UIRichText* setFontShadowOffset( const Vector2f& offset );

	const Float& getOutlineThickness() const;

	UIRichText* setOutlineThickness( const Float& outlineThickness );

	const Color& getOutlineColor() const;

	UIRichText* setOutlineColor( const Color& outlineColor );

	Uint32 getTextAlign() const;

	UIRichText* setTextAlign( const Uint32& align );

  protected:
	RichText mRichText;

	virtual void onSizeChange();
	virtual void onPaddingChange();
	virtual void onLayoutUpdate();
	virtual void onChildCountChange( Node* child, const bool& removed );
	virtual void onFontChanged();
	virtual void onFontStyleChanged();
	virtual void onAlphaChange();

	void rebuildRichText();
	void positionChildren();
	void updateDefaultSpansStyle();
};

}} // namespace EE::UI

#endif
