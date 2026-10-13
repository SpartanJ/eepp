#ifndef EE_SYSTEM_PACKFILEINDEX_HPP
#define EE_SYSTEM_PACKFILEINDEX_HPP

#include <eepp/system/container.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/scopedbuffer.hpp>
#include <eepp/system/singleton.hpp>
#include <map>
#include <shared_mutex>
#include <string>
#include <vector>

namespace EE { namespace System {

/** @brief Path index over the files of every open Pack.
 *
 * Maps virtual paths such as "fonts/DejaVuSans.ttf" to the Pack that provides them. When several
 * packs provide the same path, the most recently opened one wins. This is the storage behind the
 * `res://` scheme of the eepp-vfs module (EE::VFS::ResourceProvider); it formerly was named
 * VirtualFileSystem.
 *
 * All methods are thread-safe.
 */
class EE_API PackFileIndex : protected Container<Pack> {
	SINGLETON_DECLARE_HEADERS( PackFileIndex )

  public:
	struct Entry {
		std::string name;
		bool isDirectory{ false };
	};

	/** @return The full paths of the files directly inside path. */
	std::vector<std::string> filesGetInPath( std::string path );

	/** @return The files and directories directly inside path, sorted by name. Directories are
	 * implicit: they exist while any file below them exists. The root is "" (or "/"). */
	std::vector<Entry> listDirectory( std::string path );

	/** @return The pack that provides path, or nullptr. The pointer is only valid while the pack
	 * stays open. */
	Pack* getPackFromFile( std::string path );

	/** @return A new stream for path (owned by the caller), or nullptr. */
	IOStream* getFileFromPath( const std::string& path );

	/** Extracts path into data. @return False if the file is not indexed or extraction failed. */
	bool extractFileToMemory( const std::string& path, std::vector<Uint8>& data );

	bool fileExists( const std::string& path );

	bool directoryExists( std::string path );

	/** @return A counter that changes every time a pack is added or removed. */
	Uint64 getGeneration() const;

  protected:
	friend class Pack;

	struct IndexedFile {
		std::string path;
		Pack* pack{ nullptr };
	};

	struct IndexedDirectory {
		std::map<std::string, IndexedFile, std::less<>> files;
		std::map<std::string, IndexedDirectory, std::less<>> directories;
	};

	PackFileIndex();

	void onResourceAdd( Pack* resource );

	void onResourceRemove( Pack* resource );

	void addFile( std::string path, Pack* pack );

	static bool removePackFromDirectory( Pack* resource, IndexedDirectory& directory );

	const IndexedDirectory* findDirectory( std::string& path ) const;

	const IndexedFile* findFile( std::string path ) const;

	mutable std::shared_mutex mIndexMutex;
	IndexedDirectory mRoot;
	Uint64 mGeneration{ 0 };
};

}} // namespace EE::System

#endif
