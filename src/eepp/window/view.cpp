#include <eepp/window/view.hpp>

namespace EE { namespace Window {

View::View() :
	mCenter             (),
	mSize               (),
	mRotation           (0),
	mViewport           (0, 0, 1, 1),
	mTransformUpdated   (false),
	mInvTransformUpdated(false)
{
	reset(Rectf(0, 0, 1000, 1000));
}

View::View(const Rectf& rectangle) :
	mCenter             (),
	mSize               (),
	mRotation           (0),
	mViewport           (0, 0, 1, 1),
	mTransformUpdated   (false),
	mInvTransformUpdated(false)
{
	reset(rectangle);
}

View::View(const Vector2f& center, const Vector2f& size) :
	mCenter             (center),
	mSize               (size),
	mRotation           (0),
	mViewport           (0, 0, 1, 1),
	mTransformUpdated   (false),
	mInvTransformUpdated(false)
{}

void View::setCenter(float x, float y) {
	mCenter.x = x;
	mCenter.y = y;

	mTransformUpdated    = false;
	mInvTransformUpdated = false;
}

void View::setCenter(const Vector2f& center) {
	setCenter(center.x, center.y);
}

void View::setSize(float width, float height) {
	mSize.x = width;
	mSize.y = height;

	mTransformUpdated    = false;
	mInvTransformUpdated = false;
}

void View::setSize(const Sizef & size) {
	setSize(size.x, size.y);
}

void View::setRotation(float angle) {
	mRotation = static_cast<float>(fmod(angle, 360));
	if (mRotation < 0)
		mRotation += 360.f;

	mTransformUpdated    = false;
	mInvTransformUpdated = false;
}

void View::setViewport(const Rectf& viewport) {
	mViewport = viewport;
}

void View::reset(const Rectf& rectangle) {
	mCenter.x = rectangle.Left + rectangle.Right / 2.f;
	mCenter.y = rectangle.Top + rectangle.Bottom / 2.f;
	mSize.x   = rectangle.Right;
	mSize.y   = rectangle.Bottom;
	mRotation = 0;

	mTransformUpdated    = false;
	mInvTransformUpdated = false;
}

const Vector2f& View::getCenter() const {
	return mCenter;
}

const Sizef& View::getSize() const {
	return mSize;
}

float View::getRotation() const {
	return mRotation;
}

const Rectf& View::getViewport() const {
	return mViewport;
}

void View::move(float offsetX, float offsetY) {
	setCenter(mCenter.x + offsetX, mCenter.y + offsetY);
}

void View::move(const Vector2f& offset) {
	setCenter(mCenter + offset);
}

void View::rotate(float angle) {
	setRotation(mRotation + angle);
}

void View::zoom(float factor) {
	setSize(mSize.x * factor, mSize.y * factor);
}

const Transform& View::getTransform() const {
	// Recompute the matrix if needed
	if (!mTransformUpdated) {
		// Rotation components
		float angle  = mRotation * 3.141592654f / 180.f;
		float cosine = static_cast<float>(std::cos(angle));
		float sine   = static_cast<float>(std::sin(angle));
		float tx     = -mCenter.x * cosine - mCenter.y * sine + mCenter.x;
		float ty     =  mCenter.x * sine - mCenter.y * cosine + mCenter.y;

		// Projection components
		float a =  2.f / mSize.x;
		float b = -2.f / mSize.y;
		float c = -a * mCenter.x;
		float d = -b * mCenter.y;

		// Rebuild the projection matrix
		mTransform = Transform( a * cosine, a * sine,   a * tx + c,
								-b * sine,   b * cosine, b * ty + d,
								 0.f,        0.f,        1.f);
		mTransformUpdated = true;
	}

	return mTransform;
}

const Transform& View::getInverseTransform() const {
	// Recompute the matrix if needed
	if (!mInvTransformUpdated) {
		mInverseTransform = getTransform().getInverse();
		mInvTransformUpdated = true;
	}

	return mInverseTransform;
}

}}
