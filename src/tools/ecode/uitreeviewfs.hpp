#pragma once

#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/ui/models/modelindex.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <vector>

using namespace EE::UI;

namespace ecode {

class UITreeViewFS : public UITreeView {
  public:
	static UITreeViewFS* New() { return eeNew( UITreeViewFS, () ); }

	UITreeViewFS();

	UIWidget* createCell( UIWidget* rowWidget, const ModelIndex& index ) override;

	Uint32 onMessage( const NodeMessage* msg ) override;

	KeyBindings& getKeyBindings() { return mKeyBindings; }

	std::vector<std::string>& getSourceDragMultiplePaths() { return mSrcDragMultiplePaths; }

	std::string getSelectionPathAtIndex( int index ) const;

	void deleteSelectedFiles();

	void openSelectedFiles();

	void execute( const std::string& cmd );

	std::vector<FileInfo> getSelectionsFileInfo() const;

  protected:
	KeyBindings mKeyBindings;
	UnorderedMap<std::string, std::function<void()>> mCommands;
	std::vector<std::string> mSrcDragMultiplePaths;
	std::vector<std::string> mSrcCopyMultiplePaths;
	bool mWasCut{ false };

	friend class UITreeViewCellFS;

	std::string getSelectionPath() const;

	void moveFiles( const std::vector<std::string>& paths, const std::string& dstDir );

	void copyFile( const std::string& src, const std::string& dst );

	void copyFiles( const std::vector<std::string>& paths, const std::string& dstDir );

	void deleteItems( const std::vector<std::string>& paths );
};

} // namespace ecode
