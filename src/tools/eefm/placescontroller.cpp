#include "placescontroller.hpp"
#include "app.hpp"
#include "fileoperations.hpp"
#include "location.hpp"
#include "panemanager.hpp"

namespace eefm {

PlacesController::PlacesController( App& app, const std::string& configPath ) :
	mApp( app ), mUI( app.ui() ), mPlaces( std::make_unique<Places>( configPath ) ) {
	mPlaces->load();
}

PlacesController::~PlacesController() {
	mDeviceCancellation.cancel();
}

void PlacesController::createPanel( UIWidget* parent ) {
	mPlacesPanel = PlacesPanel::New();
	mPlacesPanel->iconProvider = [this]( const Place& place ) { return mApp.placeIcon( place ); };
	mPlacesPanel->setParent( parent );
	mPlacesPanel->onActivate = [this]( const PlacesPanel::Entry& entry, bool newTab ) {
		if ( newTab )
			mApp.panes().openTab( mApp.panes().activePane(), entry.place.uri );
		else if ( auto* tab = mApp.panes().currentTab() )
			mApp.panes().navigate( tab, entry.place.uri );
	};
	mPlacesPanel->onContextMenu = [this]( const PlacesPanel::Entry& entry ) {
		showPlaceMenu( entry );
	};
	mPlacesPanel->onPlaceMoved = [this]( size_t index, std::optional<size_t> before ) {
		mPlaces->moveBefore( index, before );
		save();
	};
	mPlacesPanel->onFoldersAdded = [this]( std::optional<size_t> before ) {
		for ( const auto& uri : FileTableView::currentDrag().uris ) {
			Place place;
			place.uri = uri;
			place.name = Location::displayName( uri ).toUtf8();
			place.icon = Places::defaultIcon( uri );
			// Each one after the previous: before moves along with the inserted places.
			if ( mPlaces->add( std::move( place ), before ) && before )
				++*before;
		}
		save();
	};
	mPlacesPanel->onSectionMenu = [this]( PlaceSection section ) { showSectionMenu( section ); };
	mPlacesPanel->onFilesDropped = [this]( const PlacesPanel::Entry& entry ) {
		mApp.fileOps().dropFiles( FileTableView::currentDrag().uris, entry.place.uri,
								  mApp.panes().activeIndex() );
	};
	refresh();
	refreshDevices();
	// The timer only schedules work. Platform change checks and discovery both run on the
	// pool, so the UI never depends on a backend check being nonblocking.
	mPlacesPanel->setInterval( [this] { refreshDevices( false ); },
#if EE_PLATFORM == EE_PLATFORM_MACOS
							   Seconds( 1 )
#else
							   Seconds( 3 )
#endif
	);
}

void PlacesController::shutdown() {
	if ( mPlacesPanel )
		mPlacesPanel->clearActions();
	mDeviceCancellation.cancel();
	mDeviceMonitor.stop();
}

void PlacesController::saveConfig() {
	mPlaces->save();
}

void PlacesController::refresh() {
	std::vector<PlacesPanel::Entry> entries;
	bool showHidden = mPlaces->showHidden();
	auto listed = [&]( const Place& place ) {
		return showHidden || ( !place.hidden && !mPlaces->isSectionHidden( place.section ) );
	};
	const auto& places = mPlaces->places();
	// Each section: the saved places, then the discovered mounts (network shares are Remote).
	for ( auto section : { PlaceSection::Places, PlaceSection::Remote, PlaceSection::Devices } ) {
		for ( size_t i = 0; i < places.size(); ++i ) {
			if ( places[i].section == section && listed( places[i] ) ) {
				entries.push_back(
					{ places[i], static_cast<int>( i ), mPlaces->isSectionHidden( section ) } );
			}
		}
		for ( const auto& device : mDevices ) {
			bool saved = std::any_of( places.begin(), places.end(), [&]( const Place& place ) {
				return place.uri == device.uri;
			} );
			if ( device.section == section && !saved && listed( device ) )
				entries.push_back( { device, -1, mPlaces->isSectionHidden( section ) } );
		}
	}
	mPlacesPanel->setEntries( std::move( entries ) );
	if ( auto* tab = mApp.panes().currentTab() )
		mPlacesPanel->setCurrent( tab->model->uri() );
}

void PlacesController::save() {
	mPlaces->save();
	refresh();
	refreshDeviceUsage();
}

void PlacesController::refreshDevices( bool force ) {
	if ( mDeviceCancellation.isCancellationRequested() )
		return;
	if ( mRefreshingDevices ) {
		// Timer ticks never queue more work behind an in-flight check. An explicit refresh
		// (including completion of an eject) must still get a fresh snapshot afterwards.
		mRefreshDevicesAgain |= force;
		return;
	}
	mRefreshingDevices = true;
	// Snapshot only the labels: the worker must not borrow the scene's translator during
	// shutdown, when the application's shared pool may still be draining. The controller and
	// monitor outlive that pool; stop() keeps resources used by in-flight checks alive.
	mApp.pool()
		->submit( [monitor = &mDeviceMonitor, force,
				   rootName = mUI->i18n( "eefm_root", "Root" ).toUtf8(),
				   shareNameFormat = mUI->i18n( "eefm_share_on_server", "%s on %s" ).toUtf8(),
				   token = mDeviceCancellation.token()]() -> std::optional<std::vector<Place>> {
			if ( token.isCancellationRequested() || ( !force && !monitor->changed() ) )
				return std::nullopt;
			return Places::devices( rootName, shareNameFormat, token );
		} )
		.then( mApp.executor(), [this]( std::optional<std::vector<Place>>&& result ) {
			mRefreshingDevices = false;
			if ( mDeviceCancellation.isCancellationRequested() )
				return;
			if ( result ) {
				auto& devices = *result;
				bool same = devices.size() == mDevices.size() &&
							std::equal( devices.begin(), devices.end(), mDevices.begin(),
										[]( const Place& a, const Place& b ) {
											return a.uri == b.uri && a.name == b.name &&
												   a.icon == b.icon && a.section == b.section &&
												   a.ejectable == b.ejectable;
										} );
				if ( !same ) {
					mDevices = std::move( devices );
					++mDeviceGeneration;
					refresh();
					refreshDeviceUsage();
				}
			}
			if ( mRefreshDevicesAgain ) {
				mRefreshDevicesAgain = false;
				refreshDevices();
			}
		} );
}

void PlacesController::ejectDevice( const URI& uri ) {
	if ( mEjecting || mDeviceCancellation.isCancellationRequested() )
		return;
	const auto device = std::find_if( mDevices.begin(), mDevices.end(), [&]( const Place& place ) {
		return place.uri == uri && place.ejectable;
	} );
	if ( device == mDevices.end() )
		return;
	mEjecting = true;
	mApp.pool()
		->submit( [path = uri.getFSPath(), token = mDeviceCancellation.token()] {
			return DeviceMonitor::eject( path, token );
		} )
		.then( mApp.executor(), [this]( std::string&& error ) {
			mEjecting = false;
			if ( mDeviceCancellation.isCancellationRequested() )
				return;
			refreshDevices();
			if ( !error.empty() )
				Dialogs::showError( mUI, String::fromUtf8( error ) );
		} );
}

void PlacesController::refreshDeviceUsage() {
	for ( const auto& device : mDevices ) {
		// statvfs on a network share blocks while its server is unreachable.
		if ( device.section != PlaceSection::Devices )
			continue;
		URI uri = device.uri;
		mApp.vfs()->spaceInfo( uri ).then(
			mApp.executor(), [this, uri = std::move( uri ),
							  generation = mDeviceGeneration]( VFSResult<SpaceInfo>&& space ) {
				if ( generation != mDeviceGeneration || !space || space.value().capacity == 0 )
					return;
				const auto& info = space.value();
				Float used = 1.f - static_cast<Float>( info.available ) /
									   static_cast<Float>( info.capacity );
				mPlacesPanel->setUsage( uri, used );
			} );
	}
}

void PlacesController::showPlaceMenu( const PlacesPanel::Entry& entry ) {
	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->add( mUI->i18n( "eefm_open_new_tab", "Open in New Tab" ), mApp.icon( "action-new-tab" ) )
		->setId( "open-tab" );
	if ( mApp.isSplit() ) {
		menu->add( mUI->i18n( "eefm_open_other_pane", "Open in Other Pane" ) )
			->setId( "open-other" );
	}
	if ( entry.place.trash ) {
		menu->addSeparator();
		menu->add( mUI->i18n( "eefm_empty_trash", "Empty Trash" ), mApp.icon( "place-trash" ) )
			->setId( "empty-trash" );
	}
	if ( entry.index >= 0 ) {
		size_t index = static_cast<size_t>( entry.index );
		menu->addSeparator();
		menu->add( mUI->i18n( "eefm_edit_place", "Edit..." ), mApp.icon( "edit" ) )
			->setId( "edit" );
		if ( entry.place.hidden )
			menu->add( mUI->i18n( "eefm_show_place", "Show" ) )->setId( "show" );
		else
			menu->add( mUI->i18n( "eefm_hide_place", "Hide" ) )->setId( "hide" );
		if ( entry.place.id.empty() ) {
			menu->add( mUI->i18n( "eefm_remove_place", "Remove" ), mApp.icon( "delete-bin" ) )
				->setId( "remove" );
		}
		if ( mPlaces->canMove( index, -1 ) ) {
			menu->add( mUI->i18n( "eefm_move_place_up", "Move Up" ), mApp.icon( "arrow-up-s" ) )
				->setId( "move-up" );
		}
		if ( mPlaces->canMove( index, 1 ) ) {
			menu->add( mUI->i18n( "eefm_move_place_down", "Move Down" ),
					   mApp.icon( "arrow-down-s" ) )
				->setId( "move-down" );
		}
	}
	bool ejectable = std::any_of( mDevices.begin(), mDevices.end(), [&]( const Place& device ) {
		return device.uri == entry.place.uri && device.ejectable;
	} );
	if ( ejectable ) {
		menu->addSeparator();
		menu->add( mUI->i18n( "eefm_eject", "Eject" ) )->setId( "eject" )->setEnabled( !mEjecting );
	}
	if ( entry.place.section == PlaceSection::Devices ) {
		menu->addSeparator();
		menu->add( mUI->i18n( "eefm_refresh_devices", "Refresh Devices" ),
				   mApp.icon( "view-refresh" ) )
			->setId( "refresh-devices" );
	}
	addPanelItems( menu, entry.place.section );
	menu->on( Event::OnItemClicked, [this, entry]( const Event* event ) {
		std::string id( event->getNode()->getId() );
		size_t index = static_cast<size_t>( std::max( entry.index, 0 ) );
		if ( onPanelItem( id, entry.place.section ) ) {
		} else if ( id == "open-tab" ) {
			mApp.panes().openTab( mApp.panes().activePane(), entry.place.uri );
		} else if ( id == "open-other" ) {
			if ( auto* tab = mApp.panes().currentTab(
					 mApp.panes().pane( 1 - mApp.panes().activeIndex() ) ) )
				mApp.panes().navigate( tab, entry.place.uri );
		} else if ( id == "empty-trash" ) {
			mApp.fileOps().emptyTrash();
		} else if ( id == "edit" ) {
			showPlaceDialog( index, entry.place );
		} else if ( id == "hide" || id == "show" ) {
			mPlaces->setHidden( index, id == "hide" );
			save();
		} else if ( id == "remove" ) {
			mPlaces->remove( index );
			save();
		} else if ( id == "move-up" || id == "move-down" ) {
			mPlaces->move( index, id == "move-up" ? -1 : 1 );
			save();
		} else if ( id == "refresh-devices" ) {
			refreshDevices();
			refreshDeviceUsage();
		} else if ( id == "eject" ) {
			ejectDevice( entry.place.uri );
		}
	} );
	menu->showOverMouseCursor();
}

void PlacesController::showSectionMenu( PlaceSection section ) {
	UIPopUpMenu* menu = UIPopUpMenu::New();
	addPanelItems( menu, section );
	menu->on( Event::OnItemClicked, [this, section]( const Event* event ) {
		onPanelItem( std::string( event->getNode()->getId() ), section );
	} );
	menu->showOverMouseCursor();
}

void PlacesController::addPanelItems( UIPopUpMenu* menu, PlaceSection section ) {
	if ( menu->getCount() > 0 )
		menu->addSeparator();
	menu->add( mUI->i18n( "eefm_add_place_entry", "Add Entry..." ),
			   mApp.icon( "action-add-place" ) )
		->setId( "add-entry" );
	if ( mPlaces->isSectionHidden( section ) )
		menu->add( mUI->i18n( "eefm_show_section", "Show Section" ) )->setId( "show-section" );
	else
		menu->add( mUI->i18n( "eefm_hide_section", "Hide Section" ) )->setId( "hide-section" );
	if ( mPlaces->hasHidden() || mPlaces->showHidden() ) {
		menu->addCheckBox( mUI->i18n( "eefm_show_hidden_places", "Show Hidden Places" ),
						   mPlaces->showHidden() )
			->setId( "show-hidden-places" );
	}
}

bool PlacesController::onPanelItem( const std::string& id, PlaceSection section ) {
	if ( id == "add-entry" ) {
		Place place;
		auto* tab = mApp.panes().currentTab();
		// Start from the current folder, or an empty sftp location for the Remote section.
		if ( section == PlaceSection::Remote ) {
			place.uri = URI( "sftp://" );
		} else if ( tab ) {
			place.uri = tab->model->uri();
			place.name = Location::displayName( place.uri ).toUtf8();
		}
		place.icon = section == PlaceSection::Remote ? "place-remote" : "";
		showPlaceDialog( std::nullopt, place );
	} else if ( id == "show-section" || id == "hide-section" ) {
		mPlaces->setSectionHidden( section, id == "hide-section" );
		save();
	} else if ( id == "show-hidden-places" ) {
		mPlaces->setShowHidden( !mPlaces->showHidden() );
		save();
	} else {
		return false;
	}
	return true;
}

void PlacesController::showPlaceDialog( std::optional<size_t> index, Place initial ) {
	static const auto layout( R"xml(
	<window id="place_dialog" layout_width="460dp" layout_height="250dp" window-flags="default|shadow|modal">
	<vbox lw="mp" lh="wrap_content" padding="8dp">
		<TextView text='@string(eefm_place_label, "Label:")' />
		<TextInput id="place_name" lw="mp" lh="wrap_content" margin-bottom="8dp" />
		<TextView text='@string(eefm_place_location, "Location:")' />
		<ComboBox id="place_location" lw="mp" lh="wrap_content" popup-to-root="true" />
		<TextView class="place_hint" lw="mp" lh="wrap_content" word-wrap="true" margin-bottom="8dp"
			text='@string(eefm_place_location_hint, "A folder path or a URI, e.g. sftp://user@host/path. Hosts from ~/.ssh/config are listed.")' />
		<TextView text='@string(eefm_place_icon, "Icon:")' />
		<hbox id="place_icons" lw="mp" lh="wrap_content" margin-bottom="8dp" />
		<hbox layout_gravity="right">
			<PushButton id="ok" text="@string(msg_box_ok, Ok)" />
			<PushButton id="cancel" text="@string(msg_box_cancel, Cancel)" margin-left="8dp" />
		</hbox>
	</vbox>
	</window>
	)xml" );
	UIWindow* window = mUI->loadLayoutFromString( layout )->asType<UIWindow>();
	window->setTitle( index ? mUI->i18n( "eefm_edit_place_title", "Edit Place" )
							: mUI->i18n( "eefm_add_place_title", "Add Place" ) );
	auto* nameInput = window->find<UITextInput>( "place_name" );
	auto* location = window->find<UIComboBox>( "place_location" );
	auto* icons = window->find<UIWidget>( "place_icons" );

	auto toText = []( const URI& uri ) {
		return uri.getScheme() == "file" ? uri.getFSPath() : uri.toString();
	};
	std::vector<String> suggestions;
	for ( const auto& host : Places::sshHosts() )
		suggestions.emplace_back( String::fromUtf8( host.toString() ) );
	location->getListBox()->addListBoxItems( suggestions );
	nameInput->setText( String::fromUtf8( initial.name ) );
	std::string locationText;
	if ( initial.uri.getScheme() == "file" || !initial.uri.getHost().empty() )
		locationText = toText( initial.uri );
	else if ( !initial.uri.getScheme().empty() )
		locationText = initial.uri.getScheme() + "://";
	location->setText( String::fromUtf8( locationText ) );

	// One toggle per icon; an empty choice follows the location (folder or remote).
	auto chosen = std::make_shared<std::string>( initial.icon );
	auto buttons = std::make_shared<std::vector<UISelectButton*>>();
	for ( const auto& name : Places::iconNames() ) {
		UISelectButton* button = UISelectButton::New();
		button->setParent( icons );
		button->addClass( "place_icon_choice" );
		if ( UIIcon* themeIcon = mUI->findIcon( name ) )
			button->setIcon( themeIcon->getSource( PixelDensity::dpToPxI( 16 ) ) );
		button->setTooltipText( mUI->i18n( name, name ) );
		if ( name == *chosen )
			button->select();
		buttons->push_back( button );
		button->onClick( [button, buttons, chosen, name]( const MouseEvent* ) {
			for ( auto* other : *buttons ) {
				if ( other != button )
					other->unselect();
			}
			button->select();
			*chosen = name;
		} );
	}

	auto accept = [this, window, nameInput, location, chosen, index, toText] {
		std::string text = String::trim( location->getText().toUtf8() );
		URI uri;
		if ( text.find( "://" ) != std::string::npos ) {
			uri = URI( text );
		} else if ( !text.empty() ) {
			std::string path = text;
			if ( String::startsWith( path, "~" ) )
				path = Sys::getUserDirectory() + path.substr( 1 );
			FileSystem::dirAddSlashAtEnd( path );
			uri = URI::fromFSPath( path );
		}
		if ( uri.getScheme().empty() || ( uri.getScheme() != "file" && uri.getHost().empty() ) ) {
			Dialogs::showError( mUI, mUI->i18n( "eefm_invalid_place_location",
												"Enter a folder path or a URI such as "
												"sftp://user@host/path." ) );
			return;
		}
		if ( uri.getPath().empty() )
			uri.setRawPath( "/" );
		std::string name = String::trim( nameInput->getText().toUtf8() );
		if ( name.empty() )
			name = Location::displayName( uri ).toUtf8();
		std::string iconName = chosen->empty() ? Places::defaultIcon( uri ) : *chosen;
		if ( index ) {
			mPlaces->edit( *index, name, uri, iconName );
		} else {
			Place place;
			place.name = name;
			place.uri = uri;
			place.icon = iconName;
			if ( !mPlaces->add( std::move( place ) ) ) {
				Dialogs::showError(
					mUI, String::format(
							 mUI->i18n( "eefm_place_exists", "%s is already in Places." ).toUtf8(),
							 toText( uri ).c_str() ) );
				return;
			}
		}
		save();
		window->closeWindow();
	};
	window->find<UIPushButton>( "ok" )->onClick( [accept]( const MouseEvent* ) { accept(); },
												 EE_BUTTON_LEFT );
	window->find<UIPushButton>( "cancel" )
		->onClick( [window]( const MouseEvent* ) { window->closeWindow(); }, EE_BUTTON_LEFT );
	nameInput->on( Event::OnPressEnter, [accept]( const Event* ) { accept(); } );
	location->getDropDownList()->on( Event::OnPressEnter, [accept]( const Event* ) { accept(); } );
	window->on( Event::KeyDown, [window]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_ESCAPE )
			window->closeWindow();
	} );
	window->center();
	window->on( Event::OnWindowReady, [nameInput, location, index]( const Event* ) {
		// Adding a remote place starts at the location; otherwise at the label.
		if ( !index && String::startsWith( location->getText().toUtf8(), "sftp://" ) &&
			 location->getText().size() <= 7 ) {
			location->getDropDownList()->setFocus();
			location->getDropDownList()->getDocument().moveToEndOfDoc();
		} else {
			nameInput->setFocus();
			nameInput->getDocument().selectAll();
		}
	} );
}

void PlacesController::add( const URI& uri ) {
	Place place;
	place.uri = uri;
	place.name = Location::displayName( uri ).toUtf8();
	place.icon = Places::defaultIcon( uri );
	if ( !mPlaces->add( std::move( place ) ) ) {
		// Already a place: adding it again shows it if it was hidden.
		const auto& places = mPlaces->places();
		for ( size_t i = 0; i < places.size(); ++i ) {
			if ( places[i].uri == uri )
				mPlaces->setHidden( i, false );
		}
	}
	save();
}

} // namespace eefm
