#ifndef ECODE_NOTIFICATIONCENTER_HPP
#define ECODE_NOTIFICATIONCENTER_HPP

#include "plugins/pluginmanager.hpp"

namespace ecode {

class NotificationCenter {
  public:
	static NotificationCenter* instance();

	NotificationCenter( UILayout* layout, PluginManager* pluginManager );

	void addNotification( const String& text, const Time& delay = Seconds( 2.5 ),
						  bool allowCopy = false );

	void addShowRequest( const String& uri, const String& actionText,
						 const Time& delay = Seconds( 2.5 ) );

	void addInteractiveNotification( String text, String actionText,
									 std::function<void()> onInteraction,
									 const Time& delay = Seconds( 7.5 ), bool allowCopy = false );

  protected:
	UILayout* mLayout{ nullptr };
	PluginManager* mPluginManager{ nullptr };
};

} // namespace ecode

#endif // ECODE_NOTIFICATIONCENTER_HPP
