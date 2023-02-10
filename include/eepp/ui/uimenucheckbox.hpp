#ifndef EE_UICUIMENUCHECKBOX_HPP
#define EE_UICUIMENUCHECKBOX_HPP

#include <eepp/ui/uimenuitem.hpp>

namespace EE { namespace UI {

class EE_API UIMenuCheckBox : public UIMenuItem {
  public:
	static UIMenuCheckBox* New();

	UIMenuCheckBox();

	virtual ~UIMenuCheckBox();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void setTheme( UITheme* Theme );

	const bool& isActive() const;

	UIMenuCheckBox* setActive( const bool& active );

	void switchActive();

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

  protected:
	bool mActive;
	UISkin* mSkinActive;
	UISkin* mSkinInactive;

	Uint32 onMessage( const NodeMessage* msg );

	virtual void onStateChange();
};

}} // namespace EE::UI

#endif
