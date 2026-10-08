#ifndef ETERM_KITTYKEYBOARDPROTOCOL_HPP
#define ETERM_KITTYKEYBOARDPROTOCOL_HPP

#include <cstdint>
#include <eepp/core/small_vector.hpp>
#include <eepp/window/keycodes.hpp>
#include <string>

using namespace EE;
using namespace EE::Window;

namespace eterm { namespace Terminal {

enum class KittyKeyboardFlag : Uint32 {
	DisambiguateEscapeCodes = 1u,
	ReportEventTypes = 2u,
	ReportAlternateKeys = 4u,
	ReportAllKeysAsEscapeCodes = 8u,
	ReportAssociatedText = 16u,
};

constexpr Uint32 kittyKeyboardFlag( KittyKeyboardFlag flag ) {
	return static_cast<Uint32>( flag );
}

constexpr Uint32 KITTY_KEYBOARD_SUPPORTED_FLAGS = 31u;

enum class KittyKeyEventType : Uint8 { Press = 1, Repeat = 2, Release = 3 };

struct KittyKeyboardState {
	static constexpr size_t MaxStackDepth = 64;

	Uint32 flags{ 0 };
	SmallVector<Uint32, 8> stack;

	void reset();
	void push( Uint32 requestedFlags );
	void pop( size_t count = 1 );
	void set( Uint32 requestedFlags, Uint32 mode = 1 );
};

struct KittyKeyEvent {
	Keycode keycode{ KEY_UNKNOWN };
	Scancode scancode{ SCANCODE_UNKNOWN };
	Uint32 character{ 0 };
	Uint32 modifiers{ 0 };
	KittyKeyEventType type{ KittyKeyEventType::Press };
};

struct KittyEncodedKey {
	std::string bytes;
	Uint32 expectedText{ 0 };
	bool handled{ false };
};

class KittyKeyboardEncoder {
  public:
	static KittyEncodedKey encode( const KittyKeyEvent& event, Uint32 activeFlags );

	static std::string encodeText( Uint32 codepoint, Uint32 activeFlags );

	static Uint32 encodeModifiers( Uint32 eeppModifiers );
};

}} // namespace eterm::Terminal

#endif
