#ifndef EE_UI_UISTACKLAYOUT_HPP
#define EE_UI_UISTACKLAYOUT_HPP

#include <eepp/ui/uilayout.hpp>

namespace EE { namespace UI {

class EE_API UIStackLayout : public UILayout {
  public:
	enum class RowValign { Top, Center, Bottom };

	static UIStackLayout* New();

	static UIStackLayout* NewWithTag( const std::string& tag = "stacklayout" );

	virtual ~UIStackLayout();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	void updateLayout();

	const RowValign& getRowValign() const;

	void setRowValign( const RowValign& rowValign );

  protected:
	RowValign mRowValign{ RowValign::Bottom };
	Node* mParentRef{ nullptr };
	Uint32 mParentSizeChangeCb{ 0 };
	Uint32 mParentCloseCb{ 0 };

	UIStackLayout();


	explicit UIStackLayout( const std::string& tag );

	virtual Uint32 onMessage( const NodeMessage* Msg );

	void applySizePolicyOnChilds();

	void setRowValign( const std::string& rowValign );

	static std::string rowValignToStr( const RowValign& rowValign );

	virtual void onParentChange();

	void listenParent();

	void clearListeners();
};

}} // namespace EE::UI

#endif // EE_UI_UISTACKLAYOUT_HPP
