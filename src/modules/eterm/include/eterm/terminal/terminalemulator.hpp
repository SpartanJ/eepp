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
#include <eepp/core/containers.hpp>
#include <eepp/core/small_vector.hpp>
#include <eepp/math/vector2.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/window/keycodes.hpp>
#include <eterm/system/iprocess.hpp>
#include <eterm/terminal/ipseudoterminal.hpp>
#include <eterm/terminal/iterminaldisplay.hpp>
#include <eterm/terminal/kittygraphicsprotocol.hpp>
#include <eterm/terminal/kittykeyboardprotocol.hpp>
#include <eterm/terminal/terminalsearch.hpp>
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

	/** Worker-only lookup. The result is borrowed until the next parser mutation. */
	const TerminalHyperlink* getHyperlink( uint16_t id ) const;

	bool hasHyperlinks() const { return mHyperlinkBytes != 0; }

	/** Worker-only lookup of a grapheme cluster's codepoints (base first); nullptr when @p id is
	 * not live. The result is borrowed until the next parser mutation. */
	const TerminalCluster* getCluster( uint32_t id ) const {
		return id != 0 && id < mClusters.size() && mClusters[id] ? &mClusters[id] : nullptr;
	}

	bool hasClusters() const { return mClusterCount != 0; }

	/** Worker-only: the registry indexed by cluster ID, for TerminalSearch. */
	const std::vector<TerminalCluster>& getClusters() const { return mClusters; }

	/** Calls @p visit with each codepoint a cell shows: its own, or its cluster's, base first. */
	template <typename Visitor>
	void visitGlyphCodepoints( const TerminalGlyph& glyph, Visitor&& visit ) const {
		if ( !isGlyphCluster( glyph ) ) {
			visit( glyph.u );
		} else if ( const auto* cluster = getCluster( glyphClusterId( glyph ) ) ) {
			for ( char32_t codepoint : **cluster )
				visit( static_cast<Rune>( codepoint ) );
		}
	}

	/** The codepoint a cell shows on its own, or its cluster's base. */
	Rune glyphBaseCodepoint( const TerminalGlyph& glyph ) const {
		if ( !isGlyphCluster( glyph ) )
			return glyph.u;
		const auto* cluster = getCluster( glyphClusterId( glyph ) );
		return cluster ? static_cast<Rune>( ( **cluster )[0] ) : ' ';
	}

	bool getOsc8HyperlinksEnabled() const { return mEnableOsc8Hyperlinks; }

	void setOsc8HyperlinksEnabled( bool enabled );

	void setAllowOsc52ClipboardWrite( bool allow ) { mAllowOsc52ClipboardWrite = allow; }

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

	bool hasSelection() const { return mSel.mode != SEL_EMPTY && mSel.ob.x != -1; }

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

	void reportFocus( bool focused );

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

	void setSearchQuery( TerminalSearchQuery query );

	void navigateSearch( int direction );

	void clearSearch();

	const std::vector<TerminalSearchMatch>& getSearchMatches() const;

	Int32 getCurrentSearchMatch() const;

	Uint64 getSearchRequestId() const { return mSearchQuery.requestId; }

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
	bool mProcessingPtyInput{ false };
	int mPtyHistoryLinesPushed{ 0 };
	Clock mPresentationClock;
	Time mPresentationInterval{ Microseconds( 1000000.0 / 60.0 ) };
	Clock mSynchronizedUpdateClock;
	bool mAllowMemoryTrimnming{ false };
	int mExitCode;

	enum { STARTING = 0, RUNNING, TERMINATED } mStatus;

	char mBuf[4 * 8192];
	int mBuflen;

	Term mTerm;
	// Printed attributes include the packed link; saved cursor/SGR attributes never do.
	TerminalGlyph mPrintAttributes;
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
	TerminalCursorMode mDefaultCursorMode{ SteadyUnderline };
	bool mColorSchemeNotifications{ false };
	bool mAllowOsc52ClipboardWrite{ true };
	bool mEnableOsc8Hyperlinks{ true };
	int mColorScheme{ 0 };

	std::string mCurrentWorkingDirectory;
	std::vector<TerminalHyperlink> mHyperlinks;
	std::vector<uint16_t> mFreeHyperlinkIds;
	UnorderedMap<size_t, SmallVector<uint16_t, 2>> mExplicitHyperlinks;
	size_t mHyperlinkBytes{ 0 };
	uint16_t mActiveHyperlinkId{ 0 };
	// Grapheme clusters, indexed by the ID stored in cluster cells (ID 0 unused). Identical
	// clusters share one ID through mClusterIndex (content hash to IDs).
	std::vector<TerminalCluster> mClusters;
	std::vector<uint32_t> mFreeClusterIds;
	UnorderedMap<size_t, SmallVector<uint32_t, 2>> mClusterIndex;
	size_t mClusterCount{ 0 };
	uint32_t mClusterRetryCountdown{ 0 };
	// One mark pass reclaims both hyperlink and cluster IDs (collectGlyphReferences()).
	Clock mGlyphReferenceGcClock;
	Time mGlyphReferenceCollectionCost; // Duration of the last collection; bounds the duty cycle.
	// Set by output/allocation; idle terminals never rescan unchanged buffers.
	bool mGlyphReferenceCollectionPending{ false };
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
	TerminalSearch mSearch;
	TerminalSearchQuery mSearchQuery;
	std::vector<TerminalSearchRowView> mSearchRows;
	Int32 mCurrentSearchMatch{ -1 };
	Clock mSearchRefreshClock;
	bool mSearchDirty{ false };

	void setClipboard( const char* str );

	void loadColors();
	int resetColor( int x, const char* name );

	void onProcessExit( int exitCode );

	void onScrollPositionChange();

	void csidump();
	void csihandle();
	bool handleKittyKeyboardProtocol();
	KittyKeyboardState& activeKeyboardState();
	void ttywriteInternal( const char* s, size_t n, int may_echo, bool scrollToBottom );
	void resetKittyKeyboardProtocol();
	void csiparse();
	void csireset();

	int eschandle( uchar );

	void strdump();
	void strhandle();

	void handleHyperlink( std::string_view sequence );

	void handleWorkingDirectory( std::string_view uri );

	void handleClipboardWrite( std::string_view sequence );

	void collectGlyphReferences();

	/* Runs collectGlyphReferences() when a registry is full, at most ~20% of the time. */
	void collectExhaustedGlyphReferences();

	/* The ID of the cluster with these codepoints, allocating one if needed; 0 when the registry is
	 * full even after a collection. */
	uint32_t internCluster( std::u32string_view codepoints );

	/* tputc() for a codepoint that may extend the previous cell's grapheme cluster: zero-width
	 * codepoints always, and with mode 2027 any codepoint above U+00FF. Returns false when it must
	 * be printed in its own cell instead. */
	bool tattach( Rune u, int width );

	/* The column of the character a codepoint attaches to (see tattach()), or -1. */
	int tattachColumn() const;

	/* tattach() without mode 2027, for a burst of zero-width codepoints. */
	void tattachZeroWidth( const char32_t* codepoints, size_t count );

	/* Copies the codepoints a cell shows (at most the cluster limit) into @p codepoints. */
	size_t tclusterCodepoints( const TerminalGlyph& glyph, char32_t* codepoints ) const;

	/* Makes the cell at x, y show cluster @p id. */
	void tsetcluster( int x, int y, uint32_t id );

	void refreshPrintAttributes() {
		mPrintAttributes = mTerm.c.attr;
		setGlyphLinkId( mPrintAttributes, mActiveHyperlinkId );
	}

	void setActiveHyperlink( uint16_t id ) {
		mActiveHyperlinkId = id;
		refreshPrintAttributes();
	}

	void strparse();
	void strreset();

	/* Appends to the pending string sequence, enforcing its size limit. */
	void strappend( const char* data, size_t len );

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

	/* Whether tputrun() may replace tputc() for printable text in the current state. */
	bool canPutPrintable() const;

	/* tputc() for each character of a run of printable text (codepoint, width and cell mode per
	 * index); requires canPutPrintable(). */
	template <typename Run> void tputrun( const Run& run, int len );

	/* Decodes and writes the printable UTF-8 text at buf[begin, end) through tputrun(); returns
	 * the end of what it wrote (begin when the first character needs tputc()). */
	int tpututf8( const char* buf, int begin, int end );
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
	void tsetchar( Rune, int, int );
	void tsetdirt( int, int );
	void tsetscroll( int, int );
	void tswapscreen();
	void tsetmode( int, int, int*, int );

	/* DECRPM state of a DEC private (@p priv) or ANSI mode: 1 set, 2 reset, 0 not recognized. */
	int modeReport( bool priv, int mode );
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
