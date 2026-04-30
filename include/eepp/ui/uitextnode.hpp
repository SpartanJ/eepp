#ifndef EE_UI_UITEXTNODE_HPP
#define EE_UI_UITEXTNODE_HPP

#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UITextNode : public UIWidget {
  public:
	static UITextNode* New();

	UITextNode();

	virtual ~UITextNode();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void draw();

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	const String& getText() const;

	void setText( const String& text );

  protected:
	String mText;
};

}} // namespace EE::UI

#endif