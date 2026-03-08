#ifndef EE_UI_DOC_MARKDOWNHELPER_HPP
#define EE_UI_DOC_MARKDOWNHELPER_HPP

#include <eepp/config.hpp>
#include <string>
#include <string_view>

namespace EE { namespace UI { namespace Doc {

class EE_API Markdown {
  public:
	enum class Dialect { CommonMark, GitHub };

	enum Flags {
		CollapseWhitespace =
			0x0001, /* In MD_TEXT_NORMAL, collapse non-trivial whitespace into single ' ' */
		PermissiveAtxHeaders = 0x0002,	 /* Do not require space in ATX headers ( ###header ) */
		PermissiveUrlAutolinks = 0x0004, /* Recognize URLs as autolinks even without '<', '>' */
		PermissiveEmailAutolinks =
			0x0008, /* Recognize e-mails as autolinks even without '<', '>' and 'mailto:' */
		NoIndentedCodeBlocks = 0x0010, /* Disable indented code blocks. (Only fenced code works.) */
		NoHtmlBlocks = 0x0020,		   /* Disable raw HTML blocks. */
		NoHtmlSpans = 0x0040,		   /* Disable raw HTML (inline). */
		Tables = 0x0100,			   /* Enable tables extension. */
		Strikethrough = 0x0200,		   /* Enable strikethrough extension. */
		PermissiveWwwAutolinks = 0x0400, /* Enable WWW autolinks (even without any scheme prefix, if
											they begin with 'www.') */
		TaskLists = 0x0800,				 /* Enable task list extension. */
		LatexMathSpans = 0x1000,		 /* Enable $ and $$ containing LaTeX equations. */
		WikiLinks = 0x2000,				 /* Enable wiki links extension. */
		Underline = 0x4000, /* Enable underline extension (and disables '_' for normal emphasis). */
		HardSoftBreaks = 0x8000, /* Force all soft breaks to act as hard breaks. */
	};

	static std::string toXHTML( std::string_view markdown, Dialect dialect = Dialect::GitHub,
								int flags = 0 );
};

}}} // namespace EE::UI::Doc

#endif
