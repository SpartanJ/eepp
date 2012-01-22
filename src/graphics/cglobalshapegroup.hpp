#ifndef EE_GRAPHICSCGLOBALSHAPEGROUP_HPP
#define EE_GRAPHICSCGLOBALSHAPEGROUP_HPP

#include "base.hpp"
#include "cshapegroup.hpp"

namespace EE { namespace Graphics {

class EE_API cGlobalShapeGroup : public cShapeGroup {
	SINGLETON_DECLARE_HEADERS(cGlobalShapeGroup)

	public:
		cGlobalShapeGroup();

		~cGlobalShapeGroup();
};

}}

#endif
