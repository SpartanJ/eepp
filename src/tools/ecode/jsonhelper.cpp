#include "jsonhelper.hpp"

namespace {

static size_t nextJSONToken( std::string_view contents, size_t pos ) {
	while ( pos < contents.size() ) {
		while ( pos < contents.size() && ( contents[pos] == ' ' || contents[pos] == '\t' ||
										   contents[pos] == '\r' || contents[pos] == '\n' ) )
			++pos;
		if ( pos + 1 >= contents.size() || contents[pos] != '/' )
			break;
		if ( contents[pos + 1] == '/' ) {
			pos += 2;
			while ( pos < contents.size() && contents[pos] != '\n' )
				++pos;
		} else if ( contents[pos + 1] == '*' ) {
			pos += 2;
			while ( pos + 1 < contents.size() &&
					!( contents[pos] == '*' && contents[pos + 1] == '/' ) )
				++pos;
			if ( pos + 1 < contents.size() )
				pos += 2;
		} else {
			break;
		}
	}
	return pos;
}

} // namespace

std::string json_strip_trailing_commas( std::string_view contents ) {
	std::string sanitized;
	sanitized.reserve( contents.size() );
	bool inString = false;
	bool escaped = false;
	bool lineComment = false;
	bool blockComment = false;
	for ( size_t pos = 0; pos < contents.size(); ++pos ) {
		const char ch = contents[pos];
		if ( lineComment ) {
			lineComment = ch != '\n';
			sanitized.push_back( ch );
			continue;
		}
		if ( blockComment ) {
			if ( ch == '*' && pos + 1 < contents.size() && contents[pos + 1] == '/' ) {
				blockComment = false;
				sanitized += "*/";
				++pos;
			} else {
				sanitized.push_back( ch );
			}
			continue;
		}
		if ( inString ) {
			sanitized.push_back( ch );
			if ( escaped )
				escaped = false;
			else if ( ch == '\\' )
				escaped = true;
			else if ( ch == '"' )
				inString = false;
			continue;
		}
		if ( ch == '"' ) {
			inString = true;
			sanitized.push_back( ch );
			continue;
		}
		if ( ch == '/' && pos + 1 < contents.size() ) {
			if ( contents[pos + 1] == '/' )
				lineComment = true;
			else if ( contents[pos + 1] == '*' )
				blockComment = true;
			sanitized.push_back( ch );
			continue;
		}
		if ( ch == ',' ) {
			const size_t next = nextJSONToken( contents, pos + 1 );
			if ( next < contents.size() && ( contents[next] == '}' || contents[next] == ']' ) )
				continue;
		}
		sanitized.push_back( ch );
	}
	return sanitized;
}
