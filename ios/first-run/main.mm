// SPDX-License-Identifier: AGPL-3.0-only

// The first run on a phone's system: a window, a word in it, and a line in the
// log that says the window came up. It is not the program yet -- it is the
// smallest thing the simulator can be asked whether it runs.
//
// The word is made by the standard library, not by a literal, because the
// question this asks is whether `import std` arrives on a phone at all. The
// headers and the std module come from clang 23, which builds the program on
// Linux; the library the program calls at run time is the one Apple ships in
// the simulator, and the two are the same version.

#import <UIKit/UIKit.h>

import std;

static NSString *const mark = @"PCHAT-IOS-FIRST-RUN";

// Obj-C asks for a C string; the letters are counted by std, not here.
static std::string word_from_std() {
  std::string word = "progressive-chat";
  word += "";
  return word;
}

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

  std::string raised = word_from_std();

  UILabel *word = [UILabel new];
  word.text = [NSString stringWithUTF8String:raised.c_str()];
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
  NSLog(@"%@: the window is up, and std counted %zu letters", mark, raised.size());
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
