#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_FREEBSD

#include "accessibilitybackendatspi.hpp"

#include <algorithm>
#include <eepp/window/input.hpp>

namespace EE { namespace UI { namespace AtSpi {

namespace {

bool refLess( AccessibilityNodeRef left, AccessibilityNodeRef right ) {
	return left.source < right.source || ( left.source == right.source && left.id < right.id );
}

} // namespace

const std::vector<AccessibilityNodeRef>&
AtSpiApplication::rememberSelection( AccessibilityNodeRef ref ) {
	auto path = pathFromRef( ref );
	auto found = mSelections.find( path );
	if ( found != mSelections.end() )
		return found->second;
	// Initialize once when a client first queries the container. Subsequent selection events
	// keep the snapshot current; state reads must not enumerate and sort it again.
	auto selected = mManager->getSelectedChildren( ref );
	std::sort( selected.begin(), selected.end(), refLess );
	return mSelections.emplace( std::move( path ), std::move( selected ) ).first->second;
}

void AtSpiApplication::sendSelectionChanged( AccessibilityNodeRef ref ) {
	auto found = mSelections.find( pathFromRef( ref ) );
	// An unqueried container only needs the query-again hint. Do not materialize selected
	// rows or allocate a snapshot that no client has asked to retain.
	if ( found == mSelections.end() ) {
		sendObjectEvent( ref, "SelectionChanged", "", 0 );
		return;
	}
	auto selected = mManager->getSelectedChildren( ref );
	std::sort( selected.begin(), selected.end(), refLess );
	auto& previous = found->second;
	// Diff only selected refs, never materialize every row just to update selected state.
	for ( const auto child : previous ) {
		if ( mManager->isValid( child ) &&
			 !std::binary_search( selected.begin(), selected.end(), child, refLess ) )
			sendObjectEvent( child, "StateChanged", "selected", 0 );
	}
	for ( const auto child : selected ) {
		if ( !std::binary_search( previous.begin(), previous.end(), child, refLess ) )
			sendObjectEvent( child, "StateChanged", "selected", 1 );
	}
	previous = std::move( selected );
	sendObjectEvent( ref, "SelectionChanged", "", 0 );
}

void AtSpiApplication::onEvent( AccessibilityManager& manager,
								const AccessibilityPendingEvent& event ) {
	if ( !isAvailable() )
		return;
	ScopedManager scopedManager( *this, &manager );
	if ( event.type == AccessibilityEvent::Destroyed && !event.related.isValid() ) {
		forgetText( manager, event.ref );
		if ( !mSelections.empty() )
			mSelections.erase( pathFromRef( event.ref ) );
		return;
	}
	if ( !hasActiveClients() )
		return;
	if ( event.type == AccessibilityEvent::ChildrenChanged ) {
		const auto role = manager.getNodeRole( event.ref );
		if ( role == AccessibilityRole::Heading || role == AccessibilityRole::Link )
			onEvent( manager, { event.ref, {}, -1, AccessibilityEvent::NameChanged } );
	}
	// Structural mutations can remove a selected child without a separate selection event.
	// Queries must then rebuild the snapshot from the new semantic membership.
	if ( !mSelections.empty() && ( event.type == AccessibilityEvent::ModelChanged ||
								   event.type == AccessibilityEvent::ChildrenChanged ||
								   event.type == AccessibilityEvent::Created ||
								   event.type == AccessibilityEvent::Destroyed ) )
		mSelections.erase( pathFromRef( event.ref ) );
	if ( event.type == AccessibilityEvent::SelectionChanged &&
		 isSelectionContainerRole( manager.getNodeRole( event.ref ) ) ) {
		sendSelectionChanged( event.ref );
		return;
	}
	AccessibilityNodeInfo info;
	if ( event.type == AccessibilityEvent::StateChanged ||
		 event.type == AccessibilityEvent::RoleChanged ) {
		// State notifications need no names, values, or text offsets.
		info.role = manager.getNodeRole( event.ref );
		info.states = manager.getNodeState( event.ref );
	} else if ( event.type == AccessibilityEvent::FocusChanged ||
				event.type == AccessibilityEvent::SelectionChanged ||
				event.type == AccessibilityEvent::EnabledChanged ||
				event.type == AccessibilityEvent::VisibilityChanged ||
				event.type == AccessibilityEvent::ValueChanged ) {
		info = manager.getNodeInfo( event.ref, false, false );
	}
	if ( info.text.valid ) {
		// Contents changes arrive as exact records through onTextChanged(). Caret and selection
		// changes are compared once per frame: one edit can move them several times.
		if ( event.type == AccessibilityEvent::ValueChanged )
			return;
		if ( event.type == AccessibilityEvent::SelectionChanged ) {
			queueText( manager, event.ref, false );
			return;
		}
		if ( event.type == AccessibilityEvent::FocusChanged &&
			 hasState( info.states, AccessibilityState::Focused ) )
			rememberText( event.ref, manager.getTextInfo( event.ref ) );
	}
	if ( event.type == AccessibilityEvent::SelectionChanged && !mSelections.empty() ) {
		// Some widgets report selection on the child itself (for example selectable tabs).
		// Its selected-state event below remains unchanged; discard any parent's cached answer
		// rather than serving its old selection if no container event follows.
		const auto parent = getParent( event.ref );
		if ( isSelectionContainerRole( manager.getNodeRole( parent ) ) )
			mSelections.erase( pathFromRef( parent ) );
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
	} else if ( event.type == AccessibilityEvent::RoleChanged ) {
		detail = "accessible-role";
	} else if ( event.type == AccessibilityEvent::StateChanged ) {
		signal = "StateChanged";
		if ( info.role == AccessibilityRole::TextBox ) {
			// AT-SPI expresses password protection through PASSWORD_TEXT, not a state bit.
			// Forget queued text/caret work and the old length when the exposed mode changes.
			forgetText( manager, event.ref );
			signal = "PropertyChange";
			detail = "accessible-role";
		} else if ( info.role == AccessibilityRole::CheckBox ||
					info.role == AccessibilityRole::RadioButton ||
					info.role == AccessibilityRole::CheckMenuItem ||
					info.role == AccessibilityRole::RadioMenuItem )
			detail = "checked";
		else if ( info.role == AccessibilityRole::ComboBox ||
				  info.role == AccessibilityRole::TreeItem )
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
	} else if ( event.type == AccessibilityEvent::RoleChanged ||
				( event.type == AccessibilityEvent::StateChanged &&
				  info.role == AccessibilityRole::TextBox ) ) {
		Uint32 nativeRole =
			hasState( info.states, AccessibilityState::Protected ) ? 40 : role( info.role );
		mDBus.messageIterOpenContainer( &iter, 'v', "u", &variant );
		appendBasic( variant, 'u', &nativeRole );
	} else {
		const char* empty = "";
		mDBus.messageIterOpenContainer( &iter, 'v', "s", &variant );
		appendBasic( variant, 's', &empty );
	}
	mDBus.messageIterCloseContainer( &iter, &variant );
	appendEventProperties( iter );
	send( message );
}

void AtSpiApplication::announce( AccessibilityManager& manager, const String& message,
								 AccessibilityLive priority ) {
	if ( !isAvailable() || !hasActiveClients() )
		return;
	ScopedManager scopedManager( *this, &manager );
	// object:announcement (at-spi2-core 2.46): detail1 is the AtspiLive politeness, 1 polite and
	// 2 assertive. Clients without support ignore the signal.
	sendObjectEvent( manager.getRoot(), "Announcement", "",
					 priority == AccessibilityLive::Assertive ? 2 : 1, 0, message );
}

AtSpiApplication::TextState& AtSpiApplication::rememberText( AccessibilityNodeRef ref,
															 const AccessibilityTextInfo& text ) {
	auto path = pathFromRef( ref );
	auto found = mTextStates.find( path );
	if ( found != mTextStates.end() )
		return found->second;
	// Counting characters scans line sizes only; the contents are never copied.
	return mTextStates
		.emplace( std::move( path ),
				  TextState{ text, mManager ? mManager->getTextLength( ref ) : -1 } )
		.first->second;
}

bool AtSpiApplication::onTextChanged( AccessibilityManager& manager, AccessibilityNodeRef ref,
									  const AccessibilityTextChange& change ) {
	if ( !isAvailable() || !hasActiveClients() )
		return false;
	if ( change.isWholeText() ) {
		queueText( manager, ref, true );
		return true;
	}
	ScopedManager scopedManager( *this, &manager );
	if ( !change.removed.empty() ) {
		sendObjectEvent( ref, "TextChanged", "delete", change.offset,
						 static_cast<Int32>( change.removed.size() ), change.removed );
	}
	if ( !change.inserted.empty() ) {
		sendObjectEvent( ref, "TextChanged", "insert", change.offset,
						 static_cast<Int32>( change.inserted.size() ), change.inserted );
	}
	auto found = mTextStates.find( pathFromRef( ref ) );
	if ( found != mTextStates.end() && found->second.length >= 0 ) {
		found->second.length += static_cast<Int32>( change.inserted.size() ) -
								static_cast<Int32>( change.removed.size() );
	}
	return true;
}

void AtSpiApplication::forgetText( AccessibilityManager& manager, AccessibilityNodeRef ref ) {
	if ( !mTextStates.empty() )
		mTextStates.erase( pathFromRef( ref ) );
	mPendingTexts.erase( std::remove_if( mPendingTexts.begin(), mPendingTexts.end(),
										 [&]( const PendingText& text ) {
											 return text.manager == &manager && text.ref == ref;
										 } ),
						 mPendingTexts.end() );
}

void AtSpiApplication::queueText( AccessibilityManager& manager, AccessibilityNodeRef ref,
								  bool wholeText ) {
	for ( auto& text : mPendingTexts ) {
		if ( text.manager == &manager && text.ref == ref ) {
			text.wholeText |= wholeText;
			return;
		}
	}
	mPendingTexts.push_back( { &manager, ref, wholeText } );
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
		if ( text.wholeText )
			sendTextReplaced( text.ref );
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
	auto path = pathFromRef( ref );
	auto found = mTextStates.find( path );
	if ( found == mTextStates.end() ) {
		// A client that never looked at this text has no caret position to compare against.
		rememberText( ref, text );
		return;
	}
	auto& previous = found->second.text;
	if ( previous.caretOffset != text.caretOffset )
		sendObjectEvent( ref, "TextCaretMoved", "", text.caretOffset );
	if ( previous.selectionStart != text.selectionStart ||
		 previous.selectionEnd != text.selectionEnd )
		sendObjectEvent( ref, "TextSelectionChanged", "", 0 );
	previous = text;
}

void AtSpiApplication::sendTextReplaced( AccessibilityNodeRef ref ) {
	// Short texts (form fields) carry their new contents; a document's would only flood speech.
	static constexpr Int32 MaxReplacedTextReported = 4096;
	const Int32 length = mManager ? mManager->getTextLength( ref ) : 0;
	auto found = mTextStates.find( pathFromRef( ref ) );
	if ( found == mTextStates.end() )
		return;
	if ( found->second.length > 0 )
		sendObjectEvent( ref, "TextChanged", "delete", 0, found->second.length );
	if ( length > 0 ) {
		sendObjectEvent( ref, "TextChanged", "insert", 0, length,
						 length <= MaxReplacedTextReported
							 ? mManager->getTextRange( ref, 0, length )
							 : String() );
	}
	found->second.length = length;
}

void AtSpiApplication::sendTextDiff( AccessibilityNodeRef ref, const String& previous,
									 const String& current ) {
	size_t prefix = 0;
	while ( prefix < previous.size() && prefix < current.size() &&
			previous[prefix] == current[prefix] )
		++prefix;
	size_t suffix = 0;
	while ( suffix < previous.size() - prefix && suffix < current.size() - prefix &&
			previous[previous.size() - suffix - 1] == current[current.size() - suffix - 1] )
		++suffix;
	const size_t removed = previous.size() - prefix - suffix;
	const size_t inserted = current.size() - prefix - suffix;
	if ( removed ) {
		sendObjectEvent( ref, "TextChanged", "delete", prefix, removed,
						 previous.substr( prefix, removed ) );
	}
	if ( inserted ) {
		sendObjectEvent( ref, "TextChanged", "insert", prefix, inserted,
						 current.substr( prefix, inserted ) );
	}
	auto found = mTextStates.find( pathFromRef( ref ) );
	if ( found != mTextStates.end() )
		found->second.length = static_cast<Int32>( current.size() );
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
