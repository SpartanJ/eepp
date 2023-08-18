#ifndef EE_UI_CSS_STYLESHEETVARIABLE_HPP
#define EE_UI_CSS_STYLESHEETVARIABLE_HPP

#include <eepp/config.hpp>
#include <eepp/core/containers.hpp>
#include <eepp/core/string.hpp>
#include <unordered_map>

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheetVariable {
  public:
	StyleSheetVariable();

	explicit StyleSheetVariable( const std::string& name, const std::string& value );

	explicit StyleSheetVariable( const std::string& name, const std::string& value,
								 const Uint32& specificity );

	const std::string& getName() const;

	const String::HashType& getNameHash() const;

	const std::string& getValue() const;

	const std::string& value() const;

	const Uint32& getSpecificity() const;

	void setSpecificity( const Uint32& specificity );

	bool isEmpty() const;

	void setName( const std::string& name );

	void setValue( const std::string& value );

	bool operator==( const StyleSheetVariable& variable );

  protected:
	std::string mName;
	String::HashType mNameHash;
	std::string mValue;
	Uint32 mSpecificity;
};

typedef UnorderedMap<Uint32, StyleSheetVariable> StyleSheetVariables;

}}} // namespace EE::UI::CSS

#endif // EE_UI_CSS_STYLESHEETVARIABLE_HPP
