#ifndef EE_GRAPHICSCGLOBALSHAPEGROUP_HPP
#define EE_GRAPHICSCGLOBALSHAPEGROUP_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/cshapegroup.hpp>

namespace EE { namespace Graphics {

class EE_API cGlobalShapeGroup : public cShapeGroup {
	SINGLETON_DECLARE_HEADERS(cGlobalShapeGroup)

	public:
		cGlobalShapeGroup();

		~cGlobalShapeGroup();
};

}}

#endif
