#ifndef ECODE_GITDIFF_HPP
#define ECODE_GITDIFF_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ecode {

enum class GitLineChange : std::uint8_t { None, Added, Modified };

struct GitLineDecoration {
	GitLineChange change{ GitLineChange::None };
	bool deletedBefore{ false };
};

struct ComputedGitLineDiff {
	std::vector<GitLineDecoration> lines;
	bool deletedAtEOF{ false };
};

void normalizeGitDiffText( std::string& text );

ComputedGitLineDiff computeGitLineDiff( std::string_view baseline, std::string_view current );

} // namespace ecode

#endif // ECODE_GITDIFF_HPP
