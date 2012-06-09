#ifndef EE_PHYSICS_CSHAPE_HPP
#define EE_PHYSICS_CSHAPE_HPP

#include <eepp/physics/base.hpp>
#include <eepp/physics/cbody.hpp>

CP_NAMESPACE_BEGIN

class cShapeCircle;
class cShapeSegment;
class cShapePoly;
class cSpace;

class CP_API cShape {
	public:
		static void ResetShapeIdCounter();

		static void Free( cShape * shape, bool DeleteBody = false );

		cpShape * Shape() const;

		virtual ~cShape();

		cBody * Body() const;

		void Body( cBody * body );

		cBB BB() const;

		void BB( const cBB& bb );

		bool Sensor();

		void Sensor( const bool& sensor );

		cpFloat e() const;

		void e( const cpFloat& e );

		cpFloat Elasticity() const;

		void Elasticity( const cpFloat& e );

		cpFloat u() const;

		void u( const cpFloat& u );

		cpFloat Friction() const;

		void Friction( const cpFloat& u );

		cVect SurfaceVel() const;

		void SurfaceVel( const cVect& vel );

		cpCollisionType CollisionType()	 const;

		void CollisionType( const cpCollisionType& type );

		cpGroup Group() const;

		void Group( const cpGroup& group );

		cpLayers Layers() const;

		void Layers( const cpLayers& layers );

		cBB CacheBB();

		cBB Update( cVect pos, cVect rot );

		bool PointQuery( cVect p );

		cpShapeType Type() const;

		cShapePoly * GetAsPoly();

		cShapeCircle * GetAsCircle();

		cShapeSegment * GetAsSegment();

		virtual void Draw( cSpace * space ) = 0;

		virtual void DrawBB();

		void * Data() const;

		void Data( void * data );
	protected:
		cShape();

		cpShape *		mShape;
		void *			mData;

		void SetData();
};

CP_NAMESPACE_END

#endif
