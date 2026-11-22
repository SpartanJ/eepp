#ifndef EE_UI_UIDENSITYREFLOW_HPP
#define EE_UI_UIDENSITYREFLOW_HPP

#include <eepp/config.hpp>
#include <eepp/core/noncopyable.hpp>
#include <eepp/core/small_unique_function.hpp>

namespace EE { namespace UI {

class UINode;
class UISceneNode;

/** Context for capturing and restoring a widget's state during a scene density reflow. */
class EE_API UIDensityReflow : NonCopyable {
  public:
	Float getOldDensity() const { return mOldDensity; }
	Float getNewDensity() const { return mNewDensity; }

	/** Checks whether the widget still belongs to this reflow after a call that may emit events. */
	bool isWidgetCurrent() const;

	/** Registers a one-shot callback owned by the widget being captured. The callback runs after
	 * layout and may accept a const UIDensityReflow& to check whether the widget is still current.
	 * Use the context only within the capture hook or the restore callback. */
	template <typename Callback> void restoreAfterLayout( Callback&& callback ) {
		if constexpr ( std::is_invocable_v<Callback&, const UIDensityReflow&> ) {
			queueRestore( std::forward<Callback>( callback ) );
		} else {
			queueRestore( [callback = std::forward<Callback>( callback )](
							  const UIDensityReflow& ) mutable { callback(); } );
		}
	}

  private:
	friend class UISceneNode;
	using RestoreCallback = SmallUniqueFunction<void( const UIDensityReflow& ), 32>;

	UIDensityReflow( UISceneNode* scene, UINode* const& widget, Float oldDensity,
					 Float newDensity ) :
		mScene( scene ), mWidget( widget ), mOldDensity( oldDensity ), mNewDensity( newDensity ) {}

	void queueRestore( RestoreCallback callback );

	UISceneNode* mScene;
	UINode* const& mWidget;
	Float mOldDensity;
	Float mNewDensity;
};

}} // namespace EE::UI

#endif
