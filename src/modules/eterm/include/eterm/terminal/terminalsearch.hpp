#ifndef ETERM_TERMINALSEARCH_HPP
#define ETERM_TERMINALSEARCH_HPP

#include <eepp/core/string.hpp>
#include <eterm/terminal/terminaltypes.hpp>

#include <vector>

namespace EE { namespace System {
class PatternMatcher;
}} // namespace EE::System

using namespace EE;
using namespace EE::System;

namespace eterm { namespace Terminal {

enum class TerminalBufferSource : Uint8 { MainHistory, MainScreen, AlternateScreen };

enum class TerminalSearchType : Uint8 { Normal, RegEx, LuaPattern };

struct TerminalBufferPosition {
	TerminalBufferSource source{ TerminalBufferSource::MainScreen };
	Int64 row{ 0 };
	Int32 column{ 0 };
};

struct TerminalSearchRowView {
	Line cells{ nullptr };
	TerminalBufferSource source{ TerminalBufferSource::MainScreen };
	Int64 row{ 0 };
	Int32 width{ 0 };
	Int32 length{ 0 };
	bool wrapped{ false };
};

struct TerminalSearchQuery {
	String text;
	Uint64 requestId{ 0 };
	bool caseSensitive{ false };
	bool wholeWord{ false };
	TerminalSearchType type{ TerminalSearchType::Normal };
};

struct TerminalSearchMatch {
	TerminalBufferPosition start;
	TerminalBufferPosition end;
};

/** Worker-side search engine over chronological, non-owning terminal rows. */
class TerminalSearch {
  public:
	static constexpr size_t MinimumQueryLength = 2;

	static bool isQuerySearchable( const TerminalSearchQuery& query ) {
		return query.text.size() >= MinimumQueryLength;
	}

	const std::vector<TerminalSearchMatch>& search( const std::vector<TerminalSearchRowView>& rows,
													const TerminalSearchQuery& query );

	const std::vector<TerminalSearchMatch>& matches() const { return mMatches; }

  private:
	struct CellSpan {
		TerminalBufferPosition start;
		TerminalBufferPosition end;
	};

	void searchLogicalLine( const TerminalSearchQuery& query, PatternMatcher* pattern,
							const String& needle );

	std::vector<TerminalSearchMatch> mMatches;
	String mText;
	String mComparableText;
	std::string mUtf8Text;
	std::vector<CellSpan> mPositions;
};

}} // namespace eterm::Terminal

#endif
