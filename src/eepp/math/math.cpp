#include <eepp/math/math.hpp>
#include <eepp/system/sys.hpp>
using namespace EE::System;

namespace EE { namespace Math {

Uint32 SetRandomSeed() {
	Uint32 Seed = static_cast<Uint32>( Sys::GetSystemTime() * 1000 );
	srand(Seed);
	return Seed;
}

}}
