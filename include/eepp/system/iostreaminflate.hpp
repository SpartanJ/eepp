#ifndef EE_SYSTEM_IOSTREAMINFLATE_HPP
#define EE_SYSTEM_IOSTREAMINFLATE_HPP

#include <eepp/system/compression.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/scopedbuffer.hpp>

namespace EE { namespace System {

struct LocalInflateStreamData;

/** @brief Implementation of a inflating stream */
class EE_API IOStreamInflate : public IOStream {
  public:
	static IOStreamInflate* New( IOStream& inOutStream, Compression::Mode mode );

	/** @brief Use a stream as a input or output buffer
	**	@param inOutStream Stream where the results will ve loaded or saved.
	**	It must be used only for reading or writing, can't mix both calls.
	**	@param mode Compression/Decompression method used
	*/
	IOStreamInflate( IOStream& inOutStream, Compression::Mode mode );

	virtual ~IOStreamInflate();

	virtual ios_size read( char* data, ios_size size );

	virtual ios_size write( const char* data, ios_size size );

	virtual ios_size seek( ios_size position );

	virtual ios_size tell();

	virtual ios_size getSize();

	virtual bool isOpen();

	const Compression::Mode& getMode() const;

	/** True after write() has consumed and validated the compressed stream's end marker. */
	bool isComplete() const;

	bool hasError() const;

	/** True once write() has received any compressed bytes. An empty input never completes. */
	bool hasInput() const;

  protected:
	IOStream& mStream;
	Compression::Mode mMode;
	ScopedBuffer mBuffer;
	LocalInflateStreamData* mLocalStream;
	bool mHasInput{ false };
};

}} // namespace EE::System

#endif // EE_SYSTEM_IOSTREAMINFLATE_HPP
