#ifndef EE_SYSTEMCZIP_HPP
#define EE_SYSTEMCZIP_HPP

#include <eepp/core/cancellationtoken.hpp>
#include <eepp/system/pack.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <span>

struct zip;

namespace EE { namespace System {

/** ZIP archive reader/writer and optional resource pack. Use Zip(false) and OpenMode::ReadOnly
 * for standalone browsing. Entry streams borrow the archive and must be destroyed before it.
 * Serialize access to an archive and its streams when sharing them between threads. */
class EE_API Zip : public Pack {
  public:
	static Zip* New();

	struct Entry {
		std::string name;
		Uint64 size{ 0 };
		Int64 modified{ 0 };
		Uint32 crc{ 0 };
		/** POSIX file type and permissions, or zero when the archive does not provide them. */
		Uint32 unixMode{ 0 };
	};

	enum class OpenMode : Uint8 { ReadOnly, ReadWrite };

	enum class Error : Uint8 {
		None,
		NotFound,
		PermissionDenied,
		InvalidArchive,
		Unsupported,
		ReadOnly,
		IO,
		Cancelled
	};

	/** Deferred entries are consumed only by close(). Factories open one stream at a time and
	 * may be called again if libzip retries. They run synchronously on the thread calling close().
	 */
	using StreamFactory = std::function<std::unique_ptr<IOStream>()>;

	struct EntryOptions {
		std::optional<Int64> modified;
		std::optional<Uint32> permissions;
		int compressionLevel;
		bool symbolicLink;

		EntryOptions( int level = 9 ) : compressionLevel( level ), symbolicLink( false ) {}
	};

	Zip();

	explicit Zip( bool registerResource );

	~Zip();

	/** Creates a new pack file */
	bool create( const std::string& path );

	/** Creates a standalone archive in an empty, writable, seekable stream. The caller owns
	 * the output through close()/discard() and handles publication/rollback. Requires Zip(false).
	 * No whole-archive buffer or intermediate copy is used. */
	bool create( IOStream& output );

	/** Discards all pending changes without writing. */
	void discard();

	/** Cancellation is polled during finalization, including compression of a single large file. */
	bool setCancellationToken( CancellationToken token );

	/** Finalization progress in [0,1], delivered synchronously on the close() thread. */
	bool setProgressCallback( std::function<void( double )> callback );

	/** Queues a sized stream. Its factory and owned state survive until close()/discard(). */
	bool addStream( StreamFactory open, Uint64 size, const std::string& name,
					const EntryOptions& options = EntryOptions() );

	/** Copies memory into an owned deferred source; the caller may immediately release it. */
	bool addData( std::span<const Uint8> data, const std::string& name,
				  const EntryOptions& options = EntryOptions() );

	bool addDirectory( const std::string& name, const EntryOptions& options = EntryOptions() );

	/** Opens an existing archive for reading and writing. Missing files are never created;
	 * use create() explicitly. */
	bool open( const std::string& path );

	/** Paths are UTF-8 on every platform. Read-only modes cannot modify or create an archive. */
	bool open( const std::string& path, OpenMode mode );

	Uint64 entryCount() const;

	bool entry( Uint64 index, Entry& result );

	/** Opens an entry without extracting it into memory. Backward seeks replay the prefix;
	 * scratch space is bounded. Reads/seeks return -1 on error, including CRC failures. */
	std::unique_ptr<IOStream> openStream( Uint64 index );

	Error error() const { return mError; }

	const std::string& errorMessage() const { return mErrorMessage; }

	/** Finalizes all queued entries once. Check the result: data is not committed until this
	 * succeeds. */
	bool close();

	/** Queues a file for streaming at close(); the path must remain valid until then.
	 * No file contents are copied into memory.
	 * Add a file to the pack file
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
	bool configureEntry( Int64 index, const EntryOptions& options, bool directory );

	void setError( int code, int systemCode = 0 );

	bool writable();

	bool extractStream( IOStream& stream, Uint8* data, size_t size );
};

}} // namespace EE::System
#endif
