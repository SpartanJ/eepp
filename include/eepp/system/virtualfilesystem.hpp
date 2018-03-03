#ifndef EE_VIRTUALFILESYSTEM_HPP
#define EE_VIRTUALFILESYSTEM_HPP

#include <eepp/system/singleton.hpp>
#include <eepp/system/container.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/pack.hpp>

namespace EE { namespace System {

class EE_API VirtualFileSystem : protected Container<Pack> {
	SINGLETON_DECLARE_HEADERS(VirtualFileSystem)

	public:
		std::vector<std::string> filesGetInPath( std::string path );

		Pack * getPackFromFile( std::string path );

		IOStream * getFileFromPath( const std::string& path );

		bool fileExists( const std::string& path );
	protected:
		friend class Pack;

		class vfsFile {
			public:
				std::string path;
				Pack * pack;

				vfsFile() {}

				vfsFile( std::string path, Pack * pack ) :
					path( path ),
					pack( pack )
				{}
		};

		class vfsDirectory {
			public:
				vfsDirectory() {}
				std::string path;
				std::map<std::string,vfsFile> files;
				std::map<std::string,vfsDirectory> directories;
		};

		VirtualFileSystem();

		void onResourceAdd( Pack * resource );

		void onResourceRemove( Pack * resource );

		void addFile( std::string path, Pack * pack );

		void removePackFromDirectory( Pack * resource, vfsDirectory& directory );

		vfsDirectory mRoot;
};

class EE_API VFS {
	public:
		static VirtualFileSystem * instance() {
			return VirtualFileSystem::instance();
		}
};

}}

#endif
