#include "accessibilitybackenduia.hpp"

#if EE_PLATFORM == EE_PLATFORM_WIN

namespace EE { namespace UI { namespace Uia {

size_t nextSourceCodePoint( const std::u16string& text, size_t index ) {
	if ( index >= text.size() )
		return text.size();
	const char16_t value = text[index++];
	if ( value >= 0xd800 && value <= 0xdbff && index < text.size() && text[index] >= 0xdc00 &&
		 text[index] <= 0xdfff )
		++index;
	return index;
}

size_t codePointToUTF16( const String& string, Int32 offset ) {
	const auto source = string.toUtf16();
	const size_t wanted = offset > 0 ? static_cast<size_t>( offset ) : 0;
	size_t sourceIndex = 0;
	size_t displayIndex = 0;
	for ( size_t codePoint = 0; sourceIndex < source.size() && codePoint < wanted; ++codePoint ) {
		const size_t next = nextSourceCodePoint( source, sourceIndex );
		if ( source[sourceIndex] == u'\n' &&
			 ( sourceIndex == 0 || source[sourceIndex - 1] != u'\r' ) )
			++displayIndex;
		displayIndex += next - sourceIndex;
		sourceIndex = next;
	}
	return displayIndex;
}

Int32 utf16ToCodePoint( const String& string, size_t offset ) {
	const auto source = string.toUtf16();
	size_t sourceIndex = 0;
	size_t displayIndex = 0;
	Int32 codePoints = 0;
	while ( sourceIndex < source.size() && displayIndex < offset ) {
		const size_t next = nextSourceCodePoint( source, sourceIndex );
		size_t displayLength = next - sourceIndex;
		if ( source[sourceIndex] == u'\n' &&
			 ( sourceIndex == 0 || source[sourceIndex - 1] != u'\r' ) )
			++displayLength;
		if ( displayIndex + displayLength > offset )
			break;
		displayIndex += displayLength;
		sourceIndex = next;
		++codePoints;
	}
	return codePoints;
}

std::u16string normalizedUTF16( const String& string ) {
	const auto source = string.toUtf16();
	std::u16string result;
	result.reserve( source.size() );
	for ( size_t index = 0; index < source.size(); ++index ) {
		if ( source[index] == u'\n' && ( index == 0 || source[index - 1] != u'\r' ) )
			result.push_back( u'\r' );
		result.push_back( source[index] );
	}
	return result;
}

HRESULT textSnapshot( const std::shared_ptr<UIAutomationProviderContext>& context,
					  AccessibilityNodeRef ref, TextSnapshot& snapshot ) {
	if ( !context || !context->isAlive() )
		return UIA_E_ELEMENTNOTAVAILABLE;
	return context->invoke(
		snapshot, [=]( AccessibilityManager& manager, TextSnapshot& output ) -> HRESULT {
			if ( !manager.isValid( ref ) )
				return UIA_E_ELEMENTNOTAVAILABLE;
			const auto info = manager.getNodeInfo( ref );
			if ( !info.text.valid || hasState( info.states, AccessibilityState::Protected ) )
				return UIA_E_INVALIDOPERATION;
			output.source = info.value;
			output.text = normalizedUTF16( info.value );
			output.actions = info.actions;
			output.selectionStart = static_cast<int>( codePointToUTF16(
				info.value, std::min( info.text.selectionStart, info.text.selectionEnd ) ) );
			output.selectionEnd = static_cast<int>( codePointToUTF16(
				info.value, std::max( info.text.selectionStart, info.text.selectionEnd ) ) );
			output.bounds = info.bounds;
			output.boundsValid = info.boundsValid;
			return S_OK;
		} );
}

bool isLowSurrogate( char16_t value ) {
	return value >= 0xdc00 && value <= 0xdfff;
}

bool isHighSurrogate( char16_t value ) {
	return value >= 0xd800 && value <= 0xdbff;
}

size_t nextCharacterBoundary( const std::u16string& text, size_t offset ) {
	if ( offset >= text.size() )
		return text.size();
	size_t next = offset + 1;
	if ( text[offset] == u'\r' && next < text.size() && text[next] == u'\n' )
		++next;
	else if ( isHighSurrogate( text[offset] ) && next < text.size() &&
			  isLowSurrogate( text[next] ) )
		++next;
	while ( next < text.size() ) {
		WORD type{};
		const wchar_t value = static_cast<wchar_t>( text[next] );
		if ( !GetStringTypeW( CT_CTYPE3, &value, 1, &type ) ||
			 !( type & ( C3_NONSPACING | C3_DIACRITIC | C3_VOWELMARK ) ) )
			break;
		++next;
	}
	return next;
}

size_t previousCharacterBoundary( const std::u16string& text, size_t offset ) {
	if ( offset == 0 )
		return 0;
	size_t previous = std::min( offset, text.size() ) - 1;
	if ( isLowSurrogate( text[previous] ) && previous > 0 && isHighSurrogate( text[previous - 1] ) )
		--previous;
	if ( text[previous] == u'\n' && previous > 0 && text[previous - 1] == u'\r' )
		--previous;
	while ( previous > 0 ) {
		WORD type{};
		const wchar_t value = static_cast<wchar_t>( text[previous] );
		if ( !GetStringTypeW( CT_CTYPE3, &value, 1, &type ) ||
			 !( type & ( C3_NONSPACING | C3_DIACRITIC | C3_VOWELMARK ) ) )
			break;
		previous = previousCharacterBoundary( text, previous );
	}
	return previous;
}

bool isTextSpace( char16_t value ) {
	return value == u'\r' || value == u'\n' || value == u'\t' || value == u' ';
}

std::pair<size_t, size_t> enclosingTextUnit( const std::u16string& text, size_t offset,
											 TextUnit unit ) {
	offset = std::min( offset, text.size() );
	if ( unit == TextUnit_Document || unit == TextUnit_Page || unit == TextUnit_Format )
		return { 0, text.size() };
	if ( unit == TextUnit_Character ) {
		if ( text.empty() )
			return { 0, 0 };
		if ( offset == text.size() )
			offset = previousCharacterBoundary( text, offset );
		return { offset, nextCharacterBoundary( text, offset ) };
	}
	if ( unit == TextUnit_Line || unit == TextUnit_Paragraph ) {
		size_t start = offset;
		while ( start > 0 && text[start - 1] != u'\n' )
			--start;
		size_t end = offset;
		while ( end < text.size() && text[end] != u'\n' )
			++end;
		if ( end < text.size() )
			++end;
		return { start, end };
	}
	if ( text.empty() )
		return { 0, 0 };
	if ( offset == text.size() )
		--offset;
	const bool space = isTextSpace( text[offset] );
	size_t start = offset;
	while ( start > 0 && isTextSpace( text[start - 1] ) == space )
		--start;
	size_t end = offset;
	while ( end < text.size() && isTextSpace( text[end] ) == space )
		++end;
	return { start, end };
}

size_t moveTextEndpoint( const std::u16string& text, size_t offset, TextUnit unit, int count,
						 int& moved ) {
	moved = 0;
	offset = std::min( offset, text.size() );
	if ( count == 0 )
		return offset;
	if ( unit == TextUnit_Document || unit == TextUnit_Page || unit == TextUnit_Format ) {
		const size_t target = count > 0 ? text.size() : 0;
		if ( target != offset )
			moved = count > 0 ? 1 : -1;
		return target;
	}
	const int direction = count > 0 ? 1 : -1;
	for ( int step = 0; step < std::abs( count ); ++step ) {
		size_t target = offset;
		if ( unit == TextUnit_Character )
			target = direction > 0 ? nextCharacterBoundary( text, offset )
								   : previousCharacterBoundary( text, offset );
		else if ( direction > 0 ) {
			const auto bounds = enclosingTextUnit( text, offset, unit );
			target = bounds.second;
		} else if ( offset > 0 ) {
			const auto bounds = enclosingTextUnit( text, offset - 1, unit );
			target = bounds.first;
		}
		if ( target == offset )
			break;
		offset = target;
		moved += direction;
	}
	return offset;
}

HRESULT STDMETHODCALLTYPE UIAutomationTextRange::QueryInterface( REFIID interfaceId,
																 void** object ) {
	if ( !object )
		return E_INVALIDARG;
	*object = nullptr;
	if ( interfaceId != __uuidof( IUnknown ) && interfaceId != __uuidof( ITextRangeProvider ) )
		return E_NOINTERFACE;
	*object = static_cast<ITextRangeProvider*>( this );
	AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE UIAutomationTextRange::Release() {
	const ULONG references = mReferences.fetch_sub( 1, std::memory_order_acq_rel ) - 1;
	if ( references == 0 )
		delete this;
	return references;
}

HRESULT STDMETHODCALLTYPE UIAutomationTextRange::Clone( ITextRangeProvider** range ) {
	if ( !range )
		return E_INVALIDARG;
	*range = nullptr;
	TextSnapshot snapshot;
	const HRESULT result = currentSnapshot( snapshot );
	if ( FAILED( result ) )
		return result;
	auto endpoints = normalizedEndpoints( snapshot.text.size() );
	*range = new UIAutomationTextRange( mContext, mRef, endpoints.first, endpoints.second );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationTextRange::Compare( ITextRangeProvider* range, BOOL* same ) {
	if ( !range || !same )
		return E_INVALIDARG;
	*same = FALSE;
	auto other = dynamic_cast<UIAutomationTextRange*>( range );
	if ( !other || other->mRef != mRef )
		return S_OK;
	const auto ours = endpoints();
	const auto theirs = other->endpoints();
	*same = ours == theirs ? TRUE : FALSE;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationTextRange::CompareEndpoints(
	TextPatternRangeEndpoint endpoint, ITextRangeProvider* targetRange,
	TextPatternRangeEndpoint targetEndpoint, int* comparison ) {
	if ( !targetRange || !comparison )
		return E_INVALIDARG;
	auto other = dynamic_cast<UIAutomationTextRange*>( targetRange );
	if ( !other || other->mRef != mRef )
		return E_INVALIDARG;
	const auto ours = endpoints();
	const auto theirs = other->endpoints();
	const int left = endpoint == TextPatternRangeEndpoint_Start ? ours.first : ours.second;
	const int right =
		targetEndpoint == TextPatternRangeEndpoint_Start ? theirs.first : theirs.second;
	*comparison = left < right ? -1 : left > right ? 1 : 0;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationTextRange::ExpandToEnclosingUnit( TextUnit unit ) {
	TextSnapshot snapshot;
	const HRESULT result = currentSnapshot( snapshot );
	if ( FAILED( result ) )
		return result;
	const auto current = normalizedEndpoints( snapshot.text.size() );
	const auto expanded = enclosingTextUnit( snapshot.text, current.first, unit );
	setEndpoints( static_cast<int>( expanded.first ), static_cast<int>( expanded.second ) );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationTextRange::FindAttribute( TEXTATTRIBUTEID, VARIANT, BOOL,
																ITextRangeProvider** range ) {
	if ( !range )
		return E_INVALIDARG;
	*range = nullptr;
	TextSnapshot snapshot;
	return currentSnapshot( snapshot );
}

HRESULT STDMETHODCALLTYPE UIAutomationTextRange::FindText( BSTR text, BOOL backward,
														   BOOL ignoreCase,
														   ITextRangeProvider** range ) {
	if ( !text || !range )
		return E_INVALIDARG;
	*range = nullptr;
	TextSnapshot snapshot;
	const HRESULT result = currentSnapshot( snapshot );
	if ( FAILED( result ) )
		return result;
	const auto current = normalizedEndpoints( snapshot.text.size() );
	const size_t rangeStart = static_cast<size_t>( current.first );
	const size_t rangeEnd = static_cast<size_t>( current.second );
	std::u16string needle( reinterpret_cast<const char16_t*>( text ), SysStringLen( text ) );
	if ( needle.empty() )
		return S_OK;
	auto equal = [=]( char16_t left, char16_t right ) {
		return ignoreCase ? std::towlower( static_cast<wchar_t>( left ) ) ==
								std::towlower( static_cast<wchar_t>( right ) )
						  : left == right;
	};
	size_t found = std::u16string::npos;
	if ( backward ) {
		for ( size_t candidate = rangeEnd; candidate-- > rangeStart; ) {
			if ( candidate + needle.size() <= rangeEnd &&
				 std::equal( needle.begin(), needle.end(), snapshot.text.begin() + candidate,
							 equal ) ) {
				found = candidate;
				break;
			}
		}
	} else {
		for ( size_t candidate = rangeStart; candidate + needle.size() <= rangeEnd; ++candidate ) {
			if ( std::equal( needle.begin(), needle.end(), snapshot.text.begin() + candidate,
							 equal ) ) {
				found = candidate;
				break;
			}
		}
	}
	if ( found != std::u16string::npos )
		*range = new UIAutomationTextRange( mContext, mRef, static_cast<int>( found ),
											static_cast<int>( found + needle.size() ) );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationTextRange::GetAttributeValue( TEXTATTRIBUTEID,
																	VARIANT* value ) {
	if ( !value )
		return E_INVALIDARG;
	VariantInit( value );
	TextSnapshot snapshot;
	const HRESULT result = currentSnapshot( snapshot );
	return FAILED( result ) ? result : setNotSupported( value );
}

HRESULT STDMETHODCALLTYPE UIAutomationTextRange::GetBoundingRectangles( SAFEARRAY** rectangles ) {
	if ( !rectangles )
		return E_INVALIDARG;
	*rectangles = nullptr;
	TextSnapshot snapshot;
	const HRESULT result = currentSnapshot( snapshot );
	if ( FAILED( result ) )
		return result;
	const auto current = normalizedEndpoints( snapshot.text.size() );
	const bool hasRectangle = current.first != current.second && snapshot.boundsValid &&
							  IsWindowVisible( mContext->window() ) &&
							  !IsIconic( mContext->window() );
	POINT origin{};
	if ( hasRectangle && !ClientToScreen( mContext->window(), &origin ) )
		return UIA_E_ELEMENTNOTAVAILABLE;
	*rectangles = SafeArrayCreateVector( VT_R8, 0, hasRectangle ? 4 : 0 );
	if ( !*rectangles )
		return E_OUTOFMEMORY;
	if ( !hasRectangle )
		return S_OK;
	double values[] = { origin.x + snapshot.bounds.Left, origin.y + snapshot.bounds.Top,
						snapshot.bounds.getWidth(), snapshot.bounds.getHeight() };
	for ( LONG index = 0; index < 4; ++index ) {
		const HRESULT result = SafeArrayPutElement( *rectangles, &index, &values[index] );
		if ( FAILED( result ) ) {
			SafeArrayDestroy( *rectangles );
			*rectangles = nullptr;
			return result;
		}
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationTextRange::GetText( int maxLength, BSTR* text ) {
	if ( !text || maxLength < -1 )
		return E_INVALIDARG;
	*text = nullptr;
	TextSnapshot snapshot;
	const HRESULT result = currentSnapshot( snapshot );
	if ( FAILED( result ) )
		return result;
	const auto current = normalizedEndpoints( snapshot.text.size() );
	size_t length = current.second - current.first;
	if ( maxLength >= 0 )
		length = std::min( length, static_cast<size_t>( maxLength ) );
	*text =
		SysAllocStringLen( reinterpret_cast<const wchar_t*>( snapshot.text.data() + current.first ),
						   static_cast<UINT>( length ) );
	return *text ? S_OK : E_OUTOFMEMORY;
}

HRESULT STDMETHODCALLTYPE UIAutomationTextRange::Move( TextUnit unit, int count, int* moved ) {
	if ( !moved )
		return E_INVALIDARG;
	*moved = 0;
	TextSnapshot snapshot;
	const HRESULT result = currentSnapshot( snapshot );
	if ( FAILED( result ) )
		return result;
	const auto current = normalizedEndpoints( snapshot.text.size() );
	int actual{};
	const size_t start = moveTextEndpoint( snapshot.text, current.first, unit, count, actual );
	if ( current.first == current.second )
		setEndpoints( static_cast<int>( start ), static_cast<int>( start ) );
	else {
		const auto expanded = enclosingTextUnit( snapshot.text, start, unit );
		setEndpoints( static_cast<int>( expanded.first ), static_cast<int>( expanded.second ) );
	}
	*moved = actual;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationTextRange::MoveEndpointByUnit(
	TextPatternRangeEndpoint endpoint, TextUnit unit, int count, int* moved ) {
	if ( !moved )
		return E_INVALIDARG;
	*moved = 0;
	TextSnapshot snapshot;
	const HRESULT result = currentSnapshot( snapshot );
	if ( FAILED( result ) )
		return result;
	auto current = normalizedEndpoints( snapshot.text.size() );
	int actual{};
	const size_t target = moveTextEndpoint(
		snapshot.text, endpoint == TextPatternRangeEndpoint_Start ? current.first : current.second,
		unit, count, actual );
	if ( endpoint == TextPatternRangeEndpoint_Start ) {
		current.first = static_cast<int>( target );
		if ( current.first > current.second )
			current.second = current.first;
	} else {
		current.second = static_cast<int>( target );
		if ( current.second < current.first )
			current.first = current.second;
	}
	setEndpoints( current.first, current.second );
	*moved = actual;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationTextRange::MoveEndpointByRange(
	TextPatternRangeEndpoint endpoint, ITextRangeProvider* targetRange,
	TextPatternRangeEndpoint targetEndpoint ) {
	if ( !targetRange )
		return E_INVALIDARG;
	auto other = dynamic_cast<UIAutomationTextRange*>( targetRange );
	if ( !other || other->mRef != mRef )
		return E_INVALIDARG;
	auto current = endpoints();
	const auto target = other->endpoints();
	const int position =
		targetEndpoint == TextPatternRangeEndpoint_Start ? target.first : target.second;
	if ( endpoint == TextPatternRangeEndpoint_Start ) {
		current.first = position;
		if ( current.first > current.second )
			current.second = current.first;
	} else {
		current.second = position;
		if ( current.second < current.first )
			current.first = current.second;
	}
	setEndpoints( current.first, current.second );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE UIAutomationTextRange::Select() {
	TextSnapshot snapshot;
	const HRESULT result = currentSnapshot( snapshot );
	if ( FAILED( result ) )
		return result;
	if ( !hasAction( snapshot.actions, AccessibilityAction::SetTextSelection ) )
		return UIA_E_INVALIDOPERATION;
	const auto current = normalizedEndpoints( snapshot.text.size() );
	const Int32 start = utf16ToCodePoint( snapshot.source, current.first );
	const Int32 end = utf16ToCodePoint( snapshot.source, current.second );
	bool ignored{};
	return invoke( ignored, [this, start, end]( AccessibilityManager& manager, bool& ) -> HRESULT {
		if ( !manager.isValid( mRef ) )
			return UIA_E_ELEMENTNOTAVAILABLE;
		return manager.performAction(
				   mRef, { AccessibilityAction::SetTextSelection,
						   String( String::toString( start ) + ":" + String::toString( end ) ) } )
				   ? S_OK
				   : E_FAIL;
	} );
}

HRESULT STDMETHODCALLTYPE UIAutomationTextRange::ScrollIntoView( BOOL ) {
	TextSnapshot snapshot;
	const HRESULT result = currentSnapshot( snapshot );
	if ( FAILED( result ) )
		return result;
	if ( !hasAction( snapshot.actions, AccessibilityAction::ScrollTo ) )
		return UIA_E_NOTSUPPORTED;
	bool ignored{};
	return invoke( ignored, [this]( AccessibilityManager& manager, bool& ) -> HRESULT {
		if ( !manager.isValid( mRef ) )
			return UIA_E_ELEMENTNOTAVAILABLE;
		return manager.performAction( mRef, { AccessibilityAction::ScrollTo, {} } ) ? S_OK : E_FAIL;
	} );
}

HRESULT STDMETHODCALLTYPE UIAutomationTextRange::GetChildren( SAFEARRAY** children ) {
	if ( !children )
		return E_INVALIDARG;
	*children = nullptr;
	TextSnapshot snapshot;
	const HRESULT result = currentSnapshot( snapshot );
	if ( FAILED( result ) )
		return result;
	*children = SafeArrayCreateVector( VT_UNKNOWN, 0, 0 );
	return *children ? S_OK : E_OUTOFMEMORY;
}

std::pair<int, int> UIAutomationTextRange::endpoints() const {
	std::lock_guard<std::mutex> lock( mMutex );
	return { mStart, mEnd };
}

std::pair<int, int> UIAutomationTextRange::normalizedEndpoints( size_t length ) const {
	auto result = endpoints();
	result.first = std::max( 0, std::min( result.first, static_cast<int>( length ) ) );
	result.second = std::max( result.first, std::min( result.second, static_cast<int>( length ) ) );
	return result;
}

void UIAutomationTextRange::setEndpoints( int start, int end ) {
	std::lock_guard<std::mutex> lock( mMutex );
	mStart = start;
	mEnd = end;
}

HRESULT STDMETHODCALLTYPE
UIAutomationTextRange::GetEnclosingElement( IRawElementProviderSimple** element ) {
	if ( !element )
		return E_INVALIDARG;
	*element = nullptr;
	TextSnapshot snapshot;
	const HRESULT result = currentSnapshot( snapshot );
	if ( FAILED( result ) )
		return result;
	UIAutomationProvider* provider = mContext->provider( mRef );
	if ( !provider )
		return UIA_E_ELEMENTNOTAVAILABLE;
	*element = static_cast<IRawElementProviderSimple*>( provider );
	return S_OK;
}

}}} // namespace EE::UI::Uia

#endif
