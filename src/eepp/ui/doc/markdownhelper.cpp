#include <eepp/ui/doc/markdownhelper.hpp>
#include <md4c/md4c-html.h>

namespace EE { namespace UI { namespace Doc {

static void process_output( const MD_CHAR* text, MD_SIZE size, void* userdata ) {
	std::string* out = static_cast<std::string*>( userdata );
	out->append( text, size );
}

std::string Markdown::toXHTML( std::string_view markdown, Dialect dialect, int flags ) {
	std::string out;
	int dialectFlag =
		( dialect == Dialect::CommonMark ? MD_DIALECT_COMMONMARK : MD_DIALECT_GITHUB ) | flags;
	md_html( markdown.data(), markdown.size(), process_output, &out, dialectFlag,
			 MD_HTML_FLAG_XHTML );
	return out;
}

}}} // namespace EE::UI::Doc
