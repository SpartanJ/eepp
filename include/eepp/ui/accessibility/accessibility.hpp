#ifndef EE_UI_ACCESSIBILITY_ACCESSIBILITY_HPP
#define EE_UI_ACCESSIBILITY_ACCESSIBILITY_HPP

#include <atomic>
#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/math/rect.hpp>

namespace EE { namespace UI {

/** Process-wide native-client activity, independent of any node or widget. */
class EE_API Accessibility {
  public:
	/** Notifications test this inline: no clients means one relaxed load and no call.
	 * Callers that compute an event or payload test it before preparing that data. */
	static bool isActive() { return sActiveScenes.load( std::memory_order_relaxed ) != 0; }

	/** Scene lifecycle bookkeeping, called only when a root scene's activity actually changes.
	 * Counts scenes rather than a boolean so closing one window cannot disable another. */
	static void onSceneClientActivityChanged( bool active );

  private:
	static std::atomic<Uint32> sActiveScenes;
};

using AccessibilitySourceId = Uint64;

/** Controls whether a root UI scene installs its native accessibility backend. */
enum class AccessibilityPolicy : Uint8 {
	/** Uses the platform default. Currently accessibility is enabled on supported platforms. */
	Auto,
	/** Requests the native backend. EEPP_DISABLE_ACCESSIBILITY still takes precedence. */
	Enabled,
	/** Keeps the scene on the null backend and avoids native accessibility integration. */
	Disabled,
};

enum class AccessibilityRole : Uint8 {
	None,
	Application,
	Window,
	Dialog,
	Group,
	Button,
	CheckBox,
	RadioButton,
	Label,
	Text,
	TextBox,
	Image,
	ComboBox,
	Slider,
	SpinButton,
	ProgressBar,
	TabList,
	Tab,
	TabPanel,
	MenuBar,
	Menu,
	MenuItem,
	CheckMenuItem,
	RadioMenuItem,
	List,
	ListItem,
	Table,
	Row,
	Cell,
	Tree,
	TreeItem,
	Link,
	Heading,
};

enum class AccessibilityState : Uint64 {
	None = 0,
	Enabled = 1ull << 0,
	Focusable = 1ull << 1,
	Focused = 1ull << 2,
	Checked = 1ull << 3,
	Selected = 1ull << 4,
	Editable = 1ull << 5,
	ReadOnly = 1ull << 6,
	Visible = 1ull << 7,
	Showing = 1ull << 8,
	Expanded = 1ull << 9,
	Active = 1ull << 10,
	Protected = 1ull << 11,
	MultiLine = 1ull << 12,
	/** Offers selection of children, unlike a non-interactive HTML list. */
	SelectionContainer = 1ull << 13,
};

inline AccessibilityState operator|( AccessibilityState left, AccessibilityState right ) {
	return static_cast<AccessibilityState>( static_cast<Uint64>( left ) |
											static_cast<Uint64>( right ) );
}

inline AccessibilityState& operator|=( AccessibilityState& left, AccessibilityState right ) {
	left = left | right;
	return left;
}

enum class AccessibilityAction : Uint8 {
	Focus,
	Press,
	Toggle,
	Select,
	Increment,
	Decrement,
	SetValue,
	SetText,
	Expand,
	Collapse,
	ScrollTo,
	SetTextSelection,
};

using AccessibilityActions = Uint32;

constexpr AccessibilityActions accessibilityActionMask( AccessibilityAction action ) {
	return 1u << static_cast<Uint32>( action );
}

/** Politeness of an announcement or live region: Polite waits for the client to finish speaking,
 * Assertive interrupts it. */
enum class AccessibilityLive : Uint8 { Off, Polite, Assertive };

enum class AccessibilityEvent : Uint8 {
	Created,
	Destroyed,
	ChildrenChanged,
	ModelChanged,
	FocusChanged,
	NameChanged,
	DescriptionChanged,
	ValueChanged,
	StateChanged,
	EnabledChanged,
	VisibilityChanged,
	SelectionChanged,
	BoundsChanged,
	RoleChanged,
};

/** A keyboard shortcut, both as displayed and decomposed for native APIs that want their own
 * format (GTK accelerators on AT-SPI, command characters on macOS). */
struct AccessibilityShortcut {
	enum Modifier : Uint8 { Control = 1 << 0, Shift = 1 << 1, Alt = 1 << 2, Meta = 1 << 3 };

	/** As displayed to the user (UTF-8), for example "Ctrl+S". */
	std::string text;
	/** The platform key name (UTF-8), for example "S", "F5" or "Return". */
	std::string key;
	Uint8 modifiers{ 0 };

	bool empty() const { return text.empty(); }
};

struct AccessibilityNodeRef {
	AccessibilitySourceId source{ 0 };
	Uint64 id{ 0 };

	bool isValid() const { return source != 0 && id != 0; }

	bool operator==( const AccessibilityNodeRef& other ) const {
		return source == other.source && id == other.id;
	}

	bool operator!=( const AccessibilityNodeRef& other ) const { return !( *this == other ); }
};

struct AccessibilityRangeInfo {
	double minimum{ 0 };
	double maximum{ 0 };
	double smallChange{ 0 };
	double largeChange{ 0 };
	bool valid{ false };
};

struct AccessibilityTextInfo {
	Int32 caretOffset{ 0 };
	Int32 selectionStart{ 0 };
	Int32 selectionEnd{ 0 };
	bool valid{ false };
};

/** Identifies one version of an element's text. Equal revisions have equal text, so backends can
 * cache converted copies (UTF-16 for UIA and AppKit) instead of rebuilding them per query. */
struct AccessibilityTextRevision {
	/** The document's UUID halves: unlike its address, never reused by another document. */
	Uint64 documentHigh{ 0 };
	Uint64 documentLow{ 0 };
	Uint64 modification{ 0 };

	bool isValid() const { return documentHigh != 0 || documentLow != 0; }

	bool operator==( const AccessibilityTextRevision& other ) const {
		return documentHigh == other.documentHigh && documentLow == other.documentLow &&
			   modification == other.modification;
	}

	bool operator!=( const AccessibilityTextRevision& other ) const { return !( *this == other ); }
};

/** One edit, in code points: at `offset`, `removed` was replaced by `inserted`. An offset of -1
 * means the whole text was replaced (a reset, a reload, or too many edits in one frame). */
struct AccessibilityTextChange {
	Int32 offset{ -1 };
	String removed;
	String inserted;

	bool isWholeText() const { return offset < 0; }
};

struct AccessibilityNodeInfo {
	String name;
	String description;
	String value;
	/** Keyboard shortcut that activates the element. */
	AccessibilityShortcut shortcut;
	AccessibilityState states{ AccessibilityState::None };
	AccessibilityRangeInfo range;
	AccessibilityTextInfo text;
	Math::Rectf bounds;
	// The small fields share one tail word instead of padding three; built per client query.
	AccessibilityActions actions{ 0 };
	AccessibilityRole role{ AccessibilityRole::None };
	bool boundsValid{ false };
	/** 1..6 for HTML headings; zero when unspecified. Uses existing tail padding. */
	Uint8 headingLevel{ 0 };
};

struct AccessibilityAnnouncement {
	String message;
	AccessibilityLive priority{ AccessibilityLive::Polite };
	/** The live-region widget that produced it; invalid for an explicit announcement. A newer
	 * change from the same widget replaces its pending announcement. */
	AccessibilityNodeRef source;
};

struct AccessibilityActionRequest {
	AccessibilityAction action{ AccessibilityAction::Focus };
	String value;
};

struct AccessibilityPendingEvent {
	AccessibilityNodeRef ref;
	AccessibilityNodeRef related;
	Int32 index{ -1 };
	AccessibilityEvent type;
};

}} // namespace EE::UI

#endif
