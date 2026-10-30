#include "fileoperations.hpp"
#include "app.hpp"
#include "arkdrop.hpp"
#include "desktopintegration.hpp"
#include "location.hpp"
#include "mime.hpp"
#include "openwithdialog.hpp"
#include "panemanager.hpp"
#include "placescontroller.hpp"
#include "propertiesdialog.hpp"
#include "trash.hpp"
#include <atomic>
#include <eepp/system/applications.hpp>
#include <eepp/system/sha1.hpp>
#include <eepp/vfs/path.hpp>

namespace eefm {

using namespace EE::VFS;

FileOperations::FileOperations( App& app ) : mApp( app ), mUI( app.ui() ), mVFS( app.vfs() ) {}

FileOperations::~FileOperations() = default;

void FileOperations::cancelOperations() {
	cancelContextMenu();
	for ( auto& transfer : mTransfers ) {
		transfer.second.batch->cancelled = true;
		transfer.second.transfer.cancel();
	}
	mTransfers.clear();
	for ( auto& weak : mProperties ) {
		if ( auto dialog = weak.lock() )
			dialog->cancel();
	}
	mProperties.clear();
}

void FileOperations::cancelTransfers( int paneIndex ) {
	for ( auto& transfer : mTransfers ) {
		if ( transfer.second.pane == paneIndex ) {
			transfer.second.batch->cancelled = true;
			transfer.second.transfer.cancel();
		}
	}
}

void FileOperations::addDroppedFile( URI uri, std::optional<Vector2f> position ) {
	mDroppedFiles.push_back( std::move( uri ) );
	// Input has processed this frame's motion; the scene dispatcher updates afterward.
	mDropPosition =
		position
			? *position
			: mApp.window()->getInput()->getMousePosFromView( mApp.window()->getDefaultView() );
}

void FileOperations::reportError( const String& action, const VFSError& error ) {
	if ( error.is( VFSErrorCode::Cancelled ) )
		return;
	Dialogs::showError( mUI,
						String::format( mUI->i18n( "eefm_operation_error", "%s:\n%s" ).toUtf8(),
										action.toUtf8(), error.toString() ) );
}

void FileOperations::runInBackground( const std::string& path, std::function<std::string()> work,
									  std::function<void()> refresh, String errorTitle ) {
	// Where the VFS runs the calls on path: on a network mount that stopped answering, the work
	// fails after the timeout instead of holding one of eefm's threads.
	mVFS->localIO()
		.submitResolved<std::string>(
			path, false,
			[work = std::move( work )]() -> VFSResult<std::string> { return Ok( work() ); } )
		.then( mApp.executor(), [this, refresh = std::move( refresh ),
								 errorTitle =
									 std::move( errorTitle )]( VFSResult<std::string> result ) {
			const std::string error =
				result ? std::move( result ).value() : std::move( result ).error().message;
			if ( !error.empty() )
				Dialogs::showError(
					mUI, String::format( mUI->i18n( "eefm_operation_error", "%s:\n%s" ).toUtf8(),
										 errorTitle.toUtf8(), error ) );
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
	const URI entry = tab->model->entryURI( row );
	if ( URI archive = Location::archiveRoot( entry ); !archive.empty() ) {
		mApp.panes().navigate( tab, archive );
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

void downloadLocalCopies( VirtualFileSystem* vfs, Executor& executor, UISceneNode* ui,
						  const std::vector<URI>& uris,
						  std::function<void( std::vector<std::string> )> done ) {
	// All completions run on the UI executor. The extra count keeps synchronous failures from
	// finishing the group before every input has been scheduled.
	struct Pending {
		std::vector<std::string> paths;
		std::function<void( std::vector<std::string> )> done;
		size_t remaining;

		void complete() {
			if ( --remaining != 0 )
				return;
			paths.erase( std::remove_if( paths.begin(), paths.end(),
										 []( const std::string& path ) { return path.empty(); } ),
						 paths.end() );
			if ( !paths.empty() )
				done( std::move( paths ) );
		}
	};
	auto pending = std::make_shared<Pending>(
		Pending{ std::vector<std::string>( uris.size() ), std::move( done ), uris.size() + 1 } );
	for ( size_t i = 0; i < uris.size(); ++i ) {
		if ( uris[i].getScheme() == "file" ) {
			pending->paths[i] = uris[i].getFSPath();
			pending->complete();
			continue;
		}
		// Each open gets a fresh private folder. Fail on any collision: never overwrite a
		// copy an external editor may have changed, including copies from earlier processes.
		static std::atomic<Uint64> serial{ 0 };
		std::string cache =
			remoteCacheDirectory() + SHA1::fromString( uris[i].toString() ).toHexString() + "/" +
			String::toString( Sys::getProcessID() ) + "-" + String::toString( Sys::getTicks() ) +
			"-" + String::toString( ++serial ) + "/";
		std::string local = cache + uris[i].getLastPathSegment();
		CopyOptions options;
		options.overwrite = OverwritePolicy::Fail;
		options.preservePermissions = false;
		// Cache directories may themselves live on a mount: create them through the VFS so
		// neither directory creation nor downloading blocks the UI thread.
		vfs->makeDirs( URI::fromFSPath( cache ), MkDirOptions{ 0700 } )
			.then( executor,
				   [vfs, source = uris[i], local,
					options]( VFSResult<void>&& made ) -> VFSTask<TransferResult> {
					   if ( !made )
						   return readyError<TransferResult>( std::move( made ).error() );
					   return vfs->copy( source, URI::fromFSPath( local ), options ).completion();
				   } )
			.then( executor, [pending, local = std::move( local ), i,
							  ui]( VFSResult<TransferResult>&& result ) mutable {
				if ( result && result.value().partial() ) {
					Dialogs::showMessage(
						ui->i18n( "eefm_local_copy_failed", "Could Not Open Local Copy" ),
						ui->i18n( "eefm_local_copy_incomplete",
								  "The file could not be copied completely. Its type may be "
								  "unsupported." ) );
				} else if ( result ) {
					pending->paths[i] = std::move( local );
				} else if ( !result.error().is( VFSErrorCode::Cancelled ) ) {
					Dialogs::showError( ui, String::format( ui->i18n( "eefm_download_failed",
																	  "Could not download %s:\n%s" )
																.toUtf8(),
															FileSystem::fileNameFromPath( local ),
															result.error().toString() ) );
				}
				pending->complete();
			} );
	}
	pending->complete();
}

/** Calls done( localPaths ) with local copies of uris (remote files are downloaded to a cache
 * folder first). */
void withLocalFiles( VirtualFileSystem* vfs, Executor& executor, UISceneNode* ui,
					 const std::vector<URI>& uris,
					 std::function<void( std::vector<std::string> )> done ) {
	if ( std::any_of( uris.begin(), uris.end(),
					  []( const URI& uri ) { return uri.getScheme() != "file"; } ) ) {
		Dialogs::confirm(
			ui, ui->i18n( "eefm_open_local_copy", "Open Local Copy" ),
			String::format( ui->i18n( "eefm_local_copy_question",
									  "These files will open as local copies. Changes are not "
									  "uploaded to the original location. Each open creates a new "
									  "copy; previous copies remain in %s. Continue?" )
								.toUtf8(),
							remoteCacheDirectory() ),
			[vfs, &executor, ui, uris, done = std::move( done )]( bool accepted ) mutable {
				if ( !accepted )
					return;
				// Download below without asking a second time.
				downloadLocalCopies( vfs, executor, ui, uris, std::move( done ) );
			} );
		return;
	}
	downloadLocalCopies( vfs, executor, ui, uris, std::move( done ) );
}

} // namespace

void FileOperations::openFiles( const std::vector<URI>& uris ) {
	if ( uris.empty() )
		return;
	// Local programs run instead (as in other file managers). Only local ones: a downloaded
	// copy of a remote file is opened.
	auto local = std::find_if( uris.begin(), uris.end(),
							   []( const URI& uri ) { return uri.getScheme() == "file"; } );
	if ( local == uris.end() ) {
		openDocuments( uris );
		return;
	}
	// Telling them apart reads the files, each where the VFS reads it (files in one folder can
	// link into different mounts): a file whose mount stopped answering opens as a document.
	using Kind = DesktopIntegration::ExecutableKind;
	// Shared by the checks, and joined on the UI thread: no lock needed.
	struct Pending {
		std::vector<URI> uris;
		std::vector<Kind> kinds;
		size_t remaining;
	};
	auto pending = std::make_shared<Pending>(
		Pending{ uris, std::vector<Kind>( uris.size(), Kind::None ), uris.size() } );
	auto checked = [this, pending] {
		if ( --pending->remaining != 0 )
			return;
		std::vector<URI> documents;
		for ( size_t i = 0; i < pending->uris.size(); ++i ) {
			if ( pending->kinds[i] == Kind::None )
				documents.push_back( pending->uris[i] );
			else
				runProgram( pending->uris[i].getFSPath(), pending->kinds[i] == Kind::Script );
		}
		openDocuments( documents );
	};
	for ( size_t i = 0; i < uris.size(); ++i ) {
		if ( uris[i].getScheme() != "file" ) {
			checked();
			continue;
		}
		std::string path = uris[i].getFSPath();
		mVFS->localIO()
			.submitResolved<Kind>( path, true,
								   [path]() -> VFSResult<Kind> {
									   return Ok( DesktopIntegration::executableKind( path ) );
								   } )
			.then( mApp.executor(), [pending, checked, i]( VFSResult<Kind> kind ) {
				if ( kind )
					pending->kinds[i] = kind.value();
				checked();
			} );
	}
}

void FileOperations::runProgram( const std::string& path, bool confirm ) {
	auto run = [this, path] {
		if ( !Applications::spawn( { path }, FileSystem::fileRemoveFileName( path ) ) )
			Dialogs::showError(
				mUI, String::format( mUI->i18n( "eefm_run_failed", "Could not run %s" ).toUtf8(),
									 path ) );
	};
	if ( !confirm ) {
		run();
		return;
	}
	// A script can be anything downloaded with the execute permission: ask first.
	Dialogs::confirm( mUI, mUI->i18n( "eefm_run_script", "Run Script" ),
					  String::format( mUI->i18n( "eefm_run_script_question",
												 "\"%s\" is an executable script. Run it?" )
										  .toUtf8()
										  .c_str(),
									  FileSystem::fileNameFromPath( path ).c_str() ),
					  [run]( bool accepted ) {
						  if ( accepted )
							  run();
					  } );
}

void FileOperations::openDocuments( const std::vector<URI>& uris ) {
	if ( uris.empty() )
		return;
	withLocalFiles( mVFS, mApp.executor(), mUI, uris, [this]( std::vector<std::string> paths ) {
		for ( const auto& path : paths ) {
#if EE_PLATFORM != EE_PLATFORM_MACOS && EE_PLATFORM != EE_PLATFORM_WIN
			const std::string& mime =
				MimeDatabase::instance().typeForName( FileSystem::fileNameFromPath( path ) );
			auto app = Applications::defaultFor( URI::fromFSPath( path ),
												 MimeDatabase::instance().withParents( mime ) );
			if ( app && Applications::launch( *app, { path } ) )
				continue;
#endif
			// The native opener resolves the system default, including per-file overrides.
			if ( !Engine::instance()->openURI( URI::fromFSPath( path ).toString() ) )
				Dialogs::showError(
					mUI,
					String::format(
						mUI->i18n( "eefm_no_application", "No application can open %s" ).toUtf8(),
						path ) );
		}
	} );
}

void FileOperations::openWithCommand( const std::vector<URI>& uris ) {
	Dialogs::promptText(
		mUI, mUI->i18n( "eefm_open_with", "Open With" ),
		mUI->i18n( "eefm_open_with_command", "Command to open the files with:" ), "", false,
		[this, uris]( std::optional<std::string> command ) {
			if ( !command || String::trim( *command ).empty() )
				return;
			withLocalFiles( mVFS, mApp.executor(), mUI, uris,
							[this, command = *command]( std::vector<std::string> paths ) {
								if ( !Applications::launchCommand( command, paths ) ) {
									Dialogs::showError(
										mUI, String::format( mUI->i18n( "eefm_run_command_failed",
																		"Could not run \"%s\"" )
																 .toUtf8(),
															 command ) );
								}
							} );
		} );
}

void FileOperations::openWith( const std::vector<URI>& uris, Application app ) {
	withLocalFiles(
		mVFS, mApp.executor(), mUI, uris,
		[this, app = std::move( app )]( std::vector<std::string> paths ) {
			mApp.pool()
				->submit( [app = std::move( app ), paths = std::move( paths )]() mutable {
					return Applications::launch( app, paths ) ? std::string{}
															  : std::move( app.name );
				} )
				.then( mApp.executor(), [this]( std::string&& error ) {
					if ( !error.empty() )
						Dialogs::showError(
							mUI, String::format( mUI->i18n( "eefm_open_application_failed",
															"Could not open the files with %s" )
													 .toUtf8(),
												 error ) );
				} );
		} );
}

void FileOperations::openTerminalHere( FileTab* tab ) {
	URI uri = tab->model->uri();
	auto rows = tab->selectedRows();
	if ( rows.size() == 1 && tab->model->entry( rows[0] ).isDirectory() )
		uri = tab->model->entryURI( rows[0] );
	if ( uri.getScheme() != "file" ) {
		Dialogs::showError( mUI, mUI->i18n( "eefm_terminal_local_only",
											"A terminal can only be opened in local folders." ) );
		return;
	}
	if ( !DesktopIntegration::openTerminal( uri.getFSPath() ) )
		Dialogs::showError( mUI, mUI->i18n( "eefm_terminal_not_found",
											"No terminal emulator was found (set $TERMINAL)." ) );
}

// ---------------------------------------------------------------------------------------------
// Context menus
// ---------------------------------------------------------------------------------------------

void FileOperations::showContextMenu( FileTab* tab, bool background ) {
	const auto generation = ++mContextMenuGeneration;
	const auto position = mApp.window()->getInput()->getMousePos().asFloat();
	const auto rows = tab->selectedRows();
	if ( background || rows.empty() || std::any_of( rows.begin(), rows.end(), [tab]( size_t row ) {
			 return tab->model->entry( row ).isDirectory();
		 } ) ) {
		buildContextMenu( tab, background, position, {} );
		return;
	}
	auto uris = tab->selectedURIs();
	std::weak_ptr<VFSDirectoryModel> model = tab->model;
	// Launch Services (and an XDG database's first load) can touch disk. Keep the queries
	// off the UI thread and own their inputs until the worker and UI continuation finish.
	auto discover = [uri = uris.front(),
					 mime = std::string( MimeDatabase::instance().typeOf( tab->model->entry(
						 rows[0] ) ) )]() -> VFSResult<std::vector<Application>> {
		return Ok( Applications::forFile( uri, MimeDatabase::instance().withParents( mime ) ) );
	};
	// A native file may live on a disconnected network mount. Its metadata lookup uses
	// the same isolated, time-limited lane as other local VFS reads.
	auto task = uris.front().getScheme() == "file"
					? mVFS->localIO().submitResolved<std::vector<Application>>(
						  uris.front().getFSPath(), true, std::move( discover ) )
					: mApp.pool()->submit( std::move( discover ) );
	std::move( task ).then( mApp.executor(), [this, tab, model, generation, position,
											  uris = std::move( uris )](
												 VFSResult<std::vector<Application>>&& result ) {
		// A tab may close or navigate while discovery runs. The weak model is checked
		// before dereferencing the borrowed tab, and a changed selection invalidates it.
		auto owner = model.lock();
		if ( generation != mContextMenuGeneration || !owner || mApp.panes().currentTab() != tab ||
			 tab->model != owner || !tab->view->hasFocusWithin() || tab->selectedURIs() != uris )
			return;
		buildContextMenu( tab, false, position,
						  result ? std::move( result ).value() : std::vector<Application>{} );
	} );
}

void FileOperations::buildContextMenu( FileTab* tab, bool background, Vector2f position,
									   std::vector<Application> apps ) {
	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->setCloseOnHide( true );
	// Enter is handled natively by UITableView rather than a scene command.
	const auto openShortcut = mUI->getKeyBindings().getShortcutString( { KEY_RETURN }, true );
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
		create
			->add( mUI->i18n( "eefm_new_folder", "Folder..." ), mApp.menuIcon( "folder-add" ),
				   mApp.getKeybind( "new-folder" ) )
			->setId( "new-folder" );
		create
			->add( mUI->i18n( "eefm_new_text_file", "Text File..." ), mApp.menuIcon( "file-add" ) )
			->setId( "new-file" );
		create->on( Event::OnItemClicked, [this, tab]( const Event* event ) {
			std::string id( event->getNode()->getId() );
			if ( id == "new-folder" )
				newFolder( tab );
			else if ( id == "new-file" )
				newTextFile( tab );
		} );
		menu->addSubMenu( mUI->i18n( "eefm_create_new", "Create New" ), mApp.menuIcon( "file-add" ),
						  create );
		menu->addSeparator();
	};
	std::optional<Application> defaultApp;
	const bool archive = !anyFolder && !uris.empty() && !Location::archiveRoot( uris[0] ).empty();
	if ( !rows.empty() ) {
		if ( !anyFolder ) {
			if ( archive ) {
				menu->add( mUI->i18n( "eefm_open_in_eefm", "Open in eefm" ),
						   mApp.menuIcon( "eefm" ), openShortcut )
					->setId( "open-in-eefm" );
			}
			if ( !archive || !apps.empty() ) {
				String openText =
					apps.empty()
						? mUI->i18n( "eefm_open", "Open" )
						: String( String::format(
							  mUI->i18n( "eefm_open_with_application", "Open with %s" ).toUtf8(),
							  apps[0].name.c_str() ) );
				auto* item = menu->add( openText, mApp.menuIcon( "action-open-with" ),
										archive ? "" : openShortcut );
				item->setId( apps.empty() ? "open" : "open-app" );
				if ( !apps.empty() )
					mApp.setApplicationIcon( item, apps.front() );
			}
			// Keep the application named by the action, independently of archive navigation.
			if ( !apps.empty() )
				defaultApp = apps.front();
			UIPopUpMenu* with = UIPopUpMenu::New();
			for ( size_t i = 0; i < apps.size(); ++i ) {
				auto* item = with->add( String::fromUtf8( apps[i].name ) );
				item->setId( "app" )->setData( i );
				mApp.setApplicationIcon( item, apps[i] );
			}
			if ( !apps.empty() )
				with->addSeparator();
			with->add( mUI->i18n( "eefm_choose_application", "Choose Application..." ) )
				->setId( "other-app" );
			menu->addSubMenu( mUI->i18n( "eefm_open_with", "Open With" ), nullptr, with )
				->setId( "open-with" );
			with->on( Event::OnItemClicked, [this, uris, apps = std::move( apps )](
												const Event* event ) mutable {
				std::string id( event->getNode()->getId() );
				if ( id == "app" ) {
					size_t index = static_cast<size_t>( event->getNode()->getData() );
					if ( index < apps.size() )
						openWith( uris, apps[index] );
				} else if ( id == "other-app" ) {
					OpenWithDialog::show(
						mApp, std::move( uris ), std::move( apps ),
						[this]( const std::vector<URI>& files, Application application ) {
							openWith( files, std::move( application ) );
						},
						[this]( const std::vector<URI>& files ) { openWithCommand( files ); } );
				}
			} );
		} else if ( allFolders ) {
			menu->add( mUI->i18n( "eefm_open", "Open" ), mApp.menuIcon( "mime-folder" ),
					   openShortcut )
				->setId( "open" );
			menu->add( mUI->i18n( "eefm_open_new_tab", "Open in New Tab" ),
					   mApp.menuIcon( "action-new-tab" ) )
				->setId( "open-tab" );
			if ( mApp.isSplit() ) {
				menu->add( mUI->i18n( "eefm_open_other_pane", "Open in Other Pane" ),
						   mApp.menuIcon( "view-split" ) )
					->setId( "open-other" );
			}
		}
		if ( !inTrash && uris.size() == 1 && archive ) {
			const URI file = uris[0];
			const std::string folderName = Location::archiveFolderName( file );
			const URI folder = file.getParent().withAppendedSegment( folderName );
			const int paneIndex = tab->pane->index;
			UIPopUpMenu* extract = UIPopUpMenu::New();
			extract
				->add( String::format(
					mUI->i18n( "eefm_extract_to_folder", "Extract to \"%s/\"" ).toUtf8(),
					folderName.c_str() ) )
				->setId( "extract-folder" );
			extract->add( mUI->i18n( "eefm_extract_here", "Extract Here" ) )
				->setId( "extract-here" );
			extract->add( mUI->i18n( "eefm_extract_to", "Extract To..." ) )->setId( "extract-to" );
			extract->on(
				Event::OnItemClicked, [this, file, folder, paneIndex]( const Event* event ) {
					const std::string& id = event->getNode()->getId();
					if ( id == "extract-here" ) {
						extractArchive( file, file.getParent(), paneIndex );
					} else if ( id == "extract-folder" ) {
						extractArchive( file, folder, paneIndex );
					} else if ( id == "extract-to" ) {
						Dialogs::promptText(
							mUI, mUI->i18n( "eefm_extract_to", "Extract To..." ),
							mUI->i18n( "eefm_extract_destination",
									   "Destination folder (path or URI):" ),
							Location::text( folder ).toUtf8(), false,
							[this, file, paneIndex]( std::optional<std::string> destination ) {
								if ( !destination || String::trim( *destination ).empty() )
									return;
								extractArchive( file, Location::parse( *destination ), paneIndex );
							} );
					}
				} );
			menu->addSubMenu( mUI->i18n( "eefm_extract", "Extract" ),
							  mApp.menuIcon( "mime-archive" ), extract );
		}
		// After the open actions, as Dolphin.
		if ( menu->getCount() > 0 )
			menu->addSeparator();
		addCreateNew();
		menu->add( mUI->i18n( "eefm_cut", "Cut" ), mApp.menuIcon( "cut" ),
				   mApp.getKeybind( "cut" ) )
			->setId( "cut" );
		menu->add( mUI->i18n( "eefm_copy", "Copy" ), mApp.menuIcon( "copy" ),
				   mApp.getKeybind( "copy" ) )
			->setId( "copy" );
		menu->add( mUI->i18n( "eefm_copy_location", "Copy Location" ),
				   mApp.menuIcon( "action-copy-location" ) )
			->setId( "copy-location" );
		if ( rows.size() == 1 && allFolders && !clipboardFiles().uris.empty() ) {
			menu->add( mUI->i18n( "eefm_paste_into", "Paste into Folder" ),
					   mApp.menuIcon( "paste" ) )
				->setId( "paste-into" );
		}
		menu->addSeparator();
		if ( rows.size() == 1 ) {
			menu->add( mUI->i18n( "eefm_rename", "Rename..." ), mApp.menuIcon( "edit" ),
					   mApp.getKeybind( "rename" ) )
				->setId( "rename" );
		}
		if ( inTrash ) {
			menu->add( mUI->i18n( "eefm_restore", "Restore" ), mApp.menuIcon( "action-restore" ) )
				->setId( "restore" );
		} else if ( Trash::supported( uris[0] ) ) {
			menu->add( mUI->i18n( "eefm_move_to_trash", "Move to Trash" ),
					   mApp.menuIcon( "place-trash" ), mApp.getKeybind( "trash" ) )
				->setId( "trash" );
		}
		menu->add( mUI->i18n( "eefm_delete", "Delete" ), mApp.menuIcon( "delete-bin" ),
				   mApp.getKeybind( "delete" ) )
			->setId( "delete" );
		menu->addSeparator();
		if ( rows.size() == 1 && allFolders ) {
			menu->add( mUI->i18n( "eefm_add_to_places", "Add to Places" ),
					   mApp.menuIcon( "action-add-place" ) )
				->setId( "add-place" );
		}
	} else {
		addCreateNew();
		if ( !clipboardFiles().uris.empty() ) {
			menu->add( mUI->i18n( "eefm_paste", "Paste" ), mApp.menuIcon( "paste" ),
					   mApp.getKeybind( "paste" ) )
				->setId( "paste" );
		}
		menu->add( mUI->i18n( "eefm_copy_location", "Copy Location" ),
				   mApp.menuIcon( "action-copy-location" ) )
			->setId( "copy-current-location" );
		menu->add( mUI->i18n( "eefm_add_to_places", "Add to Places" ),
				   mApp.menuIcon( "action-add-place" ) )
			->setId( "add-current-place" );
		menu->addSeparator();
		menu->addCheckBox( mUI->i18n( "eefm_show_hidden", "Show Hidden Files" ), mApp.showHidden(),
						   mApp.getKeybind( "toggle-hidden" ) )
			->setId( "hidden" );
		if ( inTrash ) {
			menu->add( mUI->i18n( "eefm_empty_trash", "Empty Trash" ),
					   mApp.menuIcon( "place-trash" ) )
				->setId( "empty-trash" );
		}
		menu->add( mUI->i18n( "eefm_refresh", "Reload" ), mApp.menuIcon( "view-refresh" ),
				   mApp.getKeybind( "refresh" ) )
			->setId( "refresh" );
	}
	if ( tab->model->uri().getScheme() == "file" ) {
		menu->add( mUI->i18n( "eefm_open_terminal_here", "Open Terminal Here" ),
				   mApp.menuIcon( "action-terminal" ), mApp.getKeybind( "terminal" ) )
			->setId( "terminal" );
	}
	menu->add( mUI->i18n( "eefm_properties", "Properties" ), mApp.menuIcon( "action-properties" ),
			   mApp.getKeybind( "properties" ) )
		->setId( "properties" );

	menu->on( Event::OnItemClicked, [this, tab, uris, rows,
									 defaultApp = std::move( defaultApp )]( const Event* event ) {
		std::string id( event->getNode()->getId() );
		if ( id == "open" || id == "open-in-eefm" ) {
			if ( !rows.empty() )
				openRow( tab, rows[0] );
		} else if ( id == "open-app" ) {
			if ( defaultApp )
				openWith( uris, *defaultApp );
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
	menu->showAtScreenPosition( position );
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
	menu->add( mUI->i18n( "eefm_move_here", "Move Here" ), mApp.menuIcon( "cut" ), "Shift" )
		->setId( "move" );
	menu->add( mUI->i18n( "eefm_copy_here", "Copy Here" ), mApp.menuIcon( "copy" ), "Ctrl" )
		->setId( "copy" );
	menu->addSeparator();
	menu->add( mUI->i18n( "eefm_cancel", "Cancel" ), mApp.menuIcon( "action-close" ), "Esc" )
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
	auto batch = std::make_shared<TransferBatch>();
	batch->entries.reserve( sources.size() );
	batch->pane = paneIndex;
	batch->kind = move ? TransferKind::Move : TransferKind::Copy;
	for ( const auto& source : sources ) {
		std::string name = source.getLastPathSegment();
		if ( !validateName( name, destination ) )
			continue;
		URI target = destination.withAppendedSegment( name );
		if ( target == source ) {
			if ( move )
				continue; // Moving onto itself is a no-op.
			auto [base, extension] = FileSystem::fileNameSplitExtension( name );
			target = destination.withAppendedSegment( base + " (copy)" + extension );
		}
		batch->entries.emplace_back( source, std::move( target ) );
	}
	startNextTransfer( batch );
}

void FileOperations::extractArchive( const URI& file, const URI& destination, int paneIndex ) {
	URI root = Location::archiveRoot( file );
	if ( root.empty() )
		return;
	auto batch = std::make_shared<TransferBatch>();
	batch->entries.emplace_back( std::move( root ), destination );
	batch->pane = paneIndex;
	batch->kind = TransferKind::Extract;
	startNextTransfer( batch );
}

void FileOperations::startNextTransfer( const std::shared_ptr<TransferBatch>& batch ) {
	if ( batch->cancelled || batch->next == batch->entries.size() )
		return;
	const auto& entry = batch->entries[batch->next++];
	startTransfer( entry.first, entry.second, batch );
}

void FileOperations::startTransfer( const URI& source, const URI& destination,
									const std::shared_ptr<TransferBatch>& batch ) {
	const auto kind = batch->kind;
	const int paneIndex = batch->pane;
	CopyOptions options;
	if ( kind == TransferKind::Extract )
		options.overwrite = OverwritePolicy::MergeDirectories;
	batch->conflictsSkipped = 0;
	auto onConflict = [this, batch, executor = &mApp.executor()]( TransferConflict conflict,
																  CancellationToken token ) {
		TaskResolver<VFSResult<ConflictResolution>> resolve;
		auto task = VFSTask<ConflictResolution>::create( resolve );
		// Own the request while it crosses from the transfer worker to the UI thread.
		executor->execute( [this, batch, conflict = std::move( conflict ), token,
							resolve = std::move( resolve )]() mutable {
			if ( batch->cancelled || token.isCancellationRequested() ) {
				resolve( Err( cancelledError( conflict.source ) ) );
				return;
			}
			const bool folder = conflict.sourceInfo.isDirectory();
			const auto srcTime = conflict.sourceInfo.modifiedTime();
			const auto dstTime = conflict.destinationInfo.modifiedTime();
			const bool sourceNewer = !folder && !conflict.destinationInfo.isDirectory() &&
									 srcTime && dstTime && *srcTime > *dstTime;
			auto answer = [batch, sourceNewer,
						   resolve = std::move( resolve )]( ConflictResolution decision ) mutable {
				if ( decision.action == ConflictAction::Skip ||
					 ( decision.action == ConflictAction::OverwriteOlder && !sourceNewer ) )
					++batch->conflictsSkipped;
				resolve( Ok( std::move( decision ) ) );
			};
			auto remembered = folder ? batch->folders : batch->files;
			// A remembered overwrite never authorizes replacing a different entry type.
			if ( remembered &&
				 ( *remembered == ConflictAction::Skip || *remembered == ConflictAction::Rename ||
				   conflict.sourceInfo.isDirectory() == conflict.destinationInfo.isDirectory() ) ) {
				answer( ConflictResolution{ {}, *remembered } );
				return;
			}
			batch->dialog = Dialogs::transferConflict(
				mUI, conflict,
				[batch, folder, answer = std::move( answer )]( ConflictResolution decision,
															   bool applyAll ) mutable {
					if ( decision.action == ConflictAction::Cancel )
						batch->cancelled = true;
					else if ( applyAll )
						( folder ? batch->folders : batch->files ) = decision.action;
					answer( std::move( decision ) );
				} );
			batch->dialog->on( Event::OnWindowClose,
							   [batch, dialog = batch->dialog]( const Event* ) {
								   if ( batch->dialog == dialog )
									   batch->dialog = nullptr;
							   } );
		} );
		return task;
	};
	const bool move = kind == TransferKind::Move;
	const auto labelFormat = kind == TransferKind::Extract
								 ? mUI->i18n( "eefm_extracting", "Extracting %s" )
							 : move ? mUI->i18n( "eefm_moving", "Moving %s" )
									: mUI->i18n( "eefm_copying", "Copying %s" );
	String label = String::format( labelFormat.toUtf8(), Location::displayName( source ).toUtf8() );
	Uint64 id = ++mLastTransferId;
	ActiveTransfer active{
		id,
		paneIndex,
		std::move( label ),
		move ? mVFS->move( source, destination, options, std::move( onConflict ) )
			 : mVFS->copy( source, destination, options, std::move( onConflict ) ),
		{},
		{},
		batch };
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
		.then( mApp.executor(), [this, id, destination, kind,
								 batch]( VFSResult<TransferResult>&& result ) {
			auto it = mTransfers.find( id );
			String label = it != mTransfers.end() ? it->second.label : String();
			if ( it != mTransfers.end() )
				mTransfers.erase( it );
			updateTransfers();
			if ( result ) {
				if ( result.value().filesSkipped > batch->conflictsSkipped ) {
					Dialogs::showMessage(
						mUI->i18n( "eefm_partial_transfer", "Partial Transfer" ),
						String::format(
							mUI->i18n( "eefm_transfer_skipped_entries",
									   "%s:\n%llu entries were skipped (existing destinations, "
									   "symbolic links, or unsupported file types). Skipped source "
									   "entries were kept." )
								.toUtf8(),
							label.toUtf8(),
							static_cast<unsigned long long>( result.value().filesSkipped -
															 batch->conflictsSkipped ) ) );
				}
				// Views showing the folder that received the files recount it (a watch on the
				// parent does not see changes inside a subfolder).
				URI folder = destination.getParent();
				for ( auto& pane : mApp.panes().all() ) {
					for ( auto& tab : pane->tabList ) {
						if ( ( tab->model->uri() == folder.getParent() &&
							   folder != tab->model->uri() ) ||
							 ( kind == TransferKind::Extract && tab->model->uri() == destination ) )
							tab->model->refresh();
					}
				}
				startNextTransfer( batch );
				return;
			}
			if ( result.error().is( VFSErrorCode::Cancelled ) ) {
				batch->cancelled = true;
				if ( batch->dialog && !batch->dialog->isClosing() )
					batch->dialog->closeWindow();
				return;
			}
			reportError( label, result.error() );
			batch->cancelled = true;
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
			label = String::format( mUI->i18n( "eefm_operations_count", "%zu operations" ).toUtf8(),
									count );
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
			? String::format( mUI->i18n( "eefm_confirm_trash_one",
										 "Do you really want to move \"%s\" to the trash?" )
								  .toUtf8(),
							  uris[0].getLastPathSegment().c_str() )
			: String::format(
				  mUI->i18n( "eefm_confirm_trash_many",
							 "Do you really want to move these %zu items to the trash?" )
					  .toUtf8(),
				  uris.size() );
	Dialogs::confirm( mUI, mUI->i18n( "eefm_move_to_trash", "Move to Trash" ), message,
					  [this, uris]( bool accepted ) {
						  if ( !accepted )
							  return;
						  std::vector<std::string> paths;
						  for ( const auto& uri : uris )
							  paths.push_back( uri.getFSPath() );
						  const auto routingPath = paths.front();
						  runInBackground(
							  routingPath,
							  [paths = std::move( paths ), translator = &mUI->getTranslator()] {
								  return Trash::moveToTrash( paths, *translator );
							  },
							  nullptr,
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
			? String::format( mUI->i18n( "eefm_confirm_delete_one",
										 "Do you really want to permanently delete "
										 "\"%s\"?\nThis action cannot be undone." )
								  .toUtf8(),
							  selection[0].getLastPathSegment().c_str() )
			: String::format( mUI->i18n( "eefm_confirm_delete_many",
										 "Do you really want to permanently delete these %zu "
										 "items?\nThis action cannot be undone." )
								  .toUtf8(),
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
							reportError( String::format( mUI->i18n( "eefm_delete_failed",
																	"Could not delete %s" )
															 .toUtf8(),
														 uri.getLastPathSegment() ),
										 result.error() );
							return;
						}
						// Deleting from the trash also drops what the trash recorded about it.
						if ( trashed && uri.getScheme() == "file" )
							mVFS->localIO().submitResolved<void>(
								uri.getFSPath(), false,
								[path = uri.getFSPath()]() -> VFSResult<void> {
									Trash::forget( path );
									return Ok();
								} );
					} );
			}
		} );
}

void FileOperations::restoreFromTrash( const std::vector<URI>& uris ) {
	std::vector<std::string> paths;
	for ( const auto& uri : uris )
		paths.push_back( uri.getFSPath() );
	if ( paths.empty() )
		return;
	const auto routingPath = paths.front();
	runInBackground(
		routingPath,
		[paths = std::move( paths ), translator = &mUI->getTranslator()] {
			return Trash::restore( paths, *translator );
		},
		nullptr, mUI->i18n( "eefm_restore_failed", "Could not restore" ) );
}

void FileOperations::emptyTrash() {
	Dialogs::confirm(
		mUI, mUI->i18n( "eefm_empty_trash", "Empty Trash" ),
		mUI->i18n( "eefm_confirm_empty_trash",
				   "Do you really want to empty the trash? All items will be deleted." ),
		[this]( bool accepted ) {
			if ( accepted ) {
				const auto trash = Trash::location();
				runInBackground(
					trash ? trash->getFSPath() : std::string(),
					[translator = &mUI->getTranslator()] { return Trash::empty( *translator ); },
					nullptr, mUI->i18n( "eefm_empty_trash_failed", "Could not empty the trash" ) );
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

void FileOperations::renameEntry( FileTab* tab, const VFSFileInfo& entry,
								  const std::string& name ) {
	if ( !validateName( name, tab->model->uri() ) )
		return;
	URI source = entry.uriIn( tab->model->uri() );
	URI destination = source.getParent().withAppendedSegment( name );
	tab->selectLater( name );
	mVFS->rename( source, destination )
		.then( mApp.executor(),
			   [this, source = std::move( source ),
				destination = std::move( destination )]( VFSResult<void>&& result ) {
				   if ( !result ) {
					   reportError( mUI->i18n( "eefm_rename_failed", "Could not rename" ),
									result.error() );
				   } else {
					   mApp.panes().onEntryRenamed( source, destination );
				   }
			   } );
}

void FileOperations::newFolder( FileTab* tab ) {
	URI parent = tab->model->uri();
	Dialogs::promptText(
		mUI, mUI->i18n( "eefm_new_folder_title", "New Folder" ),
		String::format( mUI->i18n( "eefm_folder_name", "Create new folder in:\n%s" ).toUtf8(),
						Location::text( parent ).toUtf8() ),
		mUI->i18n( "eefm_new_folder_default", "New Folder" ).toUtf8(), false,
		[this, parent, tab]( std::optional<std::string> name ) {
			if ( !name || !validateName( *name, parent ) )
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
		mUI->i18n( "eefm_file_name", "File name:" ),
		mUI->i18n( "eefm_new_text_file_default", "Text File.txt" ).toUtf8(), false,
		[this, parent, tab]( std::optional<std::string> name ) {
			if ( !name || !validateName( *name, parent ) )
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

bool FileOperations::validateName( const std::string& name, const URI& directory ) {
	if ( VFSPath::validName( name, directory ) )
		return true;
	Dialogs::showError( mUI,
						mUI->i18n( "eefm_invalid_name", "Enter a valid file or folder name." ) );
	return false;
}

// ---------------------------------------------------------------------------------------------
// Properties
// ---------------------------------------------------------------------------------------------

void FileOperations::showProperties( FileTab* tab ) {
	std::erase_if( mProperties, []( const auto& dialog ) { return dialog.expired(); } );
	mProperties.push_back( PropertiesDialog::show( mApp, tab ) );
}

std::optional<FileOperations::DropDestination>
FileOperations::dropDestination( Vector2f position ) {
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
		return std::nullopt;
	// Onto a folder: into it (as drops between panes); anywhere else: the pane's folder.
	URI folder = tab->model->uri();
	if ( auto row = tab->view->rowOf( over );
		 row && *row < tab->model->rowCount() && tab->model->entry( *row ).isDirectory() )
		folder = tab->model->entryURI( *row );
	mApp.panes().focusPane( target );
	return DropDestination{ std::move( folder ), target };
}

void FileOperations::dropExternalFiles() {
	if ( mDroppedFiles.empty() )
		return;
	std::vector<URI> files = std::move( mDroppedFiles );
	mDroppedFiles.clear();
	// Captured with the file event, before further input can move the pointer.
	Vector2f position = *mDropPosition;
	mDropPosition.reset();
	auto destination = dropDestination( position );
	if ( !destination )
		return;
	// eefm's own drag dropped on eefm arrives here on Wayland, macOS and Windows (the system drag
	// started with it): it is resolved as the internal drag it is. Other files are copied.
	const auto& drag = FileTableView::currentDrag();
	dropFiles( files, destination->folder, destination->pane,
			   !( drag.native && files == drag.uris ) );
}

std::optional<std::string> FileOperations::localDropFolder( Vector2f position ) {
	auto destination = dropDestination( position );
	if ( !destination )
		return std::nullopt;
	// The source writes the files itself, so it can only reach local folders.
	if ( destination->folder.getScheme() != "file" ) {
		Dialogs::showError( mUI, mUI->i18n( "eefm_drop_local_only",
											"Files dragged from this application can only be "
											"dropped on local folders." ) );
		return std::nullopt;
	}
	return destination->folder.getFSPath();
}

void FileOperations::dropArchiveEntries( DataDrop drop ) {
	auto folder = localDropFolder( drop.position );
	if ( !folder )
		return;
	// Only a D-Bus call, Ark writes the files itself: on eefm's pool, not where the folder's
	// calls run. The folder's watcher shows the extracted files as Ark writes them.
	mApp.pool()
		->submit( [drop = std::move( drop ), folder = std::move( *folder ),
				   translator = &mUI->getTranslator()] {
			return ArkDrop::extract( drop, folder, *translator );
		} )
		.then( mApp.executor(), [this]( std::string error ) {
			if ( !error.empty() ) {
				Dialogs::showError( mUI, mUI->i18n( "eefm_extract_failed", "Could not extract" ) +
											 ":\n" + String::fromUtf8( error ) );
			}
		} );
}

} // namespace eefm
