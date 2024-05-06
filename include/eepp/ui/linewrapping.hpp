#ifndef EE_UI_LINEWRAPPING_HPP
#define EE_UI_LINEWRAPPING_HPP

#include <eepp/graphics/fontstyleconfig.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/doc/textposition.hpp>

using namespace EE::Graphics;
using namespace EE::UI::Doc;

namespace EE { namespace UI {

enum class LineWrapMode { Letter, Word };

struct LineWrapInfo {
	std::vector<size_t> wraps;
	Float paddingStart;
	bool hasWraps() const { return !wraps.empty(); }
};

class LineWrapping {
  public:
	static LineWrapInfo computeLineBreaks( const String& string, const FontStyleConfig& fontStyle,
										   Float maxWidth, LineWrapMode mode, bool keepIndentation,
										   Uint32 tabWidth = 4 );

	static LineWrapInfo computeLineBreaks( const TextDocument& doc, size_t line,
										   const FontStyleConfig& fontStyle, Float maxWidth,
										   LineWrapMode mode, bool keepIndentation,
										   Uint32 tabWidth = 4 );

  protected:
	Float mMaxWidth{ 0 };
	std::unordered_map<Int64, TextPosition> mWrappedLines;
	std::unordered_map<Int64, Int64> mWrappedLineToIndex;
};

}} // namespace EE::UI

#endif
