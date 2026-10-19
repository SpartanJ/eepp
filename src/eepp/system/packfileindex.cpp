#include <eepp/core/string.hpp>
#include <eepp/system/packfileindex.hpp>
#include <mutex>

namespace EE { namespace System {

SINGLETON_DECLARE_IMPLEMENTATION( PackFileIndex )

static std::vector<std::string> splitIndexPath( std::string& path ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( path.find_first_of( '\\' ) != std::string::npos ) {
		String::replaceAll( path, "\\", "/" );
	}
#endif
	// String::split skips empty parts, so leading, trailing and repeated slashes are ignored.
	return String::split( path, '/' );
}

PackFileIndex::PackFileIndex() {}

const PackFileIndex::IndexedDirectory* PackFileIndex::findDirectory( std::string& path ) const {
	std::vector<std::string> parts = splitIndexPath( path );
	const IndexedDirectory* dir = &mRoot;
	for ( const auto& part : parts ) {
		auto it = dir->directories.find( part );
		if ( it == dir->directories.end() )
			return nullptr;
		dir = &it->second;
	}
	return dir;
}

const PackFileIndex::IndexedFile* PackFileIndex::findFile( std::string path ) const {
	std::vector<std::string> parts = splitIndexPath( path );
	if ( parts.empty() )
		return nullptr;
	const IndexedDirectory* dir = &mRoot;
	for ( size_t i = 0; i + 1 < parts.size(); ++i ) {
		auto it = dir->directories.find( parts[i] );
		if ( it == dir->directories.end() )
			return nullptr;
		dir = &it->second;
	}
	auto it = dir->files.find( parts.back() );
	return it != dir->files.end() ? &it->second : nullptr;
}

std::vector<std::string> PackFileIndex::filesGetInPath( std::string path ) {
	std::vector<std::string> files;
	std::shared_lock<std::shared_mutex> lock( mIndexMutex );
	const IndexedDirectory* dir = findDirectory( path );
	if ( !dir )
		return files;
	files.reserve( dir->files.size() );
	for ( const auto& file : dir->files )
		files.push_back( file.second.path );
	return files;
}

std::vector<PackFileIndex::Entry> PackFileIndex::listDirectory( std::string path ) {
	std::vector<Entry> entries;
	std::shared_lock<std::shared_mutex> lock( mIndexMutex );
	const IndexedDirectory* dir = findDirectory( path );
	if ( !dir )
		return entries;
	entries.reserve( dir->files.size() + dir->directories.size() );
	for ( const auto& subdir : dir->directories )
		entries.push_back( Entry{ subdir.first, true } );
	for ( const auto& file : dir->files )
		entries.push_back( Entry{ file.first, false } );
	return entries;
}

Pack* PackFileIndex::getPackFromFile( std::string path ) {
	std::shared_lock<std::shared_mutex> lock( mIndexMutex );
	const IndexedFile* file = findFile( std::move( path ) );
	return file ? file->pack : nullptr;
}

IOStream* PackFileIndex::getFileFromPath( const std::string& path ) {
	std::shared_lock<std::shared_mutex> lock( mIndexMutex );
	const IndexedFile* file = findFile( path );
	return file ? file->pack->getFileStream( file->path ) : nullptr;
}

bool PackFileIndex::extractFileToMemory( const std::string& path, std::vector<Uint8>& data ) {
	// The shared lock keeps the pack alive (packs unregister under the exclusive lock).
	std::shared_lock<std::shared_mutex> lock( mIndexMutex );
	const IndexedFile* file = findFile( path );
	return file && file->pack->extractFileToMemory( file->path, data );
}

bool PackFileIndex::fileExists( const std::string& path ) {
	std::shared_lock<std::shared_mutex> lock( mIndexMutex );
	return nullptr != findFile( path );
}

bool PackFileIndex::directoryExists( std::string path ) {
	std::shared_lock<std::shared_mutex> lock( mIndexMutex );
	return nullptr != findDirectory( path );
}

Uint64 PackFileIndex::getGeneration() const {
	std::shared_lock<std::shared_mutex> lock( mIndexMutex );
	return mGeneration;
}

void PackFileIndex::onResourceAdd( Pack* resource ) {
	// Read the file list before locking: it may take the pack's own lock.
	std::vector<std::string> files = resource->getFileList();
	std::unique_lock<std::shared_mutex> lock( mIndexMutex );
	add( resource );
	for ( auto& file : files )
		addFile( std::move( file ), resource );
	++mGeneration;
}

void PackFileIndex::onResourceRemove( Pack* resource ) {
	std::unique_lock<std::shared_mutex> lock( mIndexMutex );
	remove( resource );
	removePackFromDirectory( resource, mRoot );
	++mGeneration;
}

void PackFileIndex::addFile( std::string path, Pack* pack ) {
	std::string original( path );
	std::vector<std::string> parts = splitIndexPath( path );
	if ( parts.empty() )
		return;
	IndexedDirectory* dir = &mRoot;
	for ( size_t i = 0; i + 1 < parts.size(); ++i )
		dir = &dir->directories[parts[i]];
	dir->files[parts.back()] = IndexedFile{ std::move( original ), pack };
}

bool PackFileIndex::removePackFromDirectory( Pack* resource, IndexedDirectory& directory ) {
	for ( auto it = directory.files.begin(); it != directory.files.end(); ) {
		if ( resource == it->second.pack )
			it = directory.files.erase( it );
		else
			++it;
	}

	// Directories only exist while they contain files; drop the ones emptied by this pack.
	for ( auto it = directory.directories.begin(); it != directory.directories.end(); ) {
		if ( removePackFromDirectory( resource, it->second ) )
			it = directory.directories.erase( it );
		else
			++it;
	}

	return directory.files.empty() && directory.directories.empty();
}

}} // namespace EE::System
