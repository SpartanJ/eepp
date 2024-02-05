#include <eepp/ui/uilistview.hpp>

namespace EE { namespace UI {

UIListView* UIListView::New() {
	return eeNew( UIListView, () );
}

UIListView::UIListView() : UITableView( "listview" ) {
	setHeadersVisible( false );
	setAutoExpandOnSingleColumn( true );
	applyDefaultTheme();
}

Uint32 UIListView::getType() const {
	return UI_TYPE_LISTVIEW;
}

bool UIListView::isType( const Uint32& type ) const {
	return UIListView::getType() == type ? true : UITableView::isType( type );
}

void UIListView::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );
	setThemeSkin( Theme, "listbox" );
	onThemeLoaded();
}

}} // namespace EE::UI
