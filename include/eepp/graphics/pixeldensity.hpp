#ifndef EE_PIXELDENSITY_HPP
#define EE_PIXELDENSITY_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/math/originpoint.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/math/size.hpp>
#include <eepp/math/vector2.hpp>
#include <string>

using namespace EE::Math;

namespace EE { namespace Window {

class Engine;
class Window;

}} // namespace EE::Window

namespace EE { namespace UI {

class UISceneNode;

}} // namespace EE::UI

namespace EE { namespace Graphics {

enum class PixelDensitySize : Uint32 {
	MDPI = 1,	//!< 1dp = 1px
	HDPI = 2,	//!< 1dp = 1.5px
	XHDPI = 3,	//!< 1dp = 2px
	XXHDPI = 4, //!< 1dp = 3px
	XXXHDPI = 5 //!< 1dp = 4px
};

class EE_API PixelDensity {
  public:
	/** Thread-local density scope. Nested scopes must be restored in reverse order; moving a
	 * guard transfers restoration to its new owner. No window or graphics binding is changed. */
	class EE_API Context {
	  public:
		~Context();

		Context( const Context& ) = delete;

		Context& operator=( const Context& ) = delete;

		Context( Context&& other ) noexcept;

		Context& operator=( Context&& ) = delete;

		/** Restores the previous scoped density, at most once. */
		void restore();

	  private:
		friend class PixelDensity;
		friend class EE::Window::Engine;

		explicit Context( Float density, const Float* source = nullptr, bool target = false );

		void restoreKeepingTarget( const Float* source );

		Context*& findIncomingLink();

		Context* mPrevious;
		const Float* mSource;
		Float mDensity;
		bool mActive{ true };
		bool mTargetBinding{ false };
	};

	/** Uses a positive finite density for this scope. Zero/invalid values select the process
	 * fallback, including inside another density scope. */
	static Context makeCurrent( Float density );

	/** Binds a live density source which must outlive the scope. Used by window/scene guards
	 * so a density change inside an input callback also updates its enclosing execution scope. */
	static Context bindCurrent( const Float* density );

	static Float toFloat( PixelDensitySize pd );

	static Float toFloat( Uint32 pd );

	static PixelDensitySize fromString( std::string str );

	static PixelDensitySize fromString( String str );

	static PixelDensitySize fromDPI( Float dpi );

	/** Effective density of the innermost scope, or the process fallback outside a scope. */
	static const Float& getPixelDensity();

	/** Positive EEPP_PIXEL_DENSITY value, or zero when unset or invalid. */
	static Float getEnvironmentPixelDensity();

	/** Sets the process fallback; does not replace an active scoped density. */
	static void setPixelDensity( const Float& pixelDensity );

	static void setPixelDensity( const PixelDensitySize& pixelDensity );

	static Float pxToDp( Float px );

	static Int32 pxToDpI( Float px );

	static Float dpToPx( Float dp );

	static Int32 dpToPxI( Float dp );

	static Sizei dpToPxI( Sizei size );

	static Sizei pxToDpI( Sizei size );

	static Rect dpToPxI( Rect size );

	static Rect pxToDpI( Rect size );

	static Rectf dpToPx( Rectf size );

	static Rectf pxToDp( Rectf size );

	static Sizef dpToPx( Sizef size );

	static Sizef pxToDp( Sizef size );

	static Sizei dpToPxI( Sizef size );

	static Sizei pxToDpI( Sizef size );

	static Vector2i dpToPxI( Vector2i pos );

	static Vector2i pxToDpI( Vector2i pos );

	static Vector2f dpToPx( Vector2f pos );

	/** Converts the coordinates, keeping the origin type and equations of @p point. */
	static OriginPoint dpToPx( OriginPoint point );

	static Vector2f pxToDp( Vector2f pos );

	static Float toDpFromString( const std::string& str );

	static Float toDpFromStringI( const std::string& str );

	static Float toPxFromString( const std::string& str );

	static Float toPxFromStringI( const std::string& str );

  protected:
	static Float sPixelDensity;

  private:
	friend class EE::Window::Window;
	friend class EE::UI::UISceneNode;

	static void snapshotBindings( const Float* source );
};

}} // namespace EE::Graphics

#endif
