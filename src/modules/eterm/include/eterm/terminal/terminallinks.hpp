#ifndef ETERM_TERMINALLINKS_HPP
#define ETERM_TERMINALLINKS_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace eterm { namespace Terminal {

/** Links found in terminal text without OSC 8: URLs with a scheme and file paths, matched with
 * Ghostty's default link pattern. */
class TerminalLinks {
  public:
	/** A match as UTF-8 byte offsets, [start, end). */
	struct Match {
		std::size_t start{ 0 };
		std::size_t end{ 0 };
	};

	/** The link pattern (PCRE2). */
	static std::string_view pattern();

	/** The link in the UTF-8 @p text that contains byte @p offset, if any. The pattern's
	 * lookaheads rescan the rest of a run from every position, so the cost grows with the square of
	 * a long run that never matches: callers bound @p text (see TerminalDisplay::detectLinkAt()).
	 */
	static std::optional<Match> findAt( std::string_view text, std::size_t offset );

	/** Where a matched link opens: a URL with a scheme as written, or a file URI for an existing
	 * file or directory. Relative paths resolve against @p workingDirectory, and leading `~/`
	 * and `$VAR/` expand; a trailing `:line` or `:line:column` is dropped when only the file
	 * without it exists. @p path receives the local path of a file link. Returns an empty
	 * string when the link does not resolve. */
	static std::string resolve( std::string_view link, const std::string& workingDirectory,
								std::string* path = nullptr );
};

}} // namespace eterm::Terminal

#endif
