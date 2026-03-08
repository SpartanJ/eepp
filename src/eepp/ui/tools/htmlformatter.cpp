#include <eepp/ui/tools/htmlformatter.hpp>

#include <string_view>

#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI { namespace Tools {

bool HTMLFormatter::isInlineNode( const pugi::xml_node& node ) {
	if ( !node )
		return false;
	if ( node.type() == pugi::node_pcdata )
		return true;
	if ( node.type() != pugi::node_element )
		return false;
	std::string_view name( node.name() );
	return String::iequals( name, "a" ) || String::iequals( name, "span" ) ||
		   String::iequals( name, "textspan" ) || String::iequals( name, "b" ) ||
		   String::iequals( name, "i" ) || String::iequals( name, "strong" ) ||
		   String::iequals( name, "em" ) || String::iequals( name, "s" ) ||
		   String::iequals( name, "u" ) || String::iequals( name, "br" ) ||
		   String::iequals( name, "code" ) || String::iequals( name, "img" ) ||
		   String::iequals( name, "mark" );
}

bool HTMLFormatter::hasSignificantText( const pugi::xml_node& node ) {
	if ( !node )
		return false;
	if ( node.type() == pugi::node_pcdata ) {
		std::string_view v( node.value() );
		for ( char c : v ) {
			if ( c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\v' )
				return true;
		}
		return false;
	}
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
	return true; // Block nodes are significant boundaries
}

pugi::xml_node HTMLFormatter::getLogicalPrev( const pugi::xml_node& node ) {
	pugi::xml_node p = node;
	while ( p ) {
		if ( p.previous_sibling() ) {
			p = p.previous_sibling();
			while ( p.last_child() && isInlineNode( p ) )
				p = p.last_child();
			return p;
		}
		p = p.parent();
		if ( !isInlineNode( p ) )
			break;
	}
	return pugi::xml_node();
}

pugi::xml_node HTMLFormatter::getLogicalNext( const pugi::xml_node& node ) {
	pugi::xml_node p = node;
	while ( p ) {
		if ( p.next_sibling() ) {
			p = p.next_sibling();
			while ( p.first_child() && isInlineNode( p ) )
				p = p.first_child();
			return p;
		}
		p = p.parent();
		if ( !isInlineNode( p ) )
			break;
	}
	return pugi::xml_node();
}

String HTMLFormatter::collapseXmlWhitespace( const String& text, const pugi::xml_node& node ) {
	String res;
	res.reserve( text.size() );
	bool inSpace = false;
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

	pugi::xml_node prev = getLogicalPrev( node );
	while ( prev && prev.type() == pugi::node_pcdata && !hasSignificantText( prev ) ) {
		prev = getLogicalPrev( prev );
	}
	bool prevInline = isInlineNode( prev );

	pugi::xml_node next = getLogicalNext( node );
	while ( next && next.type() == pugi::node_pcdata && !hasSignificantText( next ) ) {
		next = getLogicalNext( next );
	}
	bool nextInline = isInlineNode( next );

	if ( !prevInline && !res.empty() && res[0] == ' ' )
		res = res.substr( 1 );

	if ( !nextInline && !res.empty() && res.back() == ' ' )
		res = res.substr( 0, res.size() - 1 );

	return res;
}

}}} // namespace EE::UI::Tools
