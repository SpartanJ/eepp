#include <eepp/core/memorymanager.hpp>
#include <eepp/ui/blocklayouter.hpp>
#include <eepp/ui/inlinelayouter.hpp>
#include <eepp/ui/nonelayouter.hpp>
#include <eepp/ui/tablelayouter.hpp>
#include <eepp/ui/uilayouter.hpp>
#include <eepp/ui/uilayoutermanager.hpp>

namespace EE { namespace UI {

UILayouter* UILayouterManager::create( CSSDisplay display, UIWidget* container ) {
	switch ( display ) {
		case CSSDisplay::Block:
		case CSSDisplay::TableCell:
			return eeNew( BlockLayouter, ( container ) );
		case CSSDisplay::Inline:
		case CSSDisplay::InlineBlock:
			return eeNew( InlineLayouter, ( container ) );
		case CSSDisplay::Table:
			return eeNew( TableLayouter, ( container ) );
		case CSSDisplay::None:
			return eeNew( NoneLayouter, ( container ) );
		default:
			return nullptr;
	}
}

}} // namespace EE::UI
