#ifndef NOTIFICATIONCENTER_HPP
#define NOTIFICATIONCENTER_HPP

#include <eepp/ee.hpp>

class NotificationCenter {
  public:
	NotificationCenter( UILayout* layout );

	UITextView* addNotification( const String& text );

  protected:
	UILayout* mLayout{ nullptr };
};

#endif // NOTIFICATIONCENTER_HPP
