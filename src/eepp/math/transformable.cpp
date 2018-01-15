#include <eepp/math/transformable.hpp>
#include <cmath>

namespace EE { namespace Math {

Transformable::Transformable() :
	mScaleOrigin(0, 0),
	mPosition(0, 0),
	mRotation(0),
	mScale(1, 1),
	mTransform(),
	mInverseTransform(),
	mTransformNeedUpdate(true),
	mInverseTransformNeedUpdate(true)
{
}

Transformable::~Transformable()
{}

void Transformable::setPosition(float x, float y) {
	mPosition.x = x;
	mPosition.y = y;
	mTransformNeedUpdate = true;
	mInverseTransformNeedUpdate = true;
}

void Transformable::setPosition(const Vector2f& position) {
	setPosition(position.x, position.y);
}

void Transformable::setRotation(float angle) {
	mRotation = static_cast<float>(fmod(angle, 360));
	if (mRotation < 0)
		mRotation += 360.f;

	mTransformNeedUpdate = true;
	mInverseTransformNeedUpdate = true;
}

void Transformable::setScale(float factorX, float factorY) {
	mScale.x = factorX;
	mScale.y = factorY;
	mTransformNeedUpdate = true;
	mInverseTransformNeedUpdate = true;
}

void Transformable::setScale(const Vector2f& factors) {
	setScale(factors.x, factors.y);
}

void Transformable::setScaleOrigin(float x, float y) {
	mScaleOrigin.x = x;
	mScaleOrigin.y = y;
	mTransformNeedUpdate = true;
	mInverseTransformNeedUpdate = true;
}

void Transformable::setScaleOrigin(const Vector2f& origin) {
	setScaleOrigin(origin.x, origin.y);
}

void Transformable::setRotationOrigin(float x, float y) {
	mRotationOrigin.x = x;
	mRotationOrigin.y = y;
	mTransformNeedUpdate = true;
	mInverseTransformNeedUpdate = true;
}

void Transformable::setRotationOrigin(const Vector2f& origin) {
	setRotationOrigin(origin.x, origin.y);
}

const Vector2f& Transformable::getPosition() const {
	return mPosition;
}

float Transformable::getRotation() const {
	return mRotation;
}

const Vector2f& Transformable::getScale() const {
	return mScale;
}

const Vector2f& Transformable::getScaleOrigin() const {
	return mScaleOrigin;
}

const Vector2f& Transformable::getRotationOrigin() const {
	return mRotationOrigin;
}

void Transformable::move(float offsetX, float offsetY) {
	setPosition(mPosition.x + offsetX, mPosition.y + offsetY);
}

void Transformable::move(const Vector2f& offset) {
	setPosition(mPosition.x + offset.x, mPosition.y + offset.y);
}

void Transformable::rotate(float angle) {
	setRotation(mRotation + angle);
}

void Transformable::scale(float factorX, float factorY) {
	setScale(mScale.x * factorX, mScale.y * factorY);
}

void Transformable::scale(const Vector2f& factor) {
	setScale(mScale.x * factor.x, mScale.y * factor.y);
}

const Transform& Transformable::getTransform() const {
	// Recompute the combined transform if needed
	if (mTransformNeedUpdate) {
		/*Transform t;

		if ( 1.f != mScale ) {
			t.translate( mScaleOrigin );
			t.scale( mScale );
			t.translate( -mScaleOrigin );
		}

		if ( 0.f != mRotation ) {
			t.translate(  mRotationOrigin );
			t.rotate( mRotation );
			t.translate( -mRotationOrigin );
		}

		t.translate( mPosition );

		mTransform = t;*/

		float angle  = -mRotation * EE_PI_180;
		float cosine = eecos(angle);
		float sine   = eesin(angle);
		float sxc    = mScale.x * cosine;
		float syc    = mScale.y * cosine;
		float sxs    = mScale.x * sine;
		float sys    = mScale.y * sine;
		float tx     = -mRotationOrigin.x * sxc - mRotationOrigin.y * sys + mPosition.x;
		float ty     =  mRotationOrigin.x * sxs - mRotationOrigin.y * syc + mPosition.y;

		mTransform = Transform( sxc, sys, tx, -sxs, syc, ty, 0.f, 0.f, 1.f );
	}

	return mTransform;
}

const Transform& Transformable::getInverseTransform() const {
	if (mInverseTransformNeedUpdate) {
		mInverseTransform = getTransform().getInverse();
		mInverseTransformNeedUpdate = false;
	}

	return mInverseTransform;
}

}}
