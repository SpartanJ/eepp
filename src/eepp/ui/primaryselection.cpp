#include "primaryselection.hpp"

#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/engine.hpp>

namespace EE { namespace UI {

UIWidget* PrimarySelection::sWidget = nullptr;
const void* PrimarySelection::sOwner = nullptr;
Uint64 PrimarySelection::sSerial = 0;
SmallFunction<24> PrimarySelection::sWrite;
String PrimarySelection::sText;
std::string PrimarySelection::sUtf8;

bool PrimarySelection::queue( UIWidget& widget, const void* owner, SmallFunction<24> write ) {
	if ( !Engine::isMainThread() || !widget.getUISceneNode() || widget.isClosing() )
		return false;

	sWidget = &widget;
	sOwner = owner;
	sWrite = std::move( write );
	Clipboard::setPrimarySelectionFlushCallback( [] { flush(); } );
	widget.debounce(
		[serial = ++sSerial] {
			if ( serial == sSerial )
				flush();
		},
		Milliseconds( 100 ), String::hash( "eepp-primary-selection" ) );
	return true;
}

void PrimarySelection::cancel( const void* owner ) {
	if ( !Engine::isMainThread() )
		return;
	if ( sOwner == owner )
		resetPending();
	Clipboard::clearEmulatedPrimarySelection( owner );
}

void PrimarySelection::flush( const void* owner ) {
	if ( !Engine::isMainThread() || !sOwner || ( owner && sOwner != owner ) )
		return;

	const void* source = sOwner;
	auto* scene = sWidget->getUISceneNode();
	const bool closing = sWidget->isClosing();
	auto write = std::move( sWrite );
	resetPending();
	if ( !scene || closing ) {
		Clipboard::clearEmulatedPrimarySelection( source );
		return;
	}

	sText.clear();
	write();
	if ( sText.empty() ) {
		Clipboard::clearEmulatedPrimarySelection( source );
		return;
	}
	sText.toUtf8( sUtf8 );
	scene->getWindow()->getClipboard()->publishPrimarySelection( sUtf8, source );
}

std::string PrimarySelection::getPasteText( UIWidget& widget ) {
	return widget.getUISceneNode()->getWindow()->getClipboard()->getPrimarySelectionPasteText();
}

void PrimarySelection::resetPending() {
	sOwner = nullptr;
	sWidget = nullptr;
	sWrite.reset();
	Clipboard::setPrimarySelectionFlushCallback( nullptr );
}

}} // namespace EE::UI
