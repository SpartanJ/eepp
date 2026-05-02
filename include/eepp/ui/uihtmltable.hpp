#ifndef EE_UI_UIHTMLTABLE_HPP
#define EE_UI_UIHTMLTABLE_HPP

#include <eepp/core/small_vector.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uirichtext.hpp>

namespace EE { namespace UI {

class EE_API UIHTMLTable : public UIHTMLWidget {
  public:
	friend class TableLayouter;
	static UIHTMLTable* New();

	UIHTMLTable();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual Float getMinIntrinsicWidth() const;

	virtual Float getMaxIntrinsicWidth() const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& state = 0 ) const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

  protected:
	virtual Uint32 onMessage( const NodeMessage* Msg );

	void computeIntrinsicWidths() const;
};

class EE_API UIHTMLTableCell : public UIRichText {
  public:
	friend class UIHTMLTable;
	friend class TableLayouter;

	static UIHTMLTableCell* New( const std::string& tag );

	explicit UIHTMLTableCell( const std::string& tag );

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual std::vector<PropertyId> getPropertiesImplemented() const;

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& state = 0 ) const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	Uint32 getColSpan() const;

	virtual void onSizeChange();

  protected:
	Uint32 mColSpan{ 1 };
};

class EE_API UIHTMLTableRow : public UIHTMLWidget {
  public:
	static UIHTMLTableRow* New();

	UIHTMLTableRow();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;
};

class EE_API UIHTMLTableHead : public UIHTMLWidget {
  public:
	static UIHTMLTableHead* New();

	explicit UIHTMLTableHead();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;
};

class EE_API UIHTMLTableFooter : public UIHTMLWidget {
  public:
	static UIHTMLTableFooter* New();

	explicit UIHTMLTableFooter();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;
};

class EE_API UIHTMLTableBody : public UIHTMLWidget {
  public:
	static UIHTMLTableBody* New();

	explicit UIHTMLTableBody();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;
};

}} // namespace EE::UI

#endif
