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
		static void resetShapeIdCounter();

		static void Free( Shape * shape, bool DeleteBody = false );

		cpShape * getShape() const;

		virtual ~Shape();

		Physics::Body * body() const;

		void body( Physics::Body * body );

		cBB bb() const;

		void bb( const cBB& bb );

		bool Sensor();

		void Sensor( const bool& sensor );

		cpFloat e() const;

		void e( const cpFloat& e );

		cpFloat elasticity() const;

		void elasticity( const cpFloat& e );

		cpFloat u() const;

		void u( const cpFloat& u );

		cpFloat friction() const;

		void friction( const cpFloat& u );

		cVect surfaceVel() const;

		void surfaceVel( const cVect& vel );

		cpCollisionType collisionType()	 const;

		void collisionType( const cpCollisionType& type );

		cpGroup group() const;

		void group( const cpGroup& group );

		cpLayers layers() const;

		void layers( const cpLayers& layers );

		cBB cacheBB();

		cBB update( cVect pos, cVect rot );

		bool pointQuery( cVect p );

		cpShapeType type() const;

		ShapePoly * getAsPoly();

		ShapeCircle * getAsCircle();

		ShapeSegment * getAsSegment();

		virtual void draw( Space * space );

		virtual void drawBorder( Space * space );

		virtual void drawBB();

		void * data() const;

		void data( void * data );
	protected:
		Shape();

		cpShape *		mShape;
		void *			mData;

		void setData();
};

CP_NAMESPACE_END

#endif
