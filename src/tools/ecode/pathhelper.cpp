#include "pathhelper.hpp"

namespace ecode {

bool PathHelper::isVideoExtension( std::string_view ext ) {
	static constexpr std::array<std::string_view, 10> extensions = {
		"mp4", "mov", "mkv", "avi", "wmv", "webm", "flv", "mpg", "mpeg", "m4v" };
	for ( const auto& cext : extensions )
		if ( String::iequals( ext, cext ) )
			return true;
	return false;
}

bool PathHelper::isDocumentExtension( std::string_view ext ) {
	static constexpr std::array<std::string_view, 5> extensions = { "doc", "docx", "pdf", "xls",
																	"xlsx" };
	for ( const auto& cext : extensions )
		if ( String::iequals( ext, cext ) )
			return true;
	return false;
}

bool PathHelper::isOpenExternalExtension( std::string_view ext ) {
	return isVideoExtension( ext ) || isDocumentExtension( ext );
}

} // namespace ecode
