#ifndef EE_PHYSICS_CSHAPE_HPP
#define EE_PHYSICS_CSHAPE_HPP

#include <eepp/physics/base.hpp>
#include <eepp/physics/body.hpp>

namespace EE { namespace Physics {

class ShapeCircle;
class ShapeSegment;
class ShapePoly;
class Space;

class EE_API Shape {
	public:
		static void resetShapeIdCounter();

		static void Free( Shape * shape, bool DeleteBody = false );

		cpShape * getShape() const;

		virtual ~Shape();

		Physics::Body * getBody() const;

		void setBody( Physics::Body * body );

		cBB getBB() const;

		void setBB( const cBB& bb );

		bool isSensor();

		void setSensor( const bool& sensor );

		cpFloat getE() const;

		void setE( const cpFloat& e );

		cpFloat getElasticity() const;

		void setElasticity( const cpFloat& e );

		cpFloat getU() const;

		void setU( const cpFloat& u );

		cpFloat getFriction() const;

		void setFriction( const cpFloat& u );

		cVect getSurfaceVel() const;

		void getSurfaceVel( const cVect& vel );

		cpCollisionType getCollisionType()	 const;

		void setCollisionType( const cpCollisionType& type );

		cpGroup getGroup() const;

		void setGroup( const cpGroup& group );

		cpLayers getLayers() const;

		void setLayers( const cpLayers& layers );

		cBB cacheBB();

		cBB update( cVect pos, cVect rot );

		bool pointQuery( cVect p );

		cpShapeType getType() const;

		ShapePoly * getAsPoly();

		ShapeCircle * getAsCircle();

		ShapeSegment * getAsSegment();

		virtual void draw( Space * space );

		virtual void drawBorder( Space * space );

		virtual void drawBB();

		void * getData() const;

		void setData( void * data );
	protected:
		Shape();

		cpShape *		mShape;
		void *			mData;

		void setData();
};

}}

#endif
