#ifndef EE_UI_CSS_ELEMENTDEFINITION_HPP
#define EE_UI_CSS_ELEMENTDEFINITION_HPP

#include <eepp/core/noncopyable.hpp>
#include <eepp/ui/css/propertyidset.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetstyle.hpp>
#include <eepp/ui/css/transitiondefinition.hpp>

namespace EE { namespace UI { namespace CSS {

class EE_API ElementDefinition : NonCopyable {
  public:
	ElementDefinition( const StyleSheetStyleVector& styleSheetStyles );

	StyleSheetProperty* getProperty( const PropertyId& id );

	StyleSheetProperty* getPropertyByNameHash( const String::HashType& id );

	const PropertyIdSet& getPropertyIds() const;

	const StyleSheetProperties& getProperties() const;

	const std::vector<const CSS::StyleSheetProperty*>& getTransitionProperties() const;

	const ComputedTransitions& getTransitions() const;

	const std::vector<const CSS::StyleSheetProperty*>& getAnimationProperties() const;

	const StyleSheetVariables& getVariables() const;

	bool isStructurallyVolatile() const;

	const StyleSheetStyleVector& getStyles() const;

	void refresh();

  protected:
	StyleSheetStyleVector mStyles;
	StyleSheetProperties mProperties;
	StyleSheetVariables mVariables;
	PropertyIdSet mPropertyIds;
	std::vector<const CSS::StyleSheetProperty*> mTransitionProperties;
	ComputedTransitions mTransitions;
	std::vector<const CSS::StyleSheetProperty*> mAnimationProperties;
	bool mStructurallyVolatile;

	void findVariables( const CSS::StyleSheetStyle* style );

	void resolveVariables();
};

}}} // namespace EE::UI::CSS

#endif // EE_UI_CSS_ELEMENTDEFINITION_HPP
