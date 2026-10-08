#include <eepp/core/memorymanager.hpp>
#include <eepp/ui/blocklayouter.hpp>
#include <eepp/ui/flexlayouter.hpp>
#include <eepp/ui/gridlayouter.hpp>
#include <eepp/ui/inlinelayouter.hpp>
#include <eepp/ui/nonelayouter.hpp>
#include <eepp/ui/tablelayouter.hpp>
#include <eepp/ui/uilayouter.hpp>
#include <eepp/ui/uilayoutermanager.hpp>
#include <eepp/ui/uitextspan.hpp>

namespace EE { namespace UI {

static bool parentIsFlexContainer( UIWidget* widget ) {
	Node* parent = widget->getParent();
	return parent && parent->isType( UI_TYPE_HTML_WIDGET ) &&
		   parent->asType<UIHTMLWidget>()->isFlex();
}

static bool parentIsGridContainer( UIWidget* widget ) {
	Node* parent = widget->getParent();
	return parent && parent->isType( UI_TYPE_HTML_WIDGET ) &&
		   parent->asType<UIHTMLWidget>()->isGrid();
}

UILayouter* UILayouterManager::create( CSSDisplay display, UIWidget* container ) {
	// Blockification per CSS Flexbox §4 and CSS Grid §6: children of flex / grid containers
	// are block-level
	if ( parentIsFlexContainer( container ) || parentIsGridContainer( container ) ) {
		// But a child that is itself a flex container still needs FlexLayouter,
		// and a child that is itself a grid container still needs GridLayouter.
		if ( display == CSSDisplay::Flex || display == CSSDisplay::InlineFlex )
			return eeNew( FlexLayouter, ( container ) );
		if ( display == CSSDisplay::Grid || display == CSSDisplay::InlineGrid )
			return eeNew( GridLayouter, ( container ) );
		return eeNew( BlockLayouter, ( container ) );
	}

	// CSS 2.1 section 9.7 blockifies floated boxes. Keep flex/grid containers using their
	// respective formatting contexts, but an ordinary floated inline (for example a <span>)
	// needs an independent block formatting context instead of InlineLayouter.
	if ( container->isType( UI_TYPE_HTML_WIDGET ) &&
		 container->asType<UIHTMLWidget>()->getCSSFloat() != CSSFloat::None ) {
		if ( display == CSSDisplay::Flex || display == CSSDisplay::InlineFlex )
			return eeNew( FlexLayouter, ( container ) );
		if ( display == CSSDisplay::Grid || display == CSSDisplay::InlineGrid )
			return eeNew( GridLayouter, ( container ) );
		return eeNew( BlockLayouter, ( container ) );
	}

	switch ( display ) {
		case CSSDisplay::Block:
		case CSSDisplay::TableCell:
		case CSSDisplay::InlineBlock:
		case CSSDisplay::ListItem:
			return eeNew( BlockLayouter, ( container ) );
		case CSSDisplay::Flex:
		case CSSDisplay::InlineFlex:
			return eeNew( FlexLayouter, ( container ) );
		case CSSDisplay::Grid:
		case CSSDisplay::InlineGrid:
			return eeNew( GridLayouter, ( container ) );
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
