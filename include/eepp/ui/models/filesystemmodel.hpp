#ifndef EE_UI_MODELS_FILESYSTEMMODEL_HPP
#define EE_UI_MODELS_FILESYSTEMMODEL_HPP

#include <eepp/system/fileinfo.hpp>
#include <eepp/ui/models/model.hpp>
#include <eepp/ui/uiicon.hpp>
#include <memory>

namespace EE { namespace UI { namespace Models {

class EE_API FileSystemModel : public Model {
  public:
	enum class Mode { DirectoriesOnly, FilesAndDirectories };

	struct DisplayConfig {
		DisplayConfig() {}
		DisplayConfig( bool sortByName, bool foldersFirst, bool ignoreHidden,
					   std::vector<std::string> acceptedExtensions = {} ) :
			sortByName( sortByName ),
			foldersFirst( foldersFirst ),
			ignoreHidden( ignoreHidden ),
			acceptedExtensions( acceptedExtensions ) {}
		bool sortByName{ true };
		bool foldersFirst{ true };
		bool ignoreHidden{ false };
		std::vector<std::string> acceptedExtensions;
		bool operator==( const DisplayConfig& other ) {
			return sortByName == other.sortByName && foldersFirst == other.foldersFirst &&
				   ignoreHidden == other.ignoreHidden &&
				   acceptedExtensions == other.acceptedExtensions;
		}
		bool operator!=( const DisplayConfig& other ) { return !( *this == other ); }
	};

	enum Column {
		Icon = 0,
		Name,
		Size,
		Owner,
		Group,
		Permissions,
		ModificationTime,
		Inode,
		Path,
		SymlinkTarget,
		Count,
	};

	struct EE_API Node {
	  public:
		Node( const std::string& rootPath, const FileSystemModel& model );

		Node( FileInfo&& info, Node* parent );

		const std::string& getName() const { return mName; }

		Node* getParent() const { return mParent; }

		const FileInfo& info() const { return mInfo; }

		bool isSelected() const { return mSelected; }

		void setSelected( bool selected ) { mSelected = selected; };

		const std::string& fullPath() const;

		const std::string& getMimeType() const { return mMimeType; }

		size_t childCount() const { return mChildren.size(); }

		const Node& getChild( const size_t& index );

		void invalidate();

		Node* findChildName( const std::string& name, const FileSystemModel& model,
							 bool forceRefresh = false );

	  private:
		friend class FileSystemModel;
		std::string mName;
		std::string mMimeType;
		Node* mParent{ nullptr };
		FileInfo mInfo;
		std::vector<Node> mChildren;
		bool mHasTraversed{ false };
		bool mInfoDirty{ true };
		bool mSelected{ false };

		ModelIndex index( const FileSystemModel& model, int column ) const;

		void traverseIfNeeded( const FileSystemModel& );

		void refreshIfNeeded( const FileSystemModel& );

		bool fetchData( const String& fullPath );
	};

	static std::shared_ptr<FileSystemModel>
	New( const std::string& rootPath, const Mode& mode = Mode::FilesAndDirectories,
		 const DisplayConfig displayConfig = DisplayConfig() );

	const Mode& getMode() const { return mMode; }

	const std::string& getRootPath() const;

	void setRootPath( const std::string& rootPath );

	Node* getNodeFromPath( std::string path, bool folderNode = false, bool invalidateTree = true );

	void reload();

	void update();

	const Node& node( const ModelIndex& index ) const;
	virtual size_t treeColumn() const { return Column::Name; }
	virtual size_t rowCount( const ModelIndex& = ModelIndex() ) const;
	virtual size_t columnCount( const ModelIndex& = ModelIndex() ) const;
	virtual std::string columnName( const size_t& column ) const;
	virtual Variant data( const ModelIndex&, Role role = Role::Display ) const;
	virtual ModelIndex parentIndex( const ModelIndex& ) const;
	virtual ModelIndex index( int row, int column = 0,
							  const ModelIndex& parent = ModelIndex() ) const;

	virtual UIIcon* iconFor( const Node& node, const ModelIndex& index ) const;

	void setMode( const Mode& mode );

	const DisplayConfig& getDisplayConfig() const;

	void setDisplayConfig( const DisplayConfig& displayConfig );

	const ModelIndex& getPreviouslySelectedIndex() const;

	void setPreviouslySelectedIndex( const ModelIndex& previouslySelectedIndex );

  protected:
	std::string mRootPath;
	std::string mRealRootPath;
	std::unique_ptr<Node> mRoot{ nullptr };
	Mode mMode{ Mode::FilesAndDirectories };
	DisplayConfig mDisplayConfig;

	ModelIndex mPreviouslySelectedIndex{};

	Node& nodeRef( const ModelIndex& index ) const;

	FileSystemModel( const std::string& rootPath, const Mode& mode,
					 const DisplayConfig displayConfig );
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODELS_FILESYSTEMMODEL_HPP
