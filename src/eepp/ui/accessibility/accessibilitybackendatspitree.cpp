#include "accessibilitybackendatspi.hpp"

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_FREEBSD

#include <algorithm>
#include <cstdlib>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/window.hpp>

namespace EE { namespace UI { namespace AtSpi {

AccessibilityNodeRef AtSpiApplication::refFromPath( const char* path ) {
	if ( !path )
		return {};
	if ( std::strcmp( path, RootPath ) == 0 ) {
		mManager = mPrimaryManager.load( std::memory_order_acquire );
		return ApplicationRef;
	}
	if ( std::strncmp( path, NodePathPrefix, std::strlen( NodePathPrefix ) ) != 0 )
		return {};
	char* end = nullptr;
	const char* encoded = path + std::strlen( NodePathPrefix );
	Uint64 managerId = std::strtoull( encoded, &end, 10 );
	if ( !end || *end != '/' )
		return {};
	auto manager =
		std::find_if( mManagers.begin(), mManagers.end(),
					  [managerId]( const auto& entry ) { return entry.first == managerId; } );
	if ( manager == mManagers.end() )
		return {};
	char* sourceEnd = nullptr;
	Uint64 source = std::strtoull( end + 1, &sourceEnd, 10 );
	if ( !sourceEnd || *sourceEnd != '/' )
		return {};
	char* idEnd = nullptr;
	Uint64 id = std::strtoull( sourceEnd + 1, &idEnd, 10 );
	if ( !idEnd || *idEnd != '\0' )
		return {};
	mManager = manager->second;
	return { static_cast<AccessibilitySourceId>( source ), id };
}

Uint64 AtSpiApplication::currentManagerId() const {
	for ( const auto& entry : mManagers ) {
		if ( entry.second == mManager )
			return entry.first;
	}
	return 0;
}

AccessibilityNodeInfo AtSpiApplication::getNodeInfo( AccessibilityNodeRef ref,
													 bool includeValue ) const {
	if ( ref != ApplicationRef ) {
		auto info = mManager ? mManager->getNodeInfo( ref, includeValue, includeValue )
							 : AccessibilityNodeInfo{};
		if ( isSceneRoot( ref ) )
			info.role = AccessibilityRole::Window;
		return info;
	}
	AccessibilityNodeInfo info;
	info.role = AccessibilityRole::Application;
	info.states =
		AccessibilityState::Enabled | AccessibilityState::Visible | AccessibilityState::Showing;
	info.name = mName;
	return info;
}

AccessibilityNodeRef AtSpiApplication::getParent( AccessibilityNodeRef ref ) {
	if ( ref == ApplicationRef )
		return {};
	if ( isSceneRoot( ref ) )
		return ApplicationRef;
	return mManager ? mManager->getParent( ref ) : AccessibilityNodeRef{};
}

size_t AtSpiApplication::getChildCount( AccessibilityNodeRef ref ) {
	if ( ref == ApplicationRef )
		return mManagers.size();
	return mManager ? mManager->getChildCount( ref ) : 0;
}

AccessibilityNodeRef AtSpiApplication::getChild( AccessibilityNodeRef ref, size_t index ) {
	return mManager && ref != ApplicationRef ? mManager->getChild( ref, index )
											 : AccessibilityNodeRef{};
}

std::string AtSpiApplication::pathFromRef( AccessibilityNodeRef ref ) {
	if ( ref == ApplicationRef )
		return RootPath;
	const Uint64 managerId = currentManagerId();
	return std::string( NodePathPrefix ) + String::toString( managerId ) + "/" +
		   String::toString( ref.source ) + "/" + String::toString( ref.id );
}

Uint32 AtSpiApplication::role( AccessibilityRole role ) const {
	switch ( role ) {
		case AccessibilityRole::Application:
			return 75;
		case AccessibilityRole::Window:
			return 69;
		case AccessibilityRole::Dialog:
			return 16;
		case AccessibilityRole::Group:
		case AccessibilityRole::TabPanel:
			return 39;
		case AccessibilityRole::Button:
			return 43;
		case AccessibilityRole::CheckBox:
			return 7;
		case AccessibilityRole::RadioButton:
			return 44;
		case AccessibilityRole::Label:
		case AccessibilityRole::Text:
			return 29;
		case AccessibilityRole::TextBox:
			return 79;
		case AccessibilityRole::Image:
			return 27;
		case AccessibilityRole::ComboBox:
			return 11;
		case AccessibilityRole::Slider:
			return 51;
		case AccessibilityRole::SpinButton:
			return 52;
		case AccessibilityRole::ProgressBar:
			return 42;
		case AccessibilityRole::Tab:
			return 37;
		case AccessibilityRole::TabList:
			return 38;
		case AccessibilityRole::MenuBar:
			return 34;
		case AccessibilityRole::Menu:
			return 33;
		case AccessibilityRole::MenuItem:
			return 35;
		case AccessibilityRole::CheckMenuItem:
			return 8;
		case AccessibilityRole::RadioMenuItem:
			return 45;
		case AccessibilityRole::List:
			return 31;
		case AccessibilityRole::ListItem:
			return 32;
		case AccessibilityRole::Table:
			return 55;
		case AccessibilityRole::Row:
			return 90;
		case AccessibilityRole::Cell:
			return 56;
		case AccessibilityRole::Tree:
			return 65;
		case AccessibilityRole::TreeItem:
			return 91;
		default:
			return 67;
	}
}

const char* AtSpiApplication::roleName( AccessibilityRole role ) const {
	switch ( role ) {
		case AccessibilityRole::Application:
			return "application";
		case AccessibilityRole::Window:
			return "window";
		case AccessibilityRole::Dialog:
			return "dialog";
		case AccessibilityRole::Group:
		case AccessibilityRole::TabPanel:
			return "panel";
		case AccessibilityRole::Button:
			return "push button";
		case AccessibilityRole::CheckBox:
			return "check box";
		case AccessibilityRole::RadioButton:
			return "radio button";
		case AccessibilityRole::Label:
			return "label";
		case AccessibilityRole::Text:
			return "text";
		case AccessibilityRole::TextBox:
			return "entry";
		case AccessibilityRole::Image:
			return "image";
		case AccessibilityRole::ComboBox:
			return "combo box";
		case AccessibilityRole::Slider:
			return "slider";
		case AccessibilityRole::SpinButton:
			return "spin button";
		case AccessibilityRole::ProgressBar:
			return "progress bar";
		case AccessibilityRole::Tab:
			return "page tab";
		case AccessibilityRole::TabList:
			return "page tab list";
		case AccessibilityRole::MenuBar:
			return "menu bar";
		case AccessibilityRole::Menu:
			return "menu";
		case AccessibilityRole::MenuItem:
			return "menu item";
		case AccessibilityRole::CheckMenuItem:
			return "check menu item";
		case AccessibilityRole::RadioMenuItem:
			return "radio menu item";
		case AccessibilityRole::List:
			return "list";
		case AccessibilityRole::ListItem:
			return "list item";
		case AccessibilityRole::Table:
			return "table";
		case AccessibilityRole::Row:
			return "table row";
		case AccessibilityRole::Cell:
			return "table cell";
		case AccessibilityRole::Tree:
			return "tree";
		case AccessibilityRole::TreeItem:
			return "tree item";
		default:
			return "unknown";
	}
}

AccessibilityAction AtSpiApplication::actionAt( AccessibilityActions actions, Int32 index ) const {
	for ( Uint32 action = 0; action <= static_cast<Uint32>( AccessibilityAction::ScrollTo );
		  ++action ) {
		if ( actions & ( 1u << action ) ) {
			if ( index-- == 0 )
				return static_cast<AccessibilityAction>( action );
		}
	}
	return AccessibilityAction::Focus;
}

AccessibilityActions AtSpiApplication::nativeActions( AccessibilityActions actions ) const {
	return actions & ~( accessibilityActionMask( AccessibilityAction::SetValue ) |
						accessibilityActionMask( AccessibilityAction::SetText ) |
						accessibilityActionMask( AccessibilityAction::SetTextSelection ) );
}

const char* AtSpiApplication::actionName( AccessibilityAction action ) const {
	switch ( action ) {
		case AccessibilityAction::Focus:
			return "focus";
		case AccessibilityAction::Press:
			return "click";
		case AccessibilityAction::Toggle:
			return "toggle";
		case AccessibilityAction::Select:
			return "select";
		case AccessibilityAction::Increment:
			return "increment";
		case AccessibilityAction::Decrement:
			return "decrement";
		case AccessibilityAction::SetValue:
			return "set value";
		case AccessibilityAction::SetText:
			return "set text";
		case AccessibilityAction::Expand:
			return "expand";
		case AccessibilityAction::Collapse:
			return "collapse";
		case AccessibilityAction::ScrollTo:
			return "scroll to";
		case AccessibilityAction::SetTextSelection:
			return "set text selection";
	}
	return "";
}

bool AtSpiApplication::readPropertySet( DBusMessage* request, const char*& interface,
										const char*& property, DBusMessageIter& variant ) {
	DBusMessageIter iter;
	if ( !mDBus.messageIterInit( request, &iter ) || mDBus.messageIterGetArgType( &iter ) != 's' )
		return false;
	mDBus.messageIterGetBasic( &iter, &interface );
	if ( !mDBus.messageIterNext( &iter ) || mDBus.messageIterGetArgType( &iter ) != 's' )
		return false;
	mDBus.messageIterGetBasic( &iter, &property );
	if ( !mDBus.messageIterNext( &iter ) || mDBus.messageIterGetArgType( &iter ) != 'v' )
		return false;
	mDBus.messageIterRecurse( &iter, &variant );
	return interface && property;
}

void AtSpiApplication::appendRef( DBusMessageIter& iter, AccessibilityNodeRef ref ) {
	DBusMessageIter structure;
	mDBus.messageIterOpenContainer( &iter, 'r', nullptr, &structure );
	const bool valid = ref == ApplicationRef || ref.isValid();
	const char* bus = valid ? mBusName.c_str() : "";
	std::string pathStorage = valid ? pathFromRef( ref ) : "/org/a11y/atspi/null";
	const char* path = pathStorage.c_str();
	appendBasic( structure, 's', &bus );
	appendBasic( structure, 'o', &path );
	mDBus.messageIterCloseContainer( &iter, &structure );
}

void AtSpiApplication::appendEventProperties( DBusMessageIter& iter ) {
	DBusMessageIter array;
	mDBus.messageIterOpenContainer( &iter, 'a', "{sv}", &array );
	mDBus.messageIterCloseContainer( &iter, &array );
}

void AtSpiApplication::appendDesktopRef( DBusMessageIter& iter ) {
	DBusMessageIter structure;
	mDBus.messageIterOpenContainer( &iter, 'r', nullptr, &structure );
	const char* bus = mRegistryBusName.c_str();
	const char* path = mRegistryPath.c_str();
	appendBasic( structure, 's', &bus );
	appendBasic( structure, 'o', &path );
	mDBus.messageIterCloseContainer( &iter, &structure );
}

void AtSpiApplication::appendInterfaces( DBusMessageIter& iter, AccessibilityNodeRef ref,
										 const AccessibilityNodeInfo& info ) {
	DBusMessageIter array;
	mDBus.messageIterOpenContainer( &iter, 'a', "s", &array );
	const char* accessible = "org.a11y.atspi.Accessible";
	const char* component = "org.a11y.atspi.Component";
	appendBasic( array, 's', &accessible );
	appendBasic( array, 's', &component );
	if ( nativeActions( info.actions ) ) {
		const char* action = "org.a11y.atspi.Action";
		appendBasic( array, 's', &action );
	}
	if ( info.range.valid ) {
		const char* value = "org.a11y.atspi.Value";
		appendBasic( array, 's', &value );
	}
	if ( info.text.valid ) {
		const char* text = "org.a11y.atspi.Text";
		appendBasic( array, 's', &text );
		if ( hasState( info.states, AccessibilityState::Editable ) ) {
			const char* editableText = "org.a11y.atspi.EditableText";
			appendBasic( array, 's', &editableText );
		}
	}
	if ( ref == ApplicationRef ) {
		const char* application = "org.a11y.atspi.Application";
		appendBasic( array, 's', &application );
	}
	mDBus.messageIterCloseContainer( &iter, &array );
}

void AtSpiApplication::appendStates( DBusMessageIter& iter, const AccessibilityNodeInfo& info ) {
	Uint32 words[2]{};
	auto addState = [&words]( bool enabled, Uint32 state ) {
		if ( enabled )
			words[state / 32] |= 1u << ( state % 32 );
	};
	Uint64 stateBits = static_cast<Uint64>( info.states );
	addState( stateBits & static_cast<Uint64>( AccessibilityState::Active ), 1 );
	addState( stateBits & static_cast<Uint64>( AccessibilityState::Checked ), 4 );
	addState( stateBits & static_cast<Uint64>( AccessibilityState::Editable ), 7 );
	addState( stateBits & static_cast<Uint64>( AccessibilityState::Enabled ), 8 );
	addState( info.actions & ( accessibilityActionMask( AccessibilityAction::Expand ) |
							   accessibilityActionMask( AccessibilityAction::Collapse ) ),
			  9 );
	addState( stateBits & static_cast<Uint64>( AccessibilityState::Expanded ), 10 );
	addState( stateBits & static_cast<Uint64>( AccessibilityState::Focusable ), 11 );
	addState( stateBits & static_cast<Uint64>( AccessibilityState::Focused ), 12 );
	addState( stateBits & static_cast<Uint64>( AccessibilityState::Selected ), 23 );
	addState( stateBits & static_cast<Uint64>( AccessibilityState::Enabled ), 24 );
	addState( stateBits & static_cast<Uint64>( AccessibilityState::Showing ), 25 );
	addState( stateBits & static_cast<Uint64>( AccessibilityState::Visible ), 30 );
	addState( info.actions & accessibilityActionMask( AccessibilityAction::Select ), 22 );
	addState( info.role == AccessibilityRole::CheckBox ||
				  info.role == AccessibilityRole::RadioButton ||
				  info.role == AccessibilityRole::CheckMenuItem ||
				  info.role == AccessibilityRole::RadioMenuItem,
			  41 );
	addState( stateBits & static_cast<Uint64>( AccessibilityState::ReadOnly ), 43 );
	addState( info.role == AccessibilityRole::TextBox &&
				  hasState( info.states, AccessibilityState::MultiLine ),
			  17 );
	addState( info.role == AccessibilityRole::TextBox &&
				  !hasState( info.states, AccessibilityState::MultiLine ),
			  26 );
	addState( info.text.valid, 38 );
	DBusMessageIter array;
	mDBus.messageIterOpenContainer( &iter, 'a', "u", &array );
	appendBasic( array, 'u', &words[0] );
	appendBasic( array, 'u', &words[1] );
	mDBus.messageIterCloseContainer( &iter, &array );
}

Int32 AtSpiApplication::indexInParent( AccessibilityNodeRef ref ) {
	if ( isSceneRoot( ref ) ) {
		for ( size_t i = 0; i < mManagers.size(); ++i ) {
			if ( mManagers[i].second == mManager )
				return static_cast<Int32>( i );
		}
		return -1;
	}
	return mManager ? mManager->getIndexInParent( ref ) : -1;
}

Math::Rectf AtSpiApplication::boundsForCoordinateType( AccessibilityNodeRef ref,
													   const AccessibilityNodeInfo& info,
													   Uint32 coordinateType ) {
	auto bounds = info.bounds;
	if ( coordinateType == 0 ) {
		auto scene = mManager ? mManager->getSceneNode() : nullptr;
		if ( scene && scene->getWindow() ) {
			auto position = scene->getWindow()->getPosition();
			bounds.move( Math::Vector2f( position.x, position.y ) );
		}
	} else if ( coordinateType == 2 ) {
		auto parent = getParent( ref );
		if ( parent.isValid() ) {
			auto parentBounds = getNodeInfo( parent, false ).bounds;
			bounds.move( Math::Vector2f( -parentBounds.Left, -parentBounds.Top ) );
		}
	}
	return bounds;
}

}}} // namespace EE::UI::AtSpi

#endif
