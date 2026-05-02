#include <eepp/core/memorymanager.hpp>
#include <eepp/ui/blocklayouter.hpp>
#include <eepp/ui/inlinelayouter.hpp>
#include <eepp/ui/nonelayouter.hpp>
#include <eepp/ui/tablelayouter.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uilayouter.hpp>
#include <eepp/ui/uilayoutermanager.hpp>

namespace EE { namespace UI {

UILayouter* UILayouterManager::create( CSSDisplay display, UIWidget* container ) {
	switch ( display ) {
		case CSSDisplay::Block:
		case CSSDisplay::TableCell:
		case CSSDisplay::InlineBlock:
		case CSSDisplay::ListItem:
		case CSSDisplay::Flex:
			return eeNew( BlockLayouter, ( container ) );
		case CSSDisplay::Inline:
			if ( container->isType( UI_TYPE_TEXTSPAN ) )
				return eeNew( InlineLayouter, ( container ) );
			return eeNew( BlockLayouter, ( container ) );
		case CSSDisplay::Table:
			return eeNew( TableLayouter, ( container ) );
		case CSSDisplay::None:
			return eeNew( NoneLayouter, ( container ) );
		default:
			return nullptr;
	}
}

}} // namespace EE::UI
