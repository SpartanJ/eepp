#include <eepp/ui/tools/htmlformatter.hpp>

#include <string_view>

#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI { namespace Tools {

// In HTML, whitespace processing depends heavily on whether elements are block-level
// or inline-level. The HTML specification states that sequences of whitespace
// (spaces, tabs, newlines) inside inline formatting contexts are collapsed into a
// single space, but leading and trailing spaces are removed entirely if they adjoin
// a block boundary (e.g. at the start or end of a `<p>` or `<div>`).
//
// For example:
// <p>
//   <a href="...">
//     <img />
//   </a>
// </p>
// In this snippet, the spaces and newlines between `<p>` and `<a>` are completely
// dropped because they touch the block boundary of `<p>`. The spaces between `<a>` and
// `<img>` are inside an inline context, but because `<img/>` and `<a>` are inline, they
// might normally produce a space, except leading/trailing rules can apply depending on
// significant text content. To properly emulate HTML's visual rendering, we must
// identify whether a node acts as an "inline" element.

bool HTMLFormatter::isInlineNode( const pugi::xml_node& node ) {
	if ( !node )
		return false;
	if ( node.type() == pugi::node_pcdata )
		return true;
	if ( node.type() != pugi::node_element )
		return false;

	// Compare element tags against known HTML inline elements and our internal equivalents.
	std::string_view name( node.name() );
	return String::iequals( name, "a" ) || String::iequals( name, "span" ) ||
		   String::iequals( name, "textspan" ) || String::iequals( name, "b" ) ||
		   String::iequals( name, "i" ) || String::iequals( name, "strong" ) ||
		   String::iequals( name, "em" ) || String::iequals( name, "s" ) ||
		   String::iequals( name, "u" ) || String::iequals( name, "br" ) ||
		   String::iequals( name, "code" ) || String::iequals( name, "img" ) ||
		   String::iequals( name, "mark" );
}

// "Significant text" in the context of HTML whitespace collapsing means any text
// that is not entirely composed of whitespace characters, or elements that have a
// visual inline presence like images (<img/>) or line breaks (<br/>).
// Empty inline elements (e.g. `<span></span>`) or those containing only whitespace
// are often ignored when evaluating boundaries for whitespace trimming.
//
// This function allows us to peer inside nodes or text blocks to see if they actually
// contain anything that visually anchors a whitespace sequence. If a node lacks
// significant text, the whitespace logic can skip over it to find the true logical
// boundary.

bool HTMLFormatter::hasSignificantText( const pugi::xml_node& node ) {
	if ( !node )
		return false;

	// For plain text, check if there's any non-whitespace character.
	if ( node.type() == pugi::node_pcdata ) {
		std::string_view v( node.value() );
		for ( char c : v ) {
			if ( c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\v' )
				return true;
		}
		return false;
	}

	// For inline elements, certain tags are inherently significant (img, br).
	// Otherwise, we recursively check their children.
	if ( isInlineNode( node ) ) {
		std::string_view name( node.name() );
		if ( String::iequals( name, "img" ) || String::iequals( name, "br" ) )
			return true;
		for ( pugi::xml_node child = node.first_child(); child; child = child.next_sibling() ) {
			if ( hasSignificantText( child ) )
				return true;
		}
		return false;
	}

	// Block nodes inherently form a significant boundary. We don't look inside them
	// because a block node interrupts the inline formatting context entirely.
	return true;
}

// In HTML, elements can be nested arbitrarily, meaning the "previous" inline node
// visually preceding a text block might not be its direct sibling in the DOM tree.
// For instance, in `<span><b>text</b></span> <img/>`, the space is technically a sibling
// of `<span>`, but logically it follows `<b>text</b>`.
//
// `getLogicalPrev` traverses the DOM tree backward, diving into the rightmost children
// of previous siblings, or walking up to the parent, as long as the traversed nodes
// remain within the inline formatting context. This effectively finds the closest
// visual element to the left of the current node.

pugi::xml_node HTMLFormatter::getLogicalPrev( const pugi::xml_node& node ) {
	pugi::xml_node p = node;
	while ( p ) {
		// If there is a previous sibling, we move to it and then drill down
		// to its last (rightmost) inline child, simulating visual left-to-right flow.
		if ( p.previous_sibling() ) {
			p = p.previous_sibling();
			while ( p.last_child() && isInlineNode( p ) )
				p = p.last_child();
			return p;
		}
		// If there are no previous siblings, we move up to the parent.
		// If the parent is a block element, we've hit a block boundary and stop.
		p = p.parent();
		if ( !isInlineNode( p ) )
			break;
	}
	return pugi::xml_node();
}

// `getLogicalNext` is the counterpart to `getLogicalPrev`. It traverses the DOM tree
// forward to find the closest visual element to the right of the current node.
// It drills into the leftmost children of next siblings, or walks up the parent tree
// to continue the forward search, bounded by block elements.

pugi::xml_node HTMLFormatter::getLogicalNext( const pugi::xml_node& node ) {
	pugi::xml_node p = node;
	while ( p ) {
		// Move to the next sibling and drill down to its first (leftmost) inline child.
		if ( p.next_sibling() ) {
			p = p.next_sibling();
			while ( p.first_child() && isInlineNode( p ) )
				p = p.first_child();
			return p;
		}
		// Move up to the parent, stopping at block boundaries.
		p = p.parent();
		if ( !isInlineNode( p ) )
			break;
	}
	return pugi::xml_node();
}

// This function implements HTML-compliant whitespace collapsing for a given text node.
//
// HTML rules dictate that:
// 1. Any contiguous sequence of whitespace characters (spaces, tabs, newlines)
//    is collapsed into a single space character (' ').
// 2. If this text node logically adjoins a block element (e.g. it is the first or last
//    thing inside a `<div>`), the leading or trailing space is completely removed.
// 3. To accurately determine boundaries, we must skip over "empty" text nodes or
//    elements that lack significant visual content.
//
// This function first collapses all whitespace into single spaces. Then it looks both
// logically backward and forward using `getLogicalPrev` and `getLogicalNext`. If it
// determines that there is no valid inline node on a given side (meaning it has hit
// a block boundary), it strips the space on that side.

String HTMLFormatter::collapseXmlWhitespace( const String& text, const pugi::xml_node& node ) {
	String res;
	res.reserve( text.size() );
	bool inSpace = false;

	// Step 1: Collapse all contiguous whitespace characters into a single space.
	for ( size_t i = 0; i < text.size(); ++i ) {
		if ( text[i] == ' ' || text[i] == '\t' || text[i] == '\n' || text[i] == '\r' ||
			 text[i] == '\v' ) {
			if ( !inSpace ) {
				res += ' ';
				inSpace = true;
			}
		} else {
			res += text[i];
			inSpace = false;
		}
	}

	// Step 2: Determine if the left boundary is a block element.
	// We use getLogicalPrev, and if the previous node is just empty space
	// (lacks significant text), we keep looking further back.
	pugi::xml_node prev = getLogicalPrev( node );
	while ( prev && prev.type() == pugi::node_pcdata && !hasSignificantText( prev ) ) {
		prev = getLogicalPrev( prev );
	}
	bool prevInline = isInlineNode( prev );

	// Step 3: Determine if the right boundary is a block element.
	// We use getLogicalNext, skipping over any non-significant text nodes.
	pugi::xml_node next = getLogicalNext( node );
	while ( next && next.type() == pugi::node_pcdata && !hasSignificantText( next ) ) {
		next = getLogicalNext( next );
	}
	bool nextInline = isInlineNode( next );

	// Step 4: Trim leading and trailing spaces if they adjoin a block boundary.
	if ( !prevInline && !res.empty() && res[0] == ' ' )
		res = res.substr( 1 );

	if ( !nextInline && !res.empty() && res.back() == ' ' )
		res = res.substr( 0, res.size() - 1 );

	return res;
}

}}} // namespace EE::UI::Tools
