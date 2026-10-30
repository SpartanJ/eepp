#include "operationspopup.hpp"
#include "location.hpp"
#include <algorithm>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/uipopup.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uiwindow.hpp>

namespace eefm {

using namespace EE::System;

std::shared_ptr<OperationsPopup> OperationsPopup::show( UISceneNode& scene, UIWidget& anchor,
														int pane,
														std::function<void( Uint64 )> cancel ) {
	auto popup = std::make_shared<OperationsPopup>( scene, anchor, pane, std::move( cancel ) );
	popup->populate();
	return popup;
}

OperationsPopup::OperationsPopup( UISceneNode& scene, UIWidget& anchor, int pane,
								  std::function<void( Uint64 )> cancel ) :
	mScene( scene ), mAnchor( anchor ), mCancel( std::move( cancel ) ), mPane( pane ) {}

bool OperationsPopup::isOpen() const {
	return mBox && !mBox->isClosing();
}

void OperationsPopup::close() {
	if ( isOpen() )
		mBox->closeWindow();
}

void OperationsPopup::populate() {
	mBox = mScene
			   .loadLayoutFromString(
#include "operationspopup.xml.hpp"
				   )
			   ->asType<UIWindow>();
	mRowsBox = mBox->find<UIWidget>( "operations_rows" );
	mBox->setMinWindowSize( 0, 0 );
	// Keep focus on the trigger until its click toggles the popup. Other outside focus closes it.
	mBox->setCheckEphemeralCloseFn( [this]( Node* focus ) {
		auto* triggers = mAnchor.getParent();
		return !focus || ( focus != triggers && !triggers->isParentOf( focus ) );
	} );
	auto self = shared_from_this();
	mBox->addKeyBinding( { KEY_ESCAPE, KEYMOD_NONE }, "close-operations" );
	mBox->setKeyBindingCommand( "close-operations", [self] { self->close(); } );
	mBox->on( Event::OnWindowClose, [self]( const Event* ) {
		self->mSceneSize.disconnect();
		self->mAnchorPosition.disconnect();
		self->mAnchorSize.disconnect();
		self->mBox = nullptr;
		self->mRowsBox = nullptr;
		self->mRows.clear();
		self->mCancel = {};
	} );
	mBox->on( Event::OnWindowReady, [self]( const Event* ) {
		self->fit();
		if ( self->isOpen() )
			self->mBox->setFocus();
	} );
	mBox->find<UIPushButton>( "operations_close" )->onClick( [self]( const MouseEvent* ) {
		self->close();
	} );
	mSceneSize = mScene.connect( Event::OnSizeChange, [this]( const Event* ) { fit(); } );
	mAnchorPosition = mAnchor.connect( Event::OnPositionChange, [this]( const Event* ) { fit(); } );
	mAnchorSize = mAnchor.connect( Event::OnSizeChange, [this]( const Event* ) { fit(); } );
	fit();
	mBox->showWhenReady();
}

void OperationsPopup::fit() {
	if ( !isOpen() )
		return;
	const auto available = mScene.getSize();
	mBox->setSizeWithDecoration( std::min( 460.f, std::max( 1.f, available.getWidth() - 16 ) ),
								 std::min( std::min( 440.f, 64.f + 112.f * mRows.size() ),
										   std::max( 1.f, available.getHeight() - 16 ) ) );
	mScene.flushDirtyStyleAndLayout();
	UIPopUp::align( &mAnchor, mBox, true );
}

void OperationsPopup::update( Uint64 id, const String& label, const TransferProgress& progress,
							  bool cancelling ) {
	if ( !isOpen() )
		return;
	auto it =
		std::find_if( mRows.begin(), mRows.end(), [id]( const Row& row ) { return row.id == id; } );
	if ( it == mRows.end() ) {
		auto* root = mScene.loadLayoutFromString( R"xml(
<vbox class="operation_row" lw="mp" lh="wc" padding="8dp 0"
 border-top="1dp solid var(--tab-line)">
 <hbox lw="mp" lh="wc">
  <TextView class="operation_label" lw="0" layout_weight="1" lh="wc"
   layout_gravity="center_vertical" text-overflow="ellipsis" clip="content-box" />
  <PushButton class="operation_cancel" lw="wc" lh="wc" margin-left="8dp"
   text='@string(eefm_cancel, "Cancel")' />
 </hbox>
 <TextView class="operation_detail" lw="mp" lh="wc" text-overflow="ellipsis" clip="content-box"
  color="var(--font-hint)" margin="3dp 0 6dp 0" />
 <hbox lw="mp" lh="wc">
  <ProgressBar class="operation_progress" lw="0" layout_weight="1" lh="12dp"
   layout_gravity="center_vertical" display-percent="false" />
  <TextView class="operation_percent" lw="44dp" lh="wc" text-align="right" />
 </hbox>
 <TextView class="operation_status" lw="mp" lh="wc" color="var(--font-hint)" margin-top="4dp" />
</vbox>
)xml",
												  mRowsBox );
		root->setId( "operation_" + String::toString( id ) );
		Row row{ id,
				 root,
				 root->findByClass<UITextView>( "operation_label" ),
				 root->findByClass<UITextView>( "operation_detail" ),
				 root->findByClass<UITextView>( "operation_status" ),
				 root->findByClass<UITextView>( "operation_percent" ),
				 root->findByClass<UIProgressBar>( "operation_progress" ),
				 root->findByClass<UIPushButton>( "operation_cancel" ),
				 {},
				 {} };
		row.cancel->setId( "operation_cancel_" + String::toString( id ) );
		row.cancel->onClick( [weak = weak_from_this(), id]( const MouseEvent* ) {
			if ( auto self = weak.lock(); self && self->isOpen() )
				self->mCancel( id );
		} );
		mRows.push_back( std::move( row ) );
		it = mRows.end() - 1;
		fit();
	}
	auto& row = *it;
	row.label->setText( label );
	row.label->setTooltipText( label );
	const bool source = !progress.currentSource.empty();
	const auto& uri = source ? progress.currentSource : progress.currentDestination;
	if ( row.showingSource != source || row.path != uri.getPath() ) {
		row.path = uri.getPath();
		row.showingSource = source;
		const auto path = Location::clipboardText( uri );
		row.detail->setText( path );
		row.detail->setTooltipText( path );
	}
	const bool known = progress.bytesTotal && *progress.bytesTotal > 0;
	const Float percent =
		known ? std::min( 100., 100. * progress.bytesDone / *progress.bytesTotal ) : 0;
	row.bar->setProgress( percent );
	const int rounded = known ? static_cast<int>( percent ) : -1;
	if ( row.percentage != rounded ) {
		row.percentage = rounded;
		row.percent->setText( known ? String::toString( rounded ) + '%' : "" );
	}
	row.cancel->setEnabled( !cancelling );
	if ( row.done == progress.bytesDone && row.total == progress.bytesTotal &&
		 row.cancelling == cancelling )
		return;
	row.done = progress.bytesDone;
	row.total = progress.bytesTotal;
	row.cancelling = cancelling;
	if ( cancelling ) {
		row.status->setText( mScene.i18n( "eefm_cancelling", "Cancelling..." ) );
	} else if ( !progress.bytesTotal ) {
		row.status->setText( mScene.i18n( "eefm_preparing_operation", "Preparing..." ) );
	} else if ( progress.bytesDone >= *progress.bytesTotal ) {
		row.status->setText( mScene.i18n( "eefm_finalizing_operation", "Finalizing..." ) );
	} else {
		row.status->setText(
			String::format( mScene.i18n( "eefm_operation_bytes", "%s of %s" ).toUtf8(),
							FileSystem::sizeToString( progress.bytesDone ),
							FileSystem::sizeToString( *progress.bytesTotal ) ) );
	}
}

void OperationsPopup::remove( Uint64 id ) {
	if ( !isOpen() )
		return;
	auto it =
		std::find_if( mRows.begin(), mRows.end(), [id]( const Row& row ) { return row.id == id; } );
	if ( it == mRows.end() )
		return;
	if ( it->root->hasFocusWithin() )
		mBox->setFocus();
	it->root->close();
	mRows.erase( it );
	fit();
}

} // namespace eefm
