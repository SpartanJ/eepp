#ifndef EE_UI_MODELS_FILESYSTEMMODEL_HPP
#define EE_UI_MODELS_FILESYSTEMMODEL_HPP

#include <eepp/system/fileinfo.hpp>
#include <eepp/ui/models/model.hpp>
#include <memory>

namespace EE { namespace UI { namespace Models {

class EE_API FileSystemModel : public Model {
  public:
	enum class Mode { DirectoriesOnly, FilesAndDirectories };

	enum Column {
		Icon = 0,
		Name,
		Size,
		Owner,
		Group,
		Permissions,
		ModificationTime,
		Inode,
		SymlinkTarget,
		Count,
	};

	struct Node {
	  public:
		Node( const std::string& rootPath, const FileSystemModel& model );
		Node( FileInfo&& info, Node* parent );
		const std::string& getName() const { return mName; }
		Node* getParent() const { return mParent; }
		const FileInfo& info() const { return mInfo; }
		bool isSelected() const { return mSelected; }
		void setSelected( bool selected );
		const std::string& fullPath() const;
		const std::string& getMimeType() const { return mMimeType; }
		size_t childCount() const { return mChildren.size(); }

	  private:
		friend class FileSystemModel;
		std::string mName;
		std::string mMimeType;
		Node* mParent{nullptr};
		FileInfo mInfo;
		std::vector<Node> mChildren;
		bool mHasTraversed{false};
		bool mSelected{false};
		ModelIndex index( const FileSystemModel& model, int column ) const;
		void traverseIfNeeded( const FileSystemModel& );
		void reifyIfNeeded( const FileSystemModel& );
		bool fetchData( const String& fullPath );
	};

	static std::shared_ptr<FileSystemModel> New( const std::string& rootPath,
												 const Mode& mode = Mode::FilesAndDirectories );

	const Mode& getMode() const { return mMode; }

	std::string getRootPath() const;

	void setRootPath( const std::string& rootPath );

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

	virtual Drawable* iconFor( const Node& node, const ModelIndex& index ) const;

	FileSystemModel( const std::string& rootPath, const Mode& mode );

  protected:
	std::string mRootPath;
	std::unique_ptr<Node> mRoot{nullptr};
	Mode mMode{Mode::FilesAndDirectories};

	Node& nodeRef( const ModelIndex& index ) const;
};

}}} // namespace EE::UI::Models

#endif // EE_UI_MODELS_FILESYSTEMMODEL_HPP
