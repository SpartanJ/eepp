#ifndef EE_SYSTEMCZIP_HPP
#define EE_SYSTEMCZIP_HPP

#include <eepp/system/pack.hpp>
#include <memory>

struct zip;

namespace EE { namespace System {

/** ZIP archive reader/writer and optional resource pack. Use Zip(false) and
 * OpenMode::ReadOnlyBounded for untrusted standalone browsing. Entry streams borrow the archive
 * and must be destroyed before it.
 * Serialize access to an archive and its streams when sharing them between threads. */
class EE_API Zip : public Pack {
  public:
	static Zip* New();

	struct Entry {
		std::string name;
		Uint64 size{ 0 };
		Int64 modified{ 0 };
		Uint32 crc{ 0 };
	};

	enum class OpenMode : Uint8 {
		ReadOnly,
		ReadWrite,
		/** Read-only with a 64 MiB metadata allocation bound, checked before decoding the
		 * central directory. Suitable for untrusted archive browsing. */
		ReadOnlyBounded
	};

	enum class Error : Uint8 {
		None,
		NotFound,
		PermissionDenied,
		InvalidArchive,
		Unsupported,
		ReadOnly,
		IO
	};

	Zip();

	explicit Zip( bool registerResource );

	~Zip();

	/** Creates a new pack file */
	bool create( const std::string& path );

	/** Opens an existing archive for reading and writing. Missing files are never created;
	 * use create() explicitly. */
	bool open( const std::string& path );

	/** ReadOnly supports Unicode native paths and cannot modify or create an archive. */
	bool open( const std::string& path, OpenMode mode );

	Uint64 entryCount() const;

	/** Conservative decoded central-directory memory estimate, including allocation slack.
	 * This excludes decompressed file data and indexes built by the caller. */
	Uint64 metadataSize() const;

	bool entry( Uint64 index, Entry& result );

	/** Opens an entry without extracting it into memory. Backward seeks replay the prefix;
	 * scratch space is bounded. Reads/seeks return -1 on error, including CRC failures. */
	std::unique_ptr<IOStream> openStream( Uint64 index );

	Error error() const { return mError; }

	const std::string& errorMessage() const { return mErrorMessage; }

	/** Close the pack file */
	bool close();

	/** Add a file to the pack file
	 * @param path Path to the file in the disk
	 * @param inpack Path that will have the file inside the pak
	 * @return True if success
	 */
	bool addFile( const std::string& path, const std::string& inpack );

	/** Add a new file from memory */
	bool addFile( const Uint8* data, const Uint32& dataSize, const std::string& inpack );

	/** Add a map of files to the pack file ( myMap[ myFilepath ] = myInPackFilepath ) */
	bool addFiles( std::map<std::string, std::string> paths );

	/** Erase a file from the pack file. ( This will create a new pack file without that file, so,
	 * can be slow ) */
	bool eraseFile( const std::string& path );

	/** Erase all passed files from the pack file. ( This will create a new pack file without that
	 * file, so, can be slow ) */
	bool eraseFiles( const std::vector<std::string>& paths );

	/** Extract a file from the pack file */
	bool extractFile( const std::string& path, const std::string& dest );

	/** Extract a file to memory from the pack file */
	bool extractFileToMemory( const std::string& path, std::vector<Uint8>& data );

	/** Extract a file to memory from the pakFile */
	bool extractFileToMemory( const std::string& path, ScopedBuffer& data );

	/** Check if a file exists in the pack file and return the number of the file, otherwise return
	 * -1. */
	Int32 exists( const std::string& path );

	/** Check the integrity of the pack file. \n If return 0 integrity OK. -1 wrong identifier. -2
	 * wrong header. */
	Int8 checkPack();

	/** @return a vector with all the files inside the pack file */
	std::vector<std::string> getFileList();

	/** Add a new file from memory */
	bool addFile( std::vector<Uint8>& data, const std::string& inpack );

	/** @return The file path of the opened package */
	std::string getPackPath();

	IOStream* getFileStream( const std::string& path );

  protected:
	friend class IOStreamZip;

	struct zip* mZip;

	std::string mZipPath;
	std::string mErrorMessage;
	OpenMode mOpenMode{ OpenMode::ReadWrite };
	Error mError{ Error::None };

	struct zip* getZip();

  private:
	bool openReadOnly( const std::string& path );

	void setError( int code, int systemCode = 0 );

	bool writable();

	bool extractStream( IOStream& stream, Uint8* data, size_t size );
};

}} // namespace EE::System
#endif
