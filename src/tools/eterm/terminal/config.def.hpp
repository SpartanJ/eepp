/* See LICENSE file for copyright and license details. */

/* identification sequence returned in DA and DECID */
const char* vtiden = "\033[?6c";

/*
 * word delimiter string
 *
 * More advanced example: L" `'\"()[]{}"
 */
const unsigned int worddelimiters[] = { ' ', 0 };

/* selection timeouts (in milliseconds) */
static unsigned int doubleclicktimeout = 300;
static unsigned int tripleclicktimeout = 600;

/*
 * bell volume. It must be a value between -100 and 100. Use 0 for disabling
 * it
 */
static int bellvolume = 0;

/* default TERM value */
const char* termname = "st-256color";

/*
 * Default shape of cursor
 * 2: Block ("█")
 * 4: Underline ("_")
 * 6: Bar ("|")
 * 7: Snowman ("☃")
 */
static unsigned int cursorshape = 2;

/*
 * spaces per tab
 *
 * When you are changing this value, don't forget to adapt the »it« value in
 * the st.info and appropriately install the st.info in the environment where
 * you use this st version.
 *
 *	it#$tabspaces,
 *
 * Secondly make sure your kernel is not expanding tabs. When running `stty
 * -a` »tab0« should appear. You can tell the terminal to not expand tabs by
 *  running following command:
 *
 *	stty tabs
 */
const unsigned int tabspaces = 8;

/* Terminal colors (16 first used in escape sequence) */
static const char* colorname[258] = {
	// /* 8 normal colors */
	// "black",
	// "red3",
	// "green3",
	// "yellow3",
	// "blue2",
	// "magenta3",
	// "cyan3",
	// "gray90",

	// /* 8 bright colors */
	// "gray50",
	// "red",
	// "green",
	// "yellow",
	// "#5c5cff",
	// "magenta",
	// "cyan",
	// "white"

	"#1e2127", "#e06c75", "#98c379", "#d19a66", "#61afef", "#c678dd", "#56b6c2",
	"#abb2bf", "#5c6370", "#e06c75", "#98c379", "#d19a66", "#61afef", "#c678dd",
	"#56b6c2", "#ffffff", "#1e2127", "#abb2bf"

};

/*
 * Printable characters in ASCII, used to estimate the advance width
 * of single wide characters.
 */
static const char ascii_printable[] = " !\"#$%&'()*+,-./0123456789:;<=>?"
									  "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
									  "`abcdefghijklmnopqrstuvwxyz{|}~";

// /*
//  * Selection types' masks.
//  * Use the same masks as usual.
//  * Button1Mask is always unset, to make masks match between ButtonPress.
//  * ButtonRelease and MotionNotify.
//  * If no match is found, regular selection is used.
//  */
// static unsigned int selmasks[] = {
// [SEL_RECTANGULAR] = Mod1Mask,
// };
