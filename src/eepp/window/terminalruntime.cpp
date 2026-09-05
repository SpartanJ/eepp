#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <eepp/system/log.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/terminal/terminalruntime.hpp>
#include <eepp/window/window.hpp>
#include <string>

#if defined( EE_PLATFORM_POSIX )
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#elif EE_PLATFORM == EE_PLATFORM_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace EE { namespace Window {

struct TerminalRuntime::State {
#if defined( EE_PLATFORM_POSIX )
	termios termiosState{};
	bool hasTermios{ false };
#elif EE_PLATFORM == EE_PLATFORM_WIN
	HANDLE input{ INVALID_HANDLE_VALUE };
	HANDLE output{ INVALID_HANDLE_VALUE };
	DWORD inputMode{ 0 };
	DWORD outputMode{ 0 };
	bool hasInputMode{ false };
	bool hasOutputMode{ false };
#endif
};

TerminalRuntime& TerminalRuntime::instance() {
	static TerminalRuntime runtime;
	return runtime;
}

TerminalRuntime::~TerminalRuntime() {
	shutdown();
}

bool TerminalRuntime::initialize() {
	if ( mState )
		return true;
#if defined( EE_PLATFORM_POSIX )
	mFd = ::open( "/dev/tty", O_RDWR | O_CLOEXEC );
	if ( mFd < 0 ) {
		Log::error( "Terminal runtime could not open /dev/tty" );
		return false;
	}
	mState = new State;
	mState->hasTermios = 0 == tcgetattr( mFd, &mState->termiosState );
	if ( mState->hasTermios ) {
		termios raw = mState->termiosState;
		cfmakeraw( &raw );
		tcsetattr( mFd, TCSAFLUSH, &raw );
	}
	static constexpr char enter[] = "\033[?1049h\033[?25l\033[>31u\033[?1003h\033[?1006h"
									"\033[?1016h\033[?1004h\033[H";
	if ( write( enter, sizeof( enter ) - 1 ) )
		return true;
	if ( mState->hasTermios )
		tcsetattr( mFd, TCSAFLUSH, &mState->termiosState );
	::close( mFd );
	mFd = -1;
	delete mState;
	mState = nullptr;
	return false;
#elif EE_PLATFORM == EE_PLATFORM_WIN
	mState = new State;
	mState->input = GetStdHandle( STD_INPUT_HANDLE );
	mState->output = GetStdHandle( STD_OUTPUT_HANDLE );
	if ( !mState->input || mState->input == INVALID_HANDLE_VALUE || !mState->output ||
		 mState->output == INVALID_HANDLE_VALUE ) {
		Log::error( "Terminal runtime could not open the Windows standard handles" );
		delete mState;
		mState = nullptr;
		return false;
	}
	mState->hasInputMode = GetConsoleMode( mState->input, &mState->inputMode );
	mState->hasOutputMode = GetConsoleMode( mState->output, &mState->outputMode );
	if ( mState->hasInputMode ) {
		const DWORD mode = ( mState->inputMode | ENABLE_VIRTUAL_TERMINAL_INPUT ) &
						   ~( ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT );
		SetConsoleMode( mState->input, mode );
	}
	if ( mState->hasOutputMode ) {
		const DWORD mode =
			mState->outputMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
		SetConsoleMode( mState->output, mode );
	}
	static constexpr char enter[] = "\033[?1049h\033[?25l\033[>31u\033[?1003h\033[?1006h"
									"\033[?1016h\033[?1004h\033[H";
	if ( write( enter, sizeof( enter ) - 1 ) )
		return true;
	if ( mState->hasInputMode )
		SetConsoleMode( mState->input, mState->inputMode );
	if ( mState->hasOutputMode )
		SetConsoleMode( mState->output, mState->outputMode );
	delete mState;
	mState = nullptr;
	return false;
#else
	Log::error( "Terminal runtime is not implemented on this platform" );
	return false;
#endif
}

void TerminalRuntime::shutdown() {
	detach();
#if defined( EE_PLATFORM_POSIX )
	if ( mFd < 0 )
		return;
	static constexpr char leave[] = "\033[?1004l\033[?1016l\033[?1006l\033[?1003l\033[<u"
									"\033[?25h\033[?1049l";
	write( leave, sizeof( leave ) - 1 );
	if ( mState && mState->hasTermios )
		tcsetattr( mFd, TCSAFLUSH, &mState->termiosState );
	::close( mFd );
	mFd = -1;
	delete mState;
	mState = nullptr;
#elif EE_PLATFORM == EE_PLATFORM_WIN
	if ( !mState )
		return;
	static constexpr char leave[] = "\033[?1004l\033[?1016l\033[?1006l\033[?1003l\033[<u"
									"\033[?25h\033[?1049l";
	write( leave, sizeof( leave ) - 1 );
	if ( mState->hasInputMode )
		SetConsoleMode( mState->input, mState->inputMode );
	if ( mState->hasOutputMode )
		SetConsoleMode( mState->output, mState->outputMode );
	delete mState;
	mState = nullptr;
#endif
}

bool TerminalRuntime::write( const char* data, size_t size ) {
#if defined( EE_PLATFORM_POSIX )
	std::lock_guard<std::mutex> lock( mWriteMutex );
	while ( size ) {
		const ssize_t written = ::write( mFd, data, size );
		if ( written < 0 && errno == EINTR )
			continue;
		if ( written <= 0 )
			return false;
		data += written;
		size -= static_cast<size_t>( written );
	}
	return true;
#elif EE_PLATFORM == EE_PLATFORM_WIN
	std::lock_guard<std::mutex> lock( mWriteMutex );
	while ( size ) {
		DWORD written = 0;
		const DWORD chunk = static_cast<DWORD>( eemin<size_t>( size, MAXDWORD ) );
		if ( !WriteFile( mState->output, data, chunk, &written, nullptr ) || written == 0 )
			return false;
		data += written;
		size -= written;
	}
	return true;
#else
	return false;
#endif
}

Math::Sizei TerminalRuntime::pixelSize() const {
#if defined( EE_PLATFORM_POSIX )
	winsize size{};
	if ( mFd >= 0 && 0 == ioctl( mFd, TIOCGWINSZ, &size ) && size.ws_xpixel && size.ws_ypixel )
		return { size.ws_xpixel, size.ws_ypixel };
#elif EE_PLATFORM == EE_PLATFORM_WIN
	if ( mState ) {
		CONSOLE_SCREEN_BUFFER_INFO info{};
		CONSOLE_FONT_INFO font{};
		if ( GetConsoleScreenBufferInfo( mState->output, &info ) &&
			 GetCurrentConsoleFont( mState->output, FALSE, &font ) ) {
			const Int32 columns = info.srWindow.Right - info.srWindow.Left + 1;
			const Int32 rows = info.srWindow.Bottom - info.srWindow.Top + 1;
			return { columns * font.dwFontSize.X, rows * font.dwFontSize.Y };
		}
	}
#endif
	return {};
}

void TerminalRuntime::attach( Window& window ) {
	if ( mReading.exchange( true ) )
		return;
	mWindow = &window;
	mInputThread = std::thread( &TerminalRuntime::readInput, this );
}

void TerminalRuntime::detach() {
	if ( !mReading.exchange( false ) )
		return;
	if ( mInputThread.joinable() )
		mInputThread.join();
	mWindow = nullptr;
}

namespace {
Uint32 decodeModifiers( unsigned int value ) {
	const unsigned int bits = value ? value - 1 : 0;
	return ( bits & 1 ? KEYMOD_SHIFT : 0 ) | ( bits & 2 ? KEYMOD_ALT : 0 ) |
		   ( bits & 4 ? KEYMOD_CTRL : 0 ) | ( bits & 8 ? KEYMOD_META : 0 );
}

Scancode decodeScancode( Uint32 code ) {
	if ( code >= 'a' && code <= 'z' )
		return static_cast<Scancode>( SCANCODE_A + code - 'a' );
	if ( code >= 'A' && code <= 'Z' )
		return static_cast<Scancode>( SCANCODE_A + code - 'A' );
	if ( code >= '1' && code <= '9' )
		return static_cast<Scancode>( SCANCODE_1 + code - '1' );
	if ( code == '0' )
		return SCANCODE_0;
	if ( code == static_cast<Uint32>( KEY_UP ) )
		return SCANCODE_UP;
	if ( code == static_cast<Uint32>( KEY_DOWN ) )
		return SCANCODE_DOWN;
	if ( code == static_cast<Uint32>( KEY_LEFT ) )
		return SCANCODE_LEFT;
	if ( code == static_cast<Uint32>( KEY_RIGHT ) )
		return SCANCODE_RIGHT;
	if ( code == static_cast<Uint32>( KEY_HOME ) )
		return SCANCODE_HOME;
	if ( code == static_cast<Uint32>( KEY_END ) )
		return SCANCODE_END;
	if ( code == static_cast<Uint32>( KEY_PAGEUP ) )
		return SCANCODE_PAGEUP;
	if ( code == static_cast<Uint32>( KEY_PAGEDOWN ) )
		return SCANCODE_PAGEDOWN;
	if ( code == static_cast<Uint32>( KEY_INSERT ) )
		return SCANCODE_INSERT;
	if ( code == static_cast<Uint32>( KEY_DELETE ) )
		return SCANCODE_DELETE;
	if ( code >= static_cast<Uint32>( KEY_F1 ) && code <= static_cast<Uint32>( KEY_F12 ) )
		return static_cast<Scancode>( SCANCODE_F1 + code - static_cast<Uint32>( KEY_F1 ) );
	switch ( code ) {
		case 9:
			return SCANCODE_TAB;
		case 13:
			return SCANCODE_RETURN;
		case 27:
			return SCANCODE_ESCAPE;
		case 32:
			return SCANCODE_SPACE;
		case 127:
			return SCANCODE_BACKSPACE;
		default:
			return SCANCODE_UNKNOWN;
	}
}

void enqueueKey( Window& window, Uint32 code, Uint32 mods, Uint32 kind, Uint32 text ) {
	InputEvent event( kind == 3 ? InputEvent::KeyUp : InputEvent::KeyDown );
	event.key = { 0,
				  static_cast<Uint8>( kind == 3 ? 0 : 1 ),
				  static_cast<Uint8>( kind == 2 ),
				  { decodeScancode( code ), static_cast<Keycode>( code ), mods, text } };
	window.getInput()->enqueueEvent( event );
	if ( text && kind != 3 ) {
		InputEvent input( InputEvent::TextInput );
		input.text = { 0, static_cast<String::StringBaseType>( text ) };
		window.getInput()->enqueueEvent( input );
	}
}

Uint32 navigationCode( char final, unsigned int parameter ) {
	if ( parameter == 1 && final >= 'P' && final <= 'S' )
		return KEY_F1 + final - 'P';
	if ( final == 'A' )
		return KEY_UP;
	if ( final == 'B' )
		return KEY_DOWN;
	if ( final == 'C' )
		return KEY_RIGHT;
	if ( final == 'D' )
		return KEY_LEFT;
	if ( final == 'H' )
		return KEY_HOME;
	if ( final == 'F' )
		return KEY_END;
	if ( final == '~' ) {
		if ( parameter == 2 )
			return KEY_INSERT;
		if ( parameter == 3 )
			return KEY_DELETE;
		if ( parameter == 5 )
			return KEY_PAGEUP;
		if ( parameter == 6 )
			return KEY_PAGEDOWN;
		switch ( parameter ) {
			case 15:
				return KEY_F5;
			case 17:
				return KEY_F6;
			case 18:
				return KEY_F7;
			case 19:
				return KEY_F8;
			case 20:
				return KEY_F9;
			case 21:
				return KEY_F10;
			case 23:
				return KEY_F11;
			case 24:
				return KEY_F12;
		}
	}
	return KEY_UNKNOWN;
}
} // namespace

void TerminalRuntime::readInput() {
	std::string pending;
	pending.reserve( 256 );
	Math::Sizei lastSize = pixelSize();
	Vector2i lastMouse;
	while ( mReading.load( std::memory_order_relaxed ) ) {
#if defined( EE_PLATFORM_POSIX )
		pollfd descriptor{ mFd, POLLIN, 0 };
		if ( ::poll( &descriptor, 1, 50 ) > 0 && descriptor.revents & POLLIN ) {
			char data[256];
			const ssize_t count = ::read( mFd, data, sizeof( data ) );
			if ( count > 0 )
				pending.append( data, static_cast<size_t>( count ) );
		}
#elif EE_PLATFORM == EE_PLATFORM_WIN
		if ( WAIT_OBJECT_0 == WaitForSingleObject( mState->input, 50 ) ) {
			char data[256];
			DWORD count = 0;
			if ( ReadFile( mState->input, data, sizeof( data ), &count, nullptr ) && count > 0 )
				pending.append( data, count );
		}
#endif
		while ( pending.size() >= 3 && pending[0] == '\033' && pending[1] == '[' ) {
			size_t end = 2;
			while ( end < pending.size() && !( pending[end] >= '@' && pending[end] <= '~' ) )
				++end;
			if ( end == pending.size() )
				break;
			const char final = pending[end];
			const std::string params = pending.substr( 2, end - 2 );
			if ( params.empty() && ( final == 'I' || final == 'O' ) ) {
				mFocused.store( final == 'I', std::memory_order_relaxed );
				InputEvent event( InputEvent::Window );
				event.window = { static_cast<Uint8>( final == 'I' ),
								 static_cast<Uint8>( final == 'I'
														 ? InputEvent::WindowKeyboardFocusGain
														 : InputEvent::WindowKeyboardFocusLost ) };
				mWindow->getInput()->enqueueEvent( event );
			} else if ( !params.empty() && params[0] == '<' && ( final == 'M' || final == 'm' ) ) {
				unsigned int button, x, y;
				if ( 3 == std::sscanf( params.c_str(), "<%u;%u;%u", &button, &x, &y ) ) {
					InputEvent event;
					if ( button & 64 ) {
						const unsigned int wheelCode = button & 3;
						const Uint8 wheelButton = wheelCode == 0   ? EE_BUTTON_WHEELUP
												  : wheelCode == 1 ? EE_BUTTON_WHEELDOWN
												  : wheelCode == 2 ? EE_BUTTON_WHEELLEFT
																   : EE_BUTTON_WHEELRIGHT;
						event.Type = InputEvent::MouseButtonDown;
						event.button = { 0, wheelButton, 1, static_cast<Int16>( x - 1 ),
										 static_cast<Int16>( y - 1 ) };
						mWindow->getInput()->enqueueEvent( event );
						event.Type = InputEvent::MouseButtonUp;
						event.button.state = 0;
						mWindow->getInput()->enqueueEvent( event );
						event.Type = InputEvent::MouseWheel;
						event.wheel = { 0,
										wheelCode == 2	 ? -1.f
										: wheelCode == 3 ? 1.f
														 : 0.f,
										wheelCode == 0	 ? 1.f
										: wheelCode == 1 ? -1.f
														 : 0.f,
										InputEvent::WheelEvent::Normal };
					} else if ( button & 32 ) {
						event.Type = InputEvent::MouseMotion;
						event.motion = { 0,
										 0,
										 static_cast<Int16>( x - 1 ),
										 static_cast<Int16>( y - 1 ),
										 static_cast<Int16>( x - 1 - lastMouse.x ),
										 static_cast<Int16>( y - 1 - lastMouse.y ) };
						lastMouse = { static_cast<Int32>( x - 1 ), static_cast<Int32>( y - 1 ) };
					} else {
						event.Type =
							final == 'm' ? InputEvent::MouseButtonUp : InputEvent::MouseButtonDown;
						event.button = { 0, static_cast<Uint8>( ( button & 3 ) + 1 ),
										 static_cast<Uint8>( final != 'm' ),
										 static_cast<Int16>( x - 1 ), static_cast<Int16>( y - 1 ) };
					}
					mWindow->getInput()->enqueueEvent( event );
				}
			} else if ( final == 'u' ) {
				unsigned int code = 0, mods = 1, kind = 1, text = 0;
				const size_t separator = params.find( ';' );
				if ( separator != std::string::npos ) {
					code = static_cast<unsigned int>( std::strtoul( params.c_str(), nullptr, 10 ) );
					std::sscanf( params.c_str() + separator + 1, "%u:%u;%u", &mods, &kind, &text );
					enqueueKey( *mWindow, code, decodeModifiers( mods ), kind, text );
				}
			} else if ( final == 'A' || final == 'B' || final == 'C' || final == 'D' ||
						final == 'H' || final == 'F' || final == 'P' || final == 'Q' ||
						final == 'R' || final == 'S' || final == '~' ) {
				unsigned int parameter = 1, mods = 1, kind = 1;
				std::sscanf( params.c_str(), "%u;%u:%u", &parameter, &mods, &kind );
				const Uint32 code = navigationCode( final, parameter );
				if ( code != KEY_UNKNOWN )
					enqueueKey( *mWindow, code, decodeModifiers( mods ), kind, 0 );
			}
			pending.erase( 0, end + 1 );
		}
		if ( !pending.empty() && pending[0] != '\033' )
			pending.erase( 0, 1 );
		const Math::Sizei size = pixelSize();
		if ( size.x > 0 && size.y > 0 && size != lastSize ) {
			lastSize = size;
			InputEvent event( InputEvent::VideoResize );
			event.resize = { size.x, size.y };
			mWindow->getInput()->enqueueEvent( event );
		}
	}
}

}} // namespace EE::Window
