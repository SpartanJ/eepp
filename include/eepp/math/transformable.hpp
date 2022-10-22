#ifndef EE_MATH_TRANSFORMABLE_HPP
#define EE_MATH_TRANSFORMABLE_HPP

#include <eepp/core.hpp>
#include <eepp/math/transform.hpp>

namespace EE { namespace Math {

class EE_API Transformable {
  public:
	Transformable();

	virtual ~Transformable();

	virtual void setPosition( const Vector2f& position );

	virtual void setRotation( float angle );

	virtual void setScale( float factorX, float factorY );

	virtual void setScale( const Vector2f& factors );

	virtual void setScaleOrigin( float x, float y );

	virtual void setScaleOrigin( const Vector2f& origin );

	virtual void setRotationOrigin( float x, float y );

	virtual void setRotationOrigin( const Vector2f& origin );

	virtual const Vector2f& getRotationOrigin() const;

	virtual const Vector2f& getPosition() const;

	virtual const float& getRotation() const;

	virtual const Vector2f& getScale() const;

	virtual const Vector2f& getScaleOrigin() const;

	void move( float offsetX, float offsetY );

	void move( const Vector2f& offset );

	void rotate( float angle );

	void scale( float factorX, float factorY );

	void scale( const Vector2f& factor );

	const Transform& getTransform() const;

	const Transform& getInverseTransform() const;

  protected:
	Vector2f mScaleOrigin;					  ///< Origin of scaling of the object
	Vector2f mRotationOrigin;				  ///< Origin of rotation of the object
	Vector2f mPosition;						  ///< Position of the object in the 2D world
	float mRotation;						  ///< Orientation of the object, in degrees
	Vector2f mScale;						  ///< Scale of the object
	mutable Transform mTransform;			  ///< Combined transformation of the object
	mutable Transform mInverseTransform;	  ///< Combined transformation of the object
	mutable bool mTransformNeedUpdate;		  ///< Does the transform need to be recomputed?
	mutable bool mInverseTransformNeedUpdate; ///< Does the transform need to be recomputed?
};

}} // namespace EE::Math

#endif
