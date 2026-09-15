#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <uiautomation.h>
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

template <typename T> class ComPtr {
  public:
	ComPtr() = default;

	explicit ComPtr( T* pointer ) : mPointer( pointer ) {}

	~ComPtr() { reset(); }

	ComPtr( const ComPtr& other ) : mPointer( other.mPointer ) {
		if ( mPointer )
			mPointer->AddRef();
	}

	ComPtr( ComPtr&& other ) noexcept : mPointer( other.mPointer ) { other.mPointer = nullptr; }

	ComPtr& operator=( ComPtr other ) noexcept {
		swap( other );
		return *this;
	}

	T* get() const { return mPointer; }

	T** put() {
		reset();
		return &mPointer;
	}

	T* operator->() const { return mPointer; }

	explicit operator bool() const { return mPointer != nullptr; }

	void reset( T* pointer = nullptr ) {
		if ( mPointer )
			mPointer->Release();
		mPointer = pointer;
	}

	void swap( ComPtr& other ) noexcept { std::swap( mPointer, other.mPointer ); }

  private:
	T* mPointer{};
};

struct ProcessHandle {
	PROCESS_INFORMATION info{};

	~ProcessHandle() {
		if ( info.hThread )
			CloseHandle( info.hThread );
		if ( info.hProcess )
			CloseHandle( info.hProcess );
	}
};

struct Result {
	std::string name;
	bool passed{};
	std::string detail;
	double milliseconds{};
};

struct WindowInfo {
	HWND window{};
	std::wstring title;
};

void requireResult( HRESULT result, const char* operation );

enum class ObservedEventKind { Automation, Property, Structure, Focus };

struct ObservedEvent {
	ObservedEventKind kind{};
	EVENTID eventId{};
	PROPERTYID propertyId{};
	StructureChangeType structureType{};
	ComPtr<IUIAutomationElement> sender;
};

class EventRecorder final : public IUIAutomationEventHandler,
							public IUIAutomationPropertyChangedEventHandler,
							public IUIAutomationStructureChangedEventHandler,
							public IUIAutomationFocusChangedEventHandler {
  public:
	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID interfaceId, void** object ) override {
		if ( !object )
			return E_INVALIDARG;
		*object = nullptr;
		if ( interfaceId == __uuidof( IUnknown ) ||
			 interfaceId == __uuidof( IUIAutomationEventHandler ) )
			*object = static_cast<IUIAutomationEventHandler*>( this );
		else if ( interfaceId == __uuidof( IUIAutomationPropertyChangedEventHandler ) )
			*object = static_cast<IUIAutomationPropertyChangedEventHandler*>( this );
		else if ( interfaceId == __uuidof( IUIAutomationStructureChangedEventHandler ) )
			*object = static_cast<IUIAutomationStructureChangedEventHandler*>( this );
		else if ( interfaceId == __uuidof( IUIAutomationFocusChangedEventHandler ) )
			*object = static_cast<IUIAutomationFocusChangedEventHandler*>( this );
		if ( !*object )
			return E_NOINTERFACE;
		AddRef();
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef() override {
		return mReferences.fetch_add( 1, std::memory_order_relaxed ) + 1;
	}

	ULONG STDMETHODCALLTYPE Release() override {
		const ULONG references = mReferences.fetch_sub( 1, std::memory_order_acq_rel ) - 1;
		if ( references == 0 )
			delete this;
		return references;
	}

	HRESULT STDMETHODCALLTYPE HandleAutomationEvent( IUIAutomationElement* sender,
													 EVENTID eventId ) override {
		add( { ObservedEventKind::Automation, eventId, 0, {}, retain( sender ) } );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE HandlePropertyChangedEvent( IUIAutomationElement* sender,
														  PROPERTYID propertyId,
														  VARIANT ) override {
		add( { ObservedEventKind::Property, 0, propertyId, {}, retain( sender ) } );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE HandleStructureChangedEvent( IUIAutomationElement* sender,
														   StructureChangeType changeType,
														   SAFEARRAY* ) override {
		add( { ObservedEventKind::Structure, 0, 0, changeType, retain( sender ) } );
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE HandleFocusChangedEvent( IUIAutomationElement* sender ) override {
		add( { ObservedEventKind::Focus, 0, 0, {}, retain( sender ) } );
		return S_OK;
	}

	size_t marker() const {
		std::lock_guard<std::mutex> lock( mMutex );
		return mEvents.size();
	}

	bool waitFor( size_t marker, const std::function<bool( const ObservedEvent& )>& predicate,
				  std::chrono::milliseconds timeout ) const {
		std::unique_lock<std::mutex> lock( mMutex );
		return mCondition.wait_for( lock, timeout, [&] {
			return std::any_of( mEvents.begin() + std::min( marker, mEvents.size() ), mEvents.end(),
								predicate );
		} );
	}

	ObservedEvent first( size_t marker,
						 const std::function<bool( const ObservedEvent& )>& predicate ) const {
		std::lock_guard<std::mutex> lock( mMutex );
		auto found = std::find_if( mEvents.begin() + std::min( marker, mEvents.size() ),
								   mEvents.end(), predicate );
		return found == mEvents.end() ? ObservedEvent{} : *found;
	}

	std::string describe( size_t marker ) const {
		std::lock_guard<std::mutex> lock( mMutex );
		std::ostringstream output;
		const auto begin = mEvents.begin() + std::min( marker, mEvents.size() );
		for ( auto iterator = begin; iterator != mEvents.end(); ++iterator ) {
			if ( iterator != begin )
				output << ',';
			output << static_cast<int>( iterator->kind ) << ':';
			if ( iterator->kind == ObservedEventKind::Automation )
				output << iterator->eventId;
			else if ( iterator->kind == ObservedEventKind::Property )
				output << iterator->propertyId;
			else if ( iterator->kind == ObservedEventKind::Structure )
				output << static_cast<int>( iterator->structureType );
			else
				output << "focus";
		}
		return output.str();
	}

  private:
	~EventRecorder() = default;

	static ComPtr<IUIAutomationElement> retain( IUIAutomationElement* sender ) {
		if ( sender )
			sender->AddRef();
		return ComPtr<IUIAutomationElement>( sender );
	}

	void add( ObservedEvent event ) {
		{
			std::lock_guard<std::mutex> lock( mMutex );
			mEvents.emplace_back( std::move( event ) );
		}
		mCondition.notify_all();
	}

	std::atomic<ULONG> mReferences{ 1 };
	mutable std::mutex mMutex;
	mutable std::condition_variable mCondition;
	std::vector<ObservedEvent> mEvents;
};

class EventSubscriptions {
  public:
	EventSubscriptions( IUIAutomation* automation, IUIAutomationElement* root,
						EventRecorder* recorder ) :
		mAutomation( automation ), mRoot( root ), mRecorder( recorder ) {}

	~EventSubscriptions() {
		if ( mFocus )
			mAutomation->RemoveFocusChangedEventHandler( mRecorder );
		if ( mStructure )
			mAutomation->RemoveStructureChangedEventHandler( mRoot, mRecorder );
		for ( EVENTID eventId : mAutomationEvents )
			mAutomation->RemoveAutomationEventHandler( eventId, mRoot, mRecorder );
		if ( mProperties )
			mAutomation->RemovePropertyChangedEventHandler( mRoot, mRecorder );
	}

	void subscribe() {
		PROPERTYID properties[] = {
			UIA_NamePropertyId,
			UIA_HelpTextPropertyId,
			UIA_ValueValuePropertyId,
			UIA_RangeValueValuePropertyId,
			UIA_IsEnabledPropertyId,
			UIA_IsOffscreenPropertyId,
			UIA_HasKeyboardFocusPropertyId,
			UIA_ToggleToggleStatePropertyId,
			UIA_SelectionItemIsSelectedPropertyId,
			UIA_ExpandCollapseExpandCollapseStatePropertyId,
		};
		requireResult( mAutomation->AddPropertyChangedEventHandlerNativeArray(
						   mRoot, TreeScope_Subtree, nullptr, mRecorder, properties,
						   static_cast<int>( std::size( properties ) ) ),
					   "AddPropertyChangedEventHandler" );
		mProperties = true;
		const EVENTID events[] = {
			UIA_Text_TextChangedEventId, UIA_Text_TextSelectionChangedEventId,
			UIA_SelectionItem_ElementSelectedEventId, UIA_LayoutInvalidatedEventId };
		for ( EVENTID eventId : events ) {
			requireResult( mAutomation->AddAutomationEventHandler(
							   eventId, mRoot, TreeScope_Subtree, nullptr, mRecorder ),
						   "AddAutomationEventHandler" );
			mAutomationEvents.emplace_back( eventId );
		}
		requireResult( mAutomation->AddStructureChangedEventHandler( mRoot, TreeScope_Subtree,
																	 nullptr, mRecorder ),
					   "AddStructureChangedEventHandler" );
		mStructure = true;
		requireResult( mAutomation->AddFocusChangedEventHandler( nullptr, mRecorder ),
					   "AddFocusChangedEventHandler" );
		mFocus = true;
	}

  private:
	IUIAutomation* mAutomation{};
	IUIAutomationElement* mRoot{};
	EventRecorder* mRecorder{};
	std::vector<EVENTID> mAutomationEvents;
	bool mProperties{};
	bool mStructure{};
	bool mFocus{};
};

void requireResult( HRESULT result, const char* operation ) {
	if ( SUCCEEDED( result ) )
		return;
	std::ostringstream message;
	message << operation << " failed with HRESULT 0x" << std::hex << std::uppercase
			<< static_cast<unsigned long>( result );
	throw std::runtime_error( message.str() );
}

void require( bool condition, const std::string& message ) {
	if ( !condition )
		throw std::runtime_error( message );
}

std::string narrow( const std::wstring& string ) {
	if ( string.empty() )
		return {};
	const int length =
		WideCharToMultiByte( CP_UTF8, 0, string.data(), static_cast<int>( string.size() ), nullptr,
							 0, nullptr, nullptr );
	std::string result( static_cast<size_t>( length ), '\0' );
	WideCharToMultiByte( CP_UTF8, 0, string.data(), static_cast<int>( string.size() ),
						 result.data(), length, nullptr, nullptr );
	return result;
}

std::string jsonEscape( const std::string& string ) {
	std::ostringstream output;
	for ( const unsigned char value : string ) {
		switch ( value ) {
			case '\\':
				output << "\\\\";
				break;
			case '"':
				output << "\\\"";
				break;
			case '\n':
				output << "\\n";
				break;
			case '\r':
				output << "\\r";
				break;
			case '\t':
				output << "\\t";
				break;
			default:
				if ( value < 0x20 )
					output << "\\u" << std::hex << std::setw( 4 ) << std::setfill( '0' )
						   << static_cast<int>( value ) << std::dec;
				else
					output << value;
		}
	}
	return output.str();
}

std::wstring elementName( IUIAutomationElement* element ) {
	BSTR name{};
	requireResult( element->get_CurrentName( &name ), "get_CurrentName" );
	std::wstring result( name ? name : L"", name ? SysStringLen( name ) : 0 );
	SysFreeString( name );
	return result;
}

std::wstring runtimeId( IUIAutomationElement* element ) {
	SAFEARRAY* runtimeId{};
	requireResult( element->GetRuntimeId( &runtimeId ), "GetRuntimeId" );
	require( runtimeId != nullptr, "provider returned a null runtime ID" );
	LONG lower{};
	LONG upper{};
	requireResult( SafeArrayGetLBound( runtimeId, 1, &lower ), "SafeArrayGetLBound" );
	requireResult( SafeArrayGetUBound( runtimeId, 1, &upper ), "SafeArrayGetUBound" );
	std::wostringstream output;
	for ( LONG index = lower; index <= upper; ++index ) {
		int value{};
		requireResult( SafeArrayGetElement( runtimeId, &index, &value ), "SafeArrayGetElement" );
		if ( index != lower )
			output << L':';
		output << value;
	}
	SafeArrayDestroy( runtimeId );
	return output.str();
}

BOOL CALLBACK collectWindows( HWND window, LPARAM parameter ) {
	auto context = reinterpret_cast<std::pair<DWORD, std::vector<WindowInfo>*>*>( parameter );
	DWORD processId{};
	GetWindowThreadProcessId( window, &processId );
	if ( processId != context->first )
		return TRUE;
	wchar_t className[128]{};
	GetClassNameW( window, className, 128 );
	if ( std::wstring_view( className ) != L"SDL_app" )
		return TRUE;
	wchar_t title[512]{};
	GetWindowTextW( window, title, 512 );
	context->second->push_back( { window, title } );
	return TRUE;
}

std::vector<WindowInfo> processWindows( DWORD processId ) {
	std::vector<WindowInfo> result;
	std::pair<DWORD, std::vector<WindowInfo>*> context{ processId, &result };
	EnumWindows( collectWindows, reinterpret_cast<LPARAM>( &context ) );
	return result;
}

HWND findWindow( DWORD processId, const std::wstring& title ) {
	for ( const auto& entry : processWindows( processId ) ) {
		if ( entry.title == title )
			return entry.window;
	}
	return nullptr;
}

bool activateWindow( HWND window ) {
	const DWORD currentThread = GetCurrentThreadId();
	const DWORD windowThread = GetWindowThreadProcessId( window, nullptr );
	const HWND foregroundWindow = GetForegroundWindow();
	const DWORD foregroundThread =
		foregroundWindow ? GetWindowThreadProcessId( foregroundWindow, nullptr ) : 0;
	const bool attachedForeground = foregroundThread && foregroundThread != currentThread &&
									AttachThreadInput( currentThread, foregroundThread, TRUE );
	const bool attachedWindow = windowThread && windowThread != currentThread &&
								AttachThreadInput( currentThread, windowThread, TRUE );
	ShowWindow( window, SW_RESTORE );
	BringWindowToTop( window );
	SetForegroundWindow( window );
	SetActiveWindow( window );
	::SetFocus( window );
	if ( attachedWindow )
		AttachThreadInput( currentThread, windowThread, FALSE );
	if ( attachedForeground )
		AttachThreadInput( currentThread, foregroundThread, FALSE );
	const auto deadline = Clock::now() + std::chrono::seconds( 2 );
	do {
		if ( GetForegroundWindow() == window )
			return true;
		std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
	} while ( Clock::now() < deadline );
	return GetForegroundWindow() == window;
}

template <typename Predicate>
bool pollUntil( std::chrono::milliseconds timeout, Predicate predicate ) {
	const auto deadline = Clock::now() + timeout;
	do {
		if ( predicate() )
			return true;
		std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
	} while ( Clock::now() < deadline );
	return predicate();
}

ComPtr<IUIAutomationElement> waitForRoot( IUIAutomation* automation, DWORD processId,
										  const std::wstring& title ) {
	ComPtr<IUIAutomationElement> result;
	const bool ready = pollUntil( std::chrono::seconds( 10 ), [&] {
		const HWND window = findWindow( processId, title );
		if ( !window )
			return false;
		ComPtr<IUIAutomationElement> candidate;
		if ( FAILED( automation->ElementFromHandle( window, candidate.put() ) ) || !candidate )
			return false;
		BSTR name{};
		const HRESULT nameResult = candidate->get_CurrentName( &name );
		const bool matches = SUCCEEDED( nameResult ) && name && title == name;
		SysFreeString( name );
		if ( matches )
			result = std::move( candidate );
		return matches;
	} );
	require( ready, "timed out waiting for UIA root: " + narrow( title ) );
	return result;
}

ComPtr<IUIAutomationElement> findByName( IUIAutomation* automation, IUIAutomationElement* root,
										 const wchar_t* name ) {
	VARIANT value;
	VariantInit( &value );
	value.vt = VT_BSTR;
	value.bstrVal = SysAllocString( name );
	ComPtr<IUIAutomationCondition> condition;
	const HRESULT conditionResult =
		automation->CreatePropertyCondition( UIA_NamePropertyId, value, condition.put() );
	VariantClear( &value );
	requireResult( conditionResult, "CreatePropertyCondition" );
	ComPtr<IUIAutomationElement> element;
	requireResult( root->FindFirst( TreeScope_Descendants, condition.get(), element.put() ),
				   "FindFirst" );
	require( static_cast<bool>( element ), "element was not found: " + narrow( name ) );
	return element;
}

template <typename T>
ComPtr<T> pattern( IUIAutomationElement* element, PATTERNID patternId, REFIID interfaceId,
				   const char* name ) {
	ComPtr<T> result;
	requireResult( element->GetCurrentPatternAs( patternId, interfaceId,
												 reinterpret_cast<void**>( result.put() ) ),
				   name );
	require( static_cast<bool>( result ), std::string( name ) + " returned null" );
	return result;
}

void checkNavigation( IUIAutomation* automation, IUIAutomationElement* root ) {
	ComPtr<IUIAutomationTreeWalker> walker;
	requireResult( automation->get_RawViewWalker( walker.put() ), "get_RawViewWalker" );
	std::set<std::wstring> runtimeIds;
	std::function<void( IUIAutomationElement*, size_t )> visit = [&]( IUIAutomationElement* parent,
																	  size_t depth ) {
		require( depth < 128, "UIA tree contains a navigation cycle" );
		ComPtr<IUIAutomationElement> child;
		requireResult( walker->GetFirstChildElement( parent, child.put() ),
					   "GetFirstChildElement" );
		ComPtr<IUIAutomationElement> previous;
		while ( child ) {
			const std::wstring id = runtimeId( child.get() );
			require( runtimeIds.emplace( id ).second, "duplicate runtime ID in one fragment" );
			ComPtr<IUIAutomationElement> actualParent;
			requireResult( walker->GetParentElement( child.get(), actualParent.put() ),
						   "GetParentElement" );
			BOOL same{};
			requireResult( automation->CompareElements( parent, actualParent.get(), &same ),
						   "CompareElements(parent)" );
			require( same, "child navigation did not return to its parent" );
			if ( previous ) {
				ComPtr<IUIAutomationElement> back;
				requireResult( walker->GetPreviousSiblingElement( child.get(), back.put() ),
							   "GetPreviousSiblingElement" );
				requireResult( automation->CompareElements( previous.get(), back.get(), &same ),
							   "CompareElements(previous)" );
				require( same, "sibling navigation is not bidirectional" );
			}
			visit( child.get(), depth + 1 );
			previous = child;
			ComPtr<IUIAutomationElement> next;
			requireResult( walker->GetNextSiblingElement( child.get(), next.put() ),
						   "GetNextSiblingElement" );
			child = std::move( next );
		}
	};
	runtimeIds.emplace( runtimeId( root ) );
	visit( root, 0 );
	require( runtimeIds.size() >= 20, "primary UIA tree is unexpectedly small" );
}

void writeResults( const std::filesystem::path& path, const std::vector<Result>& results,
				   const std::vector<double>& queryLatency, DWORD processId ) {
	std::filesystem::create_directories( path.parent_path() );
	auto sortedLatency = queryLatency;
	std::sort( sortedLatency.begin(), sortedLatency.end() );
	auto percentile = [&]( double fraction ) {
		if ( sortedLatency.empty() )
			return 0.0;
		const size_t index = std::min(
			sortedLatency.size() - 1,
			static_cast<size_t>( fraction * static_cast<double>( sortedLatency.size() - 1 ) ) );
		return sortedLatency[index];
	};
	std::ofstream output( path, std::ios::binary | std::ios::trunc );
	output << "{\n  \"process_id\": " << processId << ",\n  \"results\": [\n";
	for ( size_t index = 0; index < results.size(); ++index ) {
		const auto& result = results[index];
		output << "    {\"name\": \"" << jsonEscape( result.name )
			   << "\", \"passed\": " << ( result.passed ? "true" : "false" )
			   << ", \"milliseconds\": " << std::fixed << std::setprecision( 3 )
			   << result.milliseconds << ", \"detail\": \"" << jsonEscape( result.detail ) << "\"}"
			   << ( index + 1 == results.size() ? "\n" : ",\n" );
	}
	output << "  ],\n  \"query_latency_ms\": {\"samples\": " << sortedLatency.size()
		   << ", \"p50\": " << percentile( 0.50 ) << ", \"p95\": " << percentile( 0.95 )
		   << ", \"p99\": " << percentile( 0.99 )
		   << ", \"max\": " << ( sortedLatency.empty() ? 0.0 : sortedLatency.back() ) << "}\n}\n";
}

} // namespace

int wmain( int argc, wchar_t** argv ) {
	std::filesystem::path harnessPath;
	{
		wchar_t module[MAX_PATH]{};
		GetModuleFileNameW( nullptr, module, MAX_PATH );
		harnessPath = module;
	}
	const std::filesystem::path example =
		argc > 1 ? std::filesystem::path( argv[1] )
				 : harnessPath.parent_path().parent_path() / L"eepp-ui-accessibility.exe";
	const std::filesystem::path outputPath =
		harnessPath.parent_path() / L"output" / L"windows_accessibility_results.json";
	std::vector<Result> results;
	std::vector<double> queryLatency;
	ProcessHandle process;
	DWORD processId{};
	bool processExitedCleanly = false;

	auto record = [&]( const std::string& name, const std::function<void()>& test ) {
		const auto started = Clock::now();
		Result result{ name };
		try {
			test();
			result.passed = true;
			result.detail = "ok";
		} catch ( const std::exception& exception ) {
			result.detail = exception.what();
		}
		result.milliseconds =
			std::chrono::duration<double, std::milli>( Clock::now() - started ).count();
		results.emplace_back( result );
		std::cout << ( result.passed ? "PASS " : "FAIL " ) << result.name << " (" << std::fixed
				  << std::setprecision( 2 ) << result.milliseconds << " ms)";
		if ( !result.passed )
			std::cout << ": " << result.detail;
		std::cout << '\n';
	};

	const HRESULT initializeResult = CoInitializeEx( nullptr, COINIT_MULTITHREADED );
	if ( FAILED( initializeResult ) ) {
		std::cerr << "CoInitializeEx failed\n";
		return EXIT_FAILURE;
	}

	try {
		require( std::filesystem::is_regular_file( example ),
				 "example executable does not exist: " + narrow( example.wstring() ) );
		std::wstring command = L"\"" + example.wstring() + L"\" --multi-window";
		STARTUPINFOW startup{};
		startup.cb = sizeof( startup );
		std::wstring workingDirectory = example.parent_path().wstring();
		require( CreateProcessW( example.c_str(), command.data(), nullptr, nullptr, FALSE,
								 CREATE_NO_WINDOW, nullptr, workingDirectory.c_str(), &startup,
								 &process.info ) != FALSE,
				 "CreateProcessW failed with error " + std::to_string( GetLastError() ) );
		processId = process.info.dwProcessId;

		ComPtr<IUIAutomation> automation;
		requireResult( CoCreateInstance( CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER,
										 IID_PPV_ARGS( automation.put() ) ),
					   "CoCreateInstance(CUIAutomation)" );

		ComPtr<IUIAutomationElement> primary;
		ComPtr<IUIAutomationElement> secondary;
		record( "late discovery and multi-window roots", [&] {
			primary = waitForRoot( automation.get(), processId, L"eepp - Accessibility" );
			secondary =
				waitForRoot( automation.get(), processId, L"eepp - Accessibility Secondary" );
			require( runtimeId( primary.get() ) != runtimeId( secondary.get() ),
					 "top-level windows have colliding runtime IDs" );
		} );

		if ( primary ) {
			ComPtr<EventRecorder> eventRecorder( new EventRecorder() );
			ComPtr<IUIAutomationElement> eventRoot;
			requireResult( automation->GetRootElement( eventRoot.put() ), "GetRootElement" );
			EventSubscriptions eventSubscriptions( automation.get(), eventRoot.get(),
												   eventRecorder.get() );
			record( "native event subscriptions", [&] { eventSubscriptions.subscribe(); } );
			const size_t eventMarker = eventRecorder->marker();

			record( "root properties and runtime identity", [&] {
				require( elementName( primary.get() ) == L"eepp - Accessibility",
						 "root name mismatch" );
				CONTROLTYPEID type{};
				requireResult( primary->get_CurrentControlType( &type ), "get_CurrentControlType" );
				require( type == UIA_WindowControlTypeId, "root is not a Window control" );
				const auto first = runtimeId( primary.get() );
				const auto second = runtimeId( primary.get() );
				require( first == second, "root runtime ID is unstable" );
			} );

			record( "bidirectional raw navigation",
					[&] { checkNavigation( automation.get(), primary.get() ); } );

			record( "invoke and dynamic property update", [&] {
				auto save = findByName( automation.get(), primary.get(), L"Save settings" );
				auto invoke = pattern<IUIAutomationInvokePattern>(
					save.get(), UIA_InvokePatternId, __uuidof( IUIAutomationInvokePattern ),
					"Invoke" );
				requireResult( invoke->Invoke(), "Invoke" );
				require( pollUntil( std::chrono::seconds( 2 ),
									[&] {
										try {
											auto status =
												findByName( automation.get(), primary.get(),
															L"Settings saved" );
											return static_cast<bool>( status );
										} catch ( ... ) {
											return false;
										}
									} ),
						 "Invoke did not update the status" );
			} );

			record( "toggle pattern", [&] {
				auto checkbox = findByName( automation.get(), primary.get(), L"Enable autosave" );
				auto toggle = pattern<IUIAutomationTogglePattern>(
					checkbox.get(), UIA_TogglePatternId, __uuidof( IUIAutomationTogglePattern ),
					"Toggle" );
				ToggleState before{};
				requireResult( toggle->get_CurrentToggleState( &before ), "CurrentToggleState" );
				requireResult( toggle->Toggle(), "Toggle" );
				ToggleState after{};
				require( pollUntil( std::chrono::seconds( 2 ),
									[&] {
										return SUCCEEDED(
												   toggle->get_CurrentToggleState( &after ) ) &&
											   after != before;
									} ),
						 "toggle state did not change" );
			} );

			record( "value and UTF-16 text ranges", [&] {
				auto edit = findByName( automation.get(), primary.get(), L"Description" );
				auto value = pattern<IUIAutomationValuePattern>(
					edit.get(), UIA_ValuePatternId, __uuidof( IUIAutomationValuePattern ),
					"Value" );
				const wchar_t expected[] = L"A\U0001F600e\u0301\r\nB";
				BSTR replacement = SysAllocString( expected );
				require( replacement != nullptr, "SysAllocString failed" );
				const HRESULT setValueResult = value->SetValue( replacement );
				SysFreeString( replacement );
				requireResult( setValueResult, "Value.SetValue" );
				auto text = pattern<IUIAutomationTextPattern>(
					edit.get(), UIA_TextPatternId, __uuidof( IUIAutomationTextPattern ), "Text" );
				ComPtr<IUIAutomationTextRange> document;
				requireResult( text->get_DocumentRange( document.put() ), "DocumentRange" );
				BSTR actual{};
				requireResult( document->GetText( -1, &actual ), "TextRange.GetText" );
				const std::wstring actualText( actual ? actual : L"",
											   actual ? SysStringLen( actual ) : 0 );
				SysFreeString( actual );
				require( actualText == expected, "TextPattern changed Unicode or CRLF content" );
				require( actualText.size() == 8, "TextPattern did not report UTF-16 code units" );
				ComPtr<IUIAutomationTextRange> character;
				requireResult( document->Clone( character.put() ), "TextRange.Clone" );
				requireResult( character->ExpandToEnclosingUnit( TextUnit_Character ),
							   "ExpandToEnclosingUnit" );
				int moved{};
				requireResult( character->Move( TextUnit_Character, 1, &moved ), "TextRange.Move" );
				require( moved == 1, "TextRange did not move one character" );
				requireResult( character->Select(), "TextRange.Select" );
				ComPtr<IUIAutomationTextRangeArray> selection;
				requireResult( text->GetSelection( selection.put() ), "Text.GetSelection" );
				int selectionLength{};
				requireResult( selection->get_Length( &selectionLength ), "TextSelection.Length" );
				require( selectionLength == 1, "Text.GetSelection returned the wrong range count" );

				auto password = findByName( automation.get(), primary.get(), L"Account password" );
				BOOL protectedText{};
				requireResult( password->get_CurrentIsPassword( &protectedText ),
							   "get_CurrentIsPassword" );
				require( protectedText, "password field is not marked protected" );
				ComPtr<IUIAutomationTextPattern> forbidden;
				const HRESULT textResult = password->GetCurrentPatternAs(
					UIA_TextPatternId, __uuidof( IUIAutomationTextPattern ),
					reinterpret_cast<void**>( forbidden.put() ) );
				require( FAILED( textResult ) || !forbidden,
						 "password field exposes TextPattern content" );
			} );

			record( "range value validation", [&] {
				auto slider = findByName( automation.get(), primary.get(), L"Volume" );
				auto range = pattern<IUIAutomationRangeValuePattern>(
					slider.get(), UIA_RangeValuePatternId,
					__uuidof( IUIAutomationRangeValuePattern ), "RangeValue" );
				double minimum{};
				double maximum{};
				BOOL readOnly{};
				requireResult( range->get_CurrentMinimum( &minimum ), "RangeValue.Minimum" );
				requireResult( range->get_CurrentMaximum( &maximum ), "RangeValue.Maximum" );
				requireResult( range->get_CurrentIsReadOnly( &readOnly ), "RangeValue.IsReadOnly" );
				require( minimum == 0 && maximum == 100, "slider range is incorrect" );
				require( !readOnly, "slider unexpectedly reports a read-only range" );
				requireResult( range->SetValue( 55 ), "RangeValue.SetValue" );
				double current{};
				requireResult( range->get_CurrentValue( &current ), "RangeValue.Value" );
				require( current == 55, "slider did not accept an in-range value" );
				require( FAILED( range->SetValue( 101 ) ),
						 "slider accepted an out-of-range value" );
			} );

			record( "expand collapse pattern", [&] {
				auto combo = findByName( automation.get(), primary.get(), L"Language" );
				auto expand = pattern<IUIAutomationExpandCollapsePattern>(
					combo.get(), UIA_ExpandCollapsePatternId,
					__uuidof( IUIAutomationExpandCollapsePattern ), "ExpandCollapse" );
				requireResult( expand->Expand(), "ExpandCollapse.Expand" );
				ExpandCollapseState state{};
				require( pollUntil( std::chrono::seconds( 2 ),
									[&] {
										return SUCCEEDED( expand->get_CurrentExpandCollapseState(
												   &state ) ) &&
											   state == ExpandCollapseState_Expanded;
									} ),
						 "combo box did not expand" );
				requireResult( expand->Collapse(), "ExpandCollapse.Collapse" );
			} );

			record( "selection container and item", [&] {
				auto list = findByName( automation.get(), primary.get(), L"Colors" );
				auto selection = pattern<IUIAutomationSelectionPattern>(
					list.get(), UIA_SelectionPatternId, __uuidof( IUIAutomationSelectionPattern ),
					"Selection" );
				auto red = findByName( automation.get(), list.get(), L"Red" );
				auto item = pattern<IUIAutomationSelectionItemPattern>(
					red.get(), UIA_SelectionItemPatternId,
					__uuidof( IUIAutomationSelectionItemPattern ), "SelectionItem" );
				requireResult( item->Select(), "SelectionItem.Select" );
				BOOL selected{};
				requireResult( item->get_CurrentIsSelected( &selected ),
							   "SelectionItem.IsSelected" );
				require( selected, "selected list item did not report selected state" );
				ComPtr<IUIAutomationElementArray> selectedItems;
				requireResult( selection->GetCurrentSelection( selectedItems.put() ),
							   "Selection.GetCurrentSelection" );
				int selectionLength{};
				requireResult( selectedItems->get_Length( &selectionLength ), "Selection.Length" );
				require( selectionLength == 1, "selection container returned no selection" );
			} );

			record( "property, focus, text, selection, and structure events", [&] {
				auto propertyEvent = []( PROPERTYID propertyId ) {
					return [=]( const ObservedEvent& event ) {
						return event.kind == ObservedEventKind::Property &&
							   event.propertyId == propertyId;
					};
				};
				auto automationEvent = []( EVENTID eventId ) {
					return [=]( const ObservedEvent& event ) {
						return event.kind == ObservedEventKind::Automation &&
							   event.eventId == eventId;
					};
				};
				auto expect = [&]( const std::function<bool( const ObservedEvent& )>& predicate,
								   const char* message ) {
					require(
						eventRecorder->waitFor( eventMarker, predicate, std::chrono::seconds( 5 ) ),
						std::string( message ) + "; observed " +
							eventRecorder->describe( eventMarker ) );
				};

				const HWND primaryWindow = findWindow( processId, L"eepp - Accessibility" );
				require( primaryWindow != nullptr, "primary HWND disappeared" );
				const size_t focusMarker = eventRecorder->marker();
				const bool nativeFocusAvailable = activateWindow( primaryWindow );
				auto save = findByName( automation.get(), primary.get(), L"Save settings" );
				requireResult( save->SetFocus(), "SetFocus" );
				BOOL hasKeyboardFocus{};
				requireResult( save->get_CurrentHasKeyboardFocus( &hasKeyboardFocus ),
							   "get_CurrentHasKeyboardFocus" );
				require( hasKeyboardFocus, "provider did not move semantic keyboard focus" );
				if ( nativeFocusAvailable ) {
					ComPtr<IUIAutomationElement> focused;
					requireResult( automation->GetFocusedElement( focused.put() ),
								   "GetFocusedElement" );
					require( focused && elementName( focused.get() ) == L"Save settings",
							 "native UIA focus did not move to the requested element" );
				}
				auto metadata =
					findByName( automation.get(), primary.get(), L"Update accessible metadata" );
				auto metadataInvoke = pattern<IUIAutomationInvokePattern>(
					metadata.get(), UIA_InvokePatternId, __uuidof( IUIAutomationInvokePattern ),
					"Invoke" );
				requireResult( metadataInvoke->Invoke(), "update metadata" );
				auto replace =
					findByName( automation.get(), primary.get(), L"Replace project model" );
				auto replaceInvoke = pattern<IUIAutomationInvokePattern>(
					replace.get(), UIA_InvokePatternId, __uuidof( IUIAutomationInvokePattern ),
					"Invoke" );
				requireResult( replaceInvoke->Invoke(), "replace model" );
				auto toggleEnabled =
					findByName( automation.get(), primary.get(), L"Toggle mutable enabled" );
				auto enabledInvoke = pattern<IUIAutomationInvokePattern>(
					toggleEnabled.get(), UIA_InvokePatternId,
					__uuidof( IUIAutomationInvokePattern ), "Invoke" );
				requireResult( enabledInvoke->Invoke(), "toggle enabled" );
				auto toggleVisible =
					findByName( automation.get(), primary.get(), L"Toggle mutable visibility" );
				auto visibleInvoke = pattern<IUIAutomationInvokePattern>(
					toggleVisible.get(), UIA_InvokePatternId,
					__uuidof( IUIAutomationInvokePattern ), "Invoke" );
				requireResult( visibleInvoke->Invoke(), "toggle visibility" );

				RECT windowBounds{};
				require( primaryWindow && GetWindowRect( primaryWindow, &windowBounds ),
						 "failed to get primary window bounds" );
				require( SetWindowPos( primaryWindow, nullptr, windowBounds.left, windowBounds.top,
									   windowBounds.right - windowBounds.left + 16,
									   windowBounds.bottom - windowBounds.top + 16,
									   SWP_NOACTIVATE | SWP_NOZORDER ) != FALSE,
						 "failed to resize primary window" );

				expect( propertyEvent( UIA_NamePropertyId ), "missing name property event" );
				expect( propertyEvent( UIA_HelpTextPropertyId ), "missing help property event" );
				expect( propertyEvent( UIA_ValueValuePropertyId ), "missing value property event" );
				expect( propertyEvent( UIA_RangeValueValuePropertyId ),
						"missing range value property event" );
				expect( propertyEvent( UIA_IsEnabledPropertyId ),
						"missing enabled property event" );
				expect( propertyEvent( UIA_IsOffscreenPropertyId ),
						"missing visibility property event" );
				expect( propertyEvent( UIA_ToggleToggleStatePropertyId ),
						"missing toggle property event" );
				expect( propertyEvent( UIA_HasKeyboardFocusPropertyId ),
						"missing keyboard focus property event" );
				expect( propertyEvent( UIA_SelectionItemIsSelectedPropertyId ),
						"missing selection property event" );
				expect( propertyEvent( UIA_ExpandCollapseExpandCollapseStatePropertyId ),
						"missing expand/collapse property event" );
				expect( automationEvent( UIA_Text_TextChangedEventId ),
						"missing text changed event" );
				expect( automationEvent( UIA_Text_TextSelectionChangedEventId ),
						"missing text selection event" );
				expect( automationEvent( UIA_SelectionItem_ElementSelectedEventId ),
						"missing selection-item event" );
				expect( automationEvent( UIA_LayoutInvalidatedEventId ),
						"missing layout invalidated event" );
				if ( nativeFocusAvailable )
					require( eventRecorder->waitFor(
								 focusMarker,
								 []( const ObservedEvent& event ) {
									 return event.kind == ObservedEventKind::Focus;
								 },
								 std::chrono::seconds( 5 ) ),
							 "missing focus changed event" );
				expect(
					[]( const ObservedEvent& event ) {
						return event.kind == ObservedEventKind::Structure &&
							   event.structureType == StructureChangeType_ChildrenInvalidated;
					},
					"missing children-invalidated structure event" );

				const auto toggleEvent = eventRecorder->first(
					eventMarker, propertyEvent( UIA_ToggleToggleStatePropertyId ) );
				require( toggleEvent.sender &&
							 elementName( toggleEvent.sender.get() ) == L"Enable autosave",
						 "toggle event used the wrong sender" );
			} );

			record( "screen point hit testing", [&] {
				auto checkbox = findByName( automation.get(), primary.get(), L"Enable autosave" );
				RECT bounds{};
				requireResult( checkbox->get_CurrentBoundingRectangle( &bounds ),
							   "get_CurrentBoundingRectangle" );
				POINT center{ bounds.left + ( bounds.right - bounds.left ) / 2,
							  bounds.top + ( bounds.bottom - bounds.top ) / 2 };
				ComPtr<IUIAutomationElement> hit;
				requireResult( automation->ElementFromPoint( center, hit.put() ),
							   "ElementFromPoint" );
				BOOL same{};
				requireResult( automation->CompareElements( checkbox.get(), hit.get(), &same ),
							   "CompareElements(hit)" );
				require( same, "point hit testing returned the wrong semantic element" );
			} );

			record( "retained provider and dynamic window lifecycle", [&] {
				auto open =
					findByName( automation.get(), primary.get(), L"Open accessibility window" );
				auto invoke = pattern<IUIAutomationInvokePattern>(
					open.get(), UIA_InvokePatternId, __uuidof( IUIAutomationInvokePattern ),
					"Invoke" );
				requireResult( invoke->Invoke(), "open dynamic window" );
				auto dynamic =
					waitForRoot( automation.get(), processId, L"eepp - Accessibility Dynamic" );
				const auto firstRuntimeId = runtimeId( dynamic.get() );
				auto close = findByName( automation.get(), dynamic.get(), L"Close dynamic window" );
				auto closeInvoke = pattern<IUIAutomationInvokePattern>(
					close.get(), UIA_InvokePatternId, __uuidof( IUIAutomationInvokePattern ),
					"Invoke" );
				requireResult( closeInvoke->Invoke(), "close dynamic window" );
				require( pollUntil( std::chrono::seconds( 5 ),
									[&] {
										return findWindow( processId,
														   L"eepp - Accessibility Dynamic" ) ==
											   nullptr;
									} ),
						 "dynamic HWND was not destroyed" );
				BSTR staleName{};
				const HRESULT staleResult = dynamic->get_CurrentName( &staleName );
				SysFreeString( staleName );
				require( FAILED( staleResult ),
						 "retained provider remained live after HWND destruction" );

				requireResult( invoke->Invoke(), "reopen dynamic window" );
				auto reopened =
					waitForRoot( automation.get(), processId, L"eepp - Accessibility Dynamic" );
				require( runtimeId( reopened.get() ) != firstRuntimeId,
						 "reopened window reused the previous provider identity" );
				auto reclose =
					findByName( automation.get(), reopened.get(), L"Close dynamic window" );
				auto recloseInvoke = pattern<IUIAutomationInvokePattern>(
					reclose.get(), UIA_InvokePatternId, __uuidof( IUIAutomationInvokePattern ),
					"Invoke" );
				requireResult( recloseInvoke->Invoke(), "close reopened dynamic window" );
			} );

			record( "repeated dynamic window lifecycle", [&] {
				require( pollUntil( std::chrono::seconds( 5 ),
									[&] {
										return findWindow( processId,
														   L"eepp - Accessibility Dynamic" ) ==
											   nullptr;
									} ),
						 "reopened dynamic HWND was not destroyed" );
				auto open =
					findByName( automation.get(), primary.get(), L"Open accessibility window" );
				auto invoke = pattern<IUIAutomationInvokePattern>(
					open.get(), UIA_InvokePatternId, __uuidof( IUIAutomationInvokePattern ),
					"Invoke" );
				std::set<std::wstring> runtimeIds;
				for ( size_t iteration = 0; iteration < 100; ++iteration ) {
					requireResult( invoke->Invoke(), "stress open dynamic window" );
					auto dynamic =
						waitForRoot( automation.get(), processId, L"eepp - Accessibility Dynamic" );
					require( runtimeIds.emplace( runtimeId( dynamic.get() ) ).second,
							 "dynamic window reused a provider identity during stress" );
					auto close =
						findByName( automation.get(), dynamic.get(), L"Close dynamic window" );
					auto closeInvoke = pattern<IUIAutomationInvokePattern>(
						close.get(), UIA_InvokePatternId, __uuidof( IUIAutomationInvokePattern ),
						"Invoke" );
					requireResult( closeInvoke->Invoke(), "stress close dynamic window" );
					require( pollUntil( std::chrono::seconds( 5 ),
										[&] {
											return findWindow( processId,
															   L"eepp - Accessibility Dynamic" ) ==
												   nullptr;
										} ),
							 "stress dynamic HWND was not destroyed" );
				}
			} );

			record( "individual query latency", [&] {
				for ( size_t index = 0; index < 200; ++index ) {
					const auto started = Clock::now();
					BSTR name{};
					requireResult( primary->get_CurrentName( &name ), "latency name query" );
					SysFreeString( name );
					queryLatency.emplace_back(
						std::chrono::duration<double, std::milli>( Clock::now() - started )
							.count() );
				}
			} );
		}

		if ( secondary ) {
			record( "secondary close preserves primary", [&] {
				auto close =
					findByName( automation.get(), secondary.get(), L"Close secondary window" );
				auto invoke = pattern<IUIAutomationInvokePattern>(
					close.get(), UIA_InvokePatternId, __uuidof( IUIAutomationInvokePattern ),
					"Invoke" );
				requireResult( invoke->Invoke(), "close secondary" );
				require( pollUntil( std::chrono::seconds( 5 ),
									[&] {
										return findWindow( processId,
														   L"eepp - Accessibility Secondary" ) ==
											   nullptr;
									} ),
						 "secondary HWND was not destroyed" );
				require( elementName( primary.get() ) == L"eepp - Accessibility",
						 "primary provider failed after secondary close" );
			} );
		}

		if ( HWND primaryWindow = findWindow( processId, L"eepp - Accessibility" ) )
			PostMessageW( primaryWindow, WM_CLOSE, 0, 0 );
		processExitedCleanly = WaitForSingleObject( process.info.hProcess, 5000 ) == WAIT_OBJECT_0;
	} catch ( const std::exception& exception ) {
		results.push_back( { "harness setup", false, exception.what(), 0 } );
		std::cerr << "FAIL harness setup: " << exception.what() << '\n';
	}

	if ( process.info.hProcess && !processExitedCleanly ) {
		TerminateProcess( process.info.hProcess, 1 );
		WaitForSingleObject( process.info.hProcess, 5000 );
	}
	CoUninitialize();
	writeResults( outputPath, results, queryLatency, processId );
	const bool passed = !results.empty() &&
						std::all_of( results.begin(), results.end(),
									 []( const Result& result ) { return result.passed; } ) &&
						processExitedCleanly;
	std::cout << ( passed ? "Windows accessibility harness passed"
						  : "Windows accessibility harness failed" )
			  << "; results: " << narrow( outputPath.wstring() ) << '\n';
	return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
