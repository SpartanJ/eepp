#ifndef EE_SYSTEM_IOSTREAMINFLATE_HPP
#define EE_SYSTEM_IOSTREAMINFLATE_HPP

#include <eepp/system/iostream.hpp>
#include <eepp/system/safedatapointer.hpp>
#include <eepp/system/compression.hpp>

namespace EE { namespace System {

struct LocalStreamData;

/** @brief Implementation of a inflating stream */
class EE_API IOStreamInflate : public IOStream {
	public:
		static IOStreamInflate * New( IOStream& inOutStream, Compression::Mode mode );

		/** @brief Use a stream as a input or output buffer
		**	@param inOutStream Stream where the results will ve loaded or saved.
		**	It must be used only for reading or writing, can't mix both calls.
		**	@param mode Compression/Decompression method used
		*/
		IOStreamInflate( IOStream& inOutStream, Compression::Mode mode );

		virtual ~IOStreamInflate();

		ios_size read( char * data, ios_size size );

		ios_size write( const char * data, ios_size size );

		ios_size seek( ios_size position );

		ios_size tell();

		ios_size getSize();

		bool isOpen();

		const Compression::Mode& getMode() const;
	protected:
		IOStream& mStream;
		Compression::Mode mMode;
		SafeDataPointer mBuffer;
		LocalStreamData * mLocalStream;
};

}}

#endif // EE_SYSTEM_IOSTREAMINFLATE_HPP
