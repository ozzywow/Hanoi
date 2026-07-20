#include "Clipboard.h"
#include "platform/CCPlatformConfig.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)

#import <TargetConditionals.h>
#if TARGET_OS_IOS
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

void Clipboard::copy(const std::string& text)
{
	NSString* s = [NSString stringWithUTF8String:text.c_str()];
	if (!s) return;
#if TARGET_OS_IOS
	[UIPasteboard generalPasteboard].string = s;
#else
	NSPasteboard* pb = [NSPasteboard generalPasteboard];
	[pb clearContents];
	[pb setString:s forType:NSPasteboardTypeString];
#endif
}

#endif
