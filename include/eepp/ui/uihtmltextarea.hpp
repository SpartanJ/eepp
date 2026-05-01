#ifndef EE_UI_UIHTMLTEXTAREA_HPP
#define EE_UI_UIHTMLTEXTAREA_HPP

#include <eepp/ui/uitextedit.hpp>

namespace EE { namespace UI {

class EE_API UIHTMLTextArea : public UITextEdit {
  public:
	static UIHTMLTextArea* New();

	UIHTMLTextArea();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	virtual Float getMinIntrinsicWidth() const;

	virtual Float getMaxIntrinsicWidth() const;

	virtual Float getMinIntrinsicHeight() const;

	virtual Float getMaxIntrinsicHeight() const;

	Uint32 getRows() const;

	void setRows( Uint32 rows );

	Uint32 getCols() const;

	void setCols( Uint32 cols );

  protected:
	Uint32 mRows{ 2 };
	Uint32 mCols{ 20 };
	bool mPacking{ false };

	virtual void onAutoSize();
};

}} // namespace EE::UI

#endif
