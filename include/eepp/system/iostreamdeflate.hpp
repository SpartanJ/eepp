#ifndef EE_SYSTEM_IOSTREAMDEFLATE_HPP
#define EE_SYSTEM_IOSTREAMDEFLATE_HPP

#include <eepp/system/compression.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/scopedbuffer.hpp>

namespace EE { namespace System {

struct LocalStreamData;

/** @brief Implementation of a deflating stream */
class EE_API IOStreamDeflate : public IOStream {
  public:
	static IOStreamDeflate* New( IOStream& inOutStream, Compression::Mode mode,
								 const Compression::Config& config = Compression::Config() );

	/** @brief Use a stream as a input or output buffer
	**	@param inOutStream Stream where the results will ve loaded or saved.
	**	It must be used only for reading or writing, can't mix both calls.
	**	@param mode Compression method used
	**	@param config Compression configuration
	*/
	IOStreamDeflate( IOStream& inOutStream, Compression::Mode mode,
					 const Compression::Config& config = Compression::Config() );

	virtual ~IOStreamDeflate();

	virtual ios_size read( char* data, ios_size size );

	virtual ios_size write( const char* data, ios_size size );

	virtual ios_size seek( ios_size position );

	virtual ios_size tell();

	virtual ios_size getSize();

	virtual bool isOpen();

	const Compression::Mode& getMode() const;

  protected:
	IOStream& mStream;
	Compression::Mode mMode;
	ScopedBuffer mBuffer;
	LocalStreamData* mLocalStream;
};

}} // namespace EE::System

#endif // EE_SYSTEM_IOSTREAMDEFLATE_HPP
