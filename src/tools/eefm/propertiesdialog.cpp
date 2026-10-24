#include "propertiesdialog.hpp"
#include "app.hpp"
#include "location.hpp"
#include "mime.hpp"
#include "pane.hpp"
#include "panemanager.hpp"
#include "trash.hpp"
#include <charconv>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uiloader.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/vfs/path.hpp>

namespace eefm {

using namespace EE::VFS;

namespace {

constexpr Uint32 ModeMasks[] = { 0700, 0070, 0007, 04000, 02000, 01000 };
constexpr unsigned ModeShifts[] = { 6, 3, 0, 11, 10, 9 };

bool parseNumber( const String& text, Uint32& value, int base, Uint32 maximum ) {
	const auto str = text.toUtf8();
	if ( str.empty() )
		return false;
	const auto result = std::from_chars( str.data(), str.data() + str.size(), value, base );
	return result.ec == std::errc() && result.ptr == str.data() + str.size() && value <= maximum;
}

String octal( Uint32 mode ) {
	return String::format( "%04o", mode & 07777 );
}

} // namespace

std::shared_ptr<PropertiesDialog> PropertiesDialog::show( App& app, FileTab* tab ) {
	auto dialog = std::make_shared<PropertiesDialog>( app );
	dialog->populate( tab );
	return dialog;
}

PropertiesDialog::PropertiesDialog( App& app ) : mApp( app ) {}

PropertiesDialog::~PropertiesDialog() {
	mCancel.cancel();
	mOperationCancel.cancel();
}

void PropertiesDialog::populate( FileTab* tab ) {
	mModel = tab->model;
	const auto rows = tab->selectedRows();
	mRoots.reserve( std::max( size_t{ 1 }, rows.size() ) );
	mInfo.reserve( std::max( size_t{ 1 }, rows.size() ) );
	for ( const size_t row : rows ) {
		mRoots.push_back( tab->model->entryURI( row ) );
		// Own the listing snapshot: watches can replace model entries while this window is open.
		// Full listing metadata is already available; do not fetch it again before displaying it.
		mInfo.push_back( tab->model->entry( row ) );
	}
	if ( mRoots.empty() )
		mRoots.push_back( tab->model->uri() );
	mBox = mApp.ui()
			   ->loadLayoutFromString(
#include "propertiesdialog.xml.hpp"
				   )
			   ->asType<UIWindow>();
	mGeneral = mBox->find<UIWidget>( "properties_general" );
	mPermissions = mBox->find<UIWidget>( "properties_permissions" );
	mName = mBox->find<UITextInput>( "properties_name" );
	mOctal = mBox->find<UITextInput>( "properties_octal" );
	mUser = mBox->find<UITextInput>( "properties_user" );
	mGroup = mBox->find<UITextInput>( "properties_group" );
	mStatus = mBox->find<UITextView>( "properties_status" );
	mTotals = mBox->find<UITextView>( "properties_totals" );
	mCalculate = mBox->find<UIPushButton>( "properties_calculate" );
	mApply = mBox->find<UIPushButton>( "properties_apply" );
	mOK = mBox->find<UIPushButton>( "properties_ok" );
	mRecursive = mBox->find<UICheckBox>( "properties_recursive" );
	for ( size_t i = 0; i < mModes.size(); ++i )
		mModes[i] = mBox->find<UIDropDownList>( "properties_mode_" + std::to_string( i ) );
	mBox->find<UIWidget>( "properties_timestamps" )->setVisible( mRoots.size() == 1 );
	auto* advanced = mBox->find<UIWidget>( "properties_advanced" );
	mBox->find<UIPushButton>( "properties_advanced_toggle" )
		->onClick(
			[advanced]( const MouseEvent* ) { advanced->setVisible( !advanced->isVisible() ); } );

	mBox->on( Event::OnWindowReady, []( const Event* event ) { event->getNode()->setFocus(); } );

	// UI listeners own the controller. Worker continuations use weak references only.
	auto self = shared_from_this();
	mBox->on( Event::OnWindowClose, [self]( const Event* ) {
		self->mSceneResize.disconnect();
		self->mBox = nullptr;
		self->cancel();
	} );
	mBox->find<UIPushButton>( "properties_cancel" )->onClick( [self]( const MouseEvent* ) {
		if ( self->mBox )
			self->mBox->closeWindow();
	} );
	mBox->addKeyBinding( { KEY_ESCAPE, KEYMOD_NONE }, "close-properties" );
	mBox->setKeyBindingCommand( "close-properties", [self] {
		if ( self->mBox )
			self->mBox->closeWindow();
	} );
	mOK->onClick( [self]( const MouseEvent* ) { self->apply( true ); } );
	mBox->addKeyBinding( { KEY_RETURN, KEYMOD_NONE }, "accept-properties" );
	mBox->setKeyBindingCommand( "accept-properties", [self] { self->apply( true ); } );
	mApply->onClick( [self]( const MouseEvent* ) { self->apply( false ); } );
	mCalculate->onClick( [self]( const MouseEvent* ) {
		if ( self->mRunning )
			self->stopCalculation();
		else
			self->calculate();
	} );
	for ( auto* input : { mName, mOctal, mUser, mGroup } )
		input->on( Event::OnTextChanged, [self]( const Event* ) { self->updateActions(); } );
	for ( auto* mode : mModes ) {
		mode->on( Event::OnItemSelected, [self]( const Event* ) {
			if ( !self->mLoading )
				self->mOctal->setText( "" );
			self->updateActions();
		} );
	}
	mRecursive->on( Event::OnValueChange, [self]( const Event* ) { self->updateActions(); } );
	setLoading( true );
	updateTotals();
	const Sizef available = mApp.ui()->getSize();
	mBox->setSizeWithDecoration( std::min( 550.f, std::max( 1.f, available.getWidth() - 24 ) ),
								 std::min( 510.f, std::max( 1.f, available.getHeight() - 24 ) ) );
	fitWindow();
	mSceneResize =
		mApp.ui()->connect( Event::OnSizeChange, [weak = weak_from_this()]( const Event* ) {
			if ( auto self = weak.lock(); self && self->mBox )
				self->fitWindow();
		} );
	mBox->on( Event::OnSizeChange, [weak = weak_from_this()]( const Event* ) {
		if ( auto self = weak.lock(); self && self->mBox )
			self->updateLayout();
	} );
	mBox->showWhenReady();
	loadNext();
}

void PropertiesDialog::fitWindow() {
	const Sizef available = mApp.ui()->getSize();
	const Float width = std::max( 1.f, available.getWidth() - 24 );
	const Float height = std::max( 1.f, available.getHeight() - 24 );
	const Sizef decoration = mBox->getMinWindowSizeWithDecoration() - mBox->getMinWindowSize();
	mBox->setMinWindowSize( std::min( 480.f, std::max( 1.f, width - decoration.getWidth() ) ),
							std::min( 320.f, std::max( 1.f, height - decoration.getHeight() ) ) );
	mBox->setSizeWithDecoration( std::min( mBox->getSize().getWidth(), width ),
								 std::min( mBox->getSize().getHeight(), height ) );
	mBox->center();
	updateLayout();
}

void PropertiesDialog::updateLayout() {
	if ( mBox->getSize().getWidth() < 440 )
		mBox->addClass( "compact" );
	else
		mBox->removeClass( "compact" );
	if ( mBox->getSize().getWidth() < 320 )
		mBox->addClass( "narrow" );
	else
		mBox->removeClass( "narrow" );
}

void PropertiesDialog::setLoading( bool loading ) {
	mLoading = loading;
	mBox->find<UIWidget>( "properties_loading_overlay" )->setVisible( loading );
	// Hiding the loader itself stops its scheduled animation, as well as hiding the overlay.
	mBox->find<UILoader>( "properties_loader" )->setVisible( loading );
	mBox->find<UITabWidget>( "properties_tabs" )->setEnabled( !loading );
	if ( loading )
		mStatus->setVisible( false );
	updateActions();
}

void PropertiesDialog::loadNext() {
	if ( !mBox )
		return;
	if ( mInfo.size() == mRoots.size() ) {
		updateInfo();
		return;
	}
	mApp.vfs()
		->stat( mRoots[mInfo.size()], StatOptions{ false }, mOperationCancel.token() )
		.then( mApp.executor(), [weak = weak_from_this()]( VFSResult<VFSFileInfo>&& result ) {
			auto self = weak.lock();
			if ( !self || !self->mBox || self->mOperationCancel.isCancellationRequested() )
				return;
			if ( !result ) {
				self->mStatus->setText( result.error().toString() );
				self->mStatus->setVisible( true );
				self->setLoading( false );
				return;
			}
			self->mInfo.push_back( std::move( result ).value() );
			self->loadNext();
		} );
}

void PropertiesDialog::updateInfo() {
	auto* ui = mApp.ui();
	const auto& first = mInfo.front();
	const bool single = mRoots.size() == 1;
	mCanPermissions = true;
	mCanOwnership = true;
	bool folders = false;
	bool links = false;
	for ( size_t i = 0; i < mRoots.size(); ++i ) {
		const auto caps = mApp.vfs()->capabilities( mRoots[i] );
		links |= mInfo[i].isSymlink();
		folders |= mInfo[i].type == FileType::Directory;
		mCanPermissions &=
			caps.has( VFSCapability::Permissions ) && mInfo[i].permissions().has_value();
		mCanOwnership &= caps.has( VFSCapability::Ownership );
	}
#if EE_PLATFORM == EE_PLATFORM_WIN
	// The native provider maps only the read-only attribute, not POSIX access bits.
	// Remote providers can still expose a complete POSIX permissions editor on Windows.
	if ( mRoots[0].getScheme() == "file" )
		mCanPermissions = false;
#endif

	// A link's lstat mode describes the link, whereas chmod/chown follow its target.
	// Match Dolphin by disabling edits rather than silently modifying a different object.
	mCanPermissions &= !links;
	mCanOwnership &= !links;
	mCanRename = single && !mRoots[0].getLastPathSegment().empty() &&
				 mApp.vfs()->capabilities( mRoots[0] ).has( VFSCapability::Rename ) &&
				 !Trash::contains( mRoots[0] );
	mName->setText(
		single ? String::fromUtf8( first.name )
			   : String( String::format( ui->i18n( "eefm_properties_items", "%zu items" ).toUtf8(),
										 mRoots.size() ) ) );
	mBox->setTitle( String::format( ui->i18n( "eefm_properties_title", "%s — Properties" ).toUtf8(),
									mName->getText().toUtf8() ) );
	if ( auto* icon = mApp.fileIcon( first ) ) {
		mBox->find<UIImage>( "properties_icon" )
			->setDrawable( icon->getSource( PixelDensity::dpToPxI( 48 ) ) );
	}
	auto set = [this]( const char* id, const String& text, bool visible = true ) {
		auto* field = mBox->find<UITextView>( id );
		field->setText( text );
		field->getParent()->setVisible( visible );
	};
	std::string type = MimeDatabase::instance().typeOf( first );
	for ( const auto& info : mInfo ) {
		if ( MimeDatabase::instance().typeOf( info ) != type ) {
			type.clear();
			break;
		}
	}
	set( "properties_type",
		 type.empty()
			 ? ui->i18n( "eefm_properties_different_types", "Different types" )
			 : String( String::format(
				   ui->i18n( "eefm_type_with_mime", "%s (%s)" ).toUtf8(),
				   MimeDatabase::instance().describe( first, &ui->getTranslator() ), type ) ) );
	set( "properties_location", Location::text( mRoots[0].getParent() ) );
	set( "properties_target", String::fromUtf8( first.symlinkTarget ),
		 single && first.isSymlink() );
	std::optional<std::string> original;
	if ( single && mRoots[0].getScheme() == "file" && Trash::contains( mRoots[0] ) )
		original = Trash::originalPath( mRoots[0].getFSPath() );
	set( "properties_original", original ? String::fromUtf8( *original ) : String(),
		 original.has_value() );
	auto time = [ui]( std::optional<Int64> seconds ) {
		return seconds ? String::fromUtf8( Sys::epochToString( static_cast<Uint64>( *seconds ) ) )
					   : ui->i18n( "eefm_properties_unavailable", "Unavailable" );
	};
	set( "properties_created", time( first.createdTime() ) );
	set( "properties_modified", time( first.modifiedTime() ) );
	set( "properties_accessed", time( first.accessedTime() ) );
	auto owner = [this]( bool group ) -> std::string {
		const auto& first = mInfo.front();
		const auto id = group ? first.gid() : first.uid();
		const auto& name = group ? first.group : first.owner;
		for ( const auto& info : mInfo ) {
			if ( ( group ? info.gid() : info.uid() ) != id ||
				 ( group ? info.group : info.owner ) != name )
				return {};
		}
		return name.empty() ? ( id ? std::to_string( *id ) : "" ) : name;
	};
	mInitialUser = owner( false );
	mInitialGroup = owner( true );
	mUser->setText( String::fromUtf8( mInitialUser ) );
	mGroup->setText( String::fromUtf8( mInitialGroup ) );
	const auto ownershipHint =
		single ? ui->i18n( "eefm_properties_unavailable", "Unavailable" )
			   : ui->i18n( "eefm_properties_multiple_values", "Multiple values (leave unchanged)" );
	for ( auto* input : { mUser, mGroup } )
		input->setHint( ownershipHint );
	for ( size_t i = 0; i < mModes.size(); ++i ) {
		Uint32 selection = first.permissions()
							   ? ( ( *first.permissions() & ModeMasks[i] ) >> ModeShifts[i] ) + 1
							   : 0;
		for ( const auto& info : mInfo ) {
			if ( !info.permissions() || !first.permissions() ||
				 ( *info.permissions() & ModeMasks[i] ) != ( *first.permissions() & ModeMasks[i] ) )
				selection = 0;
		}
		mInitialModes[i] = selection;
		mModes[i]->getListBox()->setSelected( selection );
		mModes[i]->setEnabled( mCanPermissions );
	}
	mOctal->setText( "" );
	mOctal->setHint( single && first.permissions()
						 ? octal( *first.permissions() )
						 : ui->i18n( "eefm_permissions_unchanged", "Leave unchanged" ) );
	mOctal->setEnabled( mCanPermissions );
	mUser->setEnabled( mCanOwnership );
	mGroup->setEnabled( mCanOwnership );
	mRecursive->setEnabled( folders && ( mCanPermissions || mCanOwnership ) );
	set( "properties_permission_hint",
		 links ? ui->i18n( "eefm_properties_links_hint",
						   "Permissions and ownership of symbolic links cannot be changed here." )
		 : !mCanPermissions
			 ? ui->i18n( "eefm_properties_no_permissions_hint",
						 "This location does not support editing POSIX permissions." )
			 : ui->i18n( "eefm_properties_permissions_hint",
						 "Execute allows entering folders or running files as programs. Only "
						 "edited permissions are applied. Symbolic links are skipped when applying "
						 "recursively." ) );
	mStatus->setText( String::fromUtf8( mResultMessage ) );
	mStatus->setVisible( !mResultMessage.empty() );
	setLoading( false );
	set( "properties_space", ui->i18n( "eefm_properties_unavailable", "Unavailable" ) );
	if ( mApp.vfs()->capabilities( mRoots[0] ).has( VFSCapability::FreeSpace ) ) {
		mApp.vfs()
			->spaceInfo( mRoots[0], mOperationCancel.token() )
			.then( mApp.executor(), [weak = weak_from_this()]( VFSResult<SpaceInfo>&& result ) {
				if ( auto self = weak.lock(); self && self->mBox && result ) {
					self->mBox->find<UITextView>( "properties_space" )
						->setText( String::format(
							self->mApp.ui()
								->i18n( "eefm_properties_available_space", "%s available of %s" )
								.toUtf8(),
							FileSystem::sizeToString( result.value().available ),
							FileSystem::sizeToString( result.value().capacity ) ) );
				}
			} );
	}
	bool automatic = true;
	for ( const auto& root : mRoots ) {
		const auto& path = root.getPath();
		const bool driveRoot = path.size() >= 3 && path[2] == ':' &&
							   ( path.size() == 3 || ( path.size() == 4 && path.back() == '/' ) );
		if ( root.getScheme() != "file" || path == "/" || driveRoot )
			automatic = false;
	}
	if ( !mCalculated && single && first.type == FileType::Regular && first.size() ) {
		// A regular file's storage metadata needs no traversal and is already in the listing.
		mStats.size = *first.size();
		mStats.allocatedSize = first.allocatedSize();
		mStats.files = 1;
		mCalculated = true;
		updateTotals();
	}
	if ( automatic && !mCalculated ) {
		// Populate the dialog before queuing the independent recursive measurement.
		mBox->runOnMainThread( [weak = weak_from_this()] {
			if ( auto self = weak.lock();
				 self && self->mBox && !self->mCalculated && !self->mLoading && !self->mApplying )
				self->calculate();
		} );
	}
}

void PropertiesDialog::updateActions() {
	if ( !mBox )
		return;
	const bool ready = !mLoading && mInfo.size() == mRoots.size();
	bool valid = ready && !mApplying;
	bool dirty = false;
	if ( mCanRename && !mInfo.empty() && mName->getText().toUtf8() != mInfo[0].name ) {
		dirty = true;
		valid &= VFSPath::validName( mName->getText().toUtf8(), mRoots[0].getParent() );
	}
	for ( size_t i = 0; i < mModes.size(); ++i ) {
		if ( mModes[i] && mCanPermissions ) {
			const auto selected = mModes[i]->getListBox()->getItemSelectedIndex();
			dirty |= selected != 0 && selected != mInitialModes[i];
		}
	}
	Uint32 number;
	if ( mCanPermissions && !mOctal->getText().empty() ) {
		dirty = true;
		valid &= parseNumber( mOctal->getText(), number, 8, 07777 );
	}
	if ( mCanOwnership ) {
		dirty |= !mUser->getText().empty() && mUser->getText().toUtf8() != mInitialUser;
		dirty |= !mGroup->getText().empty() && mGroup->getText().toUtf8() != mInitialGroup;
	}
	mName->setEnabled( mCanRename && ready && !mApplying );
	mApply->setEnabled( valid && dirty );
	mOK->setEnabled( valid );
	mCalculate->setEnabled( !mLoading && !mApplying );
}

void PropertiesDialog::apply( bool close ) {
	updateActions();
	if ( !mOK->isEnabled() )
		return;
	if ( !mApply->isEnabled() ) {
		if ( close )
			mBox->closeWindow();
		return;
	}
	stopCalculation();
	++mGeneration;
	mCloseAfterApply = close;
	mApplying = true;
	mModeMask = mModeValue = 0;
	mNewUid.reset();
	mNewGid.reset();
	mRecursiveApply = mRecursive->isEnabled() && mRecursive->isChecked();
	for ( size_t i = 0; i < mModes.size(); ++i ) {
		const Uint32 selected = mModes[i]->getListBox()->getItemSelectedIndex();
		if ( mCanPermissions && selected && selected != mInitialModes[i] ) {
			mModeMask |= ModeMasks[i];
			mModeValue |= ( selected - 1 ) << ModeShifts[i];
		}
	}
	if ( mCanPermissions && !mOctal->getText().empty() ) {
		mModeMask = 07777;
		parseNumber( mOctal->getText(), mModeValue, 8, 07777 );
	}

	mStatus->setVisible( true );
	mStatus->setText( mApp.ui()->i18n( "eefm_properties_applying", "Applying changes..." ) );
	mGeneral->setEnabled( false );
	mPermissions->setEnabled( false );
	updateActions();
	std::string user = mUser->getText().toUtf8();
	std::string group = mGroup->getText().toUtf8();
	if ( !mCanOwnership || user == mInitialUser )
		user.clear();
	if ( !mCanOwnership || group == mInitialGroup )
		group.clear();
	if ( !user.empty() || !group.empty() ) {
		mApp.vfs()
			->resolveOwnership( mRoots[0], std::move( user ), std::move( group ),
								mOperationCancel.token() )
			.then( mApp.executor(), [weak = weak_from_this()]( VFSResult<Ownership>&& result ) {
				if ( auto self = weak.lock(); self && self->mBox ) {
					if ( !result ) {
						self->finishApply( result.error().toString() );
						return;
					}
					self->mNewUid = result.value().uid;
					self->mNewGid = result.value().gid;
					self->applyRename();
				}
			} );
	} else {
		applyRename();
	}
}

void PropertiesDialog::applyRename() {
	const auto name = mName->getText().toUtf8();
	if ( mCanRename && name != mInfo[0].name ) {
		auto destination = mRoots[0].getParent().withAppendedSegment( name );
		mApp.vfs()
			->rename( mRoots[0], destination, {}, mOperationCancel.token() )
			.then( mApp.executor(),
				   [weak = weak_from_this(),
					destination = std::move( destination )]( VFSResult<void>&& result ) mutable {
					   auto self = weak.lock();
					   if ( !self || !self->mBox )
						   return;
					   if ( !result ) {
						   self->finishApply( result.error().toString() );
						   return;
					   }
					   self->mApp.panes().onEntryRenamed( self->mRoots[0], destination );
					   self->mRoots[0] = std::move( destination );
					   self->applyPermissions();
				   } );
	} else {
		applyPermissions();
	}
}

void PropertiesDialog::applyPermissions() {
	if ( !mModeMask && !mNewUid && !mNewGid ) {
		finishApply();
		return;
	}
	PermissionChanges changes;
	changes.mask = mModeMask;
	changes.value = mModeValue;
	changes.uid = mNewUid;
	changes.gid = mNewGid;
	changes.recursive = mRecursiveApply;
	mApp.vfs()
		->changePermissions( mRoots, changes, mOperationCancel.token() )
		.then( mApp.executor(), [weak = weak_from_this()]( VFSResult<void>&& result ) {
			if ( auto self = weak.lock(); self && self->mBox )
				self->finishApply( result ? std::string() : result.error().toString() );
		} );
}

void PropertiesDialog::finishApply( const std::string& error ) {
	mApplying = false;
	mGeneral->setEnabled( true );
	mPermissions->setEnabled( true );
	if ( auto model = mModel.lock() )
		model->refresh();
	if ( error.empty() && mCloseAfterApply ) {
		mBox->closeWindow();
		return;
	}
	mResultMessage =
		error.empty()
			? mApp.ui()->i18n( "eefm_properties_applied", "Changes applied." ).toUtf8()
			: String::format( mApp.ui()
								  ->i18n( "eefm_properties_partial_apply",
										  "Some changes may already have been applied.\n%s" )
								  .toUtf8(),
							  error );
	mInfo.clear();
	setLoading( true );
	loadNext();
}

void PropertiesDialog::cancel() {
	mOperationCancel.cancel();
	stopCalculation();
}

void PropertiesDialog::calculate() {
	if ( !mBox || mApplying )
		return;
	if ( mCalculated || mCancel.isCancellationRequested() )
		mCancel = CancellationSource();
	const auto generation = ++mGeneration;
	mStats = {};
	mRunning = true;
	mStopped = false;
	mCalculated = true;
	updateTotals();
	auto weak = weak_from_this();
	auto* executor = &mApp.executor();
	auto task = mApp.vfs()->directoryStats(
		mRoots,
		[weak, generation, executor]( const DirectoryStats& stats ) {
			// The worker snapshot must be owned until the UI executor consumes it.
			executor->execute( [weak, generation, stats] {
				if ( auto self = weak.lock();
					 self && self->mBox && self->mGeneration == generation ) {
					self->mStats = stats;
					self->updateTotals();
				}
			} );
		},
		mCancel.token() );
	std::move( task ).then( *executor, [weak, generation]( VFSResult<DirectoryStats>&& result ) {
		auto self = weak.lock();
		if ( !self || !self->mBox || self->mGeneration != generation )
			return;
		self->mRunning = false;
		if ( result ) {
			self->mStats = std::move( result ).value();
		} else if ( result.error().is( VFSErrorCode::Cancelled ) ) {
			self->mStopped = true;
		} else {
			++self->mStats.unreadableEntries;
			self->mStats.firstError =
				std::make_shared<const VFSError>( std::move( result ).error() );
		}
		self->updateTotals();
	} );
}

void PropertiesDialog::stopCalculation() {
	mCancel.cancel();
	// Keep this run's final cancellation subtotal. Starting another calculation advances
	// the generation; closing the dialog clears mBox and rejects all pending deliveries.
	if ( mRunning ) {
		mRunning = false;
		mStopped = true;
		updateTotals();
	}
}

void PropertiesDialog::updateTotals() {
	if ( !mBox )
		return;
	auto* ui = mApp.ui();
	auto set = [this]( const char* id, const String& text ) {
		mBox->find<UITextView>( id )->setText( text );
	};
	String status;
	if ( !mCalculated ) {
		const auto text = ui->i18n( "eefm_properties_not_calculated", "Not calculated" );
		for ( const auto* id : { "properties_size", "properties_disk", "properties_contents" } )
			set( id, text );
	} else {
		const bool partial = mRunning || mStopped || mStats.incomplete();
		const auto sizeFormat =
			( partial ? ui->i18n( "eefm_properties_size_at_least", "At least %s (%llu bytes)" )
					  : ui->i18n( "eefm_properties_size_bytes", "%s (%llu bytes)" ) )
				.toUtf8();
		auto size = [&sizeFormat]( Uint64 bytes ) {
			return String::format( sizeFormat,
								   FileSystem::sizeToString( static_cast<Int64>( bytes ) ),
								   static_cast<unsigned long long>( bytes ) );
		};
		set( "properties_size", size( mStats.size ) );
		set( "properties_disk",
			 mStats.allocatedSize
				 ? size( *mStats.allocatedSize )
				 : ui->i18n( "eefm_properties_unavailable", "Unavailable" ).toUtf8() );
		const auto files = String::format(
			( mStats.files == 1 ? ui->i18n( "eefm_properties_file_one", "%llu file" )
								: ui->i18n( "eefm_properties_file_many", "%llu files" ) )
				.toUtf8(),
			static_cast<unsigned long long>( mStats.files ) );
		const auto folders =
			String::format( ( mStats.directories == 1
								  ? ui->i18n( "eefm_properties_folder_one", "%llu subfolder" )
								  : ui->i18n( "eefm_properties_folder_many", "%llu subfolders" ) )
								.toUtf8(),
							static_cast<unsigned long long>( mStats.directories ) );
		set( "properties_contents",
			 String::format( ui->i18n( "eefm_properties_contents_counts", "%s, %s" ).toUtf8(),
							 files, folders ) );
		if ( mRunning )
			status = ui->i18n( "eefm_properties_calculating", "Calculating..." );
		else if ( mStopped )
			status = ui->i18n( "eefm_properties_calculation_stopped", "Calculation stopped." );
		if ( mStats.incomplete() ) {
			status += "\n" + ui->i18n( "eefm_properties_incomplete",
									   "Some entries could not be measured." );
		}
		if ( mStats.firstError )
			status += "\n" + String::fromUtf8( mStats.firstError->toString() );
	}
	mTotals->setText( status );
	mTotals->getParent()->setVisible( !status.empty() );
	mCalculate->setText( mRunning	   ? ui->i18n( "eefm_properties_stop", "Stop" )
						 : mCalculated ? ui->i18n( "eefm_properties_recalculate", "Recalculate" )
									   : ui->i18n( "eefm_properties_calculate", "Calculate" ) );
}

} // namespace eefm
