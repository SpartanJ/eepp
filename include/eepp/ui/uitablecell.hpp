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

	static UITableCell*
	NewWithOpt( const std::string& tag,
				const std::function<UITextView*( UIPushButton* )>& newTextViewCb ) {
		return eeNew( UITableCell, ( tag, newTextViewCb ) );
	}

	Uint32 getType() const { return UI_TYPE_TABLECELL; }

	bool isType( const Uint32& type ) const {
		return UITableCell::getType() == type ? true : UIPushButton::isType( type );
	}

	ModelIndex getCurIndex() const { return mCurIndex; }

	void setCurIndex( const ModelIndex& curIndex ) {
		if ( curIndex != mCurIndex ) {
			mCurIndex = curIndex;
			onModelIndexChange();
		}
	}

	void setTheme( UITheme* Theme ) {
		UIPushButton::setTheme( Theme );
		setThemeSkin( Theme, "tablerow" );
		onThemeLoaded();
	}

	virtual void updateCell( Model* ){};

  protected:
	ModelIndex mCurIndex;

	UITableCell() : UITableCell( "table::cell" ) {}

	UITableCell( const std::string& tag,
				 const std::function<UITextView*( UIPushButton* )>& newTextViewCb = nullptr ) :
		UIPushButton( tag, newTextViewCb ) {
		applyDefaultTheme();
	}

	virtual void onModelIndexChange() {}
};

}} // namespace EE::UI

#endif // EE_UI_UITABLECELL_HPP
