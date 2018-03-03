#include <eepp/system/virtualfilesystem.hpp>

namespace EE { namespace System {

SINGLETON_DECLARE_IMPLEMENTATION(VirtualFileSystem)

static std::vector<std::string> vfsSplitPath( std::string& path ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( path.find_first_of( '\\' ) != std::string::npos ) {
		String::replaceAll( path, "\\", "/" );
	}
#endif
	return String::split( path, '/' );
}

VirtualFileSystem::VirtualFileSystem() {
}

std::vector<std::string> VirtualFileSystem::filesGetInPath( std::string path ) {
	std::vector<std::string> files;
	std::vector<std::string> paths = vfsSplitPath( path );
	vfsDirectory * curDir = &mRoot;
	size_t pos = 0;

	if ( paths.size() >= 1 ) {
		do {
			if ( pos < paths.size() ) {
				if ( curDir->directories.find( paths[pos] ) == curDir->directories.end() ) {
					return files;
				}

				curDir = &curDir->directories[ paths[pos] ];
			}

			pos++;
		} while ( pos < paths.size() );
	}

	for ( auto it = curDir->files.begin(); it != curDir->files.end(); ++it ) {
		vfsFile& file = it->second;
		files.push_back( file.path );
	}

	return files;
}

Pack * VirtualFileSystem::getPackFromFile( std::string path ) {
	std::vector<std::string> paths = vfsSplitPath( path );
	vfsDirectory * curDir = &mRoot;
	size_t pos = 0;

	if ( paths.size() >= 1 ) {
		do {
			if ( pos == paths.size() - 1 ) {
				if ( curDir->files.find( paths[pos] ) != curDir->files.end() ) {
					return curDir->files[ paths[pos] ].pack;
				}
			} else {
				if ( curDir->directories.find( paths[pos] ) == curDir->directories.end() ) {
					return NULL;
				}

				curDir = &curDir->directories[ paths[pos] ];
			}

			pos++;
		} while ( pos < paths.size() );
	}

	return NULL;
}

IOStream * VirtualFileSystem::getFileFromPath( const std::string& path ) {
	Pack * pack = getPackFromFile( path );
	return NULL != pack ? pack->getFileStream( path ) : NULL;
}

bool VirtualFileSystem::fileExists( const std::string& path ) {
	return NULL != getPackFromFile( path );
}

void VirtualFileSystem::onResourceAdd( Pack * resource ) {
	add( resource );

	std::vector<std::string> files = resource->getFileList();

	for ( auto it = files.begin(); it != files.end(); ++it ) {
		addFile( *it, resource );
	}
}

void VirtualFileSystem::onResourceRemove( Pack * resource ) {
	remove( resource );
	removePackFromDirectory( resource, mRoot );
}

void VirtualFileSystem::addFile( std::string path , Pack * pack ) {
	std::vector<std::string> paths = vfsSplitPath( path );
	vfsDirectory * curDir = &mRoot;
	size_t pos = 0;

	if ( paths.size() >= 1 ) {
		do {
			if ( pos == paths.size() - 1 ) {
				curDir->files[ paths[pos] ] = vfsFile( path, pack );
			} else {
				if ( curDir->directories.find( paths[pos] ) == curDir->directories.end() ) {
					curDir->directories[ paths[pos] ] = vfsDirectory();
				}

				curDir = &curDir->directories[ paths[pos] ];
			}

			pos++;
		} while ( pos < paths.size() );
	}
}

void VirtualFileSystem::removePackFromDirectory( Pack * resource, vfsDirectory& directory ) {
	std::vector<std::string> removeList;

	for ( auto it = directory.files.begin(); it != directory.files.end(); ++it ) {
		vfsFile& file = it->second;

		if ( resource == file.pack ) {
			removeList.push_back( it->first );
		}
	}

	for( auto it = removeList.begin(); it != removeList.end(); ++it ) {
		directory.files.erase( *it );
	}

	for ( auto it = directory.directories.begin(); it != directory.directories.end(); ++it ) {
		removePackFromDirectory( resource, it->second );
	}
}

}}
