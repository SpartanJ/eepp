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

#ifndef WIN32
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

#endif
