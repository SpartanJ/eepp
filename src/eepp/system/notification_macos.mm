#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOS
#include "notification_macos.hpp"

#import <AppKit/AppKit.h>
#import <UserNotifications/UserNotifications.h>
#include <memory>

@interface EEPPNotificationDelegate : NSObject <UNUserNotificationCenterDelegate>
@end

@implementation EEPPNotificationDelegate
- (void)userNotificationCenter:(UNUserNotificationCenter*)center
	   willPresentNotification:(UNNotification*)notification
		 withCompletionHandler:(void ( ^)( UNNotificationPresentationOptions ))completionHandler {
	(void)center;
	(void)notification;
	if ( @available( macOS 11.0, * ) )
		completionHandler( UNNotificationPresentationOptionBanner |
						   UNNotificationPresentationOptionList );
	else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		// macOS 10.14–10.15 use Alert; Banner and List were introduced in 11.0.
		completionHandler( UNNotificationPresentationOptionAlert );
#pragma clang diagnostic pop
	}
}
@end

namespace EE::System::Private {

namespace {

/** Shared with completion blocks so a late reply after timeout never touches a dead stack. */
struct Submission {
	dispatch_semaphore_t ready{ dispatch_semaphore_create( 0 ) };
	std::string error;
	bool success{ false };

	~Submission() { dispatch_release( ready ); }
};

bool waitForSubmission( const std::shared_ptr<Submission>& state, int seconds,
						std::string* error ) {
	if ( dispatch_semaphore_wait(
			 state->ready, dispatch_time( DISPATCH_TIME_NOW, seconds * NSEC_PER_SEC ) ) != 0 ) {
		if ( error )
			*error = "Notification request timed out. Respond to the macOS permission prompt, then "
					 "try again.";
		return false;
	}
	if ( error )
		*error = state->error;
	return state->success;
}

} // namespace

bool NotificationMacOS::isSupported() {
	@autoreleasepool {
		if ( @available( macOS 10.14, * ) ) {
			NSBundle* bundle = [NSBundle mainBundle];
			return bundle.bundleIdentifier.length > 0 &&
				   [bundle.bundleURL.pathExtension caseInsensitiveCompare:@"app"] == NSOrderedSame;
		}
		return false;
	}
}

bool NotificationMacOS::show( const std::string& title, const std::string& message,
							  std::string* error ) {
	@autoreleasepool {
		if ( !isSupported() ) {
			if ( error )
				*error = "macOS notifications require macOS 10.14 or later and launching the "
						 "signed .app bundle, rather than the standalone executable.";
			return false;
		}
		if ( [NSThread isMainThread] ) {
			if ( error )
				*error = "Call Notification::show() on a background thread so macOS can process "
						 "authorization and delivery.";
			return false;
		}
		if ( @available( macOS 10.14, * ) ) {
			NSString* nativeTitle = [NSString stringWithUTF8String:title.c_str()];
			NSString* nativeMessage = [NSString stringWithUTF8String:message.c_str()];
			if ( !nativeTitle || !nativeMessage ) {
				if ( error )
					*error = "Notification text is not valid UTF-8";
				return false;
			}
			UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
			// Keep the delegate alive for the process. Preserve an embedding app's own delegate.
			static EEPPNotificationDelegate* delegate = nil;
			static dispatch_once_t once;
			dispatch_once( &once, ^{
			  delegate = [[EEPPNotificationDelegate alloc] init];
			} );
			if ( !center.delegate )
				center.delegate = delegate;

			auto authorization = std::make_shared<Submission>();
			[center requestAuthorizationWithOptions:UNAuthorizationOptionAlert
								  completionHandler:^( BOOL granted, NSError* failure ) {
									authorization->success = granted && !failure;
									if ( failure )
										authorization->error =
											failure.localizedDescription.UTF8String;
									else if ( !granted )
										authorization->error =
											"Notifications are disabled for this app. Enable them "
											"in System Settings > Notifications.";
									dispatch_semaphore_signal( authorization->ready );
								  }];
			// Human authorization is not a helper-process timeout. Give the user time to respond.
			if ( !waitForSubmission( authorization, 60, error ) )
				return false;

			UNMutableNotificationContent* content = [[UNMutableNotificationContent alloc] init];
			content.title = nativeTitle;
			content.body = nativeMessage;
			content.sound = nil; // eterm's configurable sound channel is independent.
			UNNotificationRequest* request =
				[UNNotificationRequest requestWithIdentifier:[NSUUID UUID].UUIDString
													 content:content
													 trigger:nil];
			[content release];
			auto submission = std::make_shared<Submission>();
			[center addNotificationRequest:request
					 withCompletionHandler:^( NSError* failure ) {
					   submission->success = failure == nil;
					   if ( failure )
						   submission->error = failure.localizedDescription.UTF8String;
					   dispatch_semaphore_signal( submission->ready );
					 }];
			return waitForSubmission( submission, 5, error );
		}
		return false;
	}
}

} // namespace EE::System::Private
#endif
