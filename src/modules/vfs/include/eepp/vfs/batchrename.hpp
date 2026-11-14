#ifndef EE_VFS_BATCHRENAME_HPP
#define EE_VFS_BATCHRENAME_HPP

#include <eepp/vfs/error.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace EE { namespace VFS {

/** The original basename and its recognized extension, without the dot. Supply an empty
 * extension for directories. Callers choose their MIME/extension policy; VFS does not guess. */
struct EE_VFS_API BatchRenameItem {
	std::string name;
	std::string extension;
};

/** Pure, provider-independent filename transformation. Numbering follows input order. */
struct EE_VFS_API BatchRenamePattern {
	enum class Mode : Uint8 { Enumerate, ReplaceText, AddText };
	enum class Position : Uint8 { Replace, Before, After };

	/** Number template, literal search pattern, or text to add, according to mode. */
	std::string text;
	std::string replacement;
	Uint64 startIndex{ 1 };
	Mode mode{ Mode::Enumerate };
	Position position{ Position::Replace };
	/** Allow an enumeration without one consecutive # group (for unique extensions). */
	bool allowUnnumbered{ false };

	/** Produces one basename without I/O. One consecutive # group is replaced with the index,
	 * zero-padded to the group's length; Before/After retain the original stem. ReplaceText is
	 * literal and case-sensitive and trims leading spaces. Recognized extensions are restored
	 * once, preserving their original case. Empty stems and mismatched extensions are errors. */
	VFSResult<std::string> apply( const BatchRenameItem& item, Uint64 offset = 0 ) const;
};

/** A batch always returns the owned input, proposed names (possibly incomplete on a planning
 * error), and a completed prefix (including
 * unchanged items). An operation error or cancellation is retained alongside that prefix:
 * earlier renames remain applied. Planning/preflight errors have completed == 0. */
struct EE_VFS_API BatchRenameResult {
	std::vector<BatchRenameItem> items;
	std::vector<std::string> names;
	std::optional<VFSError> error;
	size_t completed{ 0 };
};

}} // namespace EE::VFS

#endif
