#ifndef EE_UI_CSS_SHORTHANDDEFINITION_HPP
#define EE_UI_CSS_SHORTHANDDEFINITION_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <functional>

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
	RotationOriginPoint = String::hash( "rotation-origin-point" ),
	ScaleOriginPoint = String::hash( "scale-origin-point" ),
	BorderColor = String::hash( "border-color" ),
	BorderWidth = String::hash( "border-width" ),
	BorderRadius = String::hash( "border-radius" ),
	MinSize = String::hash( "min-size" ),
	MaxSize = String::hash( "max-size" )
};

typedef std::function<std::vector<StyleSheetProperty>( const ShorthandDefinition* shorthand,
													   std::string value )>
	ShorthandParserFunc;

class EE_API ShorthandDefinition {
  public:
	static ShorthandDefinition* New( const std::string& name,
									 const std::vector<std::string>& properties,
									 const std::string& shorthandParserName );

	ShorthandDefinition( const std::string& name, const std::vector<std::string>& properties,
						 const std::string& shorthandFuncName );

	std::vector<StyleSheetProperty> parse( std::string value ) const;

	const std::string& getName() const;

	const String::HashType& getId() const;

	ShorthandDefinition& addAlias( const std::string& alias );

	bool isAlias( const std::string& alias ) const;

	bool isAlias( const String::HashType& id ) const;

	bool isDefinition( const std::string& name ) const;

	bool isDefinition( const String::HashType& id ) const;

	ShorthandId getShorthandId() const;

	const std::vector<std::string>& getProperties() const;

  protected:
	std::string mName;
	std::string mFuncName;
	String::HashType mId;
	std::vector<std::string> mAliases;
	std::vector<String::HashType> mAliasesHash;
	std::vector<std::string> mProperties;
};

}}} // namespace EE::UI::CSS

#endif
