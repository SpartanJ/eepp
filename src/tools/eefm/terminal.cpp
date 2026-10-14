#include "app.hpp"

#include <eepp/graphics/fontfamily.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/process.hpp>
#include <eterm/ui/uiterminal.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN
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
	struct kinfo_proc info{};
	size_t size = sizeof( info );
	int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, pid };
	if ( sysctl( mib, 4, &info, &size, nullptr, 0 ) == 0 && size > 0 )
		state.busy = info.kp_eproc.e_tpgid > 0 && info.kp_eproc.e_tpgid != info.kp_eproc.e_pgid;
	struct proc_vnodepathinfo vnode{};
	if ( proc_pidinfo( pid, PROC_PIDVNODEPATHINFO, 0, &vnode, sizeof( vnode ) ) == sizeof( vnode ) )
		state.directory = vnode.pvi_cdir.vip_path;
#elif EE_PLATFORM == EE_PLATFORM_BSD && defined( __FreeBSD__ )
	struct kinfo_proc info{};
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

void App::toggleTerminal() {
	showTerminal( !mTerminal || mTerminal->getParent() != mTerminalSplitter );
}

bool App::isTerminalVisible() const {
	return mTerminal && mTerminal->getParent() == mTerminalSplitter;
}

void App::showTerminal( bool show ) {
	if ( show ) {
		if ( !mTerminal && !createTerminal() )
			return;
		if ( mTerminal->getParent() != mTerminalSplitter ) {
			mTerminal->setParent( mTerminalSplitter );
			mTerminal->setVisible( true );
			mTerminalSplitter->setSplitPartition( StyleSheetLength( mTerminalPartition ) );
		}
		mTerminal->setFocus();
		syncTerminalDirectory();
	} else if ( isTerminalVisible() ) {
		rememberTerminalSize();
		// Parked, not closed: the shell keeps running until the next F4.
		mTerminal->setParent( mParking );
		if ( auto* tab = currentTab() )
			tab->view->setFocus();
	}
}

bool App::createTerminal() {
	if ( !mTerminalFont ) {
		// The configured font, else the bundled one.
		auto& scope = *mUI->getResourceScope();
		FontTrueType* font = nullptr;
		if ( !mConfig->terminal.fontPath.empty() ) {
			auto configured = FontTrueType::New( "eefm-monospace", scope );
			if ( configured->loadFromFile( mConfig->terminal.fontPath ) )
				font = configured.get();
		}
		if ( !font ) {
			auto bundled = FontTrueType::New( "eefm-monospace-default", scope );
			if ( bundled->loadFromFile( Sys::getProcessPath() +
										"assets/fonts/DejaVuSansMono.ttf" ) )
				font = bundled.get();
		}
		if ( !font ) {
			showError( mUI, mUI->i18n( "eefm_terminal_font_failed",
									   "The terminal font could not be loaded." ) );
			return false;
		}
		FontFamily::loadFromRegular( font );
		mTerminalFont = font;
	}
	std::string directory = localDirectory( currentTab() );
	if ( directory.empty() )
		directory = Sys::getUserDirectory();
	Sizef size( mTerminalSplitter->getPixelsSize().getWidth(),
				eemax( PixelDensity::dpToPx( 120 ),
					   mTerminalSplitter->getPixelsSize().getHeight() * 0.3f ) );
	mTerminal = UITerminal::New( mTerminalFont, PixelDensity::dpToPx( mConfig->terminal.fontSize ),
								 size, mConfig->terminal.shell,
								 Process::parseArgs( mConfig->terminal.shellArguments ), {},
								 directory, mConfig->terminal.historySize );
	if ( !mTerminal || !mTerminal->getTerm() ) {
		if ( mTerminal )
			mTerminal->close();
		mTerminal = nullptr;
		showError( mUI, mUI->i18n( "eefm_terminal_failed", "The terminal could not be started." ) );
		return false;
	}
	mTerminal->setParent( mParking );
	applyTerminalConfig();
	mTerminalDirectory = directory;
	mTerminalFollowed.clear();
	// F4 reaches the terminal (it takes every key): it hides the panel from there too.
	mTerminal->setCommand( "eefm-toggle-terminal", [this] { showTerminal( false ); } );
	mTerminal->addKeyBinding( { KEY_F4 }, "eefm-toggle-terminal" );
	// When the shell exits the panel closes; the next F4 starts a new one.
	mTerminal->getTerm()->pushEventCallback( [this]( const TerminalDisplay::Event& event ) {
		if ( event.type == TerminalDisplay::EventType::PROCESS_EXIT )
			mUI->runOnMainThread( [this] { closeTerminal(); } );
	} );
	return true;
}

void App::closeTerminal() {
	if ( !mTerminal )
		return;
	bool visible = isTerminalVisible();
	if ( visible )
		rememberTerminalSize();
	mTerminal->close();
	mTerminal = nullptr;
	mTerminalDirectory.clear();
	mTerminalFollowed.clear();
	if ( visible )
		if ( auto* tab = currentTab() )
			tab->view->setFocus();
}

void App::rememberTerminalSize() {
	if ( !isTerminalVisible() || mTerminalSplitter->getPixelsSize().getHeight() <= 0 )
		return;
	Float files =
		mTerminalSplitter->getPixelsSize().getHeight() - mTerminal->getPixelsSize().getHeight();
	mTerminalPartition = String::format(
		"%.2f%%",
		eeclamp( files / mTerminalSplitter->getPixelsSize().getHeight() * 100.f, 10.f, 90.f ) );
}

void App::syncTerminalDirectory() {
	// As Dolphin: the shell follows the active pane, but only while it waits at its prompt: not
	// with a full screen program (alternate screen) nor any other command running. Remote
	// folders have no local path to go to.
	if ( !mConfig->terminal.followActivePane || !isTerminalVisible() || !mTerminal->getTerm() ||
		 mTerminal->getTerm()->isAltScr() )
		return;
	std::string directory = localDirectory( currentTab() );
	if ( directory.empty() )
		return;
	// Only when the active pane or its folder changed (not on every refresh of the chrome).
	std::string followed = String::toString( mActivePane ) + ":" + directory;
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

} // namespace eefm
