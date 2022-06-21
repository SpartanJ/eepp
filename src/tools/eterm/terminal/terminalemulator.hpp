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
#pragma once

#include "../system/iprocess.hpp"
#include "ipseudoterminal.hpp"
#include "terminaldisplay.hpp"
#include "types.hpp"
#include <eepp/math/vector2.hpp>
#include <eepp/window/keycodes.hpp>
#include <memory>
#include <stdint.h>
#include <sys/types.h>

using namespace EE;
using namespace EE::Math;
using namespace EE::Window;

namespace EE { namespace Terminal {

constexpr int ESC_BUF_SIZ = 512;
constexpr int ESC_ARG_SIZ = 16;
constexpr int STR_BUF_SIZ = ESC_BUF_SIZ;
constexpr int STR_ARG_SIZ = ESC_ARG_SIZ;

/* Internal representation of the screen */
struct Term {
	int row;		  /* nb row */
	int col;		  /* nb col */
	Line* line;		  /* screen */
	Line* alt;		  /* alternate screen */
	int* dirty;		  /* dirtyness of lines */
	TerminalCursor c; /* cursor */
	int ocx;		  /* old cursor col */
	int ocy;		  /* old cursor row */
	int top;		  /* top    scroll limit */
	int bot;		  /* bottom scroll limit */
	int mode;		  /* terminal mode flags */
	int esc;		  /* escape state flags */
	char trantbl[4];  /* charset table translation */
	int charset;	  /* current charset */
	int icharset;	  /* selected charset for sequence */
	int* tabs;
	Rune lastc; /* last printed char outside of sequence, 0 if control */
};

/* CSI Escape sequence structs */
/* ESC '[' [[ [<priv>] <arg> [;]] <mode> [<mode>]] */
struct CSIEscape {
	char buf[ESC_BUF_SIZ]; /* raw string */
	size_t len;			   /* raw string length */
	char priv;
	int arg[ESC_ARG_SIZ];
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
	int narg; /* nb of args */
};

enum class TerminalMouseEventType { MouseMotion, MouseButtonDown, MouseButtonRelease };

class TerminalEmulator final {
  public:
	using DpyPtr = std::weak_ptr<TerminalDisplay>;
	using PtyPtr = std::unique_ptr<IPseudoTerminal>;
	using ProcPtr = std::unique_ptr<System::IProcess>;

	static ushort boxdrawindex( const TerminalGlyph* g );

	~TerminalEmulator();

	TerminalEmulator( const TerminalEmulator& ) = delete;

	TerminalEmulator( TerminalEmulator&& ) = delete;

	TerminalEmulator& operator=( const TerminalEmulator& ) = delete;

	TerminalEmulator& operator=( TerminalEmulator&& ) = delete;

	static std::unique_ptr<TerminalEmulator>
	create( PtyPtr&& pty, ProcPtr&& process, const std::shared_ptr<TerminalDisplay>& display );

	void resize( int columns, int rows );

	void redraw();

	void logError( const char* err );

	void update();

	void terminate();

	bool hasExited() const;

	int getExitCode() const;

	inline bool isSelected( int column, int row ) { return selected( column, row ); }

	inline uint32_t getDefaultForeground() const { return mDefaultFg; }

	inline uint32_t getDefaultBackground() const { return mDefaultBg; }

	inline uint32_t getDefaultCursorColor() const { return mDefaultCs; }

	inline uint32_t getDefaultReverseCursorColor() const { return mDefaultrCs; }

	inline int getNumColumns() const { return mTerm.col; }

	inline int getNumRows() const { return mTerm.row; }

	inline int write( const char* buf, size_t buflen ) { return mPty->write( buf, (int)buflen ); }

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

	char* getsel();

	void mousereport( const TerminalMouseEventType& type, const Vector2i& pos, const Uint32& flags,
					  const Uint32& mod );

  private:
	DpyPtr mDpy;
	PtyPtr mPty;
	ProcPtr mProcess;

	bool mColorsLoaded;
	int mExitCode;

	enum { STARTING = 0, RUNNING, TERMINATED } mStatus;

	char mBuf[8192];
	int mBuflen;

	Term mTerm;
	TerminalSelection mSel;
	CSIEscape mCsiescseq;
	STREscape mStrescseq;

	uint32_t mDefaultFg;
	uint32_t mDefaultBg;
	uint32_t mDefaultCs;
	uint32_t mDefaultrCs;

	int mAllowAltScreen;
	int mAllowWindowOps;

	void setClipboard( const char* str );

	void loadColors();
	int resetColor( int x, const char* name );

	void onProcessExit( int exitCode );

	void csidump();
	void csihandle();
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
	void tclearregion( int, int, int, int );
	void tcursor( int );
	void tdeletechar( int );
	void tdeleteline( int );
	void tinsertblank( int );
	void tinsertblankline( int );
	int tlinelen( int );
	void tmoveto( int, int );
	void tmoveato( int, int );
	void tnewline( int );
	void tputtab( int );
	void tputc( Rune );
	void treset();
	void tscrollup( int, int );
	void tscrolldown( int, int );
	void tsetattr( int*, int );
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
	int32_t tdefcolor( int*, int*, int );
	void tdeftran( char );
	void tstrsequence( uchar );

	void selnormalize();
	void selscroll( int, int );
	void selsnap( int*, int*, int );

	void _die( const char*, ... );
	void drawregion( TerminalDisplay& dpy, int, int, int, int );
	void draw();

	int tattrset( int );
	void tnew( int, int );
	void tresize( int, int );
	void tsetdirtattr( int );

	void ttyhangup();
	size_t ttyread();
	void ttywrite( const char*, size_t, int );
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

	TerminalEmulator( PtyPtr&& pty, ProcPtr&& process,
					  const std::shared_ptr<TerminalDisplay>& display );
};

}} // namespace EE::Terminal
