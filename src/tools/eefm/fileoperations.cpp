#include "fileoperations.hpp"
#include "app.hpp"
#include "apps.hpp"
#include "location.hpp"
#include "mime.hpp"
#include "panemanager.hpp"
#include "placescontroller.hpp"
#include "trash.hpp"
#include <eepp/system/sha1.hpp>

namespace eefm {

FileOperations::FileOperations( App& app ) : mApp( app ), mUI( app.ui() ), mVFS( app.vfs() ) {}

FileOperations::~FileOperations() = default;

void FileOperations::cancelTransfers() {
	for ( auto& transfer : mTransfers )
		transfer.second.transfer.cancel();
	mTransfers.clear();
}

void FileOperations::cancelTransfers( int paneIndex ) {
	for ( auto& transfer : mTransfers ) {
		if ( transfer.second.pane == paneIndex )
			transfer.second.transfer.cancel();
	}
}

void FileOperations::addDroppedFile( URI uri, std::optional<Vector2f> position ) {
	mDroppedFiles.push_back( std::move( uri ) );
	if ( position )
		mDropPosition = position;
}

void FileOperations::reportError( const String& action, const VFSError& error ) {
	if ( error.is( VFSErrorCode::Cancelled ) )
		return;
	Dialogs::showError( mUI, action + ":\n" + String::fromUtf8( error.toString() ) );
}

void FileOperations::runInBackground( std::function<std::string()> work,
									  std::function<void()> refresh, String errorTitle ) {
	mApp.pool()
		->submit( std::move( work ) )
		.then( mApp.executor(), [this, refresh = std::move( refresh ),
								 errorTitle = std::move( errorTitle )]( std::string error ) {
			if ( !error.empty() )
				Dialogs::showError( mUI, errorTitle + ":\n" + String::fromUtf8( error ) );
			if ( refresh )
				refresh();
		} );
}

// ---------------------------------------------------------------------------------------------
// Opening
// ---------------------------------------------------------------------------------------------

void FileOperations::openRow( FileTab* tab, size_t row ) {
	if ( row >= tab->model->rowCount() )
		return;
	if ( tab->model->entry( row ).isDirectory() ) {
		mApp.panes().navigate( tab, tab->model->entryURI( row ) );
		return;
	}
	// Enter / double click on a multiple selection opens all the selected files.
	auto rows = tab->selectedRows();
	if ( std::find( rows.begin(), rows.end(), row ) == rows.end() )
		rows = { row };
	std::vector<URI> files;
	for ( size_t selected : rows ) {
		if ( !tab->model->entry( selected ).isDirectory() )
			files.push_back( tab->model->entryURI( selected ) );
	}
	openFiles( files );
}

namespace {

/** Where remote files are downloaded to be opened: private to the user (the shared /tmp is
 * not, on Linux and BSD). */
std::string remoteCacheDirectory() {
	std::string cache = Sys::getCachePath( "eefm" );
	FileSystem::dirAddSlashAtEnd( cache );
	cache += "remote/";
	return cache;
}

/** Calls done( localPaths ) with local copies of uris (remote files are downloaded to a cache
 * folder first). */
void withLocalFiles( VirtualFileSystem* vfs, Executor& executor, UISceneNode* ui,
					 const std::vector<URI>& uris,
					 std::function<void( std::vector<std::string> )> done ) {
	auto paths = std::make_shared<std::vector<std::string>>( uris.size() );
	auto remaining = std::make_shared<size_t>( 0 );
	auto finish = std::make_shared<std::function<void( std::vector<std::string> )>>( done );
	for ( size_t i = 0; i < uris.size(); ++i ) {
		if ( uris[i].getScheme() == "file" ) {
			( *paths )[i] = uris[i].getFSPath();
			continue;
		}
		++*remaining;
		// One private folder per remote file (its complete URI: user, port and path included),
		// so files with the same name never overwrite each other; the file keeps its name for
		// the application that opens it.
		std::string cache =
			remoteCacheDirectory() + SHA1::fromString( uris[i].toString() ).toHexString() + "/";
		FileSystem::makeDir( cache, true, 0700 );
		std::string local = cache + uris[i].getLastPathSegment();
		CopyOptions options;
		options.overwrite = OverwritePolicy::Overwrite;
		vfs->copy( uris[i], URI::fromFSPath( local ), options )
			.completion()
			.then( executor, [paths, remaining, finish, local, i, ui,
							  name = uris[i].getLastPathSegment()](
								 VFSResult<TransferResult>&& result ) {
				if ( result ) {
					( *paths )[i] = local;
				} else if ( !result.error().is( VFSErrorCode::Cancelled ) ) {
					Dialogs::showError( ui, String::fromUtf8( "Could not download " + name + ":\n" +
															  result.error().toString() ) );
				}
				if ( --*remaining == 0 ) {
					std::vector<std::string> ready;
					for ( auto& path : *paths ) {
						if ( !path.empty() )
							ready.push_back( path );
					}
					if ( !ready.empty() )
						( *finish )( std::move( ready ) );
				}
			} );
	}
	if ( *remaining == 0 )
		done( std::move( *paths ) );
}

} // namespace

void FileOperations::openFiles( const std::vector<URI>& uris ) {
	if ( uris.empty() )
		return;
	withLocalFiles( mVFS, mApp.executor(), mUI, uris, [this]( std::vector<std::string> paths ) {
		for ( const auto& path : paths ) {
			const std::string& mime =
				MimeDatabase::instance().typeForName( FileSystem::fileNameFromPath( path ) );
			auto app = Applications::instance().defaultFor( mime );
			if ( app && Applications::launch( *app, { path } ) )
				continue;
			if ( !Engine::instance()->openURI( URI::fromFSPath( path ).toString() ) )
				Dialogs::showError( mUI, String::fromUtf8( "No application can open " + path ) );
		}
	} );
}

void FileOperations::openWith( const std::vector<URI>& uris ) {
	Dialogs::promptText(
		mUI, mUI->i18n( "eefm_open_with", "Open With" ),
		mUI->i18n( "eefm_open_with_command", "Command to open the files with:" ), "", false,
		[this, uris]( std::optional<std::string> command ) {
			if ( !command || String::trim( *command ).empty() )
				return;
			withLocalFiles( mVFS, mApp.executor(), mUI, uris,
							[this, command = *command]( std::vector<std::string> paths ) {
								if ( !Applications::launchCommand( command, paths ) ) {
									Dialogs::showError( mUI, String::fromUtf8( "Could not run \"" +
																			   command + "\"" ) );
								}
							} );
		} );
}

void FileOperations::openTerminalHere( FileTab* tab ) {
	URI uri = tab->model->uri();
	auto rows = tab->selectedRows();
	if ( rows.size() == 1 && tab->model->entry( rows[0] ).isDirectory() )
		uri = tab->model->entryURI( rows[0] );
	if ( uri.getScheme() != "file" ) {
		Dialogs::showError( mUI, "A terminal can only be opened in local folders." );
		return;
	}
	if ( !Applications::openTerminal( uri.getFSPath() ) )
		Dialogs::showError( mUI, "No terminal emulator was found (set $TERMINAL)." );
}

// ---------------------------------------------------------------------------------------------
// Context menus
// ---------------------------------------------------------------------------------------------

void FileOperations::showContextMenu( FileTab* tab, bool background ) {
	UIPopUpMenu* menu = UIPopUpMenu::New();
	auto rows = background ? std::vector<size_t>{} : tab->selectedRows();
	auto uris = background ? std::vector<URI>{} : tab->selectedURIs();
	bool inTrash = Trash::contains( tab->model->uri() );
	bool allFolders = !rows.empty(), anyFolder = false;
	for ( size_t row : rows ) {
		bool dir = tab->model->entry( row ).isDirectory();
		allFolders = allFolders && dir;
		anyFolder = anyFolder || dir;
	}
	// Create New is offered on items too (as Dolphin): with a full, scrolled list there may be
	// no empty space to right click. It creates in the current folder.
	auto addCreateNew = [this, menu, tab] {
		UIPopUpMenu* create = UIPopUpMenu::New();
		create->add( mUI->i18n( "eefm_new_folder", "Folder..." ), mApp.icon( "folder-add" ), "F10" )
			->setId( "new-folder" );
		create->add( mUI->i18n( "eefm_new_text_file", "Text File..." ), mApp.icon( "file-add" ) )
			->setId( "new-file" );
		create->on( Event::OnItemClicked, [this, tab]( const Event* event ) {
			std::string id( event->getNode()->getId() );
			if ( id == "new-folder" )
				newFolder( tab );
			else if ( id == "new-file" )
				newTextFile( tab );
		} );
		menu->addSubMenu( mUI->i18n( "eefm_create_new", "Create New" ), mApp.icon( "file-add" ),
						  create );
		menu->addSeparator();
	};
	std::vector<Application> apps;
	if ( !rows.empty() ) {
		if ( !anyFolder ) {
			const std::string& mime =
				MimeDatabase::instance().typeOf( tab->model->entry( rows[0] ) );
			apps = Applications::instance().forMime( mime );
			String openText =
				apps.empty() ? mUI->i18n( "eefm_open", "Open" )
							 : String( String::format( "Open with %s", apps[0].name.c_str() ) );
			menu->add( openText, mApp.icon( "action-open-with" ), "Return" )->setId( "open" );
			UIPopUpMenu* with = UIPopUpMenu::New();
			for ( size_t i = 0; i < apps.size(); ++i )
				with->add( String::fromUtf8( apps[i].name ) )->setId( "app" )->setData( i );
			if ( !apps.empty() )
				with->addSeparator();
			with->add( mUI->i18n( "eefm_other_application", "Other Application..." ) )
				->setId( "other-app" );
			menu->addSubMenu( mUI->i18n( "eefm_open_with", "Open With" ), nullptr, with );
			with->on( Event::OnItemClicked, [this, uris, apps]( const Event* event ) {
				std::string id( event->getNode()->getId() );
				if ( id == "app" ) {
					size_t index = static_cast<size_t>( event->getNode()->getData() );
					if ( index < apps.size() ) {
						withLocalFiles( mVFS, mApp.executor(), mUI, uris,
										[app = apps[index]]( std::vector<std::string> paths ) {
											Applications::launch( app, paths );
										} );
					}
				} else if ( id == "other-app" ) {
					openWith( uris );
				}
			} );
		} else if ( allFolders ) {
			menu->add( mUI->i18n( "eefm_open", "Open" ), mApp.icon( "mime-folder" ), "Return" )
				->setId( "open" );
			menu->add( mUI->i18n( "eefm_open_new_tab", "Open in New Tab" ),
					   mApp.icon( "action-new-tab" ) )
				->setId( "open-tab" );
			if ( mApp.isSplit() ) {
				menu->add( mUI->i18n( "eefm_open_other_pane", "Open in Other Pane" ),
						   mApp.icon( "view-split" ) )
					->setId( "open-other" );
			}
		}
		// After the open actions, as Dolphin.
		if ( menu->getCount() > 0 )
			menu->addSeparator();
		addCreateNew();
		menu->add( mUI->i18n( "eefm_cut", "Cut" ), mApp.icon( "cut" ), "Ctrl+X" )->setId( "cut" );
		menu->add( mUI->i18n( "eefm_copy", "Copy" ), mApp.icon( "copy" ), "Ctrl+C" )
			->setId( "copy" );
		menu->add( mUI->i18n( "eefm_copy_location", "Copy Location" ),
				   mApp.icon( "action-copy-location" ) )
			->setId( "copy-location" );
		if ( rows.size() == 1 && allFolders && !clipboardFiles().uris.empty() ) {
			menu->add( mUI->i18n( "eefm_paste_into", "Paste into Folder" ), mApp.icon( "paste" ) )
				->setId( "paste-into" );
		}
		menu->addSeparator();
		if ( rows.size() == 1 ) {
			menu->add( mUI->i18n( "eefm_rename", "Rename..." ), mApp.icon( "edit" ), "F2" )
				->setId( "rename" );
		}
		if ( inTrash ) {
			menu->add( mUI->i18n( "eefm_restore", "Restore" ), mApp.icon( "action-restore" ) )
				->setId( "restore" );
		} else if ( Trash::supported( uris[0] ) ) {
			menu->add( mUI->i18n( "eefm_move_to_trash", "Move to Trash" ),
					   mApp.icon( "place-trash" ), "Del" )
				->setId( "trash" );
		}
		menu->add( mUI->i18n( "eefm_delete", "Delete" ), mApp.icon( "delete-bin" ), "Shift+Del" )
			->setId( "delete" );
		menu->addSeparator();
		if ( rows.size() == 1 && allFolders ) {
			menu->add( mUI->i18n( "eefm_add_to_places", "Add to Places" ),
					   mApp.icon( "action-add-place" ) )
				->setId( "add-place" );
		}
	} else {
		addCreateNew();
		if ( !clipboardFiles().uris.empty() ) {
			menu->add( mUI->i18n( "eefm_paste", "Paste" ), mApp.icon( "paste" ), "Ctrl+V" )
				->setId( "paste" );
		}
		menu->add( mUI->i18n( "eefm_copy_location", "Copy Location" ),
				   mApp.icon( "action-copy-location" ) )
			->setId( "copy-current-location" );
		menu->add( mUI->i18n( "eefm_add_to_places", "Add to Places" ),
				   mApp.icon( "action-add-place" ) )
			->setId( "add-current-place" );
		menu->addSeparator();
		menu->addCheckBox( mUI->i18n( "eefm_show_hidden", "Show Hidden Files" ), mApp.showHidden(),
						   "Ctrl+H" )
			->setId( "hidden" );
		if ( inTrash ) {
			menu->add( mUI->i18n( "eefm_empty_trash", "Empty Trash" ), mApp.icon( "place-trash" ) )
				->setId( "empty-trash" );
		}
		menu->add( mUI->i18n( "eefm_refresh", "Reload" ), mApp.icon( "view-refresh" ), "F5" )
			->setId( "refresh" );
	}
	if ( tab->model->uri().getScheme() == "file" ) {
		menu->add( mUI->i18n( "eefm_open_terminal", "Open Terminal Here" ),
				   mApp.icon( "action-terminal" ), "Shift+F4" )
			->setId( "terminal" );
	}
	menu->add( mUI->i18n( "eefm_properties", "Properties" ), mApp.icon( "action-properties" ),
			   "Alt+Return" )
		->setId( "properties" );

	menu->on( Event::OnItemClicked, [this, tab, uris, rows]( const Event* event ) {
		std::string id( event->getNode()->getId() );
		if ( id == "open" ) {
			if ( !rows.empty() )
				openRow( tab, rows[0] );
		} else if ( id == "open-tab" ) {
			for ( const auto& uri : uris )
				mApp.panes().openTab( *tab->pane, uri, uris.size() == 1 );
		} else if ( id == "open-other" ) {
			if ( auto* other =
					 mApp.panes().currentTab( mApp.panes().pane( 1 - tab->pane->index ) ) )
				mApp.panes().navigate( other, uris[0] );
		} else if ( id == "cut" || id == "copy" ) {
			copySelection( id == "cut" );
		} else if ( id == "copy-location" ) {
			std::string text;
			for ( const auto& uri : uris )
				text += ( text.empty() ? "" : "\n" ) + Location::clipboardText( uri );
			mApp.window()->getClipboard()->setText( text );
		} else if ( id == "copy-current-location" ) {
			mApp.window()->getClipboard()->setText( Location::clipboardText( tab->model->uri() ) );
		} else if ( id == "paste" ) {
			paste( tab );
		} else if ( id == "paste-into" ) {
			paste( tab, uris[0] );
		} else if ( id == "rename" ) {
			renameSelection();
		} else if ( id == "trash" ) {
			moveToTrash();
		} else if ( id == "restore" ) {
			restoreFromTrash( uris );
		} else if ( id == "delete" ) {
			deleteSelection();
		} else if ( id == "add-place" ) {
			mApp.places().add( uris[0] );
		} else if ( id == "add-current-place" ) {
			mApp.places().add( tab->model->uri() );
		} else if ( id == "hidden" ) {
			mApp.setShowHidden( !mApp.showHidden() );
		} else if ( id == "empty-trash" ) {
			emptyTrash();
		} else if ( id == "refresh" ) {
			tab->model->refresh();
		} else if ( id == "terminal" ) {
			openTerminalHere( tab );
		} else if ( id == "properties" ) {
			showProperties( tab );
		}
	} );
	menu->showOverMouseCursor();
}

// ---------------------------------------------------------------------------------------------
// Clipboard and transfers
// ---------------------------------------------------------------------------------------------

void FileOperations::copySelection( bool cut ) {
	auto* tab = mApp.panes().currentTab();
	if ( !tab )
		return;
	auto uris = tab->selectedURIs();
	if ( uris.empty() )
		return;
	// The formats file managers exchange: text/uri-list (with CRLF), GNOME's copied-files list
	// with the operation first, and KDE's cut marker. Text readers get the URIs, one per line.
	std::string uriList, text;
	for ( const auto& uri : uris ) {
		uriList += uri.toString() + "\r\n";
		text += ( text.empty() ? "" : "\n" ) + uri.toString();
	}
	std::vector<std::pair<std::string, std::string>> data{
		{ "text/uri-list", uriList },
		{ "x-special/gnome-copied-files", ( cut ? "cut\n" : "copy\n" ) + text },
	};
	if ( cut )
		data.emplace_back( "application/x-kde-cutselection", "1" );
	for ( const auto& mimeType : EE::Window::Clipboard::getTextMimeTypes() )
		data.emplace_back( mimeType, text );
	auto* clipboard = mApp.window()->getClipboard();
	mClipboard = std::move( uris );
	mClipboardCut = cut;
	mClipboardText = text;
	if ( !clipboard->setMimeData( std::move( data ) ) )
		clipboard->setText( text );
}

FileOperations::ClipboardFiles FileOperations::clipboardFiles() const {
	auto* clipboard = mApp.window()->getClipboard();
	ClipboardFiles files;
	if ( clipboard->hasData( "text/uri-list" ) ) {
		for ( auto line : String::split( clipboard->getDataString( "text/uri-list" ), '\n' ) ) {
			// Lines end in CRLF (RFC 2483).
			String::trimInPlace( line, '\r' );
			String::trimInPlace( line );
			if ( line.empty() || line[0] == '#' )
				continue;
			URI uri( line );
			if ( !uri.getScheme().empty() )
				files.uris.push_back( std::move( uri ) );
		}
		std::string gnome = clipboard->hasData( "x-special/gnome-copied-files" )
								? clipboard->getDataString( "x-special/gnome-copied-files" )
								: std::string();
		files.cut = String::startsWith( gnome, "cut\n" ) ||
					( clipboard->hasData( "application/x-kde-cutselection" ) &&
					  String::startsWith(
						  clipboard->getDataString( "application/x-kde-cutselection" ), "1" ) );
		return files;
	}
	// Without MIME data (other backends): what was copied here, unless the text was replaced.
	if ( !mClipboard.empty() && clipboard->getText() == mClipboardText ) {
		files.uris = mClipboard;
		files.cut = mClipboardCut;
	}
	return files;
}

void FileOperations::paste( FileTab* tab, std::optional<URI> into ) {
	ClipboardFiles files = clipboardFiles();
	if ( files.uris.empty() )
		return;
	transferFiles( files.uris, into ? *into : tab->model->uri(), files.cut, tab->pane->index );
	if ( files.cut ) {
		// Cut files can be pasted once.
		mClipboard.clear();
		mApp.window()->getClipboard()->clearData();
	}
}

void FileOperations::dropFiles( const std::vector<URI>& uris, const URI& destination, int paneIndex,
								bool copy ) {
	std::vector<URI> sources;
	for ( const auto& uri : uris ) {
		// Dropping into the folder an item already is in, or a folder into itself, does nothing.
		if ( uri.getParent() == destination || destination.isSameOrChildOf( uri ) )
			continue;
		sources.push_back( uri );
	}
	if ( sources.empty() )
		return;
	if ( copy ) {
		transferFiles( sources, destination, false, paneIndex );
		return;
	}
	auto* input = mApp.window()->getInput();
	if ( input->isShiftPressed() ) {
		transferFiles( sources, destination, true, paneIndex );
		return;
	}
	if ( input->isControlPressed() ) {
		transferFiles( sources, destination, false, paneIndex );
		return;
	}
	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->add( mUI->i18n( "eefm_move_here", "Move Here" ), mApp.icon( "cut" ), "Shift" )
		->setId( "move" );
	menu->add( mUI->i18n( "eefm_copy_here", "Copy Here" ), mApp.icon( "copy" ), "Ctrl" )
		->setId( "copy" );
	menu->addSeparator();
	menu->add( mUI->i18n( "eefm_cancel", "Cancel" ), mApp.icon( "action-close" ), "Esc" )
		->setId( "cancel" );
	menu->on( Event::OnItemClicked, [this, sources, destination, paneIndex]( const Event* event ) {
		std::string id( event->getNode()->getId() );
		if ( id == "move" || id == "copy" )
			transferFiles( sources, destination, id == "move", paneIndex );
	} );
	menu->showOverMouseCursor();
}

void FileOperations::transferFiles( const std::vector<URI>& sources, const URI& destination,
									bool move, int paneIndex ) {
	for ( const auto& source : sources ) {
		std::string name = source.getLastPathSegment();
		URI target = destination.withAppendedSegment( name );
		if ( target == source ) {
			if ( move )
				continue; // Moving onto itself is a no-op.
			auto [base, extension] = FileSystem::fileNameSplitExtension( name );
			target = destination.withAppendedSegment( base + " (copy)" + extension );
		}
		startTransfer( source, target, move, paneIndex );
	}
}

void FileOperations::startTransfer( const URI& source, const URI& destination, bool move,
									int paneIndex, const CopyOptions& options ) {
	Uint64 id = ++mLastTransferId;
	ActiveTransfer active{
		id,
		paneIndex,
		( move ? mUI->i18n( "eefm_moving", "Moving " ) : mUI->i18n( "eefm_copying", "Copying " ) ) +
			Location::displayName( source ),
		move ? mVFS->move( source, destination, options )
			 : mVFS->copy( source, destination, options ),
		{},
		{} };
	active.subscription = active.transfer.onProgress(
		[this, id]( const TransferProgress& progress ) {
			auto it = mTransfers.find( id );
			if ( it != mTransfers.end() ) {
				it->second.progress = progress;
				updateTransfers();
			}
		},
		&mApp.executor() );
	auto completion = active.transfer.completion();
	mTransfers.emplace( id, std::move( active ) );
	updateTransfers();
	std::move( completion )
		.then( mApp.executor(), [this, id, source, destination, move, paneIndex,
								 options]( VFSResult<TransferResult>&& result ) {
			auto it = mTransfers.find( id );
			String label = it != mTransfers.end() ? it->second.label : String();
			if ( it != mTransfers.end() )
				mTransfers.erase( it );
			updateTransfers();
			if ( result ) {
				// Views showing the folder that received the files recount it (a watch on the
				// parent does not see changes inside a subfolder).
				URI folder = destination.getParent();
				for ( auto& pane : mApp.panes().all() ) {
					for ( auto& tab : pane->tabList ) {
						if ( tab->model->uri() == folder.getParent() &&
							 folder != tab->model->uri() )
							tab->model->refresh();
					}
				}
				return;
			}
			if ( result.error().is( VFSErrorCode::AlreadyExists ) &&
				 options.overwrite == OverwritePolicy::Fail ) {
				Dialogs::confirm(
					mUI, mUI->i18n( "eefm_file_exists", "File Already Exists" ),
					String::format( "\"%s\" already exists. Overwrite it?",
									destination.getLastPathSegment().c_str() ),
					[this, source, destination, move, paneIndex, options]( bool overwrite ) {
						if ( !overwrite )
							return;
						CopyOptions retry = options;
						retry.overwrite = OverwritePolicy::Overwrite;
						startTransfer( source, destination, move, paneIndex, retry );
					} );
				return;
			}
			reportError( label, result.error() );
		} );
}

void FileOperations::updateTransfers() {
	for ( auto& pane : mApp.panes().all() ) {
		size_t count = 0;
		Uint64 done = 0, total = 0;
		String label;
		for ( const auto& entry : mTransfers ) {
			if ( entry.second.pane != pane->index )
				continue;
			++count;
			label = entry.second.label;
			done += entry.second.progress.bytesDone;
			total += entry.second.progress.bytesTotal.value_or( 0 );
		}
		pane->transferBox->setVisible( count > 0 );
		if ( count == 0 )
			continue;
		if ( count > 1 )
			label = String::format( "%zu operations", count );
		pane->transferText->setText( label );
		pane->transferBar->setProgress(
			total > 0 ? static_cast<Float>( done ) * 100.f / static_cast<Float>( total ) : 0.f );
	}
}

// ---------------------------------------------------------------------------------------------
// Trash, delete and rename
// ---------------------------------------------------------------------------------------------

void FileOperations::moveToTrash() {
	auto* tab = mApp.panes().currentTab();
	if ( !tab )
		return;
	auto uris = tab->selectedURIs();
	if ( uris.empty() )
		return;
	if ( Trash::contains( tab->model->uri() ) || !Trash::supported( uris[0] ) ) {
		// Remote files and files already in the trash can only be deleted.
		deleteSelection();
		return;
	}
	String message =
		uris.size() == 1
			? String::format( "Do you really want to move \"%s\" to the trash?",
							  uris[0].getLastPathSegment().c_str() )
			: String::format( "Do you really want to move these %zu items to the trash?",
							  uris.size() );
	Dialogs::confirm( mUI, mUI->i18n( "eefm_move_to_trash", "Move to Trash" ), message,
					  [this, uris]( bool accepted ) {
						  if ( !accepted )
							  return;
						  std::vector<std::string> paths;
						  for ( const auto& uri : uris )
							  paths.push_back( uri.getFSPath() );
						  runInBackground(
							  [paths] { return Trash::moveToTrash( paths ); }, nullptr,
							  mUI->i18n( "eefm_trash_failed", "Could not move to the trash" ) );
					  } );
}

void FileOperations::deleteSelection() {
	auto* tab = mApp.panes().currentTab();
	if ( !tab )
		return;
	auto selection = tab->selectedURIs();
	if ( selection.empty() )
		return;
	String message =
		selection.size() == 1
			? String::format( "Do you really want to permanently delete \"%s\"?\nThis action "
							  "cannot be undone.",
							  selection[0].getLastPathSegment().c_str() )
			: String::format( "Do you really want to permanently delete these %zu "
							  "items?\nThis action cannot be undone.",
							  selection.size() );
	Dialogs::confirm(
		mUI, mUI->i18n( "eefm_delete", "Delete" ), message, [this, selection]( bool accepted ) {
			if ( !accepted )
				return;
			for ( const auto& uri : selection ) {
				bool trashed = Trash::contains( uri );
				mVFS->remove( uri, RemoveOptions{ true } )
					.then( mApp.executor(), [this, uri, trashed]( VFSResult<void>&& result ) {
						if ( !result ) {
							reportError( mUI->i18n( "eefm_delete_failed", "Could not delete " ) +
											 String::fromUtf8( uri.getLastPathSegment() ),
										 result.error() );
							return;
						}
						// Deleting from the trash also drops what the trash recorded about it.
						if ( trashed && uri.getScheme() == "file" )
							mApp.pool()->run( [path = uri.getFSPath()] { Trash::forget( path ); } );
					} );
			}
		} );
}

void FileOperations::restoreFromTrash( const std::vector<URI>& uris ) {
	std::vector<std::string> paths;
	for ( const auto& uri : uris )
		paths.push_back( uri.getFSPath() );
	runInBackground( [paths] { return Trash::restore( paths ); }, nullptr,
					 mUI->i18n( "eefm_restore_failed", "Could not restore" ) );
}

void FileOperations::emptyTrash() {
	Dialogs::confirm(
		mUI, mUI->i18n( "eefm_empty_trash", "Empty Trash" ),
		mUI->i18n( "eefm_confirm_empty_trash",
				   "Do you really want to empty the trash? All items will be deleted." ),
		[this]( bool accepted ) {
			if ( accepted ) {
				runInBackground(
					[] { return Trash::empty(); }, nullptr,
					mUI->i18n( "eefm_empty_trash_failed", "Could not empty the trash" ) );
			}
		} );
}

void FileOperations::renameSelection() {
	auto* tab = mApp.panes().currentTab();
	if ( !tab )
		return;
	auto rows = tab->selectedRows();
	if ( rows.size() == 1 )
		tab->view->beginRename( rows[0] );
}

void FileOperations::renameEntry( FileTab* tab, const FileInfo& entry, const std::string& name ) {
	if ( name.find( '/' ) != std::string::npos ) {
		Dialogs::showError( mUI, mUI->i18n( "eefm_invalid_name", "A name cannot contain \"/\"." ) );
		return;
	}
	URI source = entry.uriIn( tab->model->uri() );
	tab->selectLater( name );
	mVFS->rename( source, source.getParent().withAppendedSegment( name ) )
		.then( mApp.executor(), [this]( VFSResult<void>&& result ) {
			if ( !result ) {
				reportError( mUI->i18n( "eefm_rename_failed", "Could not rename" ),
							 result.error() );
			}
		} );
}

void FileOperations::newFolder( FileTab* tab ) {
	URI parent = tab->model->uri();
	Dialogs::promptText(
		mUI, mUI->i18n( "eefm_new_folder_title", "New Folder" ),
		mUI->i18n( "eefm_folder_name", "Create new folder in:\n" ) + Location::text( parent ),
		"New Folder", false, [this, parent, tab]( std::optional<std::string> name ) {
			if ( !name || name->empty() || name->find( '/' ) != std::string::npos )
				return;
			tab->selectLater( *name );
			mVFS->makeDir( parent.withAppendedSegment( *name ) )
				.then( mApp.executor(), [this]( VFSResult<void>&& result ) {
					if ( !result ) {
						reportError(
							mUI->i18n( "eefm_new_folder_failed", "Could not create folder" ),
							result.error() );
					}
				} );
		} );
}

void FileOperations::newTextFile( FileTab* tab ) {
	URI parent = tab->model->uri();
	Dialogs::promptText(
		mUI, mUI->i18n( "eefm_new_text_file_title", "New Text File" ),
		mUI->i18n( "eefm_file_name", "File name:" ), "Text File.txt", false,
		[this, parent, tab]( std::optional<std::string> name ) {
			if ( !name || name->empty() || name->find( '/' ) != std::string::npos )
				return;
			tab->selectLater( *name );
			mVFS->open( parent.withAppendedSegment( *name ),
						OpenFlags::Write | OpenFlags::Create | OpenFlags::Exclusive )
				.then( mApp.executor(), [this]( VFSResult<FileHandle>&& handle ) {
					if ( !handle ) {
						reportError( mUI->i18n( "eefm_new_file_failed", "Could not create file" ),
									 handle.error() );
						return;
					}
					handle.value().close();
				} );
		} );
}

// ---------------------------------------------------------------------------------------------
// Properties
// ---------------------------------------------------------------------------------------------

void FileOperations::showProperties( FileTab* tab ) {
	auto rows = tab->selectedRows();
	std::string text;
	auto line = [&text]( const char* key, const std::string& value ) {
		if ( !value.empty() )
			text += std::string( key ) + ": " + value + "\n";
	};
	auto time = []( std::optional<Int64> seconds ) {
		return seconds ? Sys::epochToString( static_cast<Uint64>( *seconds ) ) : std::string();
	};
	if ( rows.size() == 1 ) {
		const FileInfo& info = tab->model->entry( rows[0] );
		line( "Name", info.name );
		line( "Type", MimeDatabase::instance().describe( info ) + " (" +
						  MimeDatabase::instance().typeOf( info ) + ")" );
		line( "Location", Location::text( tab->model->uri() ).toUtf8() );
		if ( info.isDirectory() ) {
			if ( auto count = tab->model->directoryItemCount( rows[0] ) )
				line( "Contents", std::to_string( *count ) + ( *count == 1 ? " item" : " items" ) );
		} else if ( info.size() ) {
			line( "Size", FileSystem::sizeToString( static_cast<Int64>( *info.size() ) ) + " (" +
							  std::to_string( *info.size() ) + " bytes)" );
		}
		if ( info.isSymlink() )
			line( "Points to", info.symlinkTarget );
		line( "Modified", time( info.modifiedTime() ) );
		line( "Accessed", time( info.accessedTime() ) );
		line( "Created", time( info.createdTime() ) );
		if ( info.permissions() )
			line( "Permissions", VFSDirectoryModel::permissionsToString( info ) );
		line( "Owner", info.owner );
		line( "Group", info.group );
		if ( Trash::contains( tab->model->uri() ) && tab->model->uri().getScheme() == "file" ) {
			if ( auto original =
					 Trash::originalPath( tab->model->entryURI( rows[0] ).getFSPath() ) )
				line( "Original location", *original );
		}
	} else if ( rows.size() > 1 ) {
		Uint64 bytes = 0, folders = 0;
		for ( size_t row : rows ) {
			const FileInfo& info = tab->model->entry( row );
			if ( info.isDirectory() )
				++folders;
			else
				bytes += info.size().value_or( 0 );
		}
		line( "Selected", std::to_string( rows.size() ) + " items (" + std::to_string( folders ) +
							  " folders)" );
		line( "Size of files", FileSystem::sizeToString( static_cast<Int64>( bytes ) ) );
		line( "Location", Location::text( tab->model->uri() ).toUtf8() );
	} else {
		const URI& uri = tab->model->uri();
		line( "Folder", Location::displayName( uri ).toUtf8() );
		line( "Location", Location::clipboardText( uri ) );
		line( "Contents", std::to_string( tab->model->rowCount() ) + " visible items" );
	}
	UIMessageBox* box = UIMessageBox::New( UIMessageBox::OK, String::fromUtf8( text ) );
	box->setTitle( mUI->i18n( "eefm_properties", "Properties" ) );
	box->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	box->showWhenReady();
}

void FileOperations::dropExternalFiles() {
	std::vector<URI> files = std::move( mDroppedFiles );
	mDroppedFiles.clear();
	// Where it was dropped (the backend may not report it: then the pointer).
	Vector2f position = mDropPosition ? *mDropPosition : mUI->getEventDispatcher()->getMousePosf();
	mDropPosition.reset();
	Node* over = mUI->overFind( position );
	int target = mApp.panes().activeIndex();
	for ( int i = 0; i < 2; ++i ) {
		Pane& p = mApp.panes().pane( i );
		if ( over && p.root->getParent() == mApp.paneSplitter() &&
			 ( p.root == over || p.root->inParentTreeOf( over ) ) )
			target = i;
	}
	auto* tab = mApp.panes().currentTab( mApp.panes().pane( target ) );
	if ( !tab )
		return;
	// Onto a folder: into it (as drops between panes); anywhere else: the pane's folder.
	URI destination = tab->model->uri();
	if ( auto row = tab->view->rowOf( over );
		 row && *row < tab->model->rowCount() && tab->model->entry( *row ).isDirectory() )
		destination = tab->model->entryURI( *row );
	mApp.panes().setActivePane( target );
	// eefm's own drag dropped on eefm arrives here on Wayland, macOS and Windows (the system drag
	// started with it): it is resolved as the internal drag it is. Other files are copied.
	const auto& drag = FileTableView::currentDrag();
	dropFiles( files, destination, target, !( drag.native && files == drag.uris ) );
}

} // namespace eefm
