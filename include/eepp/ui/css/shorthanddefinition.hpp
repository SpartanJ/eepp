#ifndef EE_UI_CSS_SHORTHANDDEFINITION_HPP
#define EE_UI_CSS_SHORTHANDDEFINITION_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>

namespace EE { namespace UI { namespace CSS {

enum class ShorthandId : Uint32 {
	Margin = String::hash( "margin" ),
	Padding = String::hash( "padding" ),
	Transition = String::hash( "transition" ),
	Background = String::hash( "background" ),
	Foreground = String::hash( "foreground" ),
	BackgroundPosition = String::hash( "background-position" ),
	ForegroundPosition = String::hash( "foreground-position" ),
	LayoutMargin = String::hash( "layout-margin" ),
	LayoutMarginUnderscore = String::hash( "layout_margin" ),
	FillerPadding = String::hash( "filler-padding" ),
	RotationOriginPoint = String::hash( "rotation-origin-point" ),
	ScaleOriginPoint = String::hash( "scale-origin-point" ),
	BorderColor = String::hash( "border-color" ),
	BorderWidth = String::hash( "border-width" ),
	BorderRadius = String::hash( "border-radius" )
};

enum class ShorthandType : Uint32 {
	Box,
	Background,
	Vector2,
	SingleValueVector,
	BackgroundPosition,
	BorderBox,
	Radius
};

class ShorthandDefinition {
  public:
	static std::vector<StyleSheetProperty> parseShorthand( const ShorthandDefinition* shorthand,
														   std::string value );

	static ShorthandDefinition* New( const std::string& name,
									 const std::vector<std::string>& properties,
									 const ShorthandType& shorthandType );

	ShorthandDefinition( const std::string& name, const std::vector<std::string>& properties,
						 const ShorthandType& shorthandType );

	const std::string& getName() const;

	const Uint32& getId() const;

	ShorthandDefinition& addAlias( const std::string& alias );

	bool isAlias( const std::string& alias ) const;

	bool isAlias( const Uint32& id ) const;

	bool isDefinition( const std::string& name ) const;

	bool isDefinition( const Uint32& id ) const;

	ShorthandId getShorthandId() const;

	const std::vector<std::string>& getProperties() const;

	const ShorthandType& getType() const;

  protected:
	std::string mName;
	Uint32 mId;
	std::vector<std::string> mAliases;
	std::vector<Uint32> mAliasesHash;
	std::vector<std::string> mProperties;
	ShorthandType mType;
};

}}} // namespace EE::UI::CSS

#endif
