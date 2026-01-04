#ifndef EE_SYSTEM_CPU_HPP
#define EE_SYSTEM_CPU_HPP

#include <eepp/config.hpp>

namespace EE { namespace System {

class EE_API CPU {
  public:
	static bool hasAVX2();

	static bool hasNEON();
};

}} // namespace EE::System

#endif // EE_SYSTEM_CPU_HPP
