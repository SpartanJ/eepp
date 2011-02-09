#ifndef EE_INPUT_HELPER_HPP
#define EE_INPUT_HELPER_HPP

#include "../base.hpp"
#include "base.hpp"

namespace EE { namespace Window {

#define EE_KEYS_SPACE 		(336/8)

Uint32 EE_API eeConvertKeyCharacter( const Uint32& KeyCode, const Uint16& Unicode, const Uint32& Modifiers );

}}

#endif 
