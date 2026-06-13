#ifndef EE_SYSTEM_IOSTREAMFILEMAPPED_HPP
#define EE_SYSTEM_IOSTREAMFILEMAPPED_HPP

#include <eepp/system/filemapped.hpp>
#include <eepp/system/iostream.hpp>

namespace EE { namespace System {

class EE_API IOStreamFileMapped : public IOStream {
  public:
	IOStreamFileMapped();

	explicit IOStreamFileMapped( const std::string& path );

	explicit IOStreamFileMapped( FileMapped&& file );

	~IOStreamFileMapped() override = default;

	IOStreamFileMapped( const IOStreamFileMapped& ) = delete;
	IOStreamFileMapped& operator=( const IOStreamFileMapped& ) = delete;

	IOStreamFileMapped( IOStreamFileMapped&& ) noexcept = default;
	IOStreamFileMapped& operator=( IOStreamFileMapped&& ) noexcept = default;

	bool open( const std::string& path );

	void close();

	ios_size read( char* data, ios_size size ) override;

	ios_size write( const char* data, ios_size size ) override;

	ios_size seek( ios_size position ) override;

	ios_size tell() override;

	ios_size getSize() override;

	bool isOpen() override;

	const FileMapped& getMappedFile() const { return mFile; }
	FileMapped& getMappedFile() { return mFile; }

	const std::string& getLastError() const { return mFile.getLastError(); }

  private:
	FileMapped mFile;
	ios_size mPosition{ 0 };
};

}} // namespace EE::System

#endif
