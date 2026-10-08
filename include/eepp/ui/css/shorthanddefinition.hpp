#ifndef EE_UI_CSS_SHORTHANDDEFINITION_HPP
#define EE_UI_CSS_SHORTHANDDEFINITION_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/ui/css/propertyids.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <functional>

namespace EE { namespace UI { namespace CSS {

typedef std::function<std::vector<StyleSheetProperty>( const ShorthandDefinition* shorthand,
													   std::string value )>
	ShorthandParserFunc;

class EE_API ShorthandDefinition {
  public:
	static ShorthandDefinition* New( ShorthandId shorthandId, const std::string& name,
									 const std::vector<std::string>& properties,
									 const std::string& shorthandParserName );

	ShorthandDefinition( ShorthandId shorthandId, const std::string& name,
						 const std::vector<std::string>& properties,
						 const std::string& shorthandFuncName );

	std::vector<StyleSheetProperty> parse( std::string value ) const;

	const std::string& getName() const;

	/** @brief Returns the canonical shorthand-name hash, not the dense ShorthandId. */
	const String::HashType& getId() const;

	ShorthandDefinition& addAlias( const std::string& alias );

	bool isAlias( const std::string& alias ) const;

	bool isAlias( const String::HashType& id ) const;

	bool isDefinition( const std::string& name ) const;

	bool isDefinition( const String::HashType& id ) const;

	/** @brief Returns the dense shorthand identity used for lookup and dispatch. */
	ShorthandId getShorthandId() const;

	const std::vector<std::string>& getProperties() const;

  protected:
	std::string mName;
	std::string mFuncName;
	String::HashType mId;
	ShorthandId mShorthandId;
	std::vector<std::string> mAliases;
	std::vector<String::HashType> mAliasesHash;
	std::vector<std::string> mProperties;
};

}}} // namespace EE::UI::CSS

#endif
