#ifndef EE_UICUITOOLTIP_HPP
#define EE_UICUITOOLTIP_HPP

#include <eepp/graphics/texttransform.hpp>
#include <eepp/ui/uifontstyleconfig.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace Graphics {
class Text;
class Font;
}} // namespace EE::Graphics

namespace EE { namespace UI {

class EE_API UITooltip : public UIWidget {
  public:
	static UITooltip* New();

	static Vector2f getTooltipPosition( UITooltip* toolip, const Vector2f& requestedPosition );

	UITooltip();

	virtual ~UITooltip();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	void show();

	void hide();

	virtual void draw();

	Graphics::Font* getFont() const;

	void setFont( Graphics::Font* font );

	virtual const String& getText();

	virtual void setText( const String& text );

	const Color& getFontColor() const;

	void setFontColor( const Color& color );

	const Color& getFontShadowColor() const;

	void setFontShadowColor( const Color& color );

	virtual void onTextChanged();

	virtual void onFontChanged();

	const Text* getTextCache() const;

	Text* getTextCache();

	Float getTextWidth();

	Float getTextHeight();

	Uint32 getNumLines();

	Vector2f getAlignOffset();

	UINode* getTooltipOf() const;

	void setTooltipOf( UINode* tooltipOf );

	const UIFontStyleConfig& getFontStyleConfig() const;

	void setFontStyleConfig( const UIFontStyleConfig& styleConfig );

	Uint32 getCharacterSize() const;

	UITooltip* setFontSize( const Uint32& characterSize );

	UITooltip* setFontStyle( const Uint32& fontStyle );

	const Uint32& getFontStyle() const;

	const Float& getOutlineThickness() const;

	UITooltip* setOutlineThickness( const Float& outlineThickness );

	const Color& getOutlineColor() const;

	UITooltip* setOutlineColor( const Color& outlineColor );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	const String& getStringBuffer() const;

	void setStringBuffer( const String& stringBuffer );

	void resetTextToStringBuffer();

	bool dontAutoHideOnMouseMove() const;

	void setDontAutoHideOnMouseMove( bool dontAutoHideOnMouseMove );

	const TextTransform::Value& getTextTransform() const;

	void setTextTransform( const TextTransform::Value& textTransform );

	Vector2f getTooltipPosition( const Vector2f& requestedPosition );

	bool getUsingCustomStyling() const;

	void setUsingCustomStyling( bool usingCustomStyling );

	void setFontShadowOffset( const Vector2f& offset );

	const Vector2f& getFontShadowOffset() const;

	void notifyTextChangedFromTextCache();

	virtual void wrapText( const Uint32& maxWidth );

	void setWordWrap( bool set );

	bool isWordWrap() const;

  protected:
	Text* mTextCache;
	UIFontStyleConfig mStyleConfig;
	Vector2f mAlignOffset;
	Time mTooltipTime;
	UINode* mTooltipOf;
	String mStringBuffer;
	TextTransform::Value mTextTransform{ TextTransform::None };
	bool mDontAutoHideOnMouseMove{ false };
	bool mUsingCustomStyling{ false };

	virtual void onAlphaChange();

	virtual void onSizeChange();

	virtual void onAutoSize();

	virtual void autoAlign();

	virtual void autoPadding();

	virtual void autoWrap();

	void transformText();
};

}} // namespace EE::UI

#endif
