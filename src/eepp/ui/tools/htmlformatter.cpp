#include <eepp/system/regex.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>

#include <string_view>

#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

#include <gumbo-parser/gumbo.h>

using namespace EE::System;

namespace EE { namespace UI { namespace Tools {

// Helper to escape text so pugixml doesn't crash on <, >, or &
// and scrubs unwanted Unicode characters (like visible Non-Breaking Spaces)
static std::string escapeXML( std::string_view input ) {
	std::string out;
	out.reserve( input.size() * 1.1 );

	for ( size_t i = 0; i < input.size(); ++i ) {
		unsigned char c = input[i];

		// --- INTERCEPT UTF-8 NON-BREAKING SPACE ---
		// A non-breaking space is encoded in UTF-8 as two bytes: 0xC2 0xA0
		if ( c == 0xC2 && i + 1 < input.size() && (unsigned char)input[i + 1] == 0xA0 ) {
			out += ' '; // Inject a standard ASCII space (Dec 32)
			i++;		// Skip the second byte (0xA0)
			continue;
		}

		switch ( c ) {
			case '<':
				out += "&lt;";
				break;
			case '>':
				out += "&gt;";
				break;
			case '&':
				out += "&amp;";
				break;
			case '"':
				out += "&quot;";
				break;
			case '\'':
				out += "&apos;";
				break;
			default:
				out += c;
				break;
		}
	}
	return out;
}

// Recursive function to walk the Gumbo AST and build strict XML
static void serializeGumboNodeToXML( GumboNode* node, std::string& out ) {
	if ( !node )
		return;

	switch ( node->type ) {
		case GUMBO_NODE_DOCUMENT: {
			// Root document, process all children
			GumboVector* children = &node->v.document.children;
			for ( unsigned int i = 0; i < children->length; ++i ) {
				serializeGumboNodeToXML( static_cast<GumboNode*>( children->data[i] ), out );
			}
			break;
		}
		case GUMBO_NODE_ELEMENT: {
			// Handle the HTML tag
			std::string tag;
			if ( node->v.element.tag != GUMBO_TAG_UNKNOWN ) {
				tag = gumbo_normalized_tagname( node->v.element.tag );
			} else {
				// For custom tags (like <eepp-widget>), Gumbo stores them as unknown.
				// We extract the original tag string safely.
				GumboStringPiece* original_tag = &node->v.element.original_tag;
				gumbo_tag_from_original_text( original_tag ); // standardizes it
				if ( original_tag->data && original_tag->length > 0 ) {
					// Strip the `<` and any trailing spaces/brackets to get just the name
					std::string raw( original_tag->data, original_tag->length );
					size_t start = raw.find_first_not_of( "< " );
					size_t end = raw.find_first_of( " >\r\n\t", start );
					if ( start != std::string::npos ) {
						tag = raw.substr( start, end - start );
					}
				}
				if ( tag.empty() )
					tag = "unknown";
			}

			out += "<" + tag;

			// --- Process Attributes ---
			GumboVector* attrs = &node->v.element.attributes;
			for ( unsigned int i = 0; i < attrs->length; ++i ) {
				GumboAttribute* attr = static_cast<GumboAttribute*>( attrs->data[i] );
				std::string attr_name = attr->name;
				std::string attr_value = attr->value;

				// BOOLEAN ATTRIBUTE FIX:
				// If Gumbo parsed an attribute without a value (e.g., <input disabled>),
				// it often sets the value to empty. We enforce XML strictness.
				if ( attr_value.empty() ) {
					attr_value = attr_name;
				}

				out += " " + attr_name + "=\"" + escapeXML( attr_value ) + "\"";
			}

			// --- Handle Void Tags vs Standard Tags ---
			// We enforce XML closing rules so pugixml doesn't fail
			static const UnorderedSet<std::string> void_tags = {
				"area",	 "base", "br",	 "col",	  "embed",	"hr",	 "img",
				"input", "link", "meta", "param", "source", "track", "wbr" };

			if ( void_tags.count( tag ) ) {
				out += " />"; // Self-close
			} else {
				out += ">";

				// Recursively process children
				GumboVector* children = &node->v.element.children;
				for ( unsigned int i = 0; i < children->length; ++i ) {
					serializeGumboNodeToXML( static_cast<GumboNode*>( children->data[i] ), out );
				}

				out += "</" + tag + ">";
			}
			break;
		}
		case GUMBO_NODE_TEXT:
		case GUMBO_NODE_WHITESPACE:
		case GUMBO_NODE_CDATA: {
			// Safely escape and write all raw text (including the insides of scripts/styles)
			if ( node->v.text.text ) {
				out += escapeXML( node->v.text.text );
			}
			break;
		}
		case GUMBO_NODE_COMMENT:
		case GUMBO_NODE_TEMPLATE:
			// We silently ignore comments to prevent XML double-hyphen crashes
			break;
	}
}

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
		   String::iequals( name, "mark" ) || String::iequals( name, "font" );
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

	// Step 2: Determine if the left boundary is a block element or a forced line break (<br/>).
	// We use getLogicalPrev, and if the previous node is just empty space
	// (lacks significant text), we keep looking further back.
	pugi::xml_node prev = getLogicalPrev( node );
	while ( prev && prev.type() == pugi::node_pcdata && !hasSignificantText( prev ) ) {
		prev = getLogicalPrev( prev );
	}
	// A node is a valid inline neighbor only if it is an inline node AND not a <br/>
	// (because <br/> strips adjacent whitespace).
	bool prevInline = isInlineNode( prev ) && !String::iequals( prev.name(), "br" );

	// Step 3: Determine if the right boundary is a block element or a forced line break.
	// We use getLogicalNext, skipping over any non-significant text nodes.
	pugi::xml_node next = getLogicalNext( node );
	while ( next && next.type() == pugi::node_pcdata && !hasSignificantText( next ) ) {
		next = getLogicalNext( next );
	}
	bool nextInline = isInlineNode( next ) && !String::iequals( next.name(), "br" );

	// Step 4: Trim leading and trailing spaces if they adjoin a block boundary.
	if ( !prevInline && !res.empty() && res[0] == ' ' )
		res = res.substr( 1 );

	if ( !nextInline && !res.empty() && res.back() == ' ' )
		res = res.substr( 0, res.size() - 1 );

	return res;
}

std::string HTMLFormatter::HTMLtoXML( const std::string& layoutString ) {
	if ( layoutString.empty() )
		return "";

	// 1. Parse the dirty HTML into a Gumbo AST
	GumboOutput* output = gumbo_parse( layoutString.c_str() );

	// 2. Serialize the AST into strict XML
	std::string strict_xml;
	serializeGumboNodeToXML( output->root, strict_xml );

	// 3. Cleanup Gumbo's memory
	gumbo_destroy_output( &kGumboDefaultOptions, output );

	return strict_xml;
}

}}} // namespace EE::UI::Tools
