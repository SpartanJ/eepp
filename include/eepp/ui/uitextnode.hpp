#ifndef EE_UI_UITEXTNODE_HPP
#define EE_UI_UITEXTNODE_HPP

#include <eepp/graphics/text.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

class EE_API UITextNode : public UIWidget {
  public:
	static UITextNode* New();

	virtual ~UITextNode();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void draw();

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	const String& getText() const;

	void setText( const String& text );

	bool isWhitespaceOnly() const;

	size_t getLayoutCharCount() const { return mLayoutCharCount; }

	void setLayoutCharCount( size_t count ) { mLayoutCharCount = count; }

	Text* getFlexText();

	Float getBaseline() const;

  protected:
	String mText;
	size_t mLayoutCharCount{ 0 };
	Text* mFlexText{ nullptr };

	UITextNode();
};

}} // namespace EE::UI

#endif
