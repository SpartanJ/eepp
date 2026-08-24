#define Rect AppleRect
#import <AppKit/AppKit.h>
#undef Rect
#undef BSD

#include <eepp/system/log.hpp>
#include <eepp/ui/platformmenubar.hpp>
#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uimenubar.hpp>
#include <eepp/ui/uimenucheckbox.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <eepp/ui/uimenuradiobutton.hpp>
#include <eepp/ui/uimenuseparator.hpp>
#include <eepp/ui/uimenusubmenu.hpp>
#include <eepp/window/keycodes.hpp>
#include <unordered_map>

using namespace EE;
using namespace EE::UI;
using namespace EE::Window;

#if __has_feature( objc_arc )
#define EE_OBJC_RELEASE( object )
#else
#define EE_OBJC_RELEASE( object ) [object release]
#endif

namespace {

NSString* toNSString( const String& string ) {
	const std::string utf8( string.toUtf8() );
	return [NSString stringWithUTF8String:utf8.c_str()];
}

NSString* applicationName() {
	NSBundle* bundle = [NSBundle mainBundle];
	NSString* name = [bundle objectForInfoDictionaryKey:@"CFBundleDisplayName"];
	if ( nil == name || 0 == name.length )
		name = [bundle objectForInfoDictionaryKey:@"CFBundleName"];
	if ( nil == name || 0 == name.length )
		name = [[NSProcessInfo processInfo] processName];
	return nil != name && name.length > 0 ? name : @"Application";
}

NSString* keyEquivalent( Keycode key ) {
	if ( key >= KEY_F1 && key <= KEY_F12 ) {
		unichar character = NSF1FunctionKey + ( key - KEY_F1 );
		return [NSString stringWithCharacters:&character length:1];
	}
	if ( key >= KEY_F13 && key <= KEY_F24 ) {
		unichar character = NSF13FunctionKey + ( key - KEY_F13 );
		return [NSString stringWithCharacters:&character length:1];
	}

	unichar character = 0;
	switch ( key ) {
		case KEY_RETURN:
		case KEY_KP_ENTER:
			character = NSCarriageReturnCharacter;
			break;
		case KEY_ESCAPE:
			character = 0x1B;
			break;
		case KEY_TAB:
			character = NSTabCharacter;
			break;
		case KEY_BACKSPACE:
			character = NSBackspaceCharacter;
			break;
		case KEY_DELETE:
			character = NSDeleteFunctionKey;
			break;
		case KEY_INSERT:
			character = NSInsertFunctionKey;
			break;
		case KEY_HOME:
			character = NSHomeFunctionKey;
			break;
		case KEY_END:
			character = NSEndFunctionKey;
			break;
		case KEY_PAGEUP:
			character = NSPageUpFunctionKey;
			break;
		case KEY_PAGEDOWN:
			character = NSPageDownFunctionKey;
			break;
		case KEY_LEFT:
			character = NSLeftArrowFunctionKey;
			break;
		case KEY_RIGHT:
			character = NSRightArrowFunctionKey;
			break;
		case KEY_UP:
			character = NSUpArrowFunctionKey;
			break;
		case KEY_DOWN:
			character = NSDownArrowFunctionKey;
			break;
		default:
			if ( key > 0 && key < 0x80 )
				character = static_cast<unichar>( key );
			break;
	}

	if ( 0 == character )
		return @"";
	character = [[[NSString stringWithCharacters:&character
										  length:1] lowercaseString] characterAtIndex:0];
	return [NSString stringWithCharacters:&character length:1];
}

NSEventModifierFlags modifierMask( Uint32 modifiers ) {
	NSEventModifierFlags mask = 0;
	if ( modifiers & KEYMOD_META )
		mask |= NSEventModifierFlagCommand;
	if ( modifiers & KEYMOD_ALT )
		mask |= NSEventModifierFlagOption;
	if ( modifiers & KEYMOD_CTRL )
		mask |= NSEventModifierFlagControl;
	if ( modifiers & KEYMOD_SHIFT )
		mask |= NSEventModifierFlagShift;
	return mask;
}

struct NativeMenuSource {
	UIMenu* menu;
	UIMenuSubMenu* owner;
};

} // namespace

@interface EEPPMenuBarBridge : NSObject <NSMenuDelegate> {
  @private
	UIMenuBar* _menuBar;
	NSMenu* _installedMainMenu;
	NSMenu* _previousMainMenu;
	NSMenu* _servicesMenu;
	NSMenu* _windowsMenu;
	NSMenu* _helpMenu;
	BOOL _capturedPreviousMainMenu;
	std::unordered_map<NSMenu*, NativeMenuSource> _menuMap;
	std::unordered_map<NSMenuItem*, UIMenuItem*> _itemMap;
}

- (instancetype)initWithMenuBar:(UIMenuBar*)menuBar;
- (void)uninstall;
- (void)syncTopLevel;
- (void)menuItemActivated:(NSMenuItem*)sender;

@end

@implementation EEPPMenuBarBridge

- (instancetype)initWithMenuBar:(UIMenuBar*)menuBar {
	self = [super init];
	if ( nil != self )
		_menuBar = menuBar;
	return self;
}

- (void)dealloc {
	[self uninstall];
#if !__has_feature( objc_arc )
	[super dealloc];
#endif
}

- (UIMenuItem*)findItemWithRole:(MenuRole)role inMenu:(UIMenu*)menu {
	if ( nullptr == menu )
		return nullptr;
	for ( Uint32 i = 0; i < menu->getCount(); ++i ) {
		UIWidget* widget = menu->getItem( i );
		if ( widget->isType( UI_TYPE_MENUITEM ) ) {
			UIMenuItem* item = widget->asType<UIMenuItem>();
			if ( item->getMenuRole() == role )
				return item;
		}
		if ( widget->isType( UI_TYPE_MENUSUBMENU ) ) {
			UIMenuItem* found =
				[self findItemWithRole:role inMenu:widget->asType<UIMenuSubMenu>()->getSubMenu()];
			if ( nullptr != found )
				return found;
		}
	}
	return nullptr;
}

- (UIMenuItem*)findItemWithRole:(MenuRole)role {
	if ( nullptr == _menuBar )
		return nullptr;
	for ( Uint32 i = 0; i < _menuBar->getButtonsCount(); ++i ) {
		UIMenuItem* item = [self findItemWithRole:role inMenu:_menuBar->getPopUpMenu( i )];
		if ( nullptr != item )
			return item;
	}
	return nullptr;
}

- (NSMenuItem*)mirroredItemWithTitle:(NSString*)title source:(UIMenuItem*)source {
	const KeyBindings::Shortcut& shortcut = source->getShortcut();
	NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title
												  action:@selector( menuItemActivated: )
										   keyEquivalent:keyEquivalent( shortcut.key )];
	[item setTarget:self];
	[item setEnabled:source->isEnabled()];
	[item setKeyEquivalentModifierMask:modifierMask( shortcut.mod )];
	_itemMap[item] = source;
	return item;
}

- (void)addRoleItem:(UIMenuItem*)source
			  title:(NSString*)title
			 toMenu:(NSMenu*)menu
	  defaultAction:(SEL)defaultAction {
	NSMenuItem* item;
	if ( nullptr != source ) {
		item = [self mirroredItemWithTitle:title source:source];
		if ( source->getMenuRole() == MenuRole::Quit ) {
			[item setKeyEquivalent:@"q"];
			[item setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
		}
	} else {
		item = [[NSMenuItem alloc] initWithTitle:title action:defaultAction keyEquivalent:@""];
		[item setTarget:NSApp];
	}
	[menu addItem:item];
	EE_OBJC_RELEASE( item );
}

- (void)buildApplicationMenuInMainMenu:(NSMenu*)mainMenu {
	NSString* appName = applicationName();
	NSMenu* appMenu = [[NSMenu alloc] initWithTitle:appName];
	[appMenu setAutoenablesItems:NO];

	NSString* title = [@"About " stringByAppendingString:appName];
	[self addRoleItem:[self findItemWithRole:MenuRole::About]
				title:title
			   toMenu:appMenu
		defaultAction:@selector( orderFrontStandardAboutPanel: )];

	UIMenuItem* preferences = [self findItemWithRole:MenuRole::Preferences];
	if ( nullptr != preferences ) {
		[appMenu addItem:[NSMenuItem separatorItem]];
		[self addRoleItem:preferences title:@"Settings…" toMenu:appMenu defaultAction:nil];
	}

	[appMenu addItem:[NSMenuItem separatorItem]];
	_servicesMenu = [[NSMenu alloc] initWithTitle:@"Services"];
	NSMenuItem* servicesItem = [[NSMenuItem alloc] initWithTitle:@"Services"
														  action:nil
												   keyEquivalent:@""];
	[servicesItem setSubmenu:_servicesMenu];
	[appMenu addItem:servicesItem];
	[NSApp setServicesMenu:_servicesMenu];
	EE_OBJC_RELEASE( servicesItem );
	EE_OBJC_RELEASE( _servicesMenu );

	[appMenu addItem:[NSMenuItem separatorItem]];
	title = [@"Hide " stringByAppendingString:appName];
	NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title
												  action:@selector( hide: )
										   keyEquivalent:@"h"];
	[item setTarget:NSApp];
	[appMenu addItem:item];
	EE_OBJC_RELEASE( item );

	item = [[NSMenuItem alloc] initWithTitle:@"Hide Others"
									  action:@selector( hideOtherApplications: )
							   keyEquivalent:@"h"];
	[item setTarget:NSApp];
	[item setKeyEquivalentModifierMask:NSEventModifierFlagCommand | NSEventModifierFlagOption];
	[appMenu addItem:item];
	EE_OBJC_RELEASE( item );

	item = [[NSMenuItem alloc] initWithTitle:@"Show All"
									  action:@selector( unhideAllApplications: )
							   keyEquivalent:@""];
	[item setTarget:NSApp];
	[appMenu addItem:item];
	EE_OBJC_RELEASE( item );

	[appMenu addItem:[NSMenuItem separatorItem]];
	title = [@"Quit " stringByAppendingString:appName];
	UIMenuItem* quit = [self findItemWithRole:MenuRole::Quit];
	if ( nullptr != quit ) {
		[self addRoleItem:quit title:title toMenu:appMenu defaultAction:nil];
	} else {
		item = [[NSMenuItem alloc] initWithTitle:title
										  action:@selector( terminate: )
								   keyEquivalent:@"q"];
		[item setTarget:NSApp];
		[appMenu addItem:item];
		EE_OBJC_RELEASE( item );
	}

	NSMenuItem* appMenuItem = [[NSMenuItem alloc] initWithTitle:appName
														 action:nil
												  keyEquivalent:@""];
	[appMenuItem setSubmenu:appMenu];
	[mainMenu addItem:appMenuItem];
	EE_OBJC_RELEASE( appMenuItem );
	EE_OBJC_RELEASE( appMenu );
}

- (void)removeMappingsForMenu:(NSMenu*)menu removeMenu:(BOOL)removeMenu {
	for ( NSMenuItem* item in [menu itemArray] ) {
		_itemMap.erase( item );
		if ( nil != item.submenu )
			[self removeMappingsForMenu:item.submenu removeMenu:YES];
	}
	if ( removeMenu )
		_menuMap.erase( menu );
}

- (NSMenuItem*)createNativeItem:(UIWidget*)widget {
	if ( widget->isType( UI_TYPE_MENU_SEPARATOR ) )
		return [NSMenuItem separatorItem];
	if ( !widget->isType( UI_TYPE_MENUITEM ) ) {
		Log::warning( "Global menu bar: unsupported widget type %u skipped", widget->getType() );
		return nil;
	}

	UIMenuItem* source = widget->asType<UIMenuItem>();
	if ( source->getMenuRole() != MenuRole::NoRole )
		return nil;

	NSMenuItem* item;
	if ( widget->isType( UI_TYPE_MENUSUBMENU ) ) {
		item = [[NSMenuItem alloc] initWithTitle:toNSString( source->getText() )
										  action:nil
								   keyEquivalent:@""];
		[item setEnabled:source->isEnabled()];
		UIMenu* eeSubMenu = widget->asType<UIMenuSubMenu>()->getSubMenu();
		if ( nullptr != eeSubMenu ) {
			NSMenu* subMenu = [[NSMenu alloc] initWithTitle:toNSString( source->getText() )];
			[subMenu setAutoenablesItems:NO];
			[subMenu setDelegate:self];
			[item setSubmenu:subMenu];
			_menuMap[subMenu] = { eeSubMenu, widget->asType<UIMenuSubMenu>() };
			EE_OBJC_RELEASE( subMenu );
		}
	} else {
		item = [self mirroredItemWithTitle:toNSString( source->getText() ) source:source];
		if ( widget->isType( UI_TYPE_MENUCHECKBOX ) ) {
			[item setState:widget->asType<UIMenuCheckBox>()->isActive() ? NSControlStateValueOn
																		: NSControlStateValueOff];
		} else if ( widget->isType( UI_TYPE_MENURADIOBUTTON ) ) {
			[item setState:widget->asType<UIMenuRadioButton>()->isActive()
							   ? NSControlStateValueOn
							   : NSControlStateValueOff];
		}
	}
	return item;
}

- (void)rebuildMenu:(NSMenu*)nativeMenu fromMenu:(UIMenu*)eeMenu {
	[self removeMappingsForMenu:nativeMenu removeMenu:NO];
	[nativeMenu removeAllItems];
	bool lastWasSeparator = true;
	for ( Uint32 i = 0; i < eeMenu->getCount(); ++i ) {
		UIWidget* widget = eeMenu->getItem( i );
		if ( !widget->isVisible() )
			continue;
		const bool isSeparator = widget->isType( UI_TYPE_MENU_SEPARATOR );
		if ( isSeparator && lastWasSeparator )
			continue;
		NSMenuItem* item = [self createNativeItem:widget];
		if ( nil == item )
			continue;
		[nativeMenu addItem:item];
		lastWasSeparator = isSeparator;
		if ( !isSeparator )
			EE_OBJC_RELEASE( item );
	}
	if ( lastWasSeparator && [nativeMenu numberOfItems] > 0 )
		[nativeMenu removeItemAtIndex:[nativeMenu numberOfItems] - 1];
}

- (void)syncTopLevel {
	eeASSERT( [NSThread isMainThread] );
	if ( nullptr == _menuBar )
		return;

	if ( nil == NSApp )
		[NSApplication sharedApplication];
	if ( !_capturedPreviousMainMenu ) {
		_previousMainMenu = [NSApp mainMenu];
#if !__has_feature( objc_arc )
		[_previousMainMenu retain];
#endif
		_capturedPreviousMainMenu = YES;
	}

	if ( nil != _installedMainMenu ) {
		[self removeMappingsForMenu:_installedMainMenu removeMenu:YES];
		_itemMap.clear();
		_menuMap.clear();
		EE_OBJC_RELEASE( _installedMainMenu );
	}
	_installedMainMenu = [[NSMenu alloc] initWithTitle:@""];
	[_installedMainMenu setAutoenablesItems:NO];
	[self buildApplicationMenuInMainMenu:_installedMainMenu];

	_windowsMenu = nil;
	_helpMenu = nil;
	for ( Uint32 i = 0; i < _menuBar->getButtonsCount(); ++i ) {
		UIPopUpMenu* eeMenu = _menuBar->getPopUpMenu( i );
		if ( nullptr == eeMenu )
			continue;
		NSString* title = toNSString( _menuBar->getButton( i )->getText() );
		NSMenu* nativeMenu = [[NSMenu alloc] initWithTitle:title];
		[nativeMenu setAutoenablesItems:NO];
		[nativeMenu setDelegate:self];
		_menuMap[nativeMenu] = { eeMenu, nullptr };

		NSMenuItem* topLevelItem = [[NSMenuItem alloc] initWithTitle:title
															  action:nil
													   keyEquivalent:@""];
		[topLevelItem setSubmenu:nativeMenu];
		[_installedMainMenu addItem:topLevelItem];

		if ( eeMenu->getMenuBarRole() == MenuBarRole::Window )
			_windowsMenu = nativeMenu;
		else if ( eeMenu->getMenuBarRole() == MenuBarRole::Help )
			_helpMenu = nativeMenu;

		EE_OBJC_RELEASE( topLevelItem );
		EE_OBJC_RELEASE( nativeMenu );
	}

	[NSApp setMainMenu:_installedMainMenu];
	[NSApp setWindowsMenu:_windowsMenu];
	[NSApp setHelpMenu:_helpMenu];
}

- (void)uninstall {
	eeASSERT( [NSThread isMainThread] );
	if ( nullptr == _menuBar && nil == _installedMainMenu )
		return;

	_menuBar = nullptr;
	if ( [NSApp servicesMenu] == _servicesMenu )
		[NSApp setServicesMenu:nil];
	if ( [NSApp windowsMenu] == _windowsMenu )
		[NSApp setWindowsMenu:nil];
	if ( [NSApp helpMenu] == _helpMenu )
		[NSApp setHelpMenu:nil];
	if ( [NSApp mainMenu] == _installedMainMenu )
		[NSApp setMainMenu:_previousMainMenu];

	if ( nil != _installedMainMenu )
		[self removeMappingsForMenu:_installedMainMenu removeMenu:YES];
	_itemMap.clear();
	_menuMap.clear();
	EE_OBJC_RELEASE( _installedMainMenu );
	EE_OBJC_RELEASE( _previousMainMenu );
	_installedMainMenu = nil;
	_previousMainMenu = nil;
	_servicesMenu = nil;
	_windowsMenu = nil;
	_helpMenu = nil;
	_capturedPreviousMainMenu = NO;
}

- (void)menuNeedsUpdate:(NSMenu*)menu {
	eeASSERT( [NSThread isMainThread] );
	auto found = _menuMap.find( menu );
	if ( found == _menuMap.end() || nullptr == found->second.menu )
		return;
	NativeMenuSource source = found->second;
	if ( nullptr != source.owner ) {
		source.owner->notifySubMenuWillShow();
		found = _menuMap.find( menu );
		if ( found == _menuMap.end() || found->second.owner != source.owner )
			return;
		source.menu = source.owner->getSubMenu();
		if ( nullptr == source.menu )
			return;
		found->second.menu = source.menu;
	}
	UIMenu* eeMenu = source.menu;
	eeMenu->notifyMenuWillShow();
	found = _menuMap.find( menu );
	if ( found != _menuMap.end() && found->second.menu == eeMenu &&
		 found->second.owner == source.owner )
		[self rebuildMenu:menu fromMenu:eeMenu];
}

- (void)menuDidClose:(NSMenu*)menu {
	auto found = _menuMap.find( menu );
	if ( found != _menuMap.end() && nullptr != found->second.menu )
		found->second.menu->notifyMenuDidHide();
}

- (void)menuItemActivated:(NSMenuItem*)sender {
	eeASSERT( [NSThread isMainThread] );
	auto found = _itemMap.find( sender );
	if ( found != _itemMap.end() && nullptr != found->second )
		found->second->activate();
}

@end

namespace EE { namespace UI {

class MacOSMenuBar final : public PlatformMenuBar {
  public:
	~MacOSMenuBar() { uninstall(); }

	void install( UIMenuBar* menuBar ) {
		eeASSERT( [NSThread isMainThread] );
		uninstall();
		mBridge = [[EEPPMenuBarBridge alloc] initWithMenuBar:menuBar];
		[mBridge syncTopLevel];
	}

	void uninstall() {
		if ( nil == mBridge )
			return;
		[mBridge uninstall];
		EE_OBJC_RELEASE( mBridge );
		mBridge = nil;
	}

	void syncTopLevel() {
		if ( nil != mBridge )
			[mBridge syncTopLevel];
	}

  private:
	EEPPMenuBarBridge* mBridge{ nil };
};

std::unique_ptr<PlatformMenuBar> createMacOSPlatformMenuBar() {
	return std::make_unique<MacOSMenuBar>();
}

}} // namespace EE::UI
