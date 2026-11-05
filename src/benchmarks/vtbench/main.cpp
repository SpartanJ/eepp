#include <eterm/terminal/terminalemulator.hpp>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

using namespace eterm::Terminal;
using namespace eterm::System;

namespace {

constexpr int Columns = 120;
constexpr int Rows = 80;
constexpr size_t HistoryRows = 10000;

class StreamPty : public IPseudoTerminal {
  public:
	size_t bytes{ 0 };
	size_t acknowledgements{ 0 };
	size_t chunk{ 64 * 1024 };
	size_t replyPrefix{ 0 };
	int fd;
	bool readError{ false };

	explicit StreamPty( int descriptor ) : fd( descriptor ) {}

	~StreamPty() { ::close( fd ); }

	int getNumColumns() const override { return Columns; }

	int getNumRows() const override { return Rows; }

	bool resize( int, int, int, int ) override { return true; }

	bool isTTY() const override { return true; }

	int write( const char* data, size_t size ) override {
		// Count status responses across write boundaries without retaining an unbounded reply log.
		constexpr char Reply[] = "\x1b[0n";
		for ( size_t i = 0; i < size; ++i ) {
			if ( data[i] == Reply[replyPrefix] ) {
				if ( ++replyPrefix == sizeof( Reply ) - 1 ) {
					++acknowledgements;
					replyPrefix = 0;
				}
			} else {
				replyPrefix = data[i] == Reply[0] ? 1 : 0;
			}
		}
		return static_cast<int>( size );
	}

	int read( char* out, size_t size, bool ) override {
		ssize_t count;
		do {
			count = ::read( fd, out, std::min( size, chunk ) );
		} while ( count < 0 && errno == EINTR );
		if ( count < 0 ) {
			readError = true;
			return 0;
		}
		bytes += static_cast<size_t>( count );
		return static_cast<int>( count );
	}
};

class StreamProcess : public IProcess {
  public:
	void checkExitStatus() override {}

	bool hasExited() const override { return false; }

	int getExitCode() const override { return 0; }

	void terminate() override {}

	void waitForExit() override {}

	int pid() override { return 1; }
};

class StreamDisplay : public ITerminalDisplay {
  public:
	// Diagnostic dumps alone retain a screen (150 KiB); timed runs allocate no display cells.
	std::unique_ptr<TerminalGlyph[]> cells;
	int cursorX{ 0 };
	int cursorY{ 0 };

	explicit StreamDisplay( bool dump ) :
		cells( dump ? std::make_unique<TerminalGlyph[]>( Columns * Rows ) : nullptr ) {}

	bool drawBegin( Uint32, Uint32 ) override { return true; }

	void drawLine( Line line, int first, int row, int end ) override {
		if ( cells )
			std::copy( line + first, line + end, cells.get() + row * Columns + first );
	}

	void drawCursor( int x, int y, TerminalGlyph, int, int, TerminalGlyph ) override {
		cursorX = x;
		cursorY = y;
	}

	void drawEnd() override {}

	bool dump( const char* name, const TerminalEmulator& terminal ) const {
		FILE* out = std::fopen( name, "wb" );
		if ( !out )
			return false;
		std::fprintf( out, "cursor %d %d\n", cursorX, cursorY );
		for ( int i = 0; i < Columns * Rows; ++i ) {
			const auto& glyph = cells[i];
			terminal.visitGlyphCodepoints(
				glyph, [out]( Rune codepoint ) { std::fprintf( out, "%x,", codepoint ); } );
			std::fprintf( out, " %x %x %x", glyph.mode & GLYPH_ATTRIBUTE_MASK, glyph.fg, glyph.bg );
			// Registry IDs vary with collection timing; compare the actual URI and external ID.
			if ( const auto* link = terminal.getHyperlink( glyphLinkId( glyph ) ) ) {
				for ( std::string_view value :
					  { std::string_view( *link->uri ), std::string_view( link->externalId ) } ) {
					std::fputc( ' ', out );
					for ( unsigned char byte : value )
						std::fprintf( out, "%02x", byte );
				}
			}
			std::fputc( '\n', out );
		}
		const bool ok = std::ferror( out ) == 0;
		return std::fclose( out ) == 0 && ok;
	}
};

} // namespace

int main( int argc, char** argv ) {
	if ( argc < 2 || argc > 3 ) {
		std::fprintf( stderr, "Usage: eterm-vtbench CAPTURE [SCREEN-DUMP]\n" );
		return 2;
	}
	const int fd = ::open( argv[1], O_RDONLY );
	struct stat info{};
	if ( fd < 0 || ::fstat( fd, &info ) != 0 || info.st_size <= 0 ) {
		if ( fd >= 0 )
			::close( fd );
		std::fprintf( stderr, "Cannot read a nonempty benchmark capture\n" );
		return 2;
	}
	auto pty = std::make_unique<StreamPty>( fd );
	auto* input = pty.get();
	auto display = std::make_shared<StreamDisplay>( argc == 3 );
	auto terminal = TerminalEmulator::create( std::move( pty ), std::make_unique<StreamProcess>(),
											  display, HistoryRows );
	// This default survives RIS, matching Ghostty's --grapheme-width=unicode.
	terminal->setGraphemeClusteringEnabled( true );
	while ( input->bytes < static_cast<size_t>( info.st_size ) ) {
		const auto before = input->bytes;
		terminal->update();
		if ( input->readError || input->bytes == before ) {
			std::fprintf( stderr, "Capture ended before all bytes were consumed\n" );
			return 1;
		}
	}
	terminal->update();
	if ( input->acknowledgements < 6 ) {
		std::fprintf( stderr, "Incomplete VT processing: missing trailing DSR responses\n" );
		return 1;
	}
	if ( argc == 3 ) {
		terminal->redraw();
		terminal->update();
		if ( !display->dump( argv[2], *terminal ) ) {
			std::fprintf( stderr, "Cannot write screen dump\n" );
			return 2;
		}
	}
	std::printf( "complete bytes=%zu dsr=%zu grapheme=%d\n", input->bytes, input->acknowledgements,
				 ( terminal->getTerminalMode() & MODE_GRAPHEME ) != 0 );
	return 0;
}
