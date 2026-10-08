#ifndef EE_UI_UISCROLLCONTROLLER_HPP
#define EE_UI_UISCROLLCONTROLLER_HPP

#include <array>
#include <eepp/math/vector2.hpp>
#include <eepp/system/time.hpp>

using namespace EE::Math;
using namespace EE::System;

namespace EE { namespace UI {

/**
 * @brief Allocation-free, frame-rate-independent scroll motion state.
 *
 * The controller operates in pixel coordinates but deliberately does not own a widget. This lets
 * virtualized views, translated child views, and text editors share the same motion without
 * forcing them into a common rendering hierarchy.
 */
class EE_API UIScrollController {
  public:
	UIScrollController();

	/** Synchronizes the controller with an externally changed scroll position and stops motion. */
	void setPosition( const Vector2f& position, const Vector2f& maxPosition );

	/** Adds a wheel displacement. Returns false when the target cannot move in either axis. */
	bool scrollBy( const Vector2f& delta, const Vector2f& currentPosition,
				   const Vector2f& maxPosition, bool smooth, Float durationScale = 1.f );

	/** Starts direct manipulation at the current scroll position. */
	void beginDrag( const Vector2f& position, const Vector2f& maxPosition );

	/** Applies a drag displacement and records a filtered release velocity. */
	bool dragBy( const Vector2f& delta, const Time& elapsed, const Vector2f& maxPosition );

	/** Transitions a direct manipulation into inertial motion. */
	void endDrag();

	/** Advances smooth or inertial motion. Returns true when the position changed. */
	bool update( const Time& elapsed, const Vector2f& maxPosition );

	/** Stops all pending motion at the current controller position. */
	void stop();

	bool isActive() const;

	bool isDragging() const;

	const Vector2f& getPosition() const;

	const Vector2f& getTarget() const;

	const Vector2f& getVelocity() const;

	Time getWheelScrollDuration() const;

	/** Sets the duration used to visually interpolate one wheel displacement. */
	void setWheelScrollDuration( const Time& duration );

	Vector2f getDragDeceleration() const;

	void setDragDeceleration( const Vector2f& deceleration );

  protected:
	enum class Mode : Uint8 { Idle, Smooth, Dragging, Momentum };
	struct SmoothTween {
		Vector2f delta;
		Float elapsed{ 0.f };
		Float duration{ 0.f };
	};

	Vector2f mPosition;
	Vector2f mTarget;
	Vector2f mVelocity;
	Vector2f mDragDeceleration{ 5.f, 5.f };
	std::array<SmoothTween, 32> mSmoothTweens;
	Time mWheelScrollDuration{ Milliseconds( 100 ) };
	Uint32 mSmoothTweenCount{ 0 };
	Mode mMode{ Mode::Idle };
};

}} // namespace EE::UI

#endif // EE_UI_UISCROLLCONTROLLER_HPP
