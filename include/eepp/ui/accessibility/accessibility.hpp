#ifndef EE_UI_ACCESSIBILITY_ACCESSIBILITY_HPP
#define EE_UI_ACCESSIBILITY_ACCESSIBILITY_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/math/rect.hpp>
#include <vector>

namespace EE { namespace UI {

using AccessibilitySourceId = Uint64;

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
};

using AccessibilityActions = Uint32;

constexpr AccessibilityActions accessibilityActionMask( AccessibilityAction action ) {
	return 1u << static_cast<Uint32>( action );
}

enum class AccessibilityRelation : Uint8 { LabelledBy, DescribedBy, Controls, ControlledBy };

enum class AccessibilityEvent : Uint8 {
	Created,
	Destroyed,
	ChildrenChanged,
	FocusChanged,
	NameChanged,
	ValueChanged,
	StateChanged,
	SelectionChanged,
	BoundsChanged,
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

struct AccessibilityRelationInfo {
	AccessibilityRelation relation;
	AccessibilityNodeRef target;
};

struct AccessibilityRangeInfo {
	double minimum{ 0 };
	double maximum{ 0 };
	double smallChange{ 0 };
	double largeChange{ 0 };
	bool valid{ false };
};

struct AccessibilityNodeInfo {
	AccessibilityRole role{ AccessibilityRole::None };
	String name;
	String description;
	String value;
	AccessibilityState states{ AccessibilityState::None };
	AccessibilityActions actions{ 0 };
	AccessibilityRangeInfo range;
	Math::Rectf bounds;
	bool boundsValid{ false };
	std::vector<AccessibilityRelationInfo> relations;
};

struct AccessibilityActionRequest {
	AccessibilityAction action{ AccessibilityAction::Focus };
	String value;
};

struct AccessibilityPendingEvent {
	AccessibilityNodeRef ref;
	AccessibilityEvent type;
};

}} // namespace EE::UI

#endif
