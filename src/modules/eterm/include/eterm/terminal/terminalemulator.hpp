#ifndef ETERM_TERMINALEMULATOR_HPP
#define ETERM_TERMINALEMULATOR_HPP
// The MIT License (MIT)

// Copyright (c) 2020 Fredrik A. Kristiansen
// Copyright (c) 2014 - 2020 Hiltjo Posthuma<hiltjo at codemadness dot org>
// Copyright (c) 2018 Devin J.Pohly<djpohly at gmail dot com>
// Copyright (c) 2014 - 2017 Quentin Rameau<quinq at fifth dot space>
// Copyright (c) 2009 - 2012 Aurélien APTEL<aurelien dot aptel at gmail dot com>
// Copyright (c) 2008 - 2017 Anselm R Garbe<garbeam at gmail dot com>
// Copyright (c) 2012 - 2017 Roberto E.Vargas Caballero<k0ga at shike2 dot com>
// Copyright (c) 2012 - 2016 Christoph Lohmann<20h at r - 36 dot net>
// Copyright (c) 2013 Eon S.Jeon<esjeon at hyunmu dot am>
// Copyright (c) 2013 Alexander Sedov<alex0player at gmail dot com>
// Copyright (c) 2013 Mark Edgar<medgar123 at gmail dot com>
// Copyright (c) 2013 - 2014 Eric Pruitt<eric.pruitt at gmail dot com>
// Copyright (c) 2013 Michael Forney<mforney at mforney dot org>
// Copyright (c) 2013 - 2014 Markus Teich<markus dot teich at stusta dot mhn dot de>
// Copyright (c) 2014 - 2015 Laslo Hunhold<dev at frign dot de>

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
#include <eepp/math/vector2.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/window/keycodes.hpp>
#include <eterm/system/iprocess.hpp>
#include <eterm/terminal/ipseudoterminal.hpp>
#include <eterm/terminal/iterminaldisplay.hpp>
#include <eterm/terminal/kittygraphicsprotocol.hpp>
#include <eterm/terminal/kittykeyboardprotocol.hpp>
#include <eterm/terminal/terminaltypes.hpp>
#include <memory>
#include <stdint.h>
#include <sys/types.h>

using namespace EE;
using namespace EE::Math;
using namespace EE::Window;
using namespace EE::System;
using namespace eterm::System;

namespace eterm { namespace Terminal {

constexpr int ESC_BUF_SIZ = 512;
constexpr int ESC_ARG_SIZ = 32;
constexpr int STR_BUF_SIZ = ESC_BUF_SIZ;
constexpr int STR_ARG_SIZ = ESC_ARG_SIZ;
// Kitty recommends small chunks, but unchunked direct transmissions are valid and common in
// simple clients. Keep this independently bounded while allowing useful image-sized APCs.
constexpr size_t MAX_KITTY_GRAPHICS_APC_SIZE = 16 * 1024 * 1024;
constexpr size_t MAX_GENERIC_STRING_SEQUENCE_SIZE = 1024 * 1024;

/* Internal representation of the screen */
struct Term {
	int row{ 0 };		   /* nb row */
	int col{ 0 };		   /* nb col */
	Line* line{ nullptr }; /* screen */
	Line* alt{ nullptr };  /* alternate screen */
	Line* hist{ nullptr }; /* history buffer */
	int histcursize{ 0 };  /* history current size */
	int histsize{ 0 };	   /* history max size */
	int histi{ 0 };		   /* history index */
	int histlen{ 0 };	   /* history valid length */
	/* Allocation width of each history line. All slots are (re)allocated together
	 * on every resize/reflow, so a single shared capacity is enough to know when a
	 * recycled ring-buffer slot can be overwritten in place instead of being freed
	 * and reallocated for every pushed line (hot path on fast-scrolling output). */
	int histcapacity{ 0 };
	int max_width{ 0 };			   /* max width of lines in history */
	int scr{ 0 };				   /* scroll back */
	int* dirty{ nullptr };		   /* dirtyness of lines */
	TerminalCursor c{};			   /* cursor */
	int ocx{ 0 };				   /* old cursor col */
	int ocy{ 0 };				   /* old cursor row */
	int top{ 0 };				   /* top    scroll limit */
	int bot{ 0 };				   /* bottom scroll limit */
	int mode{ 0 };				   /* terminal mode flags */
	int esc{ 0 };				   /* escape state flags */
	char trantbl[4]{ 0, 0, 0, 0 }; /* charset table translation */
	int charset{ 0 };			   /* current charset */
	int icharset{ 0 };			   /* selected charset for sequence */
	int* tabs{ nullptr };
	Rune lastc{ 0 }; /* last printed char outside of sequence, 0 if control */
	std::string title;
	std::vector<std::string> title_stack;
	bool is_syncing{ false }; // Track DEC mode 2026

	~Term();
};

/* CSI Escape sequence structs */
/* ESC '[' [[ [<priv>] <arg> [;]] <mode> [<mode>]] */
struct CSIEscape {
	char buf[ESC_BUF_SIZ]; /* raw string */
	size_t len;			   /* raw string length */
	char priv;
	int arg[ESC_ARG_SIZ];
	/* Separator following each argument. ECMA-48 uses ';' between parameters and ':'
	 * between subparameters, so preserving it is required for modern SGR colors. */
	char sep[ESC_ARG_SIZ];
	int narg; /* nb of args */
	char mode[2];
};

/* STR Escape sequence structs */
/* ESC type [[ [<priv>] <arg> [;]] <mode>] ESC '\' */
struct STREscape {
	char type;	/* ESC type ... */
	char* buf;	/* allocated raw string */
	size_t siz; /* allocation size */
	size_t len; /* raw string length */
	char* args[STR_ARG_SIZ];
	int narg;		/* nb of args */
	bool discarded; /* oversized sequence: consume input through its terminator without storing */
};

enum class PromptState {
	WaitingPrompt,
	CommandExecuted,
	CommandExecuting,
	PromptEnded,
	Unknown,
};

enum class TerminalMouseEventType { MouseMotion, MouseButtonDown, MouseButtonRelease };

class TerminalEmulator final {
  public:
	using DpyPtr = std::weak_ptr<ITerminalDisplay>;
	using PtyPtr = std::unique_ptr<IPseudoTerminal>;
	using ProcPtr = std::unique_ptr<System::IProcess>;

	static ushort boxdrawindex( const TerminalGlyph* g );

	~TerminalEmulator();

	TerminalEmulator( const TerminalEmulator& ) = delete;

	TerminalEmulator( TerminalEmulator&& ) = delete;

	TerminalEmulator& operator=( const TerminalEmulator& ) = delete;

	TerminalEmulator& operator=( TerminalEmulator&& ) = delete;

	static std::unique_ptr<TerminalEmulator>
	create( PtyPtr&& pty, ProcPtr&& process, const std::shared_ptr<ITerminalDisplay>& display,
			const size_t& historySize = 1000 );

	void resize( int columns, int rows );

	void resize( int columns, int rows, int pixelWidth, int pixelHeight );

	void redraw();

	/** Worker-owned terminal state reset (RIS semantics without replacing the PTY/process). */
	void reset();

	void logError( const char* err );

	/** @return If the tty read was completed or there's still buffer to read (true completed) */
	bool update();

	void terminate();

	bool isStarting() const;

	bool isRunning() const;

	bool hasExited() const;

	int getExitCode() const;

	inline bool isSelected( int column, int row ) { return selected( column, row ); }

	inline uint32_t getDefaultForeground() const { return mDefaultFg; }

	inline uint32_t getDefaultBackground() const { return mDefaultBg; }

	inline uint32_t getDefaultCursorColor() const { return mDefaultCs; }

	inline uint32_t getDefaultReverseCursorColor() const { return mDefaultRCs; }

	inline int getNumColumns() const { return mTerm.col; }

	inline int getNumRows() const { return mTerm.row; }

	int getHistorySize() const;

	int write( const char* buf, size_t buflen );

	void printscreen( const TerminalArg* );

	void printsel( const TerminalArg* );

	void sendbreak( const TerminalArg* );

	void toggleprinter( const TerminalArg* );

	TerminalSelectionMode getSelectionMode() const;

	void selclear();

	void selinit();

	void selstart( int, int, int );

	void selextend( int, int, int, int );

	int selected( int, int );

	char* getsel() const;

	bool hasSelection() const;

	std::string getSelection() const;

	void mousereport( const TerminalMouseEventType& type, const Vector2i& cellPosition,
					  const Vector2i& pixelPosition, const Uint32& flags, const Uint32& mod );

	const bool& isDirty() const { return mDirty; }

	void setPtyAndProcess( PtyPtr&& pty, ProcPtr&& process );

	void kscrolldown( const TerminalArg* a );

	void kscrollup( const TerminalArg* a );

	void kscrollto( const TerminalArg* a );

	bool isScrolling() const;

	void ttywrite( const char* s, size_t n, int may_echo );

	void keyEvent( const KittyKeyEvent& event );

	void textInput( Uint32 codepoint );

	void clearPendingKeyboardInput();

	int tisaltscr();

	int scrollSize() const;

	int rowCount() const;

	void clearHistory();

	int scrollPos();

	bool getAllowMemoryTrimnming() const;

	void setAllowMemoryTrimnming( bool allowMemoryTrimnming );

	/** Worker-only maximum interval between snapshots while PTY reads remain saturated. */
	void setPresentationInterval( Time interval );

	/** Set the user-configured cursor style restored by DECSCUSR parameter 7. */
	void setDefaultCursorMode( TerminalCursorMode mode );

	/** Worker-only notification that the display palette changed. */
	void notifyColorSchemeChanged();

	void requestGraphicsResync();

	Vector2i getSize() const;

	System::IProcess* getProcess() const;

	const std::string getCurrentWorkingDirectory() const { return mCurrentWorkingDirectory; }

	PromptState getPromptState() const { return mPromptState; }

	using PromptStateChangedCb = std::function<void( PromptState, std::string_view )>;

	void setPromptStateChangedCb( PromptStateChangedCb promptStateChangedCb ) {
		mPromptStateChangedCb = promptStateChangedCb;
	}

	using DataCb = std::function<void( const char*, size_t )>;

	void setDataCb( DataCb cb ) { mDataCb = cb; }

	int getTerminalMode() const { return mTerm.mode; }

  private:
	DpyPtr mDpy;
	PtyPtr mPty;
	ProcPtr mProcess;

	bool mPendingPtyResize{ false };
	int mPendingPtyColumns{ 0 };
	int mPendingPtyRows{ 0 };
	int mPendingPtyPixelWidth{ 0 };
	int mPendingPtyPixelHeight{ 0 };
	int mPixelWidth{ 0 };
	int mPixelHeight{ 0 };
	Clock mPendingPtyResizeClock;

	bool mDirty{ true };
	bool mAllDirty{ true };
	Clock mPresentationClock;
	Time mPresentationInterval{ Microseconds( 1000000.0 / 60.0 ) };
	Clock mSynchronizedUpdateClock;
	bool mAllowMemoryTrimnming{ false };
	int mExitCode;

	enum { STARTING = 0, RUNNING, TERMINATED } mStatus;

	char mBuf[4 * 8192];
	int mBuflen;

	Term mTerm;
	TerminalSelection mSel;
	CSIEscape mCsiescseq;
	STREscape mStrescseq;
	KittyGraphicsProtocol mKittyGraphics;
	KittyKeyboardState mPrimaryKeyboardState;
	KittyKeyboardState mAlternateKeyboardState;
	KittyKeyEvent mPendingTextKey;
	Uint32 mExpectedTextInput{ 0 };
	bool mHasPendingTextKey{ false };

	uint32_t mDefaultFg;
	uint32_t mDefaultBg;
	uint32_t mDefaultCs;
	uint32_t mDefaultRCs;

	int mAllowAltScreen;
	int mAllowWindowOps;
	TerminalCursorMode mDefaultCursorMode{ SteadyUnderline };
	bool mColorSchemeNotifications{ false };
	int mColorScheme{ 0 };

	std::string mCurrentWorkingDirectory;
	Vector2i mLastMousePosition{ -1, -1 };
	PromptState mPromptState{ PromptState::Unknown };
	PromptStateChangedCb mPromptStateChangedCb;
	DataCb mDataCb;
	Vector2i mKittyPlaceholderCell{ -1, -1 };
	struct KittyPlaceholderMetadata {
		Uint32 placementId{ 0 };
		Uint16 row{ UINT16_MAX };
		Uint16 column{ UINT16_MAX };
		Uint8 imageIdMsb{ 0 };
		Uint8 diacriticCount{ 0 };
	};
	std::unordered_map<const TerminalGlyph*, KittyPlaceholderMetadata> mKittyPlaceholderMetadata;
	Uint32 mKittyUnderlineColor{ 0 };

	void setClipboard( const char* str );

	void loadColors();
	int resetColor( int x, const char* name );

	void onProcessExit( int exitCode );

	void onScrollPositionChange();

	void csidump();
	void csihandle();
	bool handleKittyKeyboardProtocol();
	KittyKeyboardState& activeKeyboardState();
	void resetKittyKeyboardProtocol();
	void csiparse();
	void csireset();

	int eschandle( uchar );

	void strdump();
	void strhandle();
	void strparse();
	void strreset();

	void tprinter( const char*, size_t );
	void tdumpsel();
	void tdumpline( int );
	void tdump();
	void tclearregion( int, int, int, int, bool skip_clear = false );
	void tcursor( int );
	void tdeletechar( int );
	void tdeleteline( int );
	void tinsertblank( int n );
	void tinsertblankline( int n );
	int tlinelen( int y ) const;
	int tlinelen( Line line, int col ) const;
	int tiswrapped( int y );
	void tmoveto( int, int );
	void tmoveato( int, int );
	void tnewline( int );
	void tputtab( int );
	void tputc( Rune );
	void treset();
	void tscrollup( int, int, int );
	void tscrolldown( int, int );
	void historyUpdateMaxWidth( Line line, int col );
	void historyPush( Line line, int col );
	/* Zero-copy variant: trades buffer ownership between the screen row and the
	 * recycled history slot. The caller must blank the returned screen row
	 * afterwards (tscrollup does this via tclearregion). */
	void historyStealPush( Line* lineSlot, int col );
	void historyReflow( int old_col, int new_col );
	void historyPopToScreen( int loaded, int col );
	void tsetattr( int*, int, const char* );
	void tsetchar( Rune, TerminalGlyph*, int, int );
	void tsetdirt( int, int );
	void tsetscroll( int, int );
	void tswapscreen();
	void tsetmode( int, int, int*, int );
	int twrite( const char*, int, int );
	void tfulldirt();
	void tcontrolcode( uchar );
	void tdectest( char );
	void tdefutf8( char );
	int32_t tdefcolor( int*, const char*, int*, int );
	void tdeftran( char );
	void tstrsequence( uchar );

	void selnormalize();
	void selmove( int );
	void selscroll( int, int );
	void selsnap( int*, int*, int );

	void _die( const char*, ... );
	void drawregion( ITerminalDisplay& dpy, int, int, int, int );
	void draw();

	int tattrset( int );
	void tnew( int, int, size_t );
	void tresize( int, int );
	void tsetdirtattr( int );

	void ttyhangup();
	size_t ttyread();
	void ttywriteraw( const char*, size_t );

	void resettitle();

	void xbell();
	void xclipcopy();

	int xsetcolorname( int, const char* );
	void xseticontitle( char* );
	void xsettitle( char* );
	int xsetcursor( int );
	void xsetmode( int, unsigned int );
	bool xgetmode( const TerminalWinMode& );

	void xsetpointermotion( int );
	void xsetsel( char* );
	void xximspot( int, int );
	int xgetcolor( int x, unsigned char* r, unsigned char* g, unsigned char* b );
	void osc_color_response( int num, int index, int is_osc4 );
	void handleDeviceAttributes();
	int colorScheme();
	void reportColorScheme();

	void trimMemory();

	TerminalEmulator( PtyPtr&& pty, ProcPtr&& process,
					  const std::shared_ptr<ITerminalDisplay>& display,
					  const size_t& historySize = 1000 );
};

}} // namespace eterm::Terminal

#endif
