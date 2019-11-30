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
	LayoutMargin = String::hash( "layout-margin" ),
	LayoutMarginUnderscore = String::hash( "layout_margin" ),
	FillerPadding = String::hash( "filler-padding" ),
	RotationOriginPoint = String::hash( "rotation-origin-point" ),
	ScaleOriginPoint = String::hash( "scale-origin-point" )
};

class ShorthandDefinition {
  public:
	enum ShorthandType { Box, Background, Vector2, SingleValueVector };

	static std::vector<StyleSheetProperty> parseShorthand(const ShorthandDefinition* shorthand,
														   std::string value );

	static ShorthandDefinition* New( const std::string& name,
									 const std::vector<std::string>& properties,
									 const ShorthandType& shorthandType );

	ShorthandDefinition( const std::string& name, const std::vector<std::string>& properties,
						 const ShorthandType& shorthandType );

	const std::string& getName() const;

	const Uint32& getId() const;

	ShorthandId getShorthandId() const;

	const std::vector<std::string>& getProperties() const;

	const ShorthandType& getType() const;

  protected:
	std::string mName;
	Uint32 mId;
	std::vector<std::string> mProperties;
	ShorthandType mType;
};

}}} // namespace EE::UI::CSS

#endif
