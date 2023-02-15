#ifndef EE_SYSTEM_FILEINFO_HPP
#define EE_SYSTEM_FILEINFO_HPP

#include <eepp/config.hpp>
#include <map>
#include <string>
#include <vector>

namespace EE { namespace System {

class EE_API FileInfo {
  public:
	static bool exists( const std::string& filePath );

	static bool isLink( const std::string& filePath );

	static bool inodeSupported();

	FileInfo();

	FileInfo( const std::string& filepath );

	FileInfo( const std::string& filepath, bool linkInfo );

	FileInfo( const FileInfo& other );

	bool operator==( const FileInfo& other ) const;

	bool operator!=( const FileInfo& other ) const;

	FileInfo& operator=( const FileInfo& other );

	bool isExecutable() const;

	bool isDirectory() const;

	bool isRegularFile() const;

	bool isReadable() const;

	bool sameInode( const FileInfo& other ) const;

	bool isLink() const;

	bool isHidden() const;

	bool linksToDirectory() const;

	std::string linksTo() const;

	bool exists() const;

	void getInfo();

	void getRealInfo();

	const std::string& getFilepath() const;

	std::string getFileName() const;

	std::string getDirectoryPath() const;

	const Uint64& getModificationTime() const;

	const Uint64& getSize() const;

	const Uint32& getOwnerId() const;

	const Uint32& getGroupId() const;

	const Uint32& getPermissions() const;

	const Uint64& getInode() const;

	bool isUninitialized() const;

	std::string getExtension( const bool& lowerExt = true ) const;

  protected:
	mutable std::string mFilepath;
	std::string mFileName;
	Uint64 mModificationTime{ 0 };
	Uint64 mSize{ 0 };
	Uint32 mOwnerId{ 0 };
	Uint32 mGroupId{ 0 };
	Uint32 mPermissions{ 0 };
	Uint64 mInode{ 0 };
};

typedef std::map<std::string, FileInfo> FileInfoMap;
typedef std::vector<FileInfo> FileInfoList;

}} // namespace EE::System

#endif
