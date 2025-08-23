#include <eepp/ui/doc/languages/gnuplot.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>

namespace EE { namespace UI { namespace Doc { namespace Language {

SyntaxDefinition& addGnuplot() {

	// Direct conversion from https://github.com/mammothb/vscode-gnuplot (MIT License)

	return SyntaxDefinitionManager::instance()
		->add(

			{ "gnuplot",
			  { "%.gp$", "%.gnuplot$", "%.gnu$", "%.plot$", "%.plt$" },
			  {
				  { { "include", "#number" }, "normal", "", SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#string_single" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "include", "#string_double" },
					"normal",
					"",
					SyntaxPatternMatchType::LuaPattern },
				  { { "\\b(for)\\b\\s*(\\[)", "\\]" },
					{ "normal", "keyword", "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#number" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#operator" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#string_double" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#string_single" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { ":" }, "operator", "", SyntaxPatternMatchType::RegEx },
						{ { "\\b([a-zA-Z]\\w*)\\b\\s*(=|in)" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::RegEx },

					} },
				  { { "\\[", "\\]" },
					{ "operator" },
					{ "operator" },
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "#number" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { "include", "#operator" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },
						{ { ":" }, "operator", "", SyntaxPatternMatchType::RegEx },

					} },
				  { { "\\\\." }, "string", "", SyntaxPatternMatchType::RegEx },
				  { { "(?<!\\$)(#)(?!\\{).*$\\n?" }, "comment", "", SyntaxPatternMatchType::RegEx },
				  { { "for" }, "keyword", "", SyntaxPatternMatchType::RegEx },
				  { { "\\b(angles|arrow|autoscale|bind|bmargin|border|boxwidth|color|colorsequence|"
					  "clabel|clip|cntrlabel|cntrparam|colorbox|colornames|contour|dashtype|"
					  "datastyle|datafile|decimalsign|dgrid3d|dummy|encoding|errorbars|fit|"
					  "fontpath|format|functionstyle|functions|grid|hidden3d|historysize|history|"
					  "isosamples|jitter|key|label|linetype|link|lmargin|loadpath|locale|logscale|"
					  "macros|mapping|margin|micro|minussign|monochrome|mouse|mttics|multiplot|"
					  "mx2tics|mxtics|my2tics|mytics|mztics|nonlinear|object|offsets|origin|output|"
					  "parametric|paxis|plot|pm3d|palette|pointintervalbox|pointsize|polar|print|"
					  "psdir|raxis|rgbmax|rlabel|rmargin|rrange|rtics|samples|size|style|surface|"
					  "table|terminal|termoption|theta|tics|ticslevel|ticscale|timestamp|timefmt|"
					  "title|tmargin|trange|ttics|urange|variables|version|view|vrange|x2data|"
					  "x2dtics|x2label|x2mtics|x2range|x2tics|x2zeroaxis|xdata|xdtics|xlabel|xl|"
					  "xmtics|xrange|xr|xtics|xyplane|xzeroaxis|y2data|y2dtics|y2label|y2mtics|"
					  "y2range|y2tics|y2zeroaxis|ydata|ydtics|ylabel|yl|ymtics|yrange|yr|ytics|"
					  "yzeroaxis|zdata|zdtics|zzeroaxis|cbdata|cbdtics|zero|zeroaxis|zlabel|zl|"
					  "zmtics|zrange|zr|ztics|cblabel|cbl|cbmtics|cbrange|cbr|cbtics)\\b" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "\\b(break|cd|call|clear|continue|do|evaluate|exit|fit|help|history|if|for|"
					  "import|load|lower|pause|plot|print|printerr|pwd|quit|raise|refresh|replot|"
					  "reread|reset|save|set|show|shell|splot|stats|syste|test|toggle|undefine|"
					  "unset|update|while|FIT_LIMIT|FIT_MAXITER|FIT_START_LAMBDA|FIT_LAMBDA_FACTOR|"
					  "FIT_LOG|FIT_SCRIPT|!|functions|var)\\b" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "\\b(abs|acos|acosh|arg|asin|asinh|atan|atan2|atanh|besj0|besj1|besy0|besy1|"
					  "ceil|cos|cosh|erf|erfc|exp|floor|gamma|ibeta|igamma|imag|int|inverf|invnorm|"
					  "lambertw|lgamma|log|log10|norm|rand|real|sgn|sin|sinh|sqrt|tan|tanh)\\b" },
					"function",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "\\b(gprintf|sprintf|strlen|strstrt|substr|system|word|words)\\b" },
					"function",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "\\b(on|off|default|inside|outside|lmargin|rmargin|tmargin|bmargin|at|left|"
					  "right|center|top|bottom|center|vertical|horizontal|Left|Right|noreverse|"
					  "noinvert|samplen|spacing|width|height|noautotitle|columnheader|title|"
					  "noenhanced|nobox|linestyle|ls|linetype|lt|linewidth|lw)\\b" },
					"literal",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "\\b(aed512|aed767|aifm|aqua|bitgraph|canvas|cgm|corel|dumb|dxf|eepic|emf|"
					  "emtex|eps|epslatex|epson_180dpi|epson_60dpi|epson_lx800|fig|gif|gpic|"
					  "hp2623A|hp2648|hp500c|hpdj|hpgl|hpljii|hppj|imagen|jpeg|kc_tek40xx|km_"
					  "tek40xx|latex|mf|mif|mp|nec_cp6|okidata|pbm|pcl5|pdf|png|pngcairo|"
					  "postscript|pslatex|pstex|pstricks|qms|regis|selanar|starc|svg|tandy_60dpi|"
					  "tek40xx|tek410x|texdraw|tgif|tkcanvas|tpic|unknown|vttek)\\b" },
					"literal",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "\\b(u(sing)?|t(it(le)?)?|notit(le)?|w(i(th)?)?|steps|fs(teps)?|notitle|l(i("
					  "nes)?)?|linespoints|via)\\b" },
					"keyword",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "(?x:\n\t\t\t\t        \\b                      # Start with a word "
					  "boundary\n\t\t\t\t        (?=\\b[\\w$]*(\\(|.*=|\\s))\t# Look-ahead for a "
					  "bracket or equals or a white space\n\t\t\t\t        (?![^(]*\\))\t          "
					  "  # negative look ahead for a closing bracket without an opening one. This "
					  "stops a from matching in f(a)\n\t\t\t\t        (\t\t\t\t\t              # "
					  "Group variable name\n\t\t\t\t\t        [A-Za-z] \t\t\t        # A "
					  "letter\n\t\t\t\t\t        [\\w$]*\t \t\t          # Any alphanumeric or "
					  "$\n\t\t\t\t        )\t\t\t\t\t              # That is it for the "
					  "name.\n\t\t\t        )" },
					"normal",
					"",
					SyntaxPatternMatchType::RegEx },
				  { { "\\b(if)\\b" }, "keyword", "", SyntaxPatternMatchType::RegEx },
				  { { "\\b(show)\\b", "(?!\\#)($\\n?)" },
					{ "keyword" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "$self" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "\\b(set)\\b\\s*\\b(terminal)\\b", "(?!\\#)($\\n?)" },
					{ "keyword", "keyword", "keyword" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "$self" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "\\b(set)\\b\\s*\\b(key)\\b", "(?!\\#)($\\n?)" },
					{ "keyword", "keyword", "keyword" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "$self" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },
				  { { "\\b(set)\\b\\s*(?!\\b(terminal|key|for)\\b)", "(?!\\#)($\\n?)" },
					{ "keyword", "keyword", "keyword" },
					{},
					"",
					SyntaxPatternMatchType::RegEx,
					{
						{ { "include", "$self" },
						  "normal",
						  "",
						  SyntaxPatternMatchType::LuaPattern },

					} },

			  },
			  {

			  },
			  "",
			  {}

			} )
		.addRepositories( {

			{ "string_single",
			  {
				  { { "'", "'" }, { "string" }, { "string" }, "", SyntaxPatternMatchType::RegEx },

			  } },
			{ "string_double",
			  {
				  { { "\"", "\"", "\\\\[\\$`\"\\\\\\n]" },
					{ "string" },
					{ "string" },
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "operator",
			  {
				  { { "\\s*(==|~=|>|>=|<|<=|&|&&|:|\\||\\|\\||\\+|-|\\*|\\.\\*|/|\\./"
					  "|\\\\|\\.\\\\|\\^|\\.\\^)\\s*" },
					"operator",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
			{ "number",
			  {
				  { { "(?x:         # turn on extended mode\n                -?         # an "
					  "optional minus\n                (?:\n                  [0-9]    # a 0-9 "
					  "character\n                  \\d*      # followed by zero or more digits\n  "
					  "              )\n                (?:\n                  \\.       # a "
					  "period\n                  \\d+      # followed by one or more digits\n      "
					  "          )?         # make decimal portion optional\n                (?:\n "
					  "                 [eE]   # an e character\n                  [+-]?  # "
					  "followed by an option +/-\n                  \\d+    # followed by one or "
					  "more digits\n                  (?:\n                    \\.   # a period\n  "
					  "                  \\d+  # followed by one or more digits\n                  "
					  ")?     # make decimal in exponent power optional\n                )?       "
					  "# make exponent optional\n              )" },
					"number",
					"",
					SyntaxPatternMatchType::RegEx },

			  } },
		} );
}

}}}} // namespace EE::UI::Doc::Language
