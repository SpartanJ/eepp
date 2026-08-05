#ifndef EE_UI_CSS_PROPERTYDEFINITION_HPP
#define EE_UI_CSS_PROPERTYDEFINITION_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/ui/css/propertyids.hpp>

namespace EE { namespace UI { namespace CSS {

enum class PropertyType : Uint32 {
	Undefined,
	String,
	Bool,
	NumberInt,
	NumberIntFixed,
	NumberFloat,
	NumberFloatFixed,
	NumberLength,
	NumberLengthFixed,
	RadiusLength,
	Color,
	Vector2,
	BackgroundSize,
	ForegroundSize,
	Time
};

enum class PropertyRelativeTarget : Uint32 {
	None,
	ContainingBlockWidth,
	ContainingBlockHeight,
	FontSize,
	ParentFontSize,
	LineHeight,
	LocalBlockWidth,
	LocalBlockHeight,
	BackgroundWidth,
	BackgroundHeight,
	ForegroundWidth,
	ForegroundHeight,
	LocalBlockRadiusWidth,
	LocalBlockRadiusHeight
};

class EE_API PropertyDefinition {
  public:
	PropertyDefinition( PropertyId propertyId, const std::string& name,
						const std::string& defaultValue, const bool& inherited = false );

	const std::string& getName() const;

	/** @brief Returns the canonical property-name hash, not the dense PropertyId. */
	const String::HashType& getId() const;

	/** @brief Returns the dense identity used for dispatch and PropertyIdSet membership. */
	PropertyId getPropertyId() const;

	const std::string& getDefaultValue() const;

	bool isInherited() const;

	const PropertyRelativeTarget& getRelativeTarget() const;

	PropertyDefinition& setRelativeTarget( const PropertyRelativeTarget& relativeTarget );

	PropertyDefinition& setType( const PropertyType& propertyType );

	const PropertyType& getType() const;

	PropertyDefinition& addAlias( const std::string& alias );

	bool isAlias( const std::string& alias ) const;

	bool isAlias( const Uint32& id ) const;

	bool isDefinition( const std::string& name ) const;

	bool isDefinition( const Uint32& id ) const;

	PropertyDefinition& setIndexed();

	const bool& isIndexed() const;

  protected:
	std::string mName;
	String::HashType mId;
	PropertyId mPropertyId;
	std::vector<std::string> mAliases;
	std::vector<Uint32> mAliasesHash;
	std::string mDefaultValue;
	bool mInherited;
	bool mIndexed;
	PropertyRelativeTarget mRelativeTarget;
	PropertyType mPropertyType;
};

}}} // namespace EE::UI::CSS

#endif
