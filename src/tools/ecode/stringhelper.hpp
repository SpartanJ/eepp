#ifndef ECODE_STRINGHELPER
#define ECODE_STRINGHELPER

#include <functional>
#include <string_view>

namespace ecode {

struct StringHelper {
	static void readBySeparator( const std::string_view& buf,
								 std::function<void( std::string_view )> onSepChunkRead,
								 char sep = '\n' );

	static size_t countLines( const std::string_view& text );
};

} // namespace ecode

#endif
