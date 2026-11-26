#ifndef EE_UI_PRIMARYSELECTION_HPP
#define EE_UI_PRIMARYSELECTION_HPP

#include <eepp/core/small_function.hpp>
#include <eepp/core/string.hpp>

namespace EE { namespace UI {

class UIWidget;

/**
 * Coordinates PRIMARY selection across widgets on the UI thread. The latest accepted request
 * replaces any pending request and publishes after 100 ms of inactivity. Selection text is
 * collected at publication time into shared UTF-32 and UTF-8 buffers that retain their capacity.
 * Emulated PRIMARY belongs to the source until the selection is cleared or the source closes.
 */
class PrimarySelection {
  public:
	/**
	 * Schedule a deferred read of the selection. The writer receives the source owner and an
	 * empty UTF-32 buffer to fill. The widget and owner are borrowed and must remain valid while
	 * the request is pending. Cancel when clearing the selection or before destroying its source.
	 * @return True if the request was queued. Requests require the UI thread and an open widget
	 * attached to a scene.
	 */
	template <typename Owner, typename Writer>
	static bool schedule( UIWidget& widget, Owner& owner, Writer writer ) {
		return queue( widget, &owner, [source = &owner, writer] { writer( *source, sText ); } );
	}

	/** Cancel the owner's pending request and clear any emulated selection it published. */
	static void cancel( const void* owner );

	/** Publish the pending selection immediately. Passing nullptr flushes any owner. */
	static void flush( const void* owner = nullptr );

	/** Flush pending publication and read PRIMARY, with clipboard fallback on other backends. */
	static std::string getPasteText( UIWidget& widget );

  private:
	static bool queue( UIWidget& widget, const void* owner, SmallFunction<24> write );

	static void resetPending();

	static UIWidget* sWidget;
	static const void* sOwner;
	static Uint64 sSerial;
	static SmallFunction<24> sWrite;
	static String sText;
	static std::string sUtf8;
};

}} // namespace EE::UI

#endif
