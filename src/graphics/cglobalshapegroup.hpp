#ifndef EE_GRAPHICSCGLOBALSHAPEGROUP_HPP
#define EE_GRAPHICSCGLOBALSHAPEGROUP_HPP

#include "base.hpp"
#include "cshapegroup.hpp"

namespace EE { namespace Graphics {

class EE_API cGlobalShapeGroup : public cShapeGroup, public tSingleton<cGlobalShapeGroup> {
	friend class tSingleton<cGlobalShapeGroup>;
	public:
		cGlobalShapeGroup();

		~cGlobalShapeGroup();
};

}}

#endif
