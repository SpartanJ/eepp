#ifndef EE_GRAPHICS_NINEPATCHMANAGER_HPP
#define EE_GRAPHICS_NINEPATCHMANAGER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/ninepatch.hpp>

#include <eepp/system/resourcemanager.hpp>
#include <eepp/system/singleton.hpp>
using namespace EE::System;

namespace EE { namespace Graphics {

class EE_API NinePatchManager : public ResourceManager<NinePatch> {
	SINGLETON_DECLARE_HEADERS( NinePatchManager )

	~NinePatchManager();
};

}} // namespace EE::Graphics

#endif
