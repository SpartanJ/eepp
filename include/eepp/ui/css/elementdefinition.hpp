#ifndef EE_UI_CSS_ELEMENTDEFINITION_HPP
#define EE_UI_CSS_ELEMENTDEFINITION_HPP

#include <eepp/core/noncopyable.hpp>
#include <eepp/ui/css/propertyidset.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetstyle.hpp>

namespace EE { namespace UI { namespace CSS {

class EE_API ElementDefinition : NonCopyable {
  public:
	ElementDefinition( const StyleSheetStyleVector& styleSheetStyles );

	/// Returns a specific property from the element definition.
	/// @param[in] id The id of the property to return.
	/// @return The property defined against the give name, or nullptr if no such property was
	/// found.
	StyleSheetProperty* getProperty( const Uint32& id );

	/// Returns the list of property ids this element definition defines.
	/// @param[out] property_names The list to store the defined property ids in.
	const PropertyIdSet& getPropertyIds() const;

	const StyleSheetProperties& getProperties() const;

  protected:
	StyleSheetProperties mProperties;
	PropertyIdSet mPropertyIds;
};

}}} // namespace EE::UI::CSS

#endif // EE_UI_CSS_ELEMENTDEFINITION_HPP
