// SPDX-License-Identifier: AGPL-3.0-only

// The first run on a phone's system: a window, a word in it, and a line in the
// log that says the window came up. It is not the program yet -- it is the
// smallest thing the simulator can be asked whether it runs.
//
// The word is made by the standard library, in the file beside this one: the
// question this asks is whether `import std` arrives on a phone at all. What
// crosses back here is a C string and a count.

#import <UIKit/UIKit.h>

// The word the window shows, made on the other side of this line.
extern const char *pchat_word(void);
extern unsigned long pchat_word_size(void);

static NSString *const mark = @"PCHAT-IOS-FIRST-RUN";

@interface SceneDelegate : UIResponder <UIWindowSceneDelegate>
@property(nonatomic, strong) UIWindow *window;
@end

@implementation SceneDelegate

- (void)scene:(UIScene *)scene
    willConnectToSession:(UISceneSession *)session
                 options:(UISceneConnectionOptions *)options {
  UIWindowScene *windowScene = (UIWindowScene *)scene;
  self.window = [[UIWindow alloc] initWithWindowScene:windowScene];

  UIViewController *view = [UIViewController new];
  view.view.backgroundColor = UIColor.blackColor;

  UILabel *word = [UILabel new];
  word.text = [NSString stringWithUTF8String:pchat_word()];
  word.textColor = UIColor.whiteColor;
  word.font = [UIFont systemFontOfSize:28 weight:UIFontWeightMedium];
  word.textAlignment = NSTextAlignmentCenter;
  word.translatesAutoresizingMaskIntoConstraints = NO;
  [view.view addSubview:word];
  [NSLayoutConstraint activateConstraints:@[
    [word.centerXAnchor constraintEqualToAnchor:view.view.centerXAnchor],
    [word.centerYAnchor constraintEqualToAnchor:view.view.centerYAnchor],
  ]];

  self.window.rootViewController = view;
  [self.window makeKeyAndVisible];
  NSLog(@"%@: the window is up, and std counted %lu letters", mark,
        pchat_word_size());
}

@end

@interface AppDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation AppDelegate

- (UISceneConfiguration *)application:(UIApplication *)application
    configurationForConnectingSceneSession:(UISceneSession *)session
                                   options:(UISceneConnectionOptions *)options {
  UISceneConfiguration *configuration =
      [[UISceneConfiguration alloc] initWithName:@"Default" sessionRole:session.role];
  configuration.delegateClass = SceneDelegate.class;
  return configuration;
}

@end

int main(int argc, char *argv[]) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass(AppDelegate.class));
  }
}
