#include "openwithdialog.hpp"
#include "app.hpp"
#include "mime.hpp"
#include <algorithm>
#include <eepp/system/fileassociation.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/models/model.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <utility>

namespace eefm {

using namespace EE::System;
using namespace EE::UI::Abstract;
using namespace EE::UI::Models;

namespace {

class ApplicationListModel final : public Model {
  public:
	ApplicationListModel( App& app, const std::vector<Application>& applications ) {
		mEntries.reserve( applications.size() );
		for ( size_t i = 0; i < applications.size(); ++i ) {
			const auto& application = applications[i];
			mEntries.push_back(
				{ i == 0
					  ? String( String::format(
							app.ui()->i18n( "eefm_application_default", "%s (Default)" ).toUtf8(),
							application.name ) )
					  : String::fromUtf8( application.name ),
				  app.applicationIcon( application ) } );
		}
	}

	size_t rowCount( const ModelIndex& = {} ) const { return mEntries.size(); }

	size_t columnCount( const ModelIndex& = {} ) const { return 1; }

	Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const {
		if ( !index.isValid() || static_cast<size_t>( index.row() ) >= mEntries.size() )
			return {};
		const auto& entry = mEntries[index.row()];
		if ( role == ModelRole::Display )
			return Variant( &entry.name );
		if ( role == ModelRole::Icon )
			return Variant( entry.icon );
		return {};
	}

  private:
	struct Entry {
		String name;
		UIIcon* icon;
	};

	// Labels stay stable for borrowed display variants; the application owns the shared icons.
	std::vector<Entry> mEntries;
};

} // namespace

void OpenWithDialog::show( App& app, std::vector<URI> uris, std::vector<Application> applications,
						   Open open, Command command ) {
	if ( uris.empty() )
		return;
	auto dialog =
		std::make_shared<OpenWithDialog>( app, std::move( uris ), std::move( applications ),
										  std::move( open ), std::move( command ) );
	dialog->populate();
}

OpenWithDialog::OpenWithDialog( App& app, std::vector<URI> uris,
								std::vector<Application> applications, Open open,
								Command command ) :
	mApp( app ),
	mUris( std::move( uris ) ),
	mApplications( std::move( applications ) ),
	mOpen( std::move( open ) ),
	mCommand( std::move( command ) ) {}

void OpenWithDialog::populate() {
	mBox = mApp.ui()
			   ->loadLayoutFromString(
#include "openwithdialog.xml.hpp"
				   )
			   ->asType<UIWindow>();
	mList = mBox->find<UIListView>( "open_with_applications" );
	mAlways = mBox->find<UICheckBox>( "open_with_always" );
	mHint = mBox->find<UITextView>( "open_with_hint" );
	mStatus = mBox->find<UITextView>( "open_with_status" );
	mOpenButton = mBox->find<UIPushButton>( "open_with_open" );
	mCommandButton = mBox->find<UIPushButton>( "open_with_command" );
	mDefaultButton = mBox->find<UIPushButton>( "open_with_default" );
#if EE_PLATFORM == EE_PLATFORM_WIN
	mAlways->setVisible( false );
	mDefaultButton->setVisible( true );
#endif
	mList->setModel( std::make_shared<ApplicationListModel>( mApp, mApplications ) );
	mList->setOnUpdateCellCb( []( UITableCell* cell, Model* ) {
		static const std::string loadedClass( "application_icon_loaded" );
		auto* image = cell->getIcon();
		const auto& source = image->getDrawable();
		if ( source && source->getDrawableType() != Drawable::GLYPH )
			image->addClass( loadedClass );
		else
			image->removeClass( loadedClass );
	} );
	if ( !mApplications.empty() )
		mList->setSelection( mList->getModel()->index( 0 ) );

	// The scene and widget listeners own this controller. Workers never retain UI widgets.
	auto self = shared_from_this();
	mBox->on( Event::OnWindowClose, [self]( const Event* ) {
		self->mSceneResize.disconnect();
		self->mBox = nullptr;
	} );
	mBox->on( Event::OnWindowReady, [self]( const Event* ) {
		if ( self->mApplications.empty() )
			self->mCommandButton->setFocus();
		else
			self->mList->setFocus();
	} );
	mBox->find<UIPushButton>( "open_with_cancel" )->onClick( [self]( const MouseEvent* ) {
		self->mBox->closeWindow();
	} );
	mBox->addKeyBinding( { KEY_ESCAPE, KEYMOD_NONE }, "close-open-with" );
	mBox->setKeyBindingCommand( "close-open-with", [self] {
		if ( self->mBox )
			self->mBox->closeWindow();
	} );
	mBox->addKeyBinding( { KEY_RETURN, KEYMOD_NONE }, "accept-open-with" );
	mBox->setKeyBindingCommand( "accept-open-with", [self] { self->open(); } );
	mOpenButton->onClick( [self]( const MouseEvent* ) { self->open(); } );
	mCommandButton->onClick( [self]( const MouseEvent* ) {
		if ( !self->mBox || self->mBusy )
			return;
		self->mBusy = true;
		self->updateActions();
		self->mBox->closeWindow();
		self->mCommand( self->mUris );
	} );
	mDefaultButton->onClick( [self]( const MouseEvent* ) { self->setDefault( false ); } );
	mList->on( Event::OnSelectionChanged, [self]( const Event* ) { self->updateActions(); } );
	mList->on( Event::OnModelEvent, [self]( const Event* event ) {
		if ( static_cast<const ModelEvent*>( event )->getModelEventType() == ModelEventType::Open )
			self->open();
	} );
	mSceneResize =
		mApp.ui()->connect( Event::OnSizeChange, [weak = weak_from_this()]( const Event* ) {
			if ( auto self = weak.lock(); self && self->mBox )
				self->fitWindow();
		} );
	const auto available = mApp.ui()->getSize();
	mBox->setSizeWithDecoration( std::min( 540.f, std::max( 1.f, available.getWidth() - 24 ) ),
								 std::min( 420.f, std::max( 1.f, available.getHeight() - 24 ) ) );
	fitWindow();
	updateActions();
	mBox->showWhenReady();
	loadFileType();
}

void OpenWithDialog::fitWindow() {
	const auto available = mApp.ui()->getSize();
	const Float width = std::max( 1.f, available.getWidth() - 24 );
	const Float height = std::max( 1.f, available.getHeight() - 24 );
	const Sizef decoration = mBox->getMinWindowSizeWithDecoration() - mBox->getMinWindowSize();
	mBox->setMinWindowSize( std::min( 360.f, std::max( 1.f, width - decoration.getWidth() ) ),
							std::min( 300.f, std::max( 1.f, height - decoration.getHeight() ) ) );
	mBox->setSizeWithDecoration( std::min( mBox->getSize().getWidth(), width ),
								 std::min( mBox->getSize().getHeight(), height ) );
	mBox->center();
}

void OpenWithDialog::loadFileType() {
	std::vector<std::string> names;
	names.reserve( mUris.size() );
	for ( const auto& uri : mUris )
		names.emplace_back( uri.getLastPathSegment() );
	// Own only the filenames across the worker handoff; no VFS metadata or downloads are needed.
	mApp.pool()
		->submit( [names = std::move( names )] {
			FileType type;
			auto& database = MimeDatabase::instance();
			type.mimeType = database.typeForName( names.front() );
			type.extension = FileAssociation::normalizeExtension(
				FileSystem::fileNameSplitExtension( names.front() ).second );
			const bool known = type.mimeType != "application/octet-stream";
			if ( known )
				type.description = database.description( type.mimeType, names.front() );
#if EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_WIN
			type.canSetDefault =
				!type.extension.empty() &&
				std::all_of( names.begin(), names.end(), [&]( const std::string& name ) {
					return type.extension ==
						   FileAssociation::normalizeExtension(
							   FileSystem::fileNameSplitExtension( name ).second );
				} );
#else
		type.canSetDefault = known &&
			std::all_of( names.begin(), names.end(), [&]( const std::string& name ) {
				return type.mimeType == database.typeForName( name );
			} );
#endif
			if ( !type.canSetDefault && names.size() > 1 )
				type.description.clear();
			type.canSetDefault = type.canSetDefault && FileAssociation::isSupported();
			return type;
		} )
		.then( mApp.executor(), [weak = weak_from_this()]( FileType&& type ) {
			if ( auto self = weak.lock(); self && self->mBox ) {
				self->mType = std::move( type );
				self->mReady = true;
				if ( self->mType.description.empty() && self->mType.canSetDefault ) {
					self->mType.description = String::format(
						self->mApp.ui()->i18n( "eefm_extension_file_type", ".%s files" ).toUtf8(),
						self->mType.extension );
				}
				if ( !self->mType.description.empty() ) {
					self->mBox->find<UITextView>( "open_with_type" )
						->setText( String::format(
							self->mApp.ui()
								->i18n( "eefm_open_with_file_type", "File type: %s" )
								.toUtf8(),
							self->mType.description ) );
				}
				self->mStatus->setVisible( self->mApplications.empty() );
				if ( self->mApplications.empty() ) {
					self->mStatus->setText( self->mApp.ui()->i18n(
						"eefm_no_registered_applications",
						"No registered applications were found. You can use a custom command." ) );
				}
				self->updateActions();
			}
		} );
}

void OpenWithDialog::updateActions() {
	if ( !mBox )
		return;
	const bool selected = selectedIndex() < mApplications.size();
	mList->setEnabled( !mBusy );
	mOpenButton->setEnabled( selected && !mBusy );
	mCommandButton->setEnabled( !mBusy );
	mAlways->setEnabled( mReady && mType.canSetDefault && selected && !mBusy );
	mDefaultButton->setEnabled( mReady && mType.canSetDefault && selected && !mBusy );
	if ( !mReady )
		return;
	if ( !mType.canSetDefault ) {
		mHint->setText( mApp.ui()->i18n(
			"eefm_default_application_unavailable",
			"To change the default, select files with one recognized file type." ) );
		return;
	}
#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( selected ) {
		mHint->setText(
			String::format( mApp.ui()
								->i18n( "eefm_windows_default_application_hint",
										"In Windows Settings, search for .%s and choose %s." )
								.toUtf8(),
							mType.extension, mApplications[selectedIndex()].name ) );
	}
#else
	mHint->setText( mApp.ui()->i18n(
		"eefm_default_application_scope",
		"Changes the default for all files of this type, including outside eefm." ) );
#endif
}

size_t OpenWithDialog::selectedIndex() const {
	return static_cast<size_t>( mList->getSelection().first().row() );
}

void OpenWithDialog::open() {
	if ( !mBox || !mOpenButton->isEnabled() )
		return;
	if ( mAlways->isVisible() && mAlways->isEnabled() && mAlways->isChecked() )
		setDefault( true );
	else
		finishOpen();
}

void OpenWithDialog::finishOpen() {
	mBusy = true;
	updateActions();
	auto application = std::move( mApplications[selectedIndex()] );
	mBox->closeWindow();
	mOpen( mUris, std::move( application ) );
}

void OpenWithDialog::setDefault( bool openAfter ) {
	if ( !mBox || mBusy || !mReady || !mType.canSetDefault ||
		 selectedIndex() >= mApplications.size() )
		return;
	mBusy = true;
	mStatus->setText( mApp.ui()->i18n( "eefm_setting_default_application",
									   "Updating the default application..." ) );
	mStatus->setVisible( true );
	updateActions();
	// The selected handler and type must survive the window closing during the OS operation.
	mApp.pool()
		->submit( [application = mApplications[selectedIndex()], extension = mType.extension,
				   mime = mType.mimeType] {
			std::string error;
			auto result =
				FileAssociation::setDefaultApplication( application, extension, mime, error );
			return std::make_pair( result, std::move( error ) );
		} )
		.then( mApp.executor(), [weak = weak_from_this(), openAfter]( auto&& result ) {
			auto self = weak.lock();
			if ( !self || !self->mBox )
				return;
			self->mBusy = false;
			self->updateActions();
			if ( result.first == FileAssociation::DefaultResult::Failed ) {
				self->mStatus->setText(
					String::format( self->mApp.ui()
										->i18n( "eefm_set_default_application_failed",
												"Could not change the default application:\n%s" )
										.toUtf8(),
									result.second ) );
			} else if ( result.first == FileAssociation::DefaultResult::UserActionRequired ) {
				self->mStatus->setText( self->mApp.ui()->i18n(
					"eefm_default_application_settings_opened",
					"Finish choosing the default application in Windows Settings." ) );
			} else if ( openAfter ) {
				self->finishOpen();
			}
		} );
}

} // namespace eefm
