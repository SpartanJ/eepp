#ifndef EE_UI_UIHTMLTABLE_HPP
#define EE_UI_UIHTMLTABLE_HPP

#include <eepp/core/small_vector.hpp>
#include <eepp/ui/uilayout.hpp>
#include <eepp/ui/uirichtext.hpp>

namespace EE { namespace UI {

class UIHTMLTableRow;
class UIHTMLTableCell;
class UIHTMLTableHead;
class UIHTMLTableBody;
class UIHTMLTableFooter;

class EE_API UIHTMLTable : public UILayout {
  public:
	static UIHTMLTable* New();

	UIHTMLTable();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void updateLayout();

	virtual Float getMinIntrinsicWidth() const;

	virtual Float getMaxIntrinsicWidth() const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

  protected:
	virtual Uint32 onMessage( const NodeMessage* Msg );

	void computeIntrinsicWidths() const;

	SmallVector<UIHTMLTableRow*> mRows;
	SmallVector<Float> mColWidths;
	SmallVector<UIHTMLTableCell*> mCells;
	SmallVector<Uint32> mRowCellOffsets;
	mutable SmallVector<Float> mColMinWidths;
	mutable SmallVector<Float> mColMaxWidths;
	mutable SmallVector<Float> mColSpecifiedWidths;
	mutable UIHTMLTableHead* mHead{ nullptr };
	mutable UIHTMLTableBody* mBody{ nullptr };
	mutable UIHTMLTableFooter* mFooter{ nullptr };
	Float mCellpadding{ 0 };
	Float mCellspacing{ 0 };
};

class EE_API UIHTMLTableCell : public UIRichText {
  public:
  	friend class UIHTMLTable;

	static UIHTMLTableCell* New( const std::string& tag );

	explicit UIHTMLTableCell( const std::string& tag );

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	Uint32 getColspan() const;

	virtual void onSizeChange();

  protected:
	Uint32 mColspan{ 1 };
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
