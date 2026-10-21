#include "propertiesdialog.hpp"
#include "app.hpp"
#include "location.hpp"
#include "mime.hpp"
#include "pane.hpp"
#include "trash.hpp"
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uipushbutton.hpp>

namespace eefm {

using namespace EE::VFS;

std::shared_ptr<PropertiesDialog> PropertiesDialog::show( App& app, FileTab* tab ) {
	auto dialog = std::make_shared<PropertiesDialog>( app );
	dialog->populate( tab );
	return dialog;
}

PropertiesDialog::PropertiesDialog( App& app ) : mApp( app ) {}

PropertiesDialog::~PropertiesDialog() {
	mCancel.cancel();
}

void PropertiesDialog::populate( FileTab* tab ) {
	const auto rows = tab->selectedRows();
	std::string text;
	auto line = [&text]( const char* key, const std::string& value ) {
		if ( !value.empty() )
			text += std::string( key ) + ": " + value + "\n";
	};
	auto time = []( std::optional<Int64> seconds ) {
		return seconds ? Sys::epochToString( static_cast<Uint64>( *seconds ) ) : std::string();
	};
	if ( rows.size() == 1 ) {
		const VFSFileInfo& info = tab->model->entry( rows[0] );
		line( "Name", info.name );
		line( "Type", MimeDatabase::instance().describe( info ) + " (" +
						  MimeDatabase::instance().typeOf( info ) + ")" );
		line( "Location", Location::text( tab->model->uri() ).toUtf8() );
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
		line( "Selected", std::to_string( rows.size() ) + " items" );
		line( "Location", Location::text( tab->model->uri() ).toUtf8() );
	} else {
		line( "Folder", Location::displayName( tab->model->uri() ).toUtf8() );
		line( "Location", Location::clipboardText( tab->model->uri() ) );
	}
	mRoots = tab->selectedURIs();
	if ( mRoots.empty() )
		mRoots.push_back( tab->model->uri() );
	mBox = UIMessageBox::New( UIMessageBox::OK, String::fromUtf8( text ) );
	mBox->setId( "file_properties" );
	mBox->setTitle( mApp.ui()->i18n( "eefm_properties", "Properties" ) );
	mBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	mBox->getTextBox()->setMaxWidthEq( "70vw" );
	mBox->getTextBox()->setWordWrap( true );
	mBox->getButtonOK()->setText( "Close" );
	mTotals = UITextView::New();
	mTotals->setId( "properties_totals" );
	mTotals->setMaxWidthEq( "70vw" );
	mTotals->setWordWrap( true );
	mTotals->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
	mTotals->setLayoutMargin( Rectf( 0, 8, 0, 0 ) );
	mTotals->setParent( mBox->getTextBox()->getParent() );
	mTotals->toPosition( 1 );
	mCalculate = UIPushButton::New();
	mCalculate->setId( "properties_calculate" );
	mCalculate->setText( "Calculate" );
	mCalculate->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent );
	mCalculate->setLayoutMargin( Rectf( 8, 0, 0, 0 ) );
	mCalculate->setParent( mBox->getButtonOK()->getParent() );
	mCalculate->onClick( [self = shared_from_this()]( const MouseEvent* ) {
		if ( self->mRunning )
			self->cancel();
		else
			self->calculate();
	} );
	mBox->on( Event::OnWindowClose, [self = shared_from_this()]( const Event* ) {
		self->mBox = nullptr;
		self->cancel();
	} );
	// Totals and error text can grow while the dialog is open.
	mBox->getLayoutCont()->on( Event::OnSizeChange, [self = shared_from_this()]( const Event* ) {
		if ( self->mBox )
			self->mBox->setMinWindowSize( self->mBox->getLayoutCont()->getSize() );
	} );
	updateTotals();
	mBox->showWhenReady();
	bool automatic = true;
	for ( const auto& root : mRoots ) {
		const auto& path = root.getPath();
		const bool driveRoot = path.size() >= 3 && path[2] == ':' &&
							   ( path.size() == 3 || ( path.size() == 4 && path.back() == '/' ) );
		if ( root.getScheme() != "file" || path == "/" || driveRoot )
			automatic = false;
	}
	if ( automatic )
		calculate();
}

void PropertiesDialog::calculate() {
	if ( !mBox )
		return;
	if ( mCalculated )
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

void PropertiesDialog::cancel() {
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
	std::string text;
	if ( !mCalculated ) {
		text = "Size: Not calculated\nSpace on disk: Not calculated";
	} else {
		const bool partial = mRunning || mStopped || mStats.incomplete();
		auto size = [partial]( Uint64 bytes ) {
			return std::string( partial ? "At least " : "" ) +
				   FileSystem::sizeToString( static_cast<Int64>( bytes ) ) + " (" +
				   std::to_string( bytes ) + " bytes)";
		};
		text = "Size: " + size( mStats.size ) + "\nSpace on disk: " +
			   ( mStats.allocatedSize ? size( *mStats.allocatedSize ) : "Unavailable" ) +
			   "\nContents: " + std::to_string( mStats.files ) +
			   ( mStats.files == 1 ? " file, " : " files, " ) +
			   std::to_string( mStats.directories ) +
			   ( mStats.directories == 1 ? " subfolder" : " subfolders" );
		if ( mRunning )
			text += "\nCalculating...";
		else if ( mStopped )
			text += "\nCalculation stopped.";
		if ( mStats.incomplete() )
			text += "\nSome entries could not be measured.";
		if ( mStats.firstError )
			text += "\n" + mStats.firstError->toString();
	}
	mTotals->setText( String::fromUtf8( text ) );
	mCalculate->setText( mRunning ? "Stop" : mCalculated ? "Recalculate" : "Calculate" );
}

} // namespace eefm
