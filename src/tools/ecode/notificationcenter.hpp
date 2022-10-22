#ifndef ECODE_NOTIFICATIONCENTER_HPP
#define ECODE_NOTIFICATIONCENTER_HPP

#include <eepp/ee.hpp>

namespace ecode {

class NotificationCenter {
  public:
	NotificationCenter( UILayout* layout );

	UITextView* addNotification( const String& text );

  protected:
	UILayout* mLayout{ nullptr };
};

} // namespace ecode

#endif // ECODE_NOTIFICATIONCENTER_HPP
