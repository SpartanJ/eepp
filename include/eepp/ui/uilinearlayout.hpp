#ifndef UI_UILINEARLAYOUT_HPP
#define UI_UILINEARLAYOUT_HPP

#include <eepp/ui/uilayout.hpp>
#include <utility>

namespace EE { namespace UI {

class EE_API UILinearLayout : public UILayout {
  public:
	static UILinearLayout* NewWithTag( const std::string& tag, const UIOrientation& orientation );

	static UILinearLayout* New();

	static UILinearLayout* NewVertical();

	static UILinearLayout* NewHorizontal();

	static UILinearLayout* NewVerticalWidthMatchParent( const std::string& tag );

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	UIOrientation getOrientation() const;

	UILinearLayout* setOrientation( const UIOrientation& getOrientation );

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	void updateLayout();

	void setUpdateLayoutEvenIfNotVisible( bool update ) { mUpdateLayoutEvenIfNotVisible = update; }

	bool updatesLayoutEvenIfNotVisible() const { return mUpdateLayoutEvenIfNotVisible; }

  protected:
	UIOrientation mOrientation;
	bool mUpdateLayoutEvenIfNotVisible{ false };

	UILinearLayout();

	UILinearLayout( const std::string& tag, const UIOrientation& orientation );

	virtual Uint32 onMessage( const NodeMessage* Msg );

	void packVertical();

	void packHorizontal();

	/**
	 * @return A pair containing the space used by visible fixed-size children and margins, followed
	 * by the sum of the visible children's positive layout weights.
	 */
	std::pair<Sizei, Float> getTotalUsedSize();

	void applyWidthPolicyOnChildren();

	void applyHeightPolicyOnChildren();
};

}} // namespace EE::UI

#endif
