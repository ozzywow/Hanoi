#import "HanoiAppDelegate.h"
#import "cocos2d.h"
#import "AppDelegate.h"
#import "RootViewController.h"
#import "MKStoreManager.h"

@implementation HanoiAppDelegate

// cocos2d-x app instance
static AppDelegate s_sharedApplication;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {

    cocos2d::Application *app = cocos2d::Application::getInstance();
    app->initGLContextAttrs();
    cocos2d::GLViewImpl::convertAttrs();

    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];

    // createWithFullScreen creates CCEAGLView internally with proper OpenGL context
    cocos2d::GLViewImpl *glviewImpl = (cocos2d::GLViewImpl*)cocos2d::GLViewImpl::createWithFullScreen("HanoiOlympic");
    cocos2d::Director::getInstance()->setOpenGLView(glviewImpl);

    // Use the created CCEAGLView as the root view controller's view
    UIView *eaglView = (__bridge UIView*)glviewImpl->getEAGLView();
    viewController = [[RootViewController alloc] init];
    viewController.view = eaglView;

    [self.window setRootViewController:viewController];
    [self.window makeKeyAndVisible];

    [MKStoreManager sharedManager];
#ifdef AD_VER
    if ([MKStoreManager isFeaturePurchased:kProductId1]) {
        // IAP feature already purchased
    }
#endif

    app->run();

    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {}

- (void)applicationDidBecomeActive:(UIApplication *)application {}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    cocos2d::Application::getInstance()->applicationDidEnterBackground();
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    cocos2d::Application::getInstance()->applicationWillEnterForeground();
}

- (void)applicationWillTerminate:(UIApplication *)application {}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application {}

@end
