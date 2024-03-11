#include "stringhelper.hpp"

namespace ecode {

void StringHelper::readBySeparator( const std::string_view& buf,
									std::function<void( std::string_view )> onSepChunkRead,
									char sep ) {
	auto lastNL = 0;
	auto nextNL = buf.find_first_of( sep );
	while ( nextNL != std::string_view::npos ) {
		onSepChunkRead( buf.substr( lastNL, nextNL - lastNL ) );
		lastNL = nextNL + 1;
		nextNL = buf.find_first_of( sep, nextNL + 1 );
	}
}

size_t StringHelper::countLines( const std::string_view& text ) {
	const char* startPtr = text.data();
	const char* endPtr = text.data() + text.size();
	size_t count = 0;
	if ( startPtr != endPtr ) {
		count = 1 + *startPtr == '\n' ? 1 : 0;
		while ( ++startPtr && startPtr != endPtr )
			count += ( '\n' == *startPtr ) ? 1 : 0;
	}
	return count;
}

} // namespace ecode
