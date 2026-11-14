#include "bulkrenamedialog.hpp"
#include "eefm.hpp"
#include "mime.hpp"
#include "pane.hpp"
#include "panemanager.hpp"
#include <algorithm>
#include <eepp/core/containers.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/vfs/path.hpp>

namespace eefm {

std::shared_ptr<BulkRenameDialog> BulkRenameDialog::show( App& app, FileTab& tab ) {
	auto dialog = std::make_shared<BulkRenameDialog>( app );
	dialog->populate( tab );
	return dialog;
}

BulkRenameDialog::BulkRenameDialog( App& app ) : mApp( app ) {}

BulkRenameDialog::~BulkRenameDialog() {
	cancel();
}

void BulkRenameDialog::cancel() {
	mCancel.cancel();
}

void BulkRenameDialog::populate( FileTab& tab ) {
	mModel = tab.model;
	mDirectory = tab.model->uri();
	const auto rows = tab.selectedRows();
	mItems.reserve( rows.size() );
	std::vector<size_t> files;
	files.reserve( rows.size() );
	for ( size_t row : rows ) {
		const auto& info = tab.model->entry( row );
		if ( info.type != FileType::Directory )
			files.push_back( mItems.size() );
		mItems.push_back( { info.name, {} } );
	}
	mBox = mApp.ui()
			   ->loadLayoutFromString(
#include "bulkrenamedialog.xml.hpp"
				   )
			   ->asType<UIWindow>();
	mForm = mBox->find<UIWidget>( "bulk_rename_form" );
	mMode = mBox->find<UIDropDownList>( "bulk_rename_mode" );
	mPosition = mBox->find<UIDropDownList>( "bulk_rename_position" );
	mAddPosition = mBox->find<UIDropDownList>( "bulk_rename_add_position" );
	mTemplate = mBox->find<UITextInput>( "bulk_rename_template" );
	mSearch = mBox->find<UITextInput>( "bulk_rename_search" );
	mReplacement = mBox->find<UITextInput>( "bulk_rename_replacement" );
	mText = mBox->find<UITextInput>( "bulk_rename_text" );
	mPreview = mBox->find<UITextInput>( "bulk_rename_preview" );
	mStart = mBox->find<UISpinBox>( "bulk_rename_start" );
	mStatus = mBox->find<UITextView>( "bulk_rename_status" );
	mRename = mBox->find<UIPushButton>( "bulk_rename_accept" );
	mMode->getListBox()->setSelected( static_cast<Uint32>( mApp.config().files.renameMode ) );
	mTemplate->setText( defaultTemplate( BatchRenamePattern::Position::Replace ) );
	mPreview->setAllowEditing( false );
	mStart->allowFloatingPoint( false );
	mStart->setMinValue( 0 )->setMaxValue( 1000000000 )->setValue( 1 );
	auto self = shared_from_this();
	mBox->on( Event::OnWindowClose, [self]( const Event* ) {
		self->mSceneResize.disconnect();
		self->mBox = nullptr;
		self->cancel();
	} );
	mBox->find<UIPushButton>( "bulk_rename_cancel" )->onClick( [self]( const MouseEvent* ) {
		if ( self->mRunning ) {
			self->cancel();
			self->mStatus->setText( self->mApp.ui()->i18n( "eefm_cancelling", "Cancelling..." ) );
		} else if ( self->mBox ) {
			self->mBox->closeWindow();
		}
	} );
	mBox->addKeyBinding( { KEY_ESCAPE, KEYMOD_NONE }, "close-bulk-rename" );
	mBox->setKeyBindingCommand( "close-bulk-rename", [self] {
		if ( self->mBox )
			self->mBox->closeWindow();
	} );
	mBox->addKeyBinding( { KEY_RETURN, KEYMOD_NONE }, "accept-bulk-rename" );
	mBox->addKeyBinding( { KEY_RETURN, KEYMOD_CTRL }, "accept-bulk-rename" );
	mBox->setKeyBindingCommand( "accept-bulk-rename", [self] { self->apply(); } );
	mRename->onClick( [self]( const MouseEvent* ) { self->apply(); } );
	mMode->on( Event::OnItemSelected, [self]( const Event* ) { self->changeMode(); } );
	mPosition->on( Event::OnItemSelected, [self]( const Event* ) { self->changePosition(); } );
	mAddPosition->on( Event::OnItemSelected, [self]( const Event* ) { self->updatePreview(); } );
	for ( auto* input : { mTemplate, mSearch, mReplacement, mText } )
		input->on( Event::OnTextChanged, [self]( const Event* ) { self->updatePreview(); } );
	for ( auto* input :
		  { mTemplate, mSearch, mReplacement, mText, mPreview, mStart->getTextInput() } )
		input->on( Event::OnPressEnter, [self]( const Event* ) { self->apply(); } );
	mStart->on( Event::OnValueChange, [self]( const Event* ) { self->updatePreview(); } );
	mSceneResize =
		mApp.ui()->connect( Event::OnSizeChange, [weak = weak_from_this()]( const Event* ) {
			if ( auto self = weak.lock(); self && self->mBox )
				self->fitWindow();
		} );
	changeMode();
	mBox->showWhenReady();
	// MIME suffix discovery may load desktop metadata. Keep it off the UI thread and move the
	// owned names through the worker once; the window can close while they are being prepared.
	mApp.pool()
		->submit( [items = std::move( mItems ), files = std::move( files )]() mutable {
			for ( size_t index : files )
				items[index].extension =
					MimeDatabase::instance().suffixForName( items[index].name );
			return std::move( items );
		} )
		.thenDroppable( mApp.executor(),
						[weak = weak_from_this()]( std::vector<BatchRenameItem>&& items ) {
							auto self = weak.lock();
							if ( !self || !self->mBox )
								return;
							self->mItems = std::move( items );
							SmallUnorderedSet<std::string_view, 8> extensions;
							self->mAllowUnnumbered = true;
							for ( const auto& item : self->mItems ) {
								if ( !extensions.insert( item.extension ).second ) {
									self->mAllowUnnumbered = false;
									break;
								}
							}
							self->mLoading = false;
							self->mForm->setEnabled( true );
							self->changeMode();
						} );
}

void BulkRenameDialog::fitWindow() {
	const auto available = mApp.ui()->getSize();
	const Float width = std::max( 1.f, available.getWidth() - 24 );
	const Float height = std::max( 1.f, available.getHeight() - 24 );
	const auto decoration = mBox->getMinWindowSizeWithDecoration() - mBox->getMinWindowSize();
	mBox->setMinWindowSize( std::min( 320.f, std::max( 1.f, width - decoration.x ) ),
							std::min( 160.f, std::max( 1.f, height - decoration.y ) ) );
	const auto mode =
		static_cast<BatchRenamePattern::Mode>( mMode->getListBox()->getItemSelectedIndex() );
	Float wanted = mode == BatchRenamePattern::Mode::ReplaceText ? 300 : 270;
	if ( mStatus->isVisible() )
		wanted += 64;
	mBox->setSizeWithDecoration( std::min( 560.f, width ), std::min( wanted, height ) );
	mBox->center();
}

String BulkRenameDialog::defaultTemplate( BatchRenamePattern::Position position ) const {
	if ( position == BatchRenamePattern::Position::Before )
		return "#_";
	if ( position == BatchRenamePattern::Position::After )
		return "_#";
	return mApp.ui()->i18n( "eefm_rename_default_template", "New name #" );
}

void BulkRenameDialog::changeMode() {
	if ( !mBox || mRunning )
		return;
	const auto mode =
		static_cast<BatchRenamePattern::Mode>( mMode->getListBox()->getItemSelectedIndex() );
	mUpdating = true;
	if ( mode == BatchRenamePattern::Mode::Enumerate ) {
		mPosition->getListBox()->setSelected( 0 );
		mLastPosition = BatchRenamePattern::Position::Replace;
		mTemplate->setText( defaultTemplate( mLastPosition ) );
		mStart->setValue( 1 );
	} else if ( mode == BatchRenamePattern::Mode::ReplaceText ) {
		mSearch->setText( "" );
		mReplacement->setText( "" );
	} else {
		mText->setText( "" );
		mAddPosition->getListBox()->setSelected( 0 );
	}
	mUpdating = false;
	mBox->find<UIWidget>( "bulk_rename_enumerate" )
		->setVisible( mode == BatchRenamePattern::Mode::Enumerate );
	mBox->find<UIWidget>( "bulk_rename_replace" )
		->setVisible( mode == BatchRenamePattern::Mode::ReplaceText );
	mBox->find<UIWidget>( "bulk_rename_add" )
		->setVisible( mode == BatchRenamePattern::Mode::AddText );
	mBox->find<UITextView>( "bulk_rename_instruction" )
		->setText( String::format( mApp.ui()
									   ->i18n( mode == BatchRenamePattern::Mode::Enumerate
												   ? "eefm_rename_selected_to"
												   : "eefm_rename_selected_by",
											   mode == BatchRenamePattern::Mode::Enumerate
												   ? "Rename the %zu selected items to:"
												   : "Rename the %zu selected items by:" )
									   .toUtf8(),
								   mItems.size() ) );
	updatePreview();
	if ( !mLoading ) {
		auto* input = mode == BatchRenamePattern::Mode::Enumerate	  ? mTemplate
					  : mode == BatchRenamePattern::Mode::ReplaceText ? mSearch
																	  : mText;
		input->setFocus();
		if ( input == mTemplate &&
			 input->getText() == defaultTemplate( BatchRenamePattern::Position::Replace ) ) {
			input->getDocument().setSelection(
				{ 0, 0 }, { 0, static_cast<Int64>( input->getText().size() - 1 ) } );
		}
	}
}

void BulkRenameDialog::changePosition() {
	if ( mUpdating || !mBox || mRunning )
		return;
	const auto position = static_cast<BatchRenamePattern::Position>(
		mPosition->getListBox()->getItemSelectedIndex() );
	if ( mTemplate->getText() == defaultTemplate( mLastPosition ) ) {
		mUpdating = true;
		mTemplate->setText( defaultTemplate( position ) );
		mUpdating = false;
	}
	mLastPosition = position;
	updatePreview();
}

BatchRenamePattern BulkRenameDialog::pattern() const {
	BatchRenamePattern result;
	result.mode =
		static_cast<BatchRenamePattern::Mode>( mMode->getListBox()->getItemSelectedIndex() );
	result.allowUnnumbered = mAllowUnnumbered;
	if ( result.mode == BatchRenamePattern::Mode::Enumerate ) {
		result.text = mTemplate->getText().toUtf8();
		result.position = static_cast<BatchRenamePattern::Position>(
			mPosition->getListBox()->getItemSelectedIndex() );
		result.startIndex = static_cast<Uint64>( mStart->getValue() );
	} else if ( result.mode == BatchRenamePattern::Mode::ReplaceText ) {
		result.text = mSearch->getText().toUtf8();
		result.replacement = mReplacement->getText().toUtf8();
	} else {
		result.text = mText->getText().toUtf8();
		result.position = mAddPosition->getListBox()->getItemSelectedIndex() == 0
							  ? BatchRenamePattern::Position::Before
							  : BatchRenamePattern::Position::After;
	}
	return result;
}

void BulkRenameDialog::updatePreview() {
	if ( mLoading || mUpdating || mRunning || mFinished || !mBox || mItems.empty() )
		return;
	const auto rule = pattern();
	auto preview = rule.apply( mItems.front() );
	bool valid = preview && VFSPath::validName( preview.value(), mDirectory );
	String error;
	if ( !preview && !rule.text.empty() )
		error = String::fromUtf8( preview.error().message );
	else if ( preview && !valid )
		error = mApp.ui()->i18n( "eefm_invalid_name", "Invalid filename" );
	if ( valid && rule.mode == BatchRenamePattern::Mode::ReplaceText ) {
		valid = std::any_of( mItems.begin(), mItems.end(), [&]( const BatchRenameItem& item ) {
			const size_t stemSize = item.extension.empty()
										? item.name.size()
										: item.name.size() - item.extension.size() - 1;
			return std::string_view( item.name ).substr( 0, stemSize ).find( rule.text ) !=
				   std::string_view::npos;
		} );
		if ( !valid )
			error = mApp.ui()->i18n( "eefm_rename_no_match", "No file name contains the pattern." );
	}
	mPreview->setText( preview ? String::fromUtf8( preview.value() ) : String() );
	mRename->setEnabled( valid );
	mStatus->setText( error );
	mStatus->setVisible( !error.empty() );
	fitWindow();
}

void BulkRenameDialog::apply() {
	if ( !mBox || mLoading || mRunning || mFinished || !mRename->isEnabled() )
		return;
	updatePreview();
	if ( !mRename->isEnabled() )
		return;
	auto rule = pattern();
	mApp.config().files.renameMode = rule.mode;
	mApp.config().save();
	mRunning = true;
	mForm->setEnabled( false );
	mRename->setEnabled( false );
	mStatus->setText( mApp.ui()->i18n( "eefm_renaming_items", "Renaming items..." ) );
	mStatus->setVisible( true );
	fitWindow();
	// Retain the controller until completed names have relocated open tabs/history, even if
	// closing the window cancels the batch. The executor drops this delivery at application exit.
	mApp.vfs()
		->batchRename( mDirectory, std::move( mItems ), std::move( rule ), mCancel.token() )
		.thenDroppable( mApp.executor(),
						[self = shared_from_this()]( VFSResult<BatchRenameResult>&& result ) {
							self->finish( std::move( result ) );
						} );
}

void BulkRenameDialog::finish( VFSResult<BatchRenameResult>&& result ) {
	mRunning = false;
	if ( !result ) {
		if ( mBox ) {
			mFinished = true;
			mStatus->setText( String::fromUtf8( result.error().toString() ) );
		}
		return;
	}
	auto report = std::move( result ).value();
	SmallVector<std::string, 1> selection;
	if ( report.completed )
		selection.reserve( report.items.size() );
	for ( size_t i = 0; i < report.items.size(); ++i ) {
		if ( report.completed == 0 )
			break;
		if ( i < report.completed ) {
			if ( report.items[i].name != report.names[i] ) {
				mApp.panes().onEntryRenamed( mDirectory.withAppendedSegment( report.items[i].name ),
											 mDirectory.withAppendedSegment( report.names[i] ) );
			}
			selection.push_back( std::move( report.names[i] ) );
		} else {
			selection.push_back( report.items[i].name );
		}
	}
	if ( auto model = mModel.lock(); report.completed && model && model->uri() == mDirectory ) {
		for ( const auto& pane : mApp.panes().all() ) {
			if ( !pane )
				continue;
			for ( const auto& tab : pane->tabList ) {
				if ( tab->model == model )
					tab->selectLater( std::move( selection ) );
			}
		}
		model->refresh();
	}
	if ( !mBox )
		return;
	if ( !report.error ) {
		mBox->closeWindow();
		return;
	}
	mItems = std::move( report.items );
	// A failed preflight can be edited and retried. A partially applied batch is shown as a
	// completed result: repeating Add Text would otherwise transform successful entries twice.
	mFinished = report.completed != 0 || report.error->is( VFSErrorCode::Cancelled );
	mForm->setEnabled( !mFinished );
	mStatus->setText( String::format(
		mApp.ui()->i18n( "eefm_rename_batch_error", "%zu of %zu items processed.\n%s" ).toUtf8(),
		report.completed, mItems.size(), report.error->toString() ) );
	mStatus->setVisible( true );
	if ( !mFinished )
		mRename->setEnabled( true );
	fitWindow();
}

} // namespace eefm
