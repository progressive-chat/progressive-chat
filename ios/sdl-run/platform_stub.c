// SPDX-License-Identifier: AGPL-3.0-only

// Stub for __isPlatformVersionAtLeast which SDL's uikit video uses for
// runtime version checks. The iOS simulator SDK doesn't export this symbol
// in the libraries we link, so we provide a trivial implementation that
// always returns true (we're on iOS 26).

#include <Availability.h>
#include <TargetConditionals.h>

#if TARGET_OS_IOS && !TARGET_OS_SIMULATOR
// On device, the symbol exists in the SDK.
#else
// On simulator, provide a stub that says "yes, we're new enough".
// The function signature matches Apple's availability checking.
int __isPlatformVersionAtLeast(int major, int minor, int subminor) {
  return 1;
}
#endif