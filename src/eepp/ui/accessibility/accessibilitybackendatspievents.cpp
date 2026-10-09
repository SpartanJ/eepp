#include "accessibilitybackendatspi.hpp"

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_FREEBSD

#include <algorithm>
#include <eepp/window/input.hpp>

namespace EE { namespace UI { namespace AtSpi {

void AtSpiApplication::onEvent( AccessibilityManager& manager,
								const AccessibilityPendingEvent& event ) {
	if ( !isAvailable() )
		return;
	ScopedManager scopedManager( *this, &manager );
	if ( event.type == AccessibilityEvent::Destroyed && !event.related.isValid() ) {
		if ( !mTextSnapshots.empty() )
			mTextSnapshots.erase( pathFromRef( event.ref ) );
		mPendingTexts.erase( std::remove_if( mPendingTexts.begin(), mPendingTexts.end(),
											 [&]( const PendingText& text ) {
												 return text.manager == &manager &&
														text.ref == event.ref;
											 } ),
							 mPendingTexts.end() );
		return;
	}
	if ( !hasActiveClients() )
		return;
	AccessibilityNodeInfo info;
	if ( event.type == AccessibilityEvent::FocusChanged ||
		 event.type == AccessibilityEvent::SelectionChanged ||
		 event.type == AccessibilityEvent::StateChanged ||
		 event.type == AccessibilityEvent::EnabledChanged ||
		 event.type == AccessibilityEvent::VisibilityChanged ||
		 event.type == AccessibilityEvent::ValueChanged )
		info = manager.getNodeInfo( event.ref, false, false );
	if ( info.text.valid ) {
		// Diff text once per frame: a multi-cursor edit or replace-all emits one change per
		// edited range, and each diff copies and scans the whole document.
		if ( event.type == AccessibilityEvent::ValueChanged ||
			 event.type == AccessibilityEvent::SelectionChanged ) {
			queueText( manager, event.ref, event.type == AccessibilityEvent::ValueChanged );
			return;
		}
		if ( event.type == AccessibilityEvent::FocusChanged &&
			 hasState( info.states, AccessibilityState::Focused ) )
			rememberText( event.ref, manager.getNodeInfo( event.ref ) );
	}
	if ( event.type == AccessibilityEvent::EnabledChanged ) {
		sendObjectEvent( event.ref, "StateChanged", "sensitive",
						 hasState( info.states, AccessibilityState::Enabled ) );
	} else if ( event.type == AccessibilityEvent::VisibilityChanged ) {
		sendObjectEvent( event.ref, "StateChanged", "showing",
						 hasState( info.states, AccessibilityState::Showing ) );
	}
	const char* signal = "PropertyChange";
	const char* detail = "accessible-state";
	if ( event.type == AccessibilityEvent::FocusChanged ) {
		signal = "StateChanged";
		detail = "focused";
	} else if ( event.type == AccessibilityEvent::SelectionChanged ) {
		signal = "StateChanged";
		detail = "selected";
	} else if ( event.type == AccessibilityEvent::NameChanged ) {
		detail = "accessible-name";
	} else if ( event.type == AccessibilityEvent::DescriptionChanged ) {
		detail = "accessible-description";
	} else if ( event.type == AccessibilityEvent::ValueChanged ) {
		detail = "accessible-value";
	} else if ( event.type == AccessibilityEvent::StateChanged ) {
		signal = "StateChanged";
		if ( info.role == AccessibilityRole::CheckBox ||
			 info.role == AccessibilityRole::RadioButton ||
			 info.role == AccessibilityRole::CheckMenuItem ||
			 info.role == AccessibilityRole::RadioMenuItem )
			detail = "checked";
		else if ( info.role == AccessibilityRole::ComboBox )
			detail = "expanded";
		else
			return;
	} else if ( event.type == AccessibilityEvent::EnabledChanged ) {
		signal = "StateChanged";
		detail = "enabled";
	} else if ( event.type == AccessibilityEvent::VisibilityChanged ) {
		signal = "StateChanged";
		detail = "visible";
	} else if ( event.type == AccessibilityEvent::BoundsChanged ) {
		signal = "BoundsChanged";
		detail = "";
	} else if ( event.type == AccessibilityEvent::ChildrenChanged ||
				event.type == AccessibilityEvent::ModelChanged ) {
		signal = "ModelChanged";
		detail = "";
	} else if ( event.type == AccessibilityEvent::Created ||
				event.type == AccessibilityEvent::Destroyed ) {
		signal = "ChildrenChanged";
		detail = event.type == AccessibilityEvent::Destroyed ? "remove" : "add";
	}
	std::string pathStorage = pathFromRef( event.ref );
	DBusMessage* message =
		mDBus.messageNewSignal( pathStorage.c_str(), "org.a11y.atspi.Event.Object", signal );
	if ( !message )
		return;
	DBusMessageIter iter;
	DBusMessageIter variant;
	mDBus.messageIterInitAppend( message, &iter );
	appendBasic( iter, 's', &detail );
	Int32 detail1 = 0;
	if ( event.type == AccessibilityEvent::FocusChanged )
		detail1 = hasState( info.states, AccessibilityState::Focused );
	else if ( event.type == AccessibilityEvent::SelectionChanged )
		detail1 = hasState( info.states, AccessibilityState::Selected );
	else if ( event.type == AccessibilityEvent::StateChanged &&
			  std::strcmp( detail, "checked" ) == 0 )
		detail1 = hasState( info.states, AccessibilityState::Checked );
	else if ( event.type == AccessibilityEvent::StateChanged &&
			  std::strcmp( detail, "expanded" ) == 0 )
		detail1 = hasState( info.states, AccessibilityState::Expanded );
	else if ( event.type == AccessibilityEvent::EnabledChanged )
		detail1 = hasState( info.states, AccessibilityState::Enabled );
	else if ( event.type == AccessibilityEvent::VisibilityChanged )
		detail1 = hasState( info.states, AccessibilityState::Visible );
	else if ( event.type == AccessibilityEvent::Created ||
			  event.type == AccessibilityEvent::Destroyed )
		detail1 = event.index;
	Int32 detail2 = 0;
	appendBasic( iter, 'i', &detail1 );
	appendBasic( iter, 'i', &detail2 );
	bool childrenChanged =
		event.type == AccessibilityEvent::Created || event.type == AccessibilityEvent::Destroyed;
	if ( childrenChanged ) {
		mDBus.messageIterOpenContainer( &iter, 'v', "(so)", &variant );
		appendRef( variant, event.related );
	} else {
		const char* empty = "";
		mDBus.messageIterOpenContainer( &iter, 'v', "s", &variant );
		appendBasic( variant, 's', &empty );
	}
	mDBus.messageIterCloseContainer( &iter, &variant );
	appendEventProperties( iter );
	send( message );
}

void AtSpiApplication::rememberText( AccessibilityNodeRef ref, const AccessibilityNodeInfo& info,
									 bool replace ) {
	if ( !info.text.valid )
		return;
	auto path = pathFromRef( ref );
	auto found = mTextSnapshots.find( path );
	if ( found == mTextSnapshots.end() )
		mTextSnapshots.emplace( std::move( path ), TextSnapshot{ info.value, info.text } );
	else if ( replace && !hasPendingText( ref ) )
		found->second = TextSnapshot{ info.value, info.text };
}

bool AtSpiApplication::hasPendingText( AccessibilityNodeRef ref ) const {
	for ( const auto& text : mPendingTexts ) {
		if ( text.manager == mManager && text.ref == ref )
			return true;
	}
	return false;
}

void AtSpiApplication::queueText( AccessibilityManager& manager, AccessibilityNodeRef ref,
								  bool valueChanged ) {
	for ( auto& text : mPendingTexts ) {
		if ( text.manager == &manager && text.ref == ref ) {
			text.valueChanged |= valueChanged;
			return;
		}
	}
	mPendingTexts.push_back( { &manager, ref, valueChanged } );
	Window::Input::wakeUpEventLoop();
}

void AtSpiApplication::flushPendingTexts() {
	if ( mPendingTexts.empty() )
		return;
	auto pending = std::move( mPendingTexts );
	mPendingTexts.clear();
	if ( !hasActiveClients() )
		return;
	for ( const auto& text : pending ) {
		if ( !text.manager->isValid( text.ref ) )
			continue;
		ScopedManager scopedManager( *this, text.manager );
		if ( text.valueChanged )
			sendTextChanged( text.ref );
		else
			sendTextSelection( text.ref, text.manager->getTextInfo( text.ref ) );
	}
}

void AtSpiApplication::sendObjectEvent( AccessibilityNodeRef ref, const char* signal,
										const char* detail, Int32 offset, Int32 length,
										const String& text ) {
	auto path = pathFromRef( ref );
	auto* message = mDBus.messageNewSignal( path.c_str(), "org.a11y.atspi.Event.Object", signal );
	if ( !message )
		return;
	DBusMessageIter iter;
	DBusMessageIter variant;
	mDBus.messageIterInitAppend( message, &iter );
	appendBasic( iter, 's', &detail );
	appendBasic( iter, 'i', &offset );
	appendBasic( iter, 'i', &length );
	auto utf8 = text.toUtf8();
	const char* value = utf8.c_str();
	mDBus.messageIterOpenContainer( &iter, 'v', "s", &variant );
	appendBasic( variant, 's', &value );
	mDBus.messageIterCloseContainer( &iter, &variant );
	appendEventProperties( iter );
	send( message );
}

void AtSpiApplication::sendTextSelection( AccessibilityNodeRef ref,
										  const AccessibilityTextInfo& text ) {
	auto found = mTextSnapshots.find( pathFromRef( ref ) );
	if ( found == mTextSnapshots.end() ) {
		rememberText( ref, getNodeInfo( ref ) );
		found = mTextSnapshots.find( pathFromRef( ref ) );
	}
	if ( found == mTextSnapshots.end() )
		return;
	auto& previous = found->second.text;
	if ( previous.caretOffset != text.caretOffset )
		sendObjectEvent( ref, "TextCaretMoved", "", text.caretOffset );
	if ( previous.selectionStart != text.selectionStart ||
		 previous.selectionEnd != text.selectionEnd )
		sendObjectEvent( ref, "TextSelectionChanged", "", 0 );
	previous = text;
}

void AtSpiApplication::sendTextChanged( AccessibilityNodeRef ref ) {
	auto info = getNodeInfo( ref );
	auto path = pathFromRef( ref );
	auto found = mTextSnapshots.find( path );
	if ( found == mTextSnapshots.end() ) {
		// A client which has not queried or focused this text has no old contents to diff.
		rememberText( ref, info );
		return;
	}
	auto& previous = found->second.value;
	size_t prefix = 0;
	while ( prefix < previous.size() && prefix < info.value.size() &&
			previous[prefix] == info.value[prefix] )
		++prefix;
	size_t suffix = 0;
	while ( suffix < previous.size() - prefix && suffix < info.value.size() - prefix &&
			previous[previous.size() - suffix - 1] == info.value[info.value.size() - suffix - 1] )
		++suffix;
	const size_t removed = previous.size() - prefix - suffix;
	const size_t inserted = info.value.size() - prefix - suffix;
	if ( removed ) {
		sendObjectEvent( ref, "TextChanged", "delete", prefix, removed,
						 previous.substr( prefix, removed ) );
	}
	if ( inserted ) {
		sendObjectEvent( ref, "TextChanged", "insert", prefix, inserted,
						 info.value.substr( prefix, inserted ) );
	}
	previous = std::move( info.value );
	sendTextSelection( ref, info.text );
}

void AtSpiApplication::sendWindowChanged( AccessibilityManager& manager, bool added, Int32 index ) {
	ScopedManager scopedManager( *this, &manager );
	auto root = manager.getRoot();
	DBusMessage* message =
		mDBus.messageNewSignal( RootPath, "org.a11y.atspi.Event.Object", "ChildrenChanged" );
	if ( !message )
		return;
	DBusMessageIter iter;
	DBusMessageIter variant;
	mDBus.messageIterInitAppend( message, &iter );
	const char* detail = added ? "add" : "remove";
	appendBasic( iter, 's', &detail );
	Int32 detail2 = 0;
	appendBasic( iter, 'i', &index );
	appendBasic( iter, 'i', &detail2 );
	mDBus.messageIterOpenContainer( &iter, 'v', "(so)", &variant );
	appendRef( variant, root );
	mDBus.messageIterCloseContainer( &iter, &variant );
	appendEventProperties( iter );
	send( message );
}

}}} // namespace EE::UI::AtSpi

#endif
