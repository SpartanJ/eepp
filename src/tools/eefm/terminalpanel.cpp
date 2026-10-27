#include "terminalpanel.hpp"
#include "app.hpp"
#include "panemanager.hpp"

#include <eepp/graphics/fontfamily.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/process.hpp>
#include <eterm/ui/uiterminal.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN
// Lean: the RPC headers declare a UUID that clashes with EE::System::UUID, which eterm's
// headers bring into the global namespace.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// windows.h first: tlhelp32.h depends on it.
#include <windows.h>

#include <tlhelp32.h>
#elif EE_PLATFORM == EE_PLATFORM_MACOS
#include <libproc.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#elif EE_PLATFORM == EE_PLATFORM_BSD
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/user.h>
#endif

#if defined( EE_PLATFORM_POSIX )
#include <climits>
#include <cstdlib>
#include <fstream>
#include <unistd.h>
#endif

using namespace eterm::Terminal;
using namespace eterm::UI;

namespace eefm {

namespace {

/** The shell command that changes the working directory to path, quoted for the shell. */
std::string changeDirectoryCommand( const std::string& path ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return "cd \"" + path + "\"";
#else
	std::string quoted = path;
	String::replaceAll( quoted, "'", "'\\''" );
	// The leading space keeps it out of the history (HISTCONTROL=ignorespace, fish, zsh).
	return " cd -- '" + quoted + "'";
#endif
}

/** What the shell is doing, as far as the platform tells. */
struct ShellState {
	/** A command runs in the foreground (or, on Windows, the shell has children). */
	bool busy{ false };
	/** The shell's working directory, empty if unknown. */
	std::string directory;
};

ShellState shellState( int pid ) {
	ShellState state;
	if ( pid <= 0 )
		return state;
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_ANDROID
	// /proc/<pid>/stat: "pid (comm) state ppid pgrp session tty_nr tpgid ...". The terminal's
	// foreground process group (tpgid) is the shell's own group only while it waits at its
	// prompt.
	std::ifstream stat( "/proc/" + std::to_string( pid ) + "/stat" );
	std::string line;
	if ( std::getline( stat, line ) ) {
		size_t close = line.rfind( ')' );
		if ( close != std::string::npos ) {
			auto fields = String::split( line.substr( close + 1 ), ' ' );
			// fields: state ppid pgrp session tty_nr tpgid
			Int64 pgrp = 0, tpgid = 0;
			if ( fields.size() > 5 && String::fromString( pgrp, fields[2] ) &&
				 String::fromString( tpgid, fields[5] ) )
				state.busy = tpgid > 0 && tpgid != pgrp;
		}
	}
	char buffer[PATH_MAX];
	ssize_t length = readlink( ( "/proc/" + std::to_string( pid ) + "/cwd" ).c_str(), buffer,
							   sizeof( buffer ) - 1 );
	if ( length > 0 )
		state.directory.assign( buffer, length );
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	struct kinfo_proc info {};
	size_t size = sizeof( info );
	int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, pid };
	if ( sysctl( mib, 4, &info, &size, nullptr, 0 ) == 0 && size > 0 )
		state.busy = info.kp_eproc.e_tpgid > 0 && info.kp_eproc.e_tpgid != info.kp_eproc.e_pgid;
	struct proc_vnodepathinfo vnode {};
	if ( proc_pidinfo( pid, PROC_PIDVNODEPATHINFO, 0, &vnode, sizeof( vnode ) ) == sizeof( vnode ) )
		state.directory = vnode.pvi_cdir.vip_path;
#elif EE_PLATFORM == EE_PLATFORM_BSD && defined( __FreeBSD__ )
	struct kinfo_proc info {};
	size_t size = sizeof( info );
	int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, pid };
	if ( sysctl( mib, 4, &info, &size, nullptr, 0 ) == 0 && size > 0 )
		state.busy = info.ki_tpgid > 0 && info.ki_tpgid != info.ki_pgid;
#elif EE_PLATFORM == EE_PLATFORM_WIN
	// No foreground process groups: any child process means the shell runs a command.
	HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if ( snapshot != INVALID_HANDLE_VALUE ) {
		PROCESSENTRY32 entry{};
		entry.dwSize = sizeof( entry );
		for ( BOOL more = Process32First( snapshot, &entry ); more && !state.busy;
			  more = Process32Next( snapshot, &entry ) )
			state.busy = entry.th32ParentProcessID == static_cast<DWORD>( pid );
		CloseHandle( snapshot );
	}
#endif
	return state;
}

/** Same directory, comparing resolved paths (the shell reports the real path). */
bool sameDirectory( std::string a, std::string b ) {
#if defined( EE_PLATFORM_POSIX )
	char buffer[PATH_MAX];
	if ( ::realpath( a.c_str(), buffer ) )
		a = buffer;
	if ( ::realpath( b.c_str(), buffer ) )
		b = buffer;
#endif
	FileSystem::dirRemoveSlashAtEnd( a );
	FileSystem::dirRemoveSlashAtEnd( b );
#if EE_PLATFORM == EE_PLATFORM_WIN
	return String::iequals( a, b );
#else
	return a == b;
#endif
}

/** The folder of a tab as a local path, empty for remote folders. */
std::string localDirectory( FileTab* tab ) {
	if ( !tab || tab->model->uri().getScheme() != "file" )
		return {};
	return tab->model->uri().getFSPath();
}

} // namespace

TerminalPanel::TerminalPanel( App& app, UISplitter* splitter ) :
	mApp( app ), mUI( app.ui() ), mSplitter( splitter ) {}

TerminalPanel::~TerminalPanel() = default;

void TerminalPanel::setTerminalFont( FontTrueType* font ) {
	mTerminalFont = font;
	FontFamily::loadFromRegular( mTerminalFont );
	if ( mTerminal ) {
		mTerminal->setFont( mTerminalFont );
		mTerminal->setFontSize( PixelDensity::dpToPx( mApp.config().terminal.fontSize ) );
	}
}

void TerminalPanel::setFontSize( Float sizeDp ) {
	if ( mTerminal )
		mTerminal->setFontSize( PixelDensity::dpToPx( sizeDp ) );
}

void TerminalPanel::syncFontRendering() {
	if ( mTerminal )
		mTerminal->syncFontRenderingConfig();
}

void TerminalPanel::toggle() {
	show( !mTerminal || mTerminal->getParent() != mSplitter );
}

bool TerminalPanel::isVisible() const {
	return mTerminal && mTerminal->getParent() == mSplitter;
}

void TerminalPanel::show( bool show ) {
	if ( show ) {
		if ( !mTerminal && !create() )
			return;
		if ( mTerminal->getParent() != mSplitter ) {
			mTerminal->setParent( mSplitter );
			mTerminal->setVisible( true );
			mSplitter->setSplitPartition( StyleSheetLength( mPartition ) );
		}
		mTerminal->setFocus();
		syncDirectory();
	} else if ( isVisible() ) {
		rememberSize();
		// Parked, not closed: the shell keeps running until the next F4.
		mTerminal->setParent( mApp.parking() );
		if ( auto* tab = mApp.panes().currentTab() )
			tab->view->setFocus();
	}
}

bool TerminalPanel::create() {
	if ( !mTerminalFont ) {
		// The configured font, else the bundled one.
		auto& scope = *mUI->getResourceScope();
		FontTrueType* font = nullptr;
		if ( !mApp.config().terminal.fontPath.empty() ) {
			auto configured = FontTrueType::New( "eefm-monospace", scope );
			if ( configured->loadFromFile( mApp.config().terminal.fontPath ) )
				font = configured.get();
		}
		if ( !font ) {
			auto bundled = FontTrueType::New( "eefm-monospace-default", scope );
			if ( bundled->loadFromFile( Sys::getResourcePath( "eefm" ) +
										"assets/fonts/DejaVuSansMono.ttf" ) )
				font = bundled.get();
		}
		if ( !font ) {
			Dialogs::showError( mUI, mUI->i18n( "eefm_terminal_font_failed",
												"The terminal font could not be loaded." ) );
			return false;
		}
		FontFamily::loadFromRegular( font );
		mTerminalFont = font;
	}
	std::string directory = localDirectory( mApp.panes().currentTab() );
	if ( directory.empty() )
		directory = Sys::getUserDirectory();
	Sizef size(
		mSplitter->getPixelsSize().getWidth(),
		eemax( PixelDensity::dpToPx( 120 ), mSplitter->getPixelsSize().getHeight() * 0.3f ) );
	mTerminal = UITerminal::New(
		mTerminalFont, PixelDensity::dpToPx( mApp.config().terminal.fontSize ), size,
		mApp.config().terminal.shell, Process::parseArgs( mApp.config().terminal.shellArguments ),
		{}, directory, mApp.config().terminal.historySize );
	if ( !mTerminal || !mTerminal->getTerm() ) {
		if ( mTerminal )
			mTerminal->close();
		mTerminal = nullptr;
		Dialogs::showError(
			mUI, mUI->i18n( "eefm_terminal_failed", "The terminal could not be started." ) );
		return false;
	}
	mTerminal->setParent( mApp.parking() );
	applyConfig();
	mTerminalDirectory = directory;
	mTerminalFollowed.clear();
	// F4 reaches the terminal (it takes every key): it hides the panel from there too.
	mTerminal->setCommand( "eefm-toggle-terminal", [this] { show( false ); } );
	mTerminal->addKeyBinding( { KEY_F4 }, "eefm-toggle-terminal" );
	// When the shell exits the panel closes; the next F4 starts a new one.
	mTerminal->getTerm()->pushEventCallback( [this]( const TerminalDisplay::Event& event ) {
		if ( event.type == TerminalDisplay::EventType::PROCESS_EXIT )
			mUI->runOnMainThread( [this] { close(); } );
	} );
	return true;
}

void TerminalPanel::close() {
	if ( !mTerminal )
		return;
	bool visible = isVisible();
	if ( visible )
		rememberSize();
	mTerminal->close();
	mTerminal = nullptr;
	mTerminalDirectory.clear();
	mTerminalFollowed.clear();
	if ( visible ) {
		if ( auto* tab = mApp.panes().currentTab() )
			tab->view->setFocus();
	}
}

void TerminalPanel::rememberSize() {
	if ( !isVisible() || mSplitter->getPixelsSize().getHeight() <= 0 )
		return;
	Float files = mSplitter->getPixelsSize().getHeight() - mTerminal->getPixelsSize().getHeight();
	mPartition = String::format(
		"%.2f%%", eeclamp( files / mSplitter->getPixelsSize().getHeight() * 100.f, 10.f, 90.f ) );
}

void TerminalPanel::syncDirectory() {
	// As Dolphin: the shell follows the active pane, but only while it waits at its prompt: not
	// with a full screen program (alternate screen) nor any other command running. Remote
	// folders have no local path to go to.
	if ( !mApp.config().terminal.followActivePane || !isVisible() || !mTerminal->getTerm() ||
		 mTerminal->getTerm()->isAltScr() )
		return;
	std::string directory = localDirectory( mApp.panes().currentTab() );
	if ( directory.empty() )
		return;
	// Only when the active pane or its folder changed (not on every refresh of the chrome).
	std::string followed = String::toString( mApp.panes().activeIndex() ) + ":" + directory;
	if ( followed == mTerminalFollowed )
		return;
	ShellState shell = shellState( mTerminal->getTerm()->getProcessId() );
	if ( shell.busy )
		return;
	mTerminalFollowed = followed;
	// Already there (the user may have changed it in the shell): nothing to type. Right after a
	// cd the shell may not have run it yet: trust what was sent.
	bool justSent = mTerminalDirectorySent.getElapsedTime() < Milliseconds( 500 );
	const std::string& current =
		shell.directory.empty() || justSent ? mTerminalDirectory : shell.directory;
	if ( sameDirectory( current, directory ) ) {
		mTerminalDirectory = directory;
		return;
	}
	mTerminalDirectory = directory;
	mTerminalDirectorySent.restart();
	mTerminal->getTerm()->executeFile( changeDirectoryCommand( directory ) );
}

const std::map<std::string, TerminalColorScheme>& TerminalPanel::colorSchemes() {
	if ( !mTerminalColorSchemesLoaded ) {
		mTerminalColorSchemesLoaded = true;
		// The schemes eterm ships, then the user's (eterm's config folder: shared with eterm).
		auto schemes = TerminalColorScheme::loadFromFile(
			Sys::getResourcePath( "eefm" ) + "assets/colorschemes/terminalcolorschemes.conf" );
		std::string userSchemes =
			Sys::getConfigPath( "eterm" ) + FileSystem::getOSSlash() + "colorschemes";
		if ( FileSystem::isDirectory( userSchemes ) ) {
			for ( const auto& file : FileSystem::filesGetInPath( userSchemes ) ) {
				auto more = TerminalColorScheme::loadFromFile( userSchemes +
															   FileSystem::getOSSlash() + file );
				schemes.insert( schemes.end(), std::make_move_iterator( more.begin() ),
								std::make_move_iterator( more.end() ) );
			}
		}
		for ( auto& scheme : schemes ) {
			std::string name = scheme.getName();
			mTerminalColorSchemes.emplace( std::move( name ), std::move( scheme ) );
		}
	}
	return mTerminalColorSchemes;
}

void TerminalPanel::applyConfig() {
	if ( !mTerminal || !mTerminal->getTerm() )
		return;
	const auto& schemes = colorSchemes();
	auto found = schemes.find( mApp.config().terminal.colorScheme );
	mTerminal->setColorScheme( found != schemes.end() ? found->second
													  : TerminalColorScheme::getDefault() );
	mTerminal->getTerm()->setCursorMode( mApp.config().terminal.cursorStyle );
	mTerminal->setVerticalScrollMode( mApp.config().terminal.scrollBarMode );
}

} // namespace eefm
