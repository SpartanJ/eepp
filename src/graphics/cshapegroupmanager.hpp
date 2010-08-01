#ifndef EE_GRAPHICSCSHAPEGROUPMANAGER_HPP
#define EE_GRAPHICSCSHAPEGROUPMANAGER_HPP

#include "base.hpp"
#include "cshape.hpp"
#include "cshapegroup.hpp"

namespace EE { namespace Graphics {

class EE_API cShapeGroupManager : public cSingleton<cShapeGroupManager> {
	friend class cSingleton<cShapeGroupManager>;
	public:
		cShapeGroupManager();

		virtual ~cShapeGroupManager();

		void Add( cShapeGroup * Group );

		void Remove( cShapeGroup * Group, bool Delete = true );

		Uint32 Count();

		cShapeGroup * GetByName( const std::string& Name );

		cShapeGroup * GetById( const Uint32& Id );

		cShape * GetShapeByName( const std::string& Name );

		cShape * GetShapeById( const Uint32& Id );

		void Destroy();
	protected:
		std::list<cShapeGroup*> mGroups;
};

}}

#endif
