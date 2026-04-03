#ifndef EE_UI_HTMLTEXTINPUT_HPP
#define EE_UI_HTMLTEXTINPUT_HPP

#include <eepp/ui/uitextinput.hpp>

namespace EE { namespace UI {

class EE_API HTMLTextInput : public UITextInput {
  public:
	static HTMLTextInput* New();

	HTMLTextInput();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	virtual Float getMinIntrinsicWidth() const;

	virtual Float getMaxIntrinsicWidth() const;

	virtual void onAutoSize();

	Uint32 getHtmlSize() const;

	void setHtmlSize( Uint32 size );

  protected:
	HTMLTextInput( const std::string& tag );

	Uint32 mHtmlSize{ 20 };
	bool mPacking{ false };
};

}} // namespace EE::UI

#endif
