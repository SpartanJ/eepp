#ifndef EE_INPUT_HELPER_HPP
#define EE_INPUT_HELPER_HPP

#include <eepp/window/base.hpp>

namespace EE { namespace Window {

class InputHelper {
  public:
	static Uint32 EE_API convertKeyCharacter( const Uint32& KeyCode, const Uint16& Unicode );
};

}} // namespace EE::Window

#endif
