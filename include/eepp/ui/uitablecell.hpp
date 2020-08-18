#ifndef EE_UI_UITABLECELL_HPP
#define EE_UI_UITABLECELL_HPP

#include <eepp/ui/models/modelindex.hpp>
#include <eepp/ui/uipushbutton.hpp>

using namespace EE::UI::Models;

namespace EE { namespace UI {

class EE_API UITableCell : public UIPushButton {
  public:
	static UITableCell* New() { return eeNew( UITableCell, () ); }

	static UITableCell* New( const std::string& tag ) { return eeNew( UITableCell, ( tag ) ); }

	Uint32 getType() const { return UI_TYPE_TABLECELL; }

	bool isType( const Uint32& type ) const {
		return UITableCell::getType() == type ? true : UIPushButton::isType( type );
	}

	ModelIndex getCurIndex() const { return mCurIndex; }

	void setCurIndex( const ModelIndex& curIndex ) { mCurIndex = curIndex; }

	void setTheme( UITheme* Theme ) {
		UIPushButton::setTheme( Theme );
		setThemeSkin( Theme, "tablerow" );
		onThemeLoaded();
	}

  protected:
	ModelIndex mCurIndex;

	UITableCell() : UITableCell( "table::cell" ) {}

	UITableCell( const std::string& tag ) : UIPushButton( tag ) { applyDefaultTheme(); }
};

}} // namespace EE::UI

#endif // EE_UI_UITABLECELL_HPP
