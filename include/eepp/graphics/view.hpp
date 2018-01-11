#ifndef EE_GRAPHICS_VIEW_H
#define EE_GRAPHICS_VIEW_H

#include <eepp/math/size.hpp>
#include <eepp/math/transform.hpp>
using namespace EE::Math;

namespace EE { namespace Graphics {

/** @brief The class defines a view like a 2D camera ( position, size, move, scale ). Basically is a 2D proyection in pixels seted over a viewport. */
class EE_API View {
	public:
		View();

		explicit View(const Rectf& rectangle);

		View(const Vector2f& center, const Vector2f& size);

		void setCenter(float x, float y);

		void setCenter(const Vector2f& center);

		void setSize(float width, float height);

		void setSize(const Sizef& size);

		void setRotation(float angle);

		void setViewport(const Rectf& viewport);

		void reset(const Rectf& rectangle);

		const Vector2f& getCenter() const;

		const Sizef & getSize() const;

		float getRotation() const;

		const Rectf& getViewport() const;

		void move(float offsetX, float offsetY);

		void move(const Vector2f& offset);

		void rotate(float angle);

		void zoom(float factor);

		const Transform& getTransform() const;
		
		const Transform& getInverseTransform() const;
	private:
		friend class Window;

		Vector2f          mCenter;              ///< Center of the view, in scene coordinates
		Sizef             mSize;                ///< Size of the view, in scene coordinates
		float             mRotation;            ///< Angle of rotation of the view rectangle, in degrees
		Rectf             mViewport;            ///< Viewport rectangle, expressed as a factor of the render-target's size
		mutable Transform mTransform;           ///< Precomputed projection transform corresponding to the view
		mutable Transform mInverseTransform;    ///< Precomputed inverse projection transform corresponding to the view
		mutable bool      mTransformUpdated;    ///< Internal state telling if the transform needs to be updated
		mutable bool      mInvTransformUpdated; ///< Internal state telling if the inverse transform needs to be updated
};

}}

#endif
