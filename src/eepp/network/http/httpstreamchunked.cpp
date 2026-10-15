#include <algorithm>
#include <charconv>
#include <eepp/network/http/httpstreamchunked.hpp>

namespace EE { namespace Network { namespace Private {

HttpStreamChunked::HttpStreamChunked( IOStream& writeTo ) : mWriteTo( writeTo ) {}

ios_size HttpStreamChunked::write( const char* data, ios_size size ) {
	if ( size < 0 || mState == State::Error )
		return -1;
	const char* begin = data;
	const char* end = data + size;
	while ( data != end && mState != State::Error ) {
		// No pipelining: bytes after the trailer terminator cannot belong to this response. Stop
		// consuming; the caller sees a short write and must not reuse the connection.
		if ( mState == State::Complete )
			break;
		if ( mState == State::Data ) {
			const auto count =
				static_cast<std::size_t>( std::min<Uint64>( mRemaining, end - data ) );
			if ( mWriteTo.write( data, count ) != static_cast<ios_size>( count ) ) {
				mState = State::Error;
				break;
			}
			data += count;
			mRemaining -= count;
			if ( mRemaining == 0 )
				mState = State::DataCR;
		} else if ( mState == State::DataCR || mState == State::DataLF ) {
			const bool cr = mState == State::DataCR;
			if ( *data++ != ( cr ? '\r' : '\n' ) )
				mState = State::Error;
			else
				mState = cr ? State::DataLF : State::Size;
		} else {
			mLine += *data++;
			// Bound chunk extensions and trailers to the same 64 KiB header budget as Http.
			if ( mLine.size() + mHeaderBuffer.size() > 64 * 1024 ) {
				mState = State::Error;
				break;
			}
			if ( mLine.back() != '\n' )
				continue;
			if ( mLine.size() < 2 || mLine[mLine.size() - 2] != '\r' ) {
				mState = State::Error;
				break;
			}
			if ( mState == State::Trailer ) {
				// The zero chunk alone is not completion: consume all trailers and the empty line.
				if ( mLine.size() == 2 )
					mState = State::Complete;
				else
					mHeaderBuffer += mLine;
			} else {
				std::string_view length( mLine.data(), mLine.size() - 2 );
				length = length.substr( 0, length.find( ';' ) );
				// RFC 9112 7.1.1 allows bad whitespace (BWS) before a chunk extension's ';'.
				while ( !length.empty() && ( length.back() == ' ' || length.back() == '\t' ) )
					length.remove_suffix( 1 );
				auto result =
					std::from_chars( length.data(), length.data() + length.size(), mRemaining, 16 );
				if ( length.empty() || result.ec != std::errc() ||
					 result.ptr != length.data() + length.size() )
					mState = State::Error;
				else
					mState = mRemaining == 0 ? State::Trailer : State::Data;
			}
			mLine.clear();
		}
	}
	return mState == State::Error ? -1 : data - begin;
}

const std::string& HttpStreamChunked::getHeaderBuffer() const {
	return mHeaderBuffer;
}

bool HttpStreamChunked::isComplete() const {
	return mState == State::Complete;
}

bool HttpStreamChunked::hasError() const {
	return mState == State::Error;
}

}}} // namespace EE::Network::Private
