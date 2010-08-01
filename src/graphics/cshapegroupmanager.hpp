#ifndef EE_GRAPHICSCSHAPEGROUPMANAGER_HPP
#define EE_GRAPHICSCSHAPEGROUPMANAGER_HPP

#include "base.hpp"
#include "cshape.hpp"
#include "cshapegroup.hpp"

namespace EE { namespace Graphics {

class EE_API cShapeGroupManager : public tResourceManager<cShapeGroup>, public cSingleton<cShapeGroupManager> {
	friend class cSingleton<cShapeGroupManager>;
	public:
		cShapeGroupManager();

		virtual ~cShapeGroupManager();

		cShape * GetShapeByName( const std::string& Name );

		cShape * GetShapeById( const Uint32& Id );
};

}}

#endif
