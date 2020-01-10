#include <eepp/network/http/httpstreamchunked.hpp>

namespace EE { namespace Network { namespace Private {

HttpStreamChunked::HttpStreamChunked( IOStream& mWriteTo ) :
	mWriteTo( mWriteTo ), mChunkNewBuffer( false ), mChunkEnded( false ) {}

ios_size HttpStreamChunked::write( const char* data, ios_size size ) {
	ios_size writeTotal = 0;

	// If the chunk reading ended we just add the buffer received as a header
	// Otherwise we process the buffer data as chunk
	if ( !mChunkEnded ) {
		// Keep a chunk buffer until the end of chunk is found
		mChunkBuffer.append( data, size );

		// If the new chunk starts with \r\n and the last removed chunk
		// did not contain the trailing \r\n, we remove it to detect
		// correctly the next length data
		if ( mChunkNewBuffer ) {
			if ( mChunkBuffer.substr( 0, 2 ) == "\r\n" ) {
				mChunkBuffer = mChunkBuffer.substr( 2 );
			}

			mChunkNewBuffer = false;
		}

		bool retry;

		do {
			retry = false;

			// Check for the first \r\n to find the end of the length definition
			std::string::size_type lenEnd = mChunkBuffer.find_first_of( "\r\n" );

			if ( lenEnd != std::string::npos ) {
				std::string::size_type firstCharPos = lenEnd + 2;
				unsigned long length;

				// Get the length of the chunk
				bool res = String::fromString( length, mChunkBuffer.substr( 0, lenEnd ), std::hex );

				// If the length is solved...
				if ( res ) {
					// And it's bigger than 0, means that there are more chunks
					if ( length > 0 ) {
						// Check if the chunk buffer size at least equals to the length reported
						if ( mChunkBuffer.size() - firstCharPos >= length ) {
							// In that case write the chunk to the file buffer
							writeTotal = mWriteTo.write( &mChunkBuffer[firstCharPos], length );

							// And keep the remaining not completed chunk
							mChunkBuffer = mChunkBuffer.substr( firstCharPos + length );

							// Check if already have the \r\n of the next length in the buffer
							if ( !mChunkBuffer.empty() ) {
								std::size_t pos = 0;

								// Remove al the \r\n remaining
								while ( pos < mChunkBuffer.size() &&
										0 == strncmp( &mChunkBuffer[pos], "\r\n", 2 ) ) {
									pos += 2;
								}

								if ( pos > 0 ) {
									// Remove it to be able to read the next length
									mChunkBuffer = mChunkBuffer.substr( pos );
								}

								// If still the chunk is not empty it could be another chunk
								// already received, so we check that retrying
								if ( !mChunkBuffer.empty() ) {
									std::string::size_type lenEnd =
										mChunkBuffer.find_first_of( "\r\n" );

									if ( lenEnd != std::string::npos ) {
										bool res = String::fromString(
											length, mChunkBuffer.substr( 0, lenEnd ), std::hex );

										if ( res && length > 0 ) {
											retry = true;
										}
									}
								}
							}

							// If the next chunk received starts with \r\n it's because
							// it's part of the chunk size information, so we need to flag it
							// to remove it
							mChunkNewBuffer = true;
						}
					} else {
						// If the value is 0 means that the data ended
						// But after this we can receive extra headers
						mChunkEnded = true;
						mChunkBuffer.clear();
					}
				}
			}
		} while ( retry );
	} else {
		mHeaderBuffer.append( data, size );
	}

	return writeTotal;
}

const std::string& HttpStreamChunked::getHeaderBuffer() const {
	return mHeaderBuffer;
}

}}} // namespace EE::Network::Private
