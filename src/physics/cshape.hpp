#ifndef EE_PHYSICS_CSHAPE_HPP
#define EE_PHYSICS_CSHAPE_HPP

#include "base.hpp"
#include "cbody.hpp"

namespace EE { namespace Physics {

class cShapeCircle;
class cShapeSegment;
class cShapePoly;
class cSpace;

class cShape {
	public:
		static void ResetShapeIdCounter();

		cpShape * Shape() const;

		~cShape();

		cBody * Body() const;

		void Body( cBody * body );

		cpBB BB() const;

		void BB( const cpBB& bb );

		bool Sensor();

		void Sensor( const bool& sensor );

		cpFloat e() const;

		void e( const cpFloat& e );

		cpFloat u() const;

		void u( const cpFloat& u );

		cpVect SurfaceVel() const;

		void SurfaceVel( const cpVect& vel );

		cpCollisionType CollisionType()	 const;

		void CollisionType( const cpCollisionType& type );

		cpGroup Group() const;

		void Group( const cpGroup& group );

		cpLayers Layers() const;

		void Layers( const cpLayers& layers );

		cpBB CacheBB();

		cpBB Update( cpVect pos, cpVect rot );

		bool PointQuery( cpVect p );

		cpShapeType Type() const;

		cShapePoly * GetAsPoly();

		cShapeCircle * GetAsCircle();

		cShapeSegment * GetAsSegment();

		virtual void Draw( cSpace * space ) = 0;

		virtual void DrawBB();
	protected:
		cShape();

		cpShape *		mShape;
		cBody *			mBody;

		void SetData();
};

}}

#endif
