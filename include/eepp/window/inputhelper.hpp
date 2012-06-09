#ifndef EE_INPUT_HELPER_HPP
#define EE_INPUT_HELPER_HPP

#include <eepp/window/base.hpp>

namespace EE { namespace Window {

#define EE_KEYS_NUM			(336)
#define EE_KEYS_SPACE 		(EE_KEYS_NUM/8)

Uint32 EE_API eeConvertKeyCharacter( const Uint32& KeyCode, const Uint16& Unicode, const Uint32& Modifiers );

}}

#endif
