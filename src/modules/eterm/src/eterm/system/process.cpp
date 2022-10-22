// The MIT License (MIT)

// Copyright (c) 2020 Fredrik A. Kristiansen

//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.

#ifndef _WIN32
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eterm/system/process.hpp>
#include <poll.h>
#include <pwd.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined( __linux )
#include <pty.h>
#elif defined( __OpenBSD__ ) || defined( __NetBSD__ ) || defined( __APPLE__ )
#include <sys/errno.h>
#include <util.h>
#elif defined( __FreeBSD__ ) || defined( __DragonFly__ )
#include <libutil.h>
#include <sys/errno.h>
#endif

using namespace EE::System;

namespace eterm { namespace System {

Process::~Process() {
	if ( mPID != -1 ) {
		kill( mPID, SIGHUP );
	}
}

void Process::checkExitStatus() {
	if ( mStatus == ProcessStatus::EXITED ) {
		return;
	}
	int status = 0;
	int p = waitpid( mPID, &status, WNOHANG );
	if ( p < 0 ) {
		perror( "Process::checkExitStatus(waitpid)" );
	}
	if ( p == 0 ) {
		return;
	}
	mStatus = ProcessStatus::EXITED;
	mExitCode = WEXITSTATUS( status );
}

void Process::terminate() {
	if ( mStatus == ProcessStatus::EXITED ) {
		return;
	}
	if ( mPID != -1 ) {
		kill( mPID, SIGHUP );
		mExitCode = 1;
		mStatus = ProcessStatus::EXITED;
	}
}

void Process::waitForExit() {
	if ( mStatus == ProcessStatus::EXITED ) {
		return;
	}
	int status = 0;
	int p = waitpid( mPID, &status, 0 );
	if ( p < 0 ) {
		perror( "Process::waitForExit(waitpid)" );
		return;
	}
	mStatus = ProcessStatus::EXITED;
	mExitCode = WEXITSTATUS( status );
}

bool Process::hasExited() const {
	return mStatus == ProcessStatus::EXITED;
}

int Process::getExitCode() const {
	return mStatus == ProcessStatus::EXITED ? mExitCode : 255;
}

Process::Process( int pid ) : mPID( pid ) {}

static void execshell( const char* cmd, const char* const* args, std::string workingDirectory ) {
	const struct passwd* pw;
	const char* sh;

	errno = 0;
	if ( ( pw = getpwuid( getuid() ) ) == NULL ) {
		if ( errno ) {
			fprintf( stderr, "getpwuid: %s\n", strerror( errno ) );
			_exit( 1 );
		} else {
			fprintf( stderr, "who are you?\n" );
			_exit( 1 );
		}
	}

	if ( ( sh = getenv( "SHELL" ) ) == NULL )
		sh = ( pw->pw_shell[0] ) ? pw->pw_shell : cmd;

	if ( workingDirectory.empty() )
		workingDirectory = pw->pw_dir;

	FileSystem::changeWorkingDirectory( workingDirectory );

	unsetenv( "COLUMNS" );
	unsetenv( "LINES" );
	unsetenv( "TERMCAP" );
	setenv( "LOGNAME", pw->pw_name, 1 );
	setenv( "USER", pw->pw_name, 1 );
	setenv( "SHELL", sh, 1 );
	setenv( "HOME", pw->pw_dir, 1 );
	setenv( "TERM", "xterm-256color", 1 );

	signal( SIGCHLD, SIG_DFL );
	signal( SIGHUP, SIG_DFL );
	signal( SIGINT, SIG_DFL );
	signal( SIGQUIT, SIG_DFL );
	signal( SIGTERM, SIG_DFL );
	signal( SIGALRM, SIG_DFL );

	execvp( cmd, (char* const*)args );
	_exit( 1 );
}

std::unique_ptr<Process> Process::createWithPipe( const std::string& /*program*/,
												  const std::vector<std::string>& /*args*/,
												  const std::string& /*workingDirectory*/,
												  std::unique_ptr<IPipe>& outPipe,
												  bool /*withStderr*/ ) {
	outPipe = nullptr;
	return nullptr;
}

std::unique_ptr<Process>
Process::createWithPseudoTerminal( const std::string& program, const std::vector<std::string>& args,
								   const std::string& workingDirectory,
								   Terminal::PseudoTerminal& pseudoTerminal ) {
	int pid = fork();
	if ( pid == -1 ) {
		fprintf( stderr, "Failed to fork process\n" );
		return nullptr;
	} else if ( pid == 0 ) {
		setsid();
		dup2( (int)pseudoTerminal.mSlave, 0 );
		dup2( (int)pseudoTerminal.mSlave, 1 );
		dup2( (int)pseudoTerminal.mSlave, 2 );

		if ( ioctl( (int)pseudoTerminal.mSlave, TIOCSCTTY, NULL ) < 0 ) {
			fprintf( stderr, "ioctl TIOCSCTTY failed: %s", strerror( errno ) );
			exit( 1 );
		}

		// Make sure all non stdio file descriptors are closed before exec
		int fdlimit = (int)sysconf( _SC_OPEN_MAX );
		for ( int i = 3; i < fdlimit; i++ )
			close( i );

#ifdef __OpenBSD__
		if ( pledge( "stdio getpw proc exec", NULL ) == -1 ) {
			fprintf( stderr, "pledge\n" );
			exit( 1 );
		}
#endif
		std::vector<const char*> argsV;
		argsV.push_back( program.c_str() );
		for ( auto& a : args ) {
			argsV.push_back( a.c_str() );
		}
		argsV.push_back( nullptr );
		execshell( program.c_str(), argsV.data(), workingDirectory );
	} else {
		pseudoTerminal.mSlave.release();
		return std::unique_ptr<Process>( new Process( pid ) );
	}
	return nullptr;
}

}} // namespace eterm::System

#else

#include <assert.h>
#include <eterm/system/pipe.hpp>
#include <eterm/system/process.hpp>
#include <eterm/terminal/windowserrors.hpp>
#include <iomanip>
#include <sstream>
#define NTDDI_VERSION NTDDI_WIN10_RS5
#include <windows.h>

using namespace EE;

namespace eterm { namespace System {

static std::wstring stringToWideString( const std::string& str ) {
	if ( str.empty() )
		return std::wstring();
	int size_needed = MultiByteToWideChar( CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0 );
	std::wstring wstrTo( size_needed, 0 );
	MultiByteToWideChar( CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed );
	return wstrTo;
}

#define HANDLE_WIN_ERR( err ) HRESULT_FROM_WIN32( err ), PrintWinApiError( err )

static HRESULT InitializeStartupInfoAttachedToPseudoConsole( STARTUPINFOEXW* pStartupInfo,
															 HPCON hpc ) {
	// Prepare Startup Information structure
	STARTUPINFOEXW si;
	ZeroMemory( &si, sizeof( si ) );
	si.StartupInfo.cb = sizeof( STARTUPINFOEXW );

	// Discover the size required for the list
	SIZE_T bytesRequired;
	InitializeProcThreadAttributeList( NULL, 1, 0, &bytesRequired );

	// Allocate memory to represent the list
	si.lpAttributeList =
		(PPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc( GetProcessHeap(), 0, bytesRequired );
	if ( !si.lpAttributeList ) {
		return E_OUTOFMEMORY;
	}

	// Initialize the list memory location
	if ( !InitializeProcThreadAttributeList( si.lpAttributeList, 1, 0, &bytesRequired ) ) {
		HeapFree( GetProcessHeap(), 0, si.lpAttributeList );
		return HRESULT_FROM_WIN32( GetLastError() );
	}

	// Set the pseudoconsole information into the list
	if ( !UpdateProcThreadAttribute( si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
									 hpc, sizeof( hpc ), NULL, NULL ) ) {
		HeapFree( GetProcessHeap(), 0, si.lpAttributeList );
		return HRESULT_FROM_WIN32( GetLastError() );
	}

	*pStartupInfo = si;

	return S_OK;
}

Process::Process( AutoHandle&& hProcess, void* lpAttributeList, int pid ) :
	mStatus( ProcessStatus::RUNNING ),
	mExitCode( 1 ),
	mLeaveRunning( false ),
	mProcessHandle( std::move( hProcess ) ),
	mLpAttributeList( lpAttributeList ),
	mPID( pid ) {}

Process::~Process() {
	if ( mLpAttributeList ) {
		DeleteProcThreadAttributeList( (LPPROC_THREAD_ATTRIBUTE_LIST)mLpAttributeList );
		HeapFree( GetProcessHeap(), 0, mLpAttributeList );
	}
	if ( !mLeaveRunning ) {
		terminate();
	}
}

void Process::checkExitStatus() {
	if ( mStatus == ProcessStatus::RUNNING ) {
		DWORD exitCode;
		if ( GetExitCodeProcess( (HANDLE)mProcessHandle, &exitCode ) && exitCode != STILL_ACTIVE ) {
			mStatus = ProcessStatus::EXITED;
			mExitCode = (int)exitCode;
		}
	}
}

bool Process::hasExited() const {
	return mStatus == ProcessStatus::EXITED;
}

int Process::getExitCode() const {
	return mStatus == ProcessStatus::EXITED ? mExitCode : STILL_ACTIVE;
}

void Process::terminate() {
	if ( mStatus == ProcessStatus::RUNNING ) {
		TerminateProcess( (HANDLE)mProcessHandle, EXIT_FAILURE );
		mExitCode = EXIT_FAILURE;
		mStatus = ProcessStatus::EXITED;
		mProcessHandle.release();
	}
}

void Process::waitForExit() {
	checkExitStatus();

	if ( mStatus == ProcessStatus::RUNNING ) {
		WaitForSingleObject( (HANDLE)mProcessHandle, INFINITE );
	}
}

std::unique_ptr<Process> Process::createWithPipe( const std::string& program,
												  const std::vector<std::string>& args,
												  const std::string& workingDirectory,
												  std::unique_ptr<IPipe>& outPipe,
												  bool withStderr ) {
	outPipe = nullptr;

	SECURITY_ATTRIBUTES saAttr;
	ZeroMemory( &saAttr, sizeof( saAttr ) );
	saAttr.nLength = sizeof( saAttr );
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	AutoHandle g_hChildStd_IN_Rd{};
	AutoHandle g_hChildStd_IN_Wr{};
	AutoHandle g_hChildStd_OUT_Rd{};
	AutoHandle g_hChildStd_OUT_Wr{};

	if ( !CreatePipe( g_hChildStd_OUT_Rd.get(), g_hChildStd_OUT_Wr.get(), &saAttr, 0 ) ) {
		fprintf( stderr, "Failed to create pipe (Stdout)\n" );
		return nullptr;
	}

	if ( !SetHandleInformation( (HANDLE)g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0 ) ) {
		fprintf( stderr, "Failed to set handle information (Stdout)\n" );
		return nullptr;
	}

	if ( !CreatePipe( g_hChildStd_IN_Rd.get(), g_hChildStd_IN_Wr.get(), &saAttr, 0 ) ) {
		fprintf( stderr, "Failed to create pipe (Stdin)\n" );
		return nullptr;
	}

	if ( !SetHandleInformation( (HANDLE)g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0 ) ) {
		fprintf( stderr, "Failed to set handle information (Stdin)\n" );
		return nullptr;
	}

	PROCESS_INFORMATION piProcInfo;
	STARTUPINFOW siStartInfo;

	ZeroMemory( &piProcInfo, sizeof( PROCESS_INFORMATION ) );
	ZeroMemory( &siStartInfo, sizeof( STARTUPINFOW ) );

	siStartInfo.cb = sizeof( STARTUPINFOW );
	siStartInfo.hStdError = withStderr ? (HANDLE)g_hChildStd_OUT_Wr : INVALID_HANDLE_VALUE;
	siStartInfo.hStdOutput = (HANDLE)g_hChildStd_OUT_Wr;
	siStartInfo.hStdInput = (HANDLE)g_hChildStd_IN_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	std::ostringstream oss;
	oss << std::quoted( program );
	for ( auto& arg : args ) {
		oss << ' ' << arg;
	}
	std::wstring commandLine = stringToWideString( oss.str() );
	// TODO: If workingDirectory is relative, make it absolute (relative to the
	// current process working directory)
	std::wstring workingDir = stringToWideString( workingDirectory );

	if ( CreateProcessW( NULL, (LPWSTR)commandLine.c_str(), NULL, NULL, TRUE, 0, NULL,
						 workingDir.empty() ? NULL : workingDir.c_str(), &siStartInfo,
						 &piProcInfo ) ) {
		std::unique_ptr<Pipe> pipe = std::unique_ptr<Pipe>(
			new Pipe( std::move( g_hChildStd_OUT_Rd ), std::move( g_hChildStd_IN_Wr ) ) );
		outPipe = std::move( pipe );

		AutoHandle hProcess{ piProcInfo.hProcess };
		AutoHandle hThread{ piProcInfo.hThread };

		return std::unique_ptr<Process>(
			new Process( std::move( hProcess ), NULL, piProcInfo.dwProcessId ) );
	}

	PrintWinApiError( GetLastError() );

	return nullptr;
}

std::unique_ptr<Process>
Process::createWithPseudoTerminal( const std::string& program, const std::vector<std::string>& args,
								   const std::string& workingDirectory,
								   Terminal::PseudoTerminal& pseudoTerminal ) {
	SetEnvironmentVariableA( "WSLENV", "TERM/u" );
	SetEnvironmentVariableA( "TERM", "xterm-256color" );

	HRESULT hr{ E_UNEXPECTED };

	AutoHandle hProcess{};
	AutoHandle hThread{};

	STARTUPINFOEXW startupInfo{};
	PROCESS_INFORMATION piClient{};

	std::ostringstream oss;
	oss << std::quoted( program );
	for ( auto& arg : args ) {
		oss << ' ' << arg;
	}
	std::wstring commandLine = stringToWideString( oss.str() );
	// TODO: If workingDirectory is relative, make it absolute (relative to the
	// current process working directory)
	std::wstring workingDir = stringToWideString( workingDirectory );

	if ( ( hr = InitializeStartupInfoAttachedToPseudoConsole( &startupInfo,
															  pseudoTerminal.mPHPC ) ) != S_OK ) {
		Log::error( "InitializeStartupInfoAttachedToPseudoConsole failed." );
		PrintErrorResult( hr );
		goto fail;
	}

	hr = CreateProcessW( NULL,							// No module name - use Command Line
						 (wchar_t*)commandLine.c_str(), // Command Line
						 NULL,							// Process handle not inheritable
						 NULL,							// Thread handle not inheritable
						 FALSE,							// Inherit handles
						 EXTENDED_STARTUPINFO_PRESENT,	// Creation flags
						 NULL,							// Use parent's environment block
						 workingDir.empty() ? NULL
											: workingDir.c_str(), // Use parent's starting directory
						 &startupInfo.StartupInfo,				  // Pointer to STARTUPINFO
						 &piClient )							  // Pointer to PROCESS_INFORMATION
			 ? S_OK
			 : HANDLE_WIN_ERR( GetLastError() );

	if ( hr != S_OK ) {
		Log::error( "CreateProcessW failed\n" );
		goto fail;
	}

	hProcess = AutoHandle( piClient.hProcess );
	hThread = AutoHandle( piClient.hThread );

	pseudoTerminal.mAttached = true;

	return std::unique_ptr<Process>(
		new Process( std::move( hProcess ), startupInfo.lpAttributeList, piClient.dwProcessId ) );
fail:
	return std::unique_ptr<Process>();
}

}} // namespace eterm::System

#endif
