#ifndef EE_UI_UIHTMLTABLE_HPP
#define EE_UI_UIHTMLTABLE_HPP

#include <eepp/ui/uilayout.hpp>
#include <eepp/ui/uirichtext.hpp>

namespace EE { namespace UI {

class UIHTMLTableRow;
class UIHTMLTableCell;

class EE_API UIHTMLTable : public UILayout {
  public:
	static UIHTMLTable* New();

	UIHTMLTable();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void updateLayout();

  protected:
	virtual Uint32 onMessage( const NodeMessage* Msg );

	std::vector<UIHTMLTableRow*> mRows;
	std::vector<Float> mColWidths;
	std::vector<UIHTMLTableCell*> mCells;
	std::vector<Uint32> mRowCellOffsets;
};

class EE_API UIHTMLTableCell : public UIRichText {
  public:
	static UIHTMLTableCell* New( const std::string& tag );

	explicit UIHTMLTableCell( const std::string& tag );

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;
};

class EE_API UIHTMLTableRow : public UIWidget {
  public:
	static UIHTMLTableRow* New();

	UIHTMLTableRow();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;
};

class EE_API UIHTMLTableHead : public UIWidget {
  public:
	static UIHTMLTableHead* New();

	explicit UIHTMLTableHead();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;
};

class EE_API UIHTMLTableFooter : public UIWidget {
  public:
	static UIHTMLTableFooter* New();

	explicit UIHTMLTableFooter();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;
};

class EE_API UIHTMLTableBody : public UIWidget {
  public:
	static UIHTMLTableBody* New();

	explicit UIHTMLTableBody();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;
};

}} // namespace EE::UI

#endif
