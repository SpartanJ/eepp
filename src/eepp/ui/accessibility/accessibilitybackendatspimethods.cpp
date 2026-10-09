#include "accessibilitybackendatspi.hpp"

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_FREEBSD

#include <algorithm>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/window.hpp>
#include <unistd.h>

namespace EE { namespace UI { namespace AtSpi {

DBusHandlerResult AtSpiApplication::handleMessage( DBusMessage* request ) {
	const char* path = mDBus.messageGetPath( request );
	const char* interface = mDBus.messageGetInterface( request );
	const char* member = mDBus.messageGetMember( request );
	if ( !interface || !member )
		return 1;
	if ( path && std::strcmp( path, CachePath ) == 0 )
		return handleCache( request, interface, member );
	auto ref = refFromPath( path );
	if ( !isValid( ref ) )
		return 1;
	if ( std::strcmp( interface, "org.a11y.atspi.Accessible" ) == 0 )
		return handleAccessible( request, ref, member );
	if ( std::strcmp( interface, "org.a11y.atspi.Application" ) == 0 )
		return handleApplication( request, ref, member );
	if ( std::strcmp( interface, "org.a11y.atspi.Component" ) == 0 )
		return handleComponent( request, ref, member );
	if ( std::strcmp( interface, "org.a11y.atspi.Action" ) == 0 )
		return handleAction( request, ref, member );
	if ( std::strcmp( interface, "org.a11y.atspi.Text" ) == 0 )
		return handleText( request, ref, member );
	if ( std::strcmp( interface, "org.a11y.atspi.EditableText" ) == 0 )
		return handleEditableText( request, ref, member );
	if ( std::strcmp( interface, "org.freedesktop.DBus.Properties" ) == 0 ) {
		if ( std::strcmp( member, "Get" ) == 0 )
			return handlePropertiesGet( request, ref );
		if ( std::strcmp( member, "Set" ) == 0 )
			return handlePropertiesSet( request, ref );
	}
	return 1;
}

DBusHandlerResult AtSpiApplication::handleCache( DBusMessage* request, const char* interface,
												 const char* member ) {
	if ( std::strcmp( interface, "org.a11y.atspi.Cache" ) == 0 &&
		 std::strcmp( member, "GetItems" ) == 0 ) {
		DBusMessage* reply = mDBus.messageNewMethodReturn( request );
		DBusMessageIter iter;
		DBusMessageIter array;
		mDBus.messageIterInitAppend( reply, &iter );
		mDBus.messageIterOpenContainer( &iter, 'a', "((so)(so)a(so)assusau)", &array );
		mDBus.messageIterCloseContainer( &iter, &array );
		send( reply );
		return 0;
	}
	if ( std::strcmp( interface, "org.freedesktop.DBus.Properties" ) == 0 &&
		 std::strcmp( member, "Get" ) == 0 ) {
		const char* requestedInterface = nullptr;
		const char* property = nullptr;
		if ( !mDBus.messageGetArgs( request, nullptr, 's', &requestedInterface, 's', &property,
									0 ) ||
			 !requestedInterface || !property ||
			 std::strcmp( requestedInterface, "org.a11y.atspi.Cache" ) != 0 ||
			 std::strcmp( property, "version" ) != 0 )
			return 1;
		DBusMessage* reply = mDBus.messageNewMethodReturn( request );
		DBusMessageIter iter;
		DBusMessageIter variant;
		mDBus.messageIterInitAppend( reply, &iter );
		mDBus.messageIterOpenContainer( &iter, 'v', "u", &variant );
		Uint32 version = 1;
		appendBasic( variant, 'u', &version );
		mDBus.messageIterCloseContainer( &iter, &variant );
		send( reply );
		return 0;
	}
	return 1;
}

DBusHandlerResult AtSpiApplication::handleAccessible( DBusMessage* request,
													  AccessibilityNodeRef ref,
													  const char* member ) {
	if ( std::strcmp( member, "GetRole" ) == 0 ) {
		auto info = getNodeInfo( ref, false );
		Uint32 value =
			hasState( info.states, AccessibilityState::Protected ) ? 40 : role( info.role );
		sendBasic( request, 'u', &value );
	} else if ( std::strcmp( member, "GetRoleName" ) == 0 ||
				std::strcmp( member, "GetLocalizedRoleName" ) == 0 ) {
		auto info = getNodeInfo( ref, false );
		const char* value = hasState( info.states, AccessibilityState::Protected )
								? "password text"
								: roleName( info.role );
		sendBasic( request, 's', &value );
	} else if ( std::strcmp( member, "GetChildAtIndex" ) == 0 ) {
		Int32 index = -1;
		mDBus.messageGetArgs( request, nullptr, 'i', &index, 0 );
		DBusMessage* reply = mDBus.messageNewMethodReturn( request );
		DBusMessageIter iter;
		mDBus.messageIterInitAppend( reply, &iter );
		if ( ref == ApplicationRef && index >= 0 &&
			 static_cast<size_t>( index ) < mManagers.size() ) {
			auto manager = std::next( mManagers.begin(), index )->second;
			ScopedManager scopedManager( *this, manager );
			appendRef( iter, manager->getRoot() );
		} else {
			appendRef( iter, index >= 0 ? getChild( ref, index ) : AccessibilityNodeRef{} );
		}
		send( reply );
	} else if ( std::strcmp( member, "GetChildren" ) == 0 ) {
		DBusMessage* reply = mDBus.messageNewMethodReturn( request );
		DBusMessageIter iter;
		DBusMessageIter array;
		mDBus.messageIterInitAppend( reply, &iter );
		mDBus.messageIterOpenContainer( &iter, 'a', "(so)", &array );
		if ( ref == ApplicationRef ) {
			for ( const auto& entry : mManagers ) {
				ScopedManager scopedManager( *this, entry.second );
				appendRef( array, entry.second->getRoot() );
			}
		} else {
			for ( const auto& child : mManager->getChildren( ref ) )
				appendRef( array, child );
		}
		mDBus.messageIterCloseContainer( &iter, &array );
		send( reply );
	} else if ( std::strcmp( member, "GetIndexInParent" ) == 0 ) {
		Int32 index = indexInParent( ref );
		sendBasic( request, 'i', &index );
	} else if ( std::strcmp( member, "GetApplication" ) == 0 ) {
		DBusMessage* reply = mDBus.messageNewMethodReturn( request );
		DBusMessageIter iter;
		mDBus.messageIterInitAppend( reply, &iter );
		appendApplicationRef( iter );
		send( reply );
	} else if ( std::strcmp( member, "GetInterfaces" ) == 0 ) {
		auto info = getNodeInfo( ref, false );
		DBusMessage* reply = mDBus.messageNewMethodReturn( request );
		DBusMessageIter iter;
		mDBus.messageIterInitAppend( reply, &iter );
		appendInterfaces( iter, ref, info );
		send( reply );
	} else if ( std::strcmp( member, "GetState" ) == 0 ) {
		auto info = getNodeInfo( ref, false );
		DBusMessage* reply = mDBus.messageNewMethodReturn( request );
		DBusMessageIter iter;
		mDBus.messageIterInitAppend( reply, &iter );
		appendStates( iter, info );
		send( reply );
	} else if ( std::strcmp( member, "GetAttributes" ) == 0 ) {
		DBusMessage* reply = mDBus.messageNewMethodReturn( request );
		DBusMessageIter iter;
		DBusMessageIter attributes;
		mDBus.messageIterInitAppend( reply, &iter );
		mDBus.messageIterOpenContainer( &iter, 'a', "{ss}", &attributes );
		mDBus.messageIterCloseContainer( &iter, &attributes );
		send( reply );
	} else if ( std::strcmp( member, "GetRelationSet" ) == 0 ) {
		DBusMessage* reply = mDBus.messageNewMethodReturn( request );
		DBusMessageIter iter;
		DBusMessageIter relations;
		mDBus.messageIterInitAppend( reply, &iter );
		mDBus.messageIterOpenContainer( &iter, 'a', "(ua(so))", &relations );
		mDBus.messageIterCloseContainer( &iter, &relations );
		send( reply );
	} else if ( std::strcmp( member, "GetAccessibleId" ) == 0 ) {
		std::string valueStorage = pathFromRef( ref );
		const char* value = valueStorage.c_str();
		sendBasic( request, 's', &value );
	} else if ( std::strcmp( member, "GetLocale" ) == 0 ) {
		const char* locale = "C";
		sendBasic( request, 's', &locale );
	} else {
		return 1;
	}
	return 0;
}

DBusHandlerResult AtSpiApplication::handleApplication( DBusMessage* request,
													   AccessibilityNodeRef ref,
													   const char* member ) {
	if ( std::strcmp( member, "GetLocale" ) == 0 ) {
		const char* locale = "C";
		sendBasic( request, 's', &locale );
	} else if ( std::strcmp( member, "GetApplicationBusAddress" ) == 0 ) {
		const char* address = "";
		sendBasic( request, 's', &address );
	} else {
		return 1;
	}
	return 0;
}

DBusHandlerResult AtSpiApplication::handleComponent( DBusMessage* request, AccessibilityNodeRef ref,
													 const char* member ) {
	if ( std::strcmp( member, "GetExtents" ) == 0 ) {
		auto info = getNodeInfo( ref, false );
		Uint32 coordinateType = 0;
		mDBus.messageGetArgs( request, nullptr, 'u', &coordinateType, 0 );
		auto bounds = boundsForCoordinateType( ref, info, coordinateType );
		DBusMessage* reply = mDBus.messageNewMethodReturn( request );
		DBusMessageIter iter;
		DBusMessageIter structure;
		mDBus.messageIterInitAppend( reply, &iter );
		mDBus.messageIterOpenContainer( &iter, 'r', nullptr, &structure );
		Int32 values[] = { static_cast<Int32>( bounds.Left ), static_cast<Int32>( bounds.Top ),
						   static_cast<Int32>( bounds.getWidth() ),
						   static_cast<Int32>( bounds.getHeight() ) };
		for ( auto value : values )
			appendBasic( structure, 'i', &value );
		mDBus.messageIterCloseContainer( &iter, &structure );
		send( reply );
	} else if ( std::strcmp( member, "Contains" ) == 0 ) {
		auto info = getNodeInfo( ref, false );
		Int32 x = 0;
		Int32 y = 0;
		Uint32 coordinateType = 0;
		mDBus.messageGetArgs( request, nullptr, 'i', &x, 'i', &y, 'u', &coordinateType, 0 );
		int contains =
			boundsForCoordinateType( ref, info, coordinateType ).contains( Math::Vector2f( x, y ) );
		sendBasic( request, 'b', &contains );
	} else if ( std::strcmp( member, "GetAccessibleAtPoint" ) == 0 ) {
		Int32 x = 0;
		Int32 y = 0;
		Uint32 coordinateType = 0;
		mDBus.messageGetArgs( request, nullptr, 'i', &x, 'i', &y, 'u', &coordinateType, 0 );
		if ( coordinateType == 0 ) {
			auto scene = mManager ? mManager->getSceneNode() : nullptr;
			if ( scene && scene->getWindow() ) {
				auto position = scene->getWindow()->getPosition();
				x -= position.x;
				y -= position.y;
			}
		} else if ( coordinateType == 2 ) {
			auto parent = getNodeInfo( getParent( ref ), false );
			x += static_cast<Int32>( parent.bounds.Left );
			y += static_cast<Int32>( parent.bounds.Top );
		}
		DBusMessage* reply = mDBus.messageNewMethodReturn( request );
		DBusMessageIter iter;
		mDBus.messageIterInitAppend( reply, &iter );
		appendRef( iter, mManager ? mManager->hitTest( Math::Vector2f( x, y ) )
								  : AccessibilityNodeRef{} );
		send( reply );
	} else if ( std::strcmp( member, "GrabFocus" ) == 0 ) {
		int success =
			mManager && mManager->performAction( ref, { AccessibilityAction::Focus, {} } );
		sendBasic( request, 'b', &success );
	} else if ( std::strcmp( member, "GetPosition" ) == 0 ) {
		auto info = getNodeInfo( ref, false );
		Uint32 coordinateType = 0;
		mDBus.messageGetArgs( request, nullptr, 'u', &coordinateType, 0 );
		auto bounds = boundsForCoordinateType( ref, info, coordinateType );
		DBusMessage* reply = mDBus.messageNewMethodReturn( request );
		DBusMessageIter iter;
		mDBus.messageIterInitAppend( reply, &iter );
		Int32 x = static_cast<Int32>( bounds.Left );
		Int32 y = static_cast<Int32>( bounds.Top );
		appendBasic( iter, 'i', &x );
		appendBasic( iter, 'i', &y );
		send( reply );
	} else if ( std::strcmp( member, "GetSize" ) == 0 ) {
		auto info = getNodeInfo( ref, false );
		DBusMessage* reply = mDBus.messageNewMethodReturn( request );
		DBusMessageIter iter;
		mDBus.messageIterInitAppend( reply, &iter );
		Int32 width = static_cast<Int32>( info.bounds.getWidth() );
		Int32 height = static_cast<Int32>( info.bounds.getHeight() );
		appendBasic( iter, 'i', &width );
		appendBasic( iter, 'i', &height );
		send( reply );
	} else if ( std::strcmp( member, "GetLayer" ) == 0 ) {
		Uint32 layer = 1;
		sendBasic( request, 'u', &layer );
	} else if ( std::strcmp( member, "GetMDIZOrder" ) == 0 ) {
		Int16 order = 0;
		sendBasic( request, 'n', &order );
	} else if ( std::strcmp( member, "GetAlpha" ) == 0 ) {
		double alpha = 1;
		sendBasic( request, 'd', &alpha );
	} else {
		return 1;
	}
	return 0;
}

DBusHandlerResult AtSpiApplication::handleAction( DBusMessage* request, AccessibilityNodeRef ref,
												  const char* member ) {
	auto info = getNodeInfo( ref, false );
	const auto actions = nativeActions( info.actions );
	Int32 count = static_cast<Int32>( __builtin_popcount( actions ) );
	if ( std::strcmp( member, "GetNActions" ) == 0 ) {
		sendBasic( request, 'i', &count );
	} else if ( std::strcmp( member, "GetActions" ) == 0 ) {
		DBusMessage* reply = mDBus.messageNewMethodReturn( request );
		DBusMessageIter iter;
		DBusMessageIter array;
		mDBus.messageIterInitAppend( reply, &iter );
		mDBus.messageIterOpenContainer( &iter, 'a', "(sss)", &array );
		for ( Int32 index = 0; index < count; ++index ) {
			DBusMessageIter structure;
			auto action = actionAt( actions, index );
			const char* name = actionName( action );
			const char* empty = "";
			mDBus.messageIterOpenContainer( &array, 'r', nullptr, &structure );
			appendBasic( structure, 's', &name );
			appendBasic( structure, 's', &empty );
			appendBasic( structure, 's', &empty );
			mDBus.messageIterCloseContainer( &array, &structure );
		}
		mDBus.messageIterCloseContainer( &iter, &array );
		send( reply );
	} else {
		Int32 index = -1;
		mDBus.messageGetArgs( request, nullptr, 'i', &index, 0 );
		if ( index < 0 || index >= count )
			return 1;
		auto action = actionAt( actions, index );
		if ( std::strcmp( member, "DoAction" ) == 0 ) {
			int success = mManager && mManager->performAction( ref, { action, {} } );
			sendBasic( request, 'b', &success );
		} else if ( std::strcmp( member, "GetName" ) == 0 ||
					std::strcmp( member, "GetLocalizedName" ) == 0 ) {
			const char* name = actionName( action );
			sendBasic( request, 's', &name );
		} else if ( std::strcmp( member, "GetDescription" ) == 0 ||
					std::strcmp( member, "GetKeyBinding" ) == 0 ) {
			const char* empty = "";
			sendBasic( request, 's', &empty );
		} else {
			return 1;
		}
	}
	return 0;
}

DBusHandlerResult AtSpiApplication::handleText( DBusMessage* request, AccessibilityNodeRef ref,
												const char* member ) {
	auto info = getNodeInfo( ref, true );
	if ( !info.text.valid )
		return 1;
	rememberText( ref, info, std::strcmp( member, "GetText" ) == 0 );
	const Int32 characterCount = static_cast<Int32>( info.value.size() );
	if ( std::strcmp( member, "GetText" ) == 0 ) {
		Int32 start = 0;
		Int32 end = -1;
		mDBus.messageGetArgs( request, nullptr, 'i', &start, 'i', &end, 0 );
		start = std::max( 0, std::min( start, characterCount ) );
		end = end < 0 ? characterCount : std::max( start, std::min( end, characterCount ) );
		std::string textStorage = info.value.substr( start, end - start ).toUtf8();
		const char* text = textStorage.c_str();
		sendBasic( request, 's', &text );
	} else if ( std::strcmp( member, "GetStringAtOffset" ) == 0 ) {
		Int32 offset = 0;
		Uint32 granularity = 0;
		mDBus.messageGetArgs( request, nullptr, 'i', &offset, 'u', &granularity, 0 );
		Int32 start = std::max( 0, std::min( offset, characterCount ) );
		Int32 end = start;
		if ( granularity == 0 && start < characterCount ) {
			end = start + 1;
		} else if ( granularity == 1 ) {
			auto isSpace = [&info]( Int32 index ) {
				auto character = info.value[index];
				return character == ' ' || character == '\t' || character == '\n' ||
					   character == '\r';
			};
			while ( start > 0 && !isSpace( start - 1 ) )
				--start;
			end = std::max( 0, std::min( offset, characterCount ) );
			while ( end < characterCount && !isSpace( end ) )
				++end;
		} else if ( granularity == 3 || granularity == 4 ) {
			while ( start > 0 && info.value[start - 1] != '\n' )
				--start;
			while ( end < characterCount && info.value[end] != '\n' )
				++end;
			if ( end < characterCount )
				++end;
		} else {
			start = 0;
			end = characterCount;
		}
		std::string textStorage = info.value.substr( start, end - start ).toUtf8();
		const char* text = textStorage.c_str();
		DBusMessage* reply = mDBus.messageNewMethodReturn( request );
		DBusMessageIter iter;
		mDBus.messageIterInitAppend( reply, &iter );
		appendBasic( iter, 's', &text );
		appendBasic( iter, 'i', &start );
		appendBasic( iter, 'i', &end );
		send( reply );
	} else if ( std::strcmp( member, "GetCharacterAtOffset" ) == 0 ) {
		Int32 offset = 0;
		mDBus.messageGetArgs( request, nullptr, 'i', &offset, 0 );
		Int32 character = offset >= 0 && offset < characterCount ? info.value[offset] : 0;
		sendBasic( request, 'i', &character );
	} else if ( std::strcmp( member, "GetAttributes" ) == 0 ) {
		DBusMessage* reply = mDBus.messageNewMethodReturn( request );
		DBusMessageIter iter;
		DBusMessageIter attributes;
		mDBus.messageIterInitAppend( reply, &iter );
		mDBus.messageIterOpenContainer( &iter, 'a', "{ss}", &attributes );
		mDBus.messageIterCloseContainer( &iter, &attributes );
		Int32 start = 0;
		appendBasic( iter, 'i', &start );
		appendBasic( iter, 'i', &characterCount );
		send( reply );
	} else if ( std::strcmp( member, "GetDefaultAttributes" ) == 0 ||
				std::strcmp( member, "GetDefaultAttributeSet" ) == 0 ) {
		DBusMessage* reply = mDBus.messageNewMethodReturn( request );
		DBusMessageIter iter;
		DBusMessageIter attributes;
		mDBus.messageIterInitAppend( reply, &iter );
		mDBus.messageIterOpenContainer( &iter, 'a', "{ss}", &attributes );
		mDBus.messageIterCloseContainer( &iter, &attributes );
		send( reply );
	} else if ( std::strcmp( member, "GetNSelections" ) == 0 ) {
		Int32 count = info.text.selectionStart != info.text.selectionEnd ? 1 : 0;
		sendBasic( request, 'i', &count );
	} else if ( std::strcmp( member, "GetSelection" ) == 0 ) {
		Int32 selection = -1;
		mDBus.messageGetArgs( request, nullptr, 'i', &selection, 0 );
		if ( selection != 0 || info.text.selectionStart == info.text.selectionEnd )
			return 1;
		DBusMessage* reply = mDBus.messageNewMethodReturn( request );
		DBusMessageIter iter;
		mDBus.messageIterInitAppend( reply, &iter );
		appendBasic( iter, 'i', &info.text.selectionStart );
		appendBasic( iter, 'i', &info.text.selectionEnd );
		send( reply );
	} else if ( std::strcmp( member, "SetCaretOffset" ) == 0 ) {
		Int32 offset = -1;
		mDBus.messageGetArgs( request, nullptr, 'i', &offset, 0 );
		int success = offset >= 0 && offset <= characterCount && mManager &&
					  mManager->performAction( ref, { AccessibilityAction::SetTextSelection,
													  String( String::toString( offset ) + ":" +
															  String::toString( offset ) ) } );
		sendBasic( request, 'b', &success );
	} else if ( std::strcmp( member, "SetSelection" ) == 0 ||
				std::strcmp( member, "AddSelection" ) == 0 ||
				std::strcmp( member, "RemoveSelection" ) == 0 ) {
		Int32 selection = 0;
		Int32 start = info.text.caretOffset;
		Int32 end = start;
		bool valid = std::strcmp( member, "AddSelection" ) == 0
						 ? mDBus.messageGetArgs( request, nullptr, 'i', &start, 'i', &end, 0 )
					 : std::strcmp( member, "SetSelection" ) == 0
						 ? mDBus.messageGetArgs( request, nullptr, 'i', &selection, 'i', &start,
												 'i', &end, 0 )
						 : mDBus.messageGetArgs( request, nullptr, 'i', &selection, 0 );
		int success = valid && selection == 0 && start >= 0 && end >= start &&
					  end <= characterCount && mManager &&
					  mManager->performAction( ref, { AccessibilityAction::SetTextSelection,
													  String( String::toString( start ) + ":" +
															  String::toString( end ) ) } );
		sendBasic( request, 'b', &success );
	} else {
		return 1;
	}
	return 0;
}

DBusHandlerResult AtSpiApplication::handleEditableText( DBusMessage* request,
														AccessibilityNodeRef ref,
														const char* member ) {
	auto info = getNodeInfo( ref, true );
	if ( !info.text.valid || !hasState( info.states, AccessibilityState::Editable ) ||
		 std::strcmp( member, "SetTextContents" ) != 0 )
		return 1;
	const char* contents = nullptr;
	if ( !mDBus.messageGetArgs( request, nullptr, 's', &contents, 0 ) )
		return 1;
	rememberText( ref, info, true );
	int success =
		mManager && mManager->performAction(
						ref, { AccessibilityAction::SetText,
							   String::fromUtf8( std::string_view( contents ? contents : "" ) ) } );
	sendBasic( request, 'b', &success );
	return 0;
}

DBusHandlerResult AtSpiApplication::handlePropertiesGet( DBusMessage* request,
														 AccessibilityNodeRef ref ) {
	const char* requestedInterface = nullptr;
	const char* property = nullptr;
	if ( !mDBus.messageGetArgs( request, nullptr, 's', &requestedInterface, 's', &property, 0 ) ||
		 !requestedInterface || !property )
		return 1;
	const bool includeValue = ( std::strcmp( requestedInterface, "org.a11y.atspi.Text" ) == 0 &&
								std::strcmp( property, "CharacterCount" ) == 0 ) ||
							  ( std::strcmp( requestedInterface, "org.a11y.atspi.Value" ) == 0 &&
								std::strcmp( property, "CurrentValue" ) == 0 );
	const bool needsInfo = std::strcmp( requestedInterface, "org.a11y.atspi.Action" ) == 0 ||
						   std::strcmp( requestedInterface, "org.a11y.atspi.Text" ) == 0 ||
						   std::strcmp( requestedInterface, "org.a11y.atspi.Value" ) == 0 ||
						   std::strcmp( property, "Name" ) == 0 ||
						   std::strcmp( property, "Description" ) == 0;
	AccessibilityNodeInfo info;
	if ( needsInfo )
		info = getNodeInfo( ref, includeValue );
	if ( mManager && std::strcmp( requestedInterface, "org.a11y.atspi.Text" ) == 0 &&
		 std::strcmp( property, "CaretOffset" ) == 0 )
		info.text = mManager->getTextInfo( ref );
	DBusMessage* reply = mDBus.messageNewMethodReturn( request );
	DBusMessageIter iter;
	DBusMessageIter variant;
	mDBus.messageIterInitAppend( reply, &iter );
	if ( std::strcmp( requestedInterface, "org.a11y.atspi.Action" ) == 0 &&
		 std::strcmp( property, "NActions" ) == 0 ) {
		Int32 value = static_cast<Int32>( __builtin_popcount( nativeActions( info.actions ) ) );
		mDBus.messageIterOpenContainer( &iter, 'v', "i", &variant );
		appendBasic( variant, 'i', &value );
	} else if ( std::strcmp( requestedInterface, "org.a11y.atspi.Text" ) == 0 && info.text.valid &&
				( std::strcmp( property, "CharacterCount" ) == 0 ||
				  std::strcmp( property, "CaretOffset" ) == 0 ) ) {
		Int32 value = std::strcmp( property, "CharacterCount" ) == 0
						  ? static_cast<Int32>( info.value.size() )
						  : info.text.caretOffset;
		mDBus.messageIterOpenContainer( &iter, 'v', "i", &variant );
		appendBasic( variant, 'i', &value );
	} else if ( std::strcmp( requestedInterface, "org.a11y.atspi.Value" ) == 0 &&
				info.range.valid ) {
		double value = 0;
		if ( std::strcmp( property, "CurrentValue" ) == 0 )
			String::fromString( value, info.value.toUtf8() );
		else if ( std::strcmp( property, "MaximumValue" ) == 0 )
			value = info.range.maximum;
		else if ( std::strcmp( property, "MinimumValue" ) == 0 )
			value = info.range.minimum;
		else if ( std::strcmp( property, "MinimumIncrement" ) == 0 )
			value = info.range.smallChange;
		else {
			mDBus.messageUnref( reply );
			return 1;
		}
		mDBus.messageIterOpenContainer( &iter, 'v', "d", &variant );
		appendBasic( variant, 'd', &value );
	} else if ( std::strcmp( property, "Name" ) == 0 ||
				std::strcmp( property, "Description" ) == 0 ) {
		std::string text =
			( std::strcmp( property, "Name" ) == 0 ? info.name : info.description ).toUtf8();
		const char* value = text.c_str();
		mDBus.messageIterOpenContainer( &iter, 'v', "s", &variant );
		appendBasic( variant, 's', &value );
	} else if ( std::strcmp( property, "HelpText" ) == 0 ) {
		const char* value = "";
		mDBus.messageIterOpenContainer( &iter, 'v', "s", &variant );
		appendBasic( variant, 's', &value );
	} else if ( std::strcmp( property, "Locale" ) == 0 ) {
		const char* value = "C";
		mDBus.messageIterOpenContainer( &iter, 'v', "s", &variant );
		appendBasic( variant, 's', &value );
	} else if ( std::strcmp( property, "AccessibleId" ) == 0 ) {
		std::string valueStorage = pathFromRef( ref );
		const char* value = valueStorage.c_str();
		mDBus.messageIterOpenContainer( &iter, 'v', "s", &variant );
		appendBasic( variant, 's', &value );
	} else if ( std::strcmp( property, "ChildCount" ) == 0 ) {
		Int32 value = static_cast<Int32>( getChildCount( ref ) );
		mDBus.messageIterOpenContainer( &iter, 'v', "i", &variant );
		appendBasic( variant, 'i', &value );
	} else if ( std::strcmp( property, "Parent" ) == 0 ) {
		mDBus.messageIterOpenContainer( &iter, 'v', "(so)", &variant );
		if ( ref == ApplicationRef )
			appendDesktopRef( variant );
		else
			appendRef( variant, getParent( ref ) );
	} else if ( std::strcmp( property, "ToolkitName" ) == 0 ||
				std::strcmp( property, "Version" ) == 0 ||
				std::strcmp( property, "AtspiVersion" ) == 0 ) {
		const char* value = std::strcmp( property, "ToolkitName" ) == 0 ? "eepp" : "0.1";
		mDBus.messageIterOpenContainer( &iter, 'v', "s", &variant );
		appendBasic( variant, 's', &value );
	} else if ( std::strcmp( property, "ProcessId" ) == 0 ) {
		Int32 value = static_cast<Int32>( getpid() );
		mDBus.messageIterOpenContainer( &iter, 'v', "i", &variant );
		appendBasic( variant, 'i', &value );
	} else if ( std::strcmp( property, "Id" ) == 0 ) {
		mDBus.messageIterOpenContainer( &iter, 'v', "i", &variant );
		appendBasic( variant, 'i', &mApplicationId );
	} else {
		mDBus.messageUnref( reply );
		return 1;
	}
	mDBus.messageIterCloseContainer( &iter, &variant );
	send( reply );
	return 0;
}

DBusHandlerResult AtSpiApplication::handlePropertiesSet( DBusMessage* request,
														 AccessibilityNodeRef ref ) {
	// The registry assigns the application id; every other writable property is Value.
	if ( ref == ApplicationRef ) {
		DBusMessageIter variant;
		const char* requestedInterface = nullptr;
		const char* property = nullptr;
		if ( !readPropertySet( request, requestedInterface, property, variant ) ||
			 mDBus.messageIterGetArgType( &variant ) != 'i' ||
			 std::strcmp( requestedInterface, "org.a11y.atspi.Application" ) != 0 ||
			 std::strcmp( property, "Id" ) != 0 )
			return 1;
		mDBus.messageIterGetBasic( &variant, &mApplicationId );
		send( mDBus.messageNewMethodReturn( request ) );
		return 0;
	}
	DBusMessageIter variant;
	const char* requestedInterface = nullptr;
	const char* property = nullptr;
	double value = 0;
	if ( !readPropertySet( request, requestedInterface, property, variant ) ||
		 mDBus.messageIterGetArgType( &variant ) != 'd' )
		return 1;
	mDBus.messageIterGetBasic( &variant, &value );
	if ( !requestedInterface || !property ||
		 std::strcmp( requestedInterface, "org.a11y.atspi.Value" ) != 0 ||
		 std::strcmp( property, "CurrentValue" ) != 0 )
		return 1;
	auto info = getNodeInfo( ref, false );
	if ( !info.range.valid ||
		 !( info.actions & accessibilityActionMask( AccessibilityAction::SetValue ) ) )
		return 1;
	if ( !mManager || !mManager->performAction( ref, { AccessibilityAction::SetValue,
													   String( String::toString( value ) ) } ) )
		return 1;
	send( mDBus.messageNewMethodReturn( request ) );
	return 0;
}

}}} // namespace EE::UI::AtSpi

#endif
