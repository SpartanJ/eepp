#include <eepp/core/containers.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>

#include <string_view>

#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

#include <gumbo-parser/gumbo.h>

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
				std::string attr_value = attr->value ? attr->value : "";

				// BOOLEAN ATTRIBUTE FIX:
				// If Gumbo parsed an attribute without a value (e.g., <input disabled>),
				// it often sets the value to empty. We enforce XML strictness.
				static const UnorderedSet<std::string> boolean_attrs = {
					"allowfullscreen", "async",	   "autofocus", "autoplay",	  "checked",
					"controls",		   "default",  "defer",		"disabled",	  "formnovalidate",
					"hidden",		   "inert",	   "ismap",		"itemscope",  "loop",
					"multiple",		   "muted",	   "nomodule",	"novalidate", "open",
					"readonly",		   "required", "reversed",	"selected" };

				bool value_was_omitted = attr->original_value.length == 0;

				if ( value_was_omitted ) {
					if ( boolean_attrs.count( attr_name ) )
						attr_value = attr_name;
					else
						attr_value = "";
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

static GumboNode* findElement( GumboNode* node, GumboTag tag ) {
	if ( !node )
		return nullptr;
	if ( node->type == GUMBO_NODE_ELEMENT && node->v.element.tag == tag )
		return node;

	GumboVector* children = nullptr;
	if ( node->type == GUMBO_NODE_DOCUMENT )
		children = &node->v.document.children;
	else if ( node->type == GUMBO_NODE_ELEMENT )
		children = &node->v.element.children;
	if ( !children )
		return nullptr;

	for ( unsigned int i = 0; i < children->length; ++i ) {
		if ( auto* element = findElement( static_cast<GumboNode*>( children->data[i] ), tag ) )
			return element;
	}
	return nullptr;
}

static std::string htmlToXML( const std::string& layoutString, bool bodyChildrenOnly ) {
	if ( layoutString.empty() )
		return "";

	// Strip UTF-8 BOM if present (Gumbo does not strip it, which corrupts the parse -
	// head children end up moved to body)
	size_t offset = 0;
	if ( layoutString.size() >= 3 && static_cast<unsigned char>( layoutString[0] ) == 0xEF &&
		 static_cast<unsigned char>( layoutString[1] ) == 0xBB &&
		 static_cast<unsigned char>( layoutString[2] ) == 0xBF ) {
		offset = 3;
	}

	// 1. Parse the dirty HTML into a Gumbo AST
	GumboOutput* output = gumbo_parse( layoutString.c_str() + offset );

	// 2. Serialize the AST into strict XML
	std::string strict_xml;
	if ( bodyChildrenOnly ) {
		if ( auto* body = findElement( output->root, GUMBO_TAG_BODY ) ) {
			GumboVector* children = &body->v.element.children;
			for ( unsigned int i = 0; i < children->length; ++i )
				serializeGumboNodeToXML( static_cast<GumboNode*>( children->data[i] ), strict_xml );
		}
	} else {
		serializeGumboNodeToXML( output->root, strict_xml );
	}

	// 3. Cleanup Gumbo's memory
	gumbo_destroy_output( &kGumboDefaultOptions, output );

	return strict_xml;
}

std::string HTMLFormatter::HTMLtoXML( const std::string& layoutString ) {
	return htmlToXML( layoutString, false );
}

std::string HTMLFormatter::HTMLBodyToXML( const std::string& layoutString ) {
	return htmlToXML( layoutString, true );
}

}}} // namespace EE::UI::Tools
