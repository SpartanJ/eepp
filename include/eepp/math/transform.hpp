#ifndef SFML_TRANSFORM_HPP
#define SFML_TRANSFORM_HPP

#include <eepp/config.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/math/vector2.hpp>

namespace EE { namespace Math {

class EE_API Transform {
	public:
		Transform();

		Transform(float a00, float a01, float a02, float a10, float a11, float a12, float a20, float a21, float a22 );

		const float* getMatrix() const;

		Transform getInverse() const;

		Vector2f transformPoint(float x, float y) const;

		Vector2f transformPoint(const Vector2f& point) const;

		Rectf transformRect(const Rectf& rectangle) const;

		Transform& combine(const Transform& transform);

		Transform& translate(float x, float y);

		Transform& translate(const Vector2f& offset);

		Transform& rotate(float angle);

		Transform& rotate(float angle, float centerX, float centerY);

		Transform& rotate(float angle, const Vector2f& center);

		Transform& scale(float scaleX, float scaleY);

		Transform& scale(float scaleX, float scaleY, float centerX, float centerY);

		Transform& scale(const Vector2f& factors);

		Transform& scale(const Vector2f& factors, const Vector2f& center);

		static const Transform Identity;
	private:

		float mMatrix[16];
};

EE_API Transform operator *(const Transform& left, const Transform& right);

EE_API Transform& operator *=(Transform& left, const Transform& right);

EE_API Vector2f operator *(const Transform& left, const Vector2f& right);

EE_API bool operator ==(const Transform& left, const Transform& right);

EE_API bool operator !=(const Transform& left, const Transform& right);

}}

#endif
