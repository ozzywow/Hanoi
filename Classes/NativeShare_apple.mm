#include "NativeShare.h"
#include "platform/CCPlatformConfig.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)

#import <TargetConditionals.h>

#if TARGET_OS_IOS
#import <UIKit/UIKit.h>

bool NativeShare::isSupported() { return true; }

void NativeShare::share(const std::string& text)
{
	NSString* s = [NSString stringWithUTF8String:text.c_str()];
	if (!s) return;
	dispatch_async(dispatch_get_main_queue(), ^{
		UIViewController* root = [UIApplication sharedApplication].keyWindow.rootViewController;
		while (root.presentedViewController) root = root.presentedViewController;
		if (!root) return;

		// 순수 URL이면 NSString이 아니라 NSURL로 넘긴다.
		//  - 공유 시트의 "복사"가 문자열이 아닌 URL로 클립보드에 올려 붙여넣기가 깨지지 않음
		//  - Safari / 읽기목록 등 URL 전용 액티비티가 목록에 노출되고 링크 미리보기가 붙음
		id item = s;
		if ([s hasPrefix:@"http"] && [s rangeOfString:@" "].location == NSNotFound) {
			NSURL* u = [NSURL URLWithString:s];
			if (u) item = u;
		}

		UIActivityViewController* av =
			[[UIActivityViewController alloc] initWithActivityItems:@[item] applicationActivities:nil];

		// iPad: 공유 시트는 popover라 소스 지정 필요(없으면 크래시)
		if (av.popoverPresentationController) {
			av.popoverPresentationController.sourceView = root.view;
			av.popoverPresentationController.sourceRect =
				CGRectMake(root.view.bounds.size.width / 2, root.view.bounds.size.height / 2, 1, 1);
			av.popoverPresentationController.permittedArrowDirections = 0;
		}
		[root presentViewController:av animated:YES completion:nil];
	});
}

void NativeShare::share(const std::string& text, const std::string& url)
{
	NSString* t = text.empty() ? nil : [NSString stringWithUTF8String:text.c_str()];
	NSString* u = [NSString stringWithUTF8String:url.c_str()];
	if (!u) { if (t) share(text); return; }

	NSURL* nsurl = [NSURL URLWithString:u];
	if (!nsurl) {                       // URL 파싱 실패 → 합쳐서 문자열로라도 내보낸다
		share(text.empty() ? url : (text + "\n" + url));
		return;
	}

	dispatch_async(dispatch_get_main_queue(), ^{
		UIViewController* root = [UIApplication sharedApplication].keyWindow.rootViewController;
		while (root.presentedViewController) root = root.presentedViewController;
		if (!root) return;

		// 문구(NSString)와 링크(NSURL)를 분리해 넘기는 것이 핵심.
		// 메신저는 둘을 합쳐 보여 주고, "복사"·Safari 등 URL 전용 액티비티는 NSURL만 집어간다
		// → 주소창 붙여넣기가 문구로 오염되지 않는다.
		NSArray* items = t ? @[t, nsurl] : @[nsurl];

		UIActivityViewController* av =
			[[UIActivityViewController alloc] initWithActivityItems:items applicationActivities:nil];

		// iPad: 공유 시트는 popover라 소스 지정 필요(없으면 크래시)
		if (av.popoverPresentationController) {
			av.popoverPresentationController.sourceView = root.view;
			av.popoverPresentationController.sourceRect =
				CGRectMake(root.view.bounds.size.width / 2, root.view.bounds.size.height / 2, 1, 1);
			av.popoverPresentationController.permittedArrowDirections = 0;
		}
		[root presentViewController:av animated:YES completion:nil];
	});
}

#else
// ── Mac 데스크톱: 공유 시트 미사용 → 클립보드 복사로 폴백 ──
bool NativeShare::isSupported() { return false; }
void NativeShare::share(const std::string&) {}
void NativeShare::share(const std::string&, const std::string&) {}
#endif

#endif
