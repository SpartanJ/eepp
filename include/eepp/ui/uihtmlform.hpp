#ifndef EE_UI_UIHTMLFORM_HPP
#define EE_UI_UIHTMLFORM_HPP

#include <string>
#include <utility>
#include <vector>

#include <eepp/ui/uirichtext.hpp>

namespace EE { namespace UI {

class UISceneNode;

class EE_API UIHTMLForm : public UIRichText {
  public:
	static UIHTMLForm* New();

	UIHTMLForm( const std::string& tag = "form" );

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	void submit();

	const std::string& getAction() const { return mAction; }
	void setAction( const std::string& action ) { mAction = action; }

	const std::string& getMethod() const { return mMethod; }
	void setMethod( const std::string& method ) { mMethod = method; }

	const std::string& getEnctype() const { return mEnctype; }
	void setEnctype( const std::string& enctype ) { mEnctype = enctype; }

  protected:
	std::string mAction;
	std::string mMethod{ "GET" };
	std::string mEnctype{ "application/x-www-form-urlencoded" };

	virtual Uint32 onMessage( const NodeMessage* msg );

	static void collectFormData( Node* node,
								  std::vector<std::pair<std::string, std::string>>& fields );
	bool isSubmitTrigger( Node* sender ) const;
};

}} // namespace EE::UI

#endif
