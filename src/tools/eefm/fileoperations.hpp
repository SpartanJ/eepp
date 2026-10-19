#ifndef EEFM_FILEOPERATIONS_HPP
#define EEFM_FILEOPERATIONS_HPP

#include "pane.hpp"

#include <eepp/vfs/transfer.hpp>
#include <eepp/vfs/virtualfilesystem.hpp>
#include <eepp/window/draganddrop.hpp>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace eefm {

struct Application;

class App;

/** @brief What eefm does with files: opening, the context menu, the clipboard, transfers, the
 * trash, renaming, creating and drops. */
class FileOperations {
  public:
	explicit FileOperations( App& app );

	~FileOperations();

	/** Opens a folder row (navigates there) or the selected files. */
	void openRow( FileTab* tab, size_t row );

	void showContextMenu( FileTab* tab, bool background );

	void copySelection( bool cut );

	void paste( FileTab* tab, std::optional<URI> into = std::nullopt );

	void moveToTrash();

	void deleteSelection();

	void emptyTrash();

	/** Renames the selected item in place (an editor over its name). */
	void renameSelection();

	/** A name typed in place: renames the entry. */
	void renameEntry( FileTab* tab, const EE::VFS::VFSFileInfo& entry, const std::string& name );

	void newFolder( FileTab* tab );

	void showProperties( FileTab* tab );

	void openTerminalHere( FileTab* tab );

	/** Moves or copies (as the user picks) the dropped files to destination; with copy, they are
	 * copied without asking. Files already there are skipped. */
	void dropFiles( const std::vector<URI>& uris, const URI& destination, int paneIndex,
					bool copy = false );

	/** A file dropped from another application (one event per file), where it was dropped when
	 * the backend reports it. */
	void addDroppedFile( URI uri, std::optional<Vector2f> position );

	bool hasDroppedFiles() const { return !mDroppedFiles.empty(); }

	/** Copies the files dropped from other applications into the pane (or the folder row)
	 * where they were dropped. */
	void dropExternalFiles();

	/** Entries dragged out of Ark: Ark extracts them into the pane (or the folder row) where
	 * they were dropped. */
	void dropArchiveEntries( EE::Window::DataDrop drop );

	/** The local folder of the pane (or the folder row) under position, for a drop whose source
	 * writes the files itself (an archive manager extracting entries, a browser saving an
	 * image). A remote folder is refused, with an error. */
	std::optional<std::string> localDropFolder( Vector2f position );

	/** Cancels the transfers shown in pane paneIndex. */
	void cancelTransfers( int paneIndex );

	/** Cancels every transfer and forgets them (shutdown). */
	void cancelTransfers();

  private:
	enum class TransferKind : Uint8 { Copy, Move, Extract };

	struct ActiveTransfer {
		Uint64 id;
		int pane;
		String label;
		EE::VFS::Transfer transfer;
		EE::VFS::TransferProgressSubscription subscription;
		EE::VFS::TransferProgress progress;
	};

	/** Where a drop lands: a folder row, else the folder of the pane under the pointer. */
	struct DropDestination {
		URI folder;
		int pane;
	};

	struct ClipboardFiles {
		std::vector<URI> uris;
		bool cut{ false };
	};

	/** The destination of a drop at position (the active pane's folder when no pane is there),
	 * which makes that pane the active one. */
	std::optional<DropDestination> dropDestination( Vector2f position );

	/** Runs the local programs among uris and opens the other files. */
	void openFiles( const std::vector<URI>& uris );

	/** Starts the program at path in its folder, after asking when confirm. */
	void runProgram( const std::string& path, bool confirm );

	/** Opens the files with their default applications. */
	void openDocuments( const std::vector<URI>& uris );

	void openWith( const std::vector<URI>& uris );

	/** Opens files with the chosen application, including archives. */
	void openWith( const std::vector<URI>& uris, Application app );

	/** Files in the clipboard: copied here or in another file manager (text/uri-list, with the
	 * GNOME / KDE cut markers). */
	ClipboardFiles clipboardFiles() const;

	void restoreFromTrash( const std::vector<URI>& uris );

	void newTextFile( FileTab* tab );

	bool validateName( const std::string& name, const URI& directory );

	void transferFiles( const std::vector<URI>& sources, const URI& destination, bool move,
						int paneIndex );

	void extractArchive( const URI& file, const URI& destination, int paneIndex );

	void startTransfer( const URI& source, const URI& destination, TransferKind kind, int paneIndex,
						const EE::VFS::CopyOptions& options = {} );

	void updateTransfers();

	void reportError( const String& action, const EE::VFS::VFSError& error );

	/** Runs work (blocking, it returns an error message) on the pool, then refresh on the UI
	 * thread; a failure is reported under errorTitle. */
	void runInBackground( const std::string& path, std::function<std::string()> work,
						  std::function<void()> refresh, String errorTitle );

	App& mApp;
	UISceneNode* mUI;
	EE::VFS::VirtualFileSystem* mVFS;
	std::map<Uint64, ActiveTransfer> mTransfers;
	Uint64 mLastTransferId{ 0 };
	/** Fallback for backends without clipboard MIME data: the files copied here, while the
	 * clipboard text is still mClipboardText. */
	std::vector<URI> mClipboard;
	std::string mClipboardText;
	bool mClipboardCut{ false };
	/** Files dropped from other applications in the current frame, and where. */
	std::vector<URI> mDroppedFiles;
	std::optional<Vector2f> mDropPosition;
};

} // namespace eefm

#endif
