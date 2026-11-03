#ifndef ETERM_PTYOUTPUTQUEUE_HPP
#define ETERM_PTYOUTPUTQUEUE_HPP

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>

namespace eterm { namespace Terminal {

/** PTY output handed from a reader thread to the session worker, in arrival order. Consumed bytes
 * are reclaimed once they are at least as many as the unread ones, so storage stays below twice
 * the unread bytes even when the queue never empties, at an amortized constant cost per byte. Not
 * synchronized: callers hold their own lock. */
class PtyOutputQueue {
  public:
	void append( const char* data, std::size_t size ) { mBuffer.append( data, size ); }

	std::size_t read( char* out, std::size_t size ) {
		const std::size_t count = std::min( size, pending() );
		std::memcpy( out, mBuffer.data() + mOffset, count );
		mOffset += count;
		if ( mOffset == mBuffer.size() ) {
			mBuffer.clear();
			mOffset = 0;
		} else if ( mOffset >= pending() ) {
			mBuffer.erase( 0, mOffset );
			mOffset = 0;
		}
		return count;
	}

	std::size_t pending() const { return mBuffer.size() - mOffset; }

	/** Bytes held, consumed or not. */
	std::size_t storedSize() const { return mBuffer.size(); }

  private:
	std::string mBuffer;
	std::size_t mOffset{ 0 };
};

}} // namespace eterm::Terminal

#endif
