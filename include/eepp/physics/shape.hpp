#ifndef EE_PHYSICS_CSHAPE_HPP
#define EE_PHYSICS_CSHAPE_HPP

#include <eepp/physics/base.hpp>
#include <eepp/physics/body.hpp>

CP_NAMESPACE_BEGIN

class ShapeCircle;
class ShapeSegment;
class ShapePoly;
class Space;

class CP_API Shape {
	public:
		static void ResetShapeIdCounter();

		static void Free( Shape * shape, bool DeleteBody = false );

		cpShape * GetShape() const;

		virtual ~Shape();

		Physics::Body * Body() const;

		void Body( Physics::Body * body );

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

		ShapePoly * GetAsPoly();

		ShapeCircle * GetAsCircle();

		ShapeSegment * GetAsSegment();

		virtual void Draw( Space * space );

		virtual void DrawBorder( Space * space );

		virtual void DrawBB();

		void * Data() const;

		void Data( void * data );
	protected:
		Shape();

		cpShape *		mShape;
		void *			mData;

		void SetData();
};

CP_NAMESPACE_END

#endif
