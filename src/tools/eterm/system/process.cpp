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
#include "process.hpp"
#include <poll.h>
#include <pwd.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define DEFAULT( a, b ) ( a ) = ( a ) ? ( a ) : ( b )

#if defined( __linux )
#include <pty.h>
#elif defined( __OpenBSD__ ) || defined( __NetBSD__ ) || defined( __APPLE__ )
#include <util.h>
#elif defined( __FreeBSD__ ) || defined( __DragonFly__ )
#include <libutil.h>
#endif

using namespace Hexe::System;

Process::~Process() {
	if ( m_pid != -1 ) {
		kill( m_pid, SIGHUP );
	}
}

void Process::CheckExitStatus() {
	if ( m_status == ProcessStatus::EXITED ) {
		return;
	}
	int status = 0;
	int p = waitpid( m_pid, &status, WNOHANG );
	if ( p < 0 ) {
		perror( "Process::CheckExitStatus(waitpid)" );
	}
	if ( p == 0 ) {
		return;
	}
	m_status = ProcessStatus::EXITED;
	m_exitCode = WEXITSTATUS( status );
}

void Process::Terminate() {
	if ( m_status == ProcessStatus::EXITED ) {
		return;
	}
	if ( m_pid != -1 ) {
		kill( m_pid, SIGHUP );
		m_exitCode = 1;
		m_status = ProcessStatus::EXITED;
	}
}

void Process::WaitForExit() {
	if ( m_status == ProcessStatus::EXITED ) {
		return;
	}
	int status = 0;
	int p = waitpid( m_pid, &status, 0 );
	if ( p < 0 ) {
		perror( "Process::WaitForExit(waitpid)" );
		return;
	}
	m_status = ProcessStatus::EXITED;
	m_exitCode = WEXITSTATUS( status );
}

bool Process::HasExited() const {
	return m_status == Hexe::System::ProcessStatus::EXITED;
}

int Process::GetExitCode() const {
	return m_status == ProcessStatus::EXITED ? m_exitCode : 255;
}

Process::Process( int pid ) : m_pid( pid ) {}

static void execshell( const char* cmd, const char* const* args ) {
	const struct passwd* pw;

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

	unsetenv( "COLUMNS" );
	unsetenv( "LINES" );
	unsetenv( "TERMCAP" );
	setenv( "LOGNAME", pw->pw_name, 1 );
	setenv( "USER", pw->pw_name, 1 );
	// setenv( "SHELL", sh, 1 );
	setenv( "HOME", pw->pw_dir, 1 );
	setenv( "TERM", "st-256color", 1 );

	signal( SIGCHLD, SIG_DFL );
	signal( SIGHUP, SIG_DFL );
	signal( SIGINT, SIG_DFL );
	signal( SIGQUIT, SIG_DFL );
	signal( SIGTERM, SIG_DFL );
	signal( SIGALRM, SIG_DFL );

	execvp( cmd, (char* const*)args );
	_exit( 1 );
}

std::unique_ptr<Process> Process::CreateWithPipe( const std::string& /*program*/,
												  const std::vector<std::string>& /*args*/,
												  const std::string& /*workingDirectory*/,
												  std::unique_ptr<IPipe>& outPipe,
												  bool /*withStderr*/ ) {
	outPipe = nullptr;
	return nullptr;
}

std::unique_ptr<Process>
Process::CreateWithPseudoTerminal( const std::string& program, const std::vector<std::string>& args,
								   const std::string& /*workingDirectory*/,
								   Terminal::PseudoTerminal& pseudoTerminal ) {
	int pid = fork();
	if ( pid == -1 ) {
		fprintf( stderr, "Failed to fork process\n" );
		return nullptr;
	} else if ( pid == 0 ) {
		setsid();
		dup2( (int)pseudoTerminal.m_slave, 0 );
		dup2( (int)pseudoTerminal.m_slave, 1 );
		dup2( (int)pseudoTerminal.m_slave, 2 );

		if ( ioctl( (int)pseudoTerminal.m_slave, TIOCSCTTY, NULL ) < 0 ) {
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
		execshell( program.c_str(), argsV.data() );
	} else {
		pseudoTerminal.m_slave.Release();
		return std::unique_ptr<Process>( new Process( pid ) );
	}
	return nullptr;
}

#else

#include "../terminal/windowserrors.hpp"
#include "pipe.hpp"
#include "process.hpp"
#include <assert.h>
#include <iomanip>
#include <sstream>

using namespace Hexe::System;
using namespace Hexe;

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
															 void* hPC ) {
	HRESULT hr{ E_UNEXPECTED };

	if ( pStartupInfo ) {
		PSIZE_T attrListSize{};

		pStartupInfo->StartupInfo.cb = sizeof( STARTUPINFOEXW );
		pStartupInfo->StartupInfo.dwFlags = STARTF_USESTDHANDLES;

		// Get the size of the thread attribute list.
		InitializeProcThreadAttributeList( NULL, 1, 0, attrListSize );

		// Allocate a thread attribute list of the correct size
		pStartupInfo->lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
			HeapAlloc( GetProcessHeap(), 0, *attrListSize ) );

		// Initialize thread attribute list
		if ( pStartupInfo->lpAttributeList &&
			 InitializeProcThreadAttributeList( pStartupInfo->lpAttributeList, 1, 0,
												attrListSize ) ) {
			// Set Pseudo Console attribute
			hr = UpdateProcThreadAttribute( pStartupInfo->lpAttributeList, 0,
											PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, hPC, sizeof( hPC ),
											NULL, NULL )
					 ? S_OK
					 : HANDLE_WIN_ERR( GetLastError() );
		} else {
			hr = HANDLE_WIN_ERR( GetLastError() );
		}
	}
	return hr;
}

Process::Process( AutoHandle&& hProcess, LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList ) :
	m_status( ProcessStatus::RUNNING ),
	m_leaveRunning( false ),
	m_exitCode( 1 ),
	m_hProcess( std::move( hProcess ) ),
	m_lpAttributeList( lpAttributeList ) {}

Process::~Process() {
	if ( m_lpAttributeList ) {
		DeleteProcThreadAttributeList( m_lpAttributeList );
		HeapFree( GetProcessHeap(), 0, m_lpAttributeList );
	}
	if ( !m_leaveRunning ) {
		Terminate();
	}
}

void Process::CheckExitStatus() {
	if ( m_status == ProcessStatus::RUNNING ) {
		DWORD exitCode;
		if ( GetExitCodeProcess( (HANDLE)m_hProcess, &exitCode ) && exitCode != STILL_ACTIVE ) {
			m_status = ProcessStatus::EXITED;
			m_exitCode = (int)exitCode;
		}
	}
}

bool Process::HasExited() const {
	return m_status == ProcessStatus::EXITED;
}

int Process::GetExitCode() const {
	return m_status == ProcessStatus::EXITED ? m_exitCode : STILL_ACTIVE;
}

void Process::Terminate() {
	if ( m_status == ProcessStatus::RUNNING ) {
		TerminateProcess( (HANDLE)m_hProcess, EXIT_FAILURE );
		m_exitCode = EXIT_FAILURE;
		m_status = ProcessStatus::EXITED;
		m_hProcess.Release();
	}
}

void Process::WaitForExit() {
	CheckExitStatus();

	if ( m_status == ProcessStatus::RUNNING ) {
		WaitForSingleObject( (HANDLE)m_hProcess, INFINITE );
	}
}

std::unique_ptr<Process> Process::CreateWithPipe( const std::string& program,
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

	if ( !CreatePipe( g_hChildStd_OUT_Rd.Get(), g_hChildStd_OUT_Wr.Get(), &saAttr, 0 ) ) {
		fprintf( stderr, "Failed to create pipe (Stdout)\n" );
		return nullptr;
	}

	if ( !SetHandleInformation( (HANDLE)g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0 ) ) {
		fprintf( stderr, "Failed to set handle information (Stdout)\n" );
		return nullptr;
	}

	if ( !CreatePipe( g_hChildStd_IN_Rd.Get(), g_hChildStd_IN_Wr.Get(), &saAttr, 0 ) ) {
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

		return std::unique_ptr<Process>( new Process( std::move( hProcess ), NULL ) );
	}

	PrintWinApiError( GetLastError() );

	return nullptr;
}

std::unique_ptr<Process>
Process::CreateWithPseudoTerminal( const std::string& program, const std::vector<std::string>& args,
								   const std::string& workingDirectory,
								   Terminal::PseudoTerminal& pseudoTerminal ) {
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
															  pseudoTerminal.m_phPC ) ) != S_OK ) {
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
		goto fail;
	}

	hProcess = AutoHandle( piClient.hProcess );
	hThread = AutoHandle( piClient.hThread );

	pseudoTerminal.m_attached = true;

	return std::unique_ptr<Process>(
		new Process( std::move( hProcess ), startupInfo.lpAttributeList ) );
fail:
	return std::unique_ptr<Process>();
}

#endif
