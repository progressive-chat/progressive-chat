// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.notified.backend -- Android's notifications: one posted
// through NotificationManager, in a channel of this program's own (Android 8
// and later need one and list an app without one among those that send
// nothing), opening the program when pressed. Asked through JNI on whichever
// thread asks: SDL attaches it to the virtual machine.
//
// Android 13 and later post only for an app the user let post them: the first
// press asks for that and shows nothing, the answer not being given while
// this call runs. The system keeps the grant, so it is asked once and never
// again.
module;
#include <SDL3/SDL.h>
#include <jni.h>
// SPDX-License-Identifier: AGPL-3.0-only
export module pchat.platform.notified.backend;

import std;
import pchat.platform.jni;

namespace {

// The channel a test notification is posted in, made each time: making it
// again changes nothing the user set.
constexpr const char *channel_id = "tests";
constexpr const char *channel_name = "Tests";
constexpr jint importance_high = 4;  // NotificationManager.IMPORTANCE_HIGH

// Whether this program may post: PackageManager.PERMISSION_GRANTED is 0.
bool let_post(JNIEnv *env, jobject activity) {
  jclass context = env->FindClass("android/content/Context");
  if (!context) return false;
  return env->CallIntMethod(activity,
                            env->GetMethodID(context, "checkSelfPermission",
                                             "(Ljava/lang/String;)I"),
                            env->NewStringUTF(
                                "android.permission.POST_NOTIFICATIONS")) == 0;
}

// Asked for it once, where it was not given yet: the system's own request;
// nothing here waits for its answer.
void ask_to_post(JNIEnv *env, jobject activity) {
  jobjectArray asked = env->NewObjectArray(
      1, env->FindClass("java/lang/String"), nullptr);
  if (!asked) return;
  env->SetObjectArrayElement(
      asked, 0, env->NewStringUTF("android.permission.POST_NOTIFICATIONS"));
  jclass activity_class = env->FindClass("android/app/Activity");
  if (!activity_class) return;
  env->CallVoidMethod(activity,
                      env->GetMethodID(activity_class, "requestPermissions",
                                       "([Ljava/lang/String;I)V"),
                      asked, 0);
}

void make_channel(JNIEnv *env, jobject manager) {
  jclass channel_class = env->FindClass("android/app/NotificationChannel");
  if (!channel_class) return;
  jobject channel = env->NewObject(
      channel_class,
      env->GetMethodID(channel_class, "<init>",
                       "(Ljava/lang/String;Ljava/lang/CharSequence;I)V"),
      env->NewStringUTF(channel_id), pchat::platform::string_of(env, channel_name),
      importance_high);
  if (!channel) return;
  env->CallVoidMethod(
      manager,
      env->GetMethodID(env->GetObjectClass(manager),
                       "createNotificationChannel",
                       "(Landroid/app/NotificationChannel;)V"),
      channel);
}

// What a press on it does: the program brought up, as its own icon would.
jobject opening(JNIEnv *env, jobject activity, int sdk) {
  jclass context = env->FindClass("android/content/Context");
  if (!context) return nullptr;
  jobject packages = env->CallObjectMethod(
      activity, env->GetMethodID(context, "getPackageManager",
                                 "()Landroid/content/pm/PackageManager;"));
  jobject name = env->CallObjectMethod(
      activity,
      env->GetMethodID(context, "getPackageName", "()Ljava/lang/String;"));
  if (!packages || !name) return nullptr;
  jobject intent = env->CallObjectMethod(
      packages,
      env->GetMethodID(env->GetObjectClass(packages),
                       "getLaunchIntentForPackage",
                       "(Ljava/lang/String;)Landroid/content/Intent;"),
      name);
  if (!intent) return nullptr;
  // FLAG_IMMUTABLE, required of every PendingIntent on Android 12 and later;
  // FLAG_UPDATE_CURRENT, the intent the newest one given.
  constexpr jint immutable = 0x04000000, update_current = 0x08000000;
  jclass pending = env->FindClass("android/app/PendingIntent");
  if (!pending) return nullptr;
  return env->CallStaticObjectMethod(
      pending,
      env->GetStaticMethodID(
          pending, "getActivity",
          "(Landroid/content/Context;ILandroid/content/Intent;I)"
          "Landroid/app/PendingIntent;"),
      activity, 1, intent, (sdk >= 23 ? immutable : 0) | update_current);
}

// The small icon at its left: the platform's own chat icon, asked for by
// name. This program ships no drawable of its own.
jint icon(JNIEnv *env, jobject activity) {
  jclass context = env->FindClass("android/content/Context");
  if (!context) return 0;
  jobject resources = env->CallObjectMethod(
      activity, env->GetMethodID(context, "getResources",
                                 "()Landroid/content/res/Resources;"));
  if (!resources) return 0;
  return env->CallIntMethod(
      resources,
      env->GetMethodID(env->GetObjectClass(resources), "getIdentifier",
                       "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/"
                       "String;)I"),
      env->NewStringUTF("stat_notify_chat"), env->NewStringUTF("drawable"),
      env->NewStringUTF("android"));
}

}  // namespace

export namespace pchat::platform::notified::backend {

bool say(std::string_view title, std::string_view text) {
  const pchat::platform::jni_frame frame;
  JNIEnv *env = frame.env();
  if (!env) return false;
  const auto activity = static_cast<jobject>(SDL_GetAndroidActivity());
  if (!activity) return false;
  const int sdk = SDL_GetAndroidSDKVersion();
  if (sdk >= 33 && !let_post(env, activity)) {
    ask_to_post(env, activity);
    return false;
  }
  jclass context = env->FindClass("android/content/Context");
  if (!context) return false;
  jobject manager = env->CallObjectMethod(
      activity, env->GetMethodID(context, "getSystemService",
                                 "(Ljava/lang/String;)Ljava/lang/Object;"),
      env->NewStringUTF("notification"));
  if (!manager) return false;
  if (sdk >= 26) make_channel(env, manager);
  jclass builder_class = env->FindClass("android/app/Notification$Builder");
  if (!builder_class) return false;
  jobject builder =
      sdk >= 26
          ? env->NewObject(builder_class,
                           env->GetMethodID(builder_class, "<init>",
                                            "(Landroid/content/Context;"
                                            "Ljava/lang/String;)V"),
                           activity, env->NewStringUTF(channel_id))
          : env->NewObject(builder_class,
                           env->GetMethodID(
                               builder_class, "<init>",
                               "(Landroid/content/Context;)V"),
                           activity);
  if (!builder) return false;
  const auto set = [&](const char *method, const char *signature,
                       auto... arguments) {
    env->CallObjectMethod(builder,
                          env->GetMethodID(builder_class, method, signature),
                          arguments...);
  };
  constexpr const char *says_builder =
      "(Ljava/lang/CharSequence;)Landroid/app/Notification$Builder;";
  set("setSmallIcon", "(I)Landroid/app/Notification$Builder;",
      icon(env, activity));
  set("setContentTitle", says_builder,
      pchat::platform::string_of(env, title));
  set("setContentText", says_builder, pchat::platform::string_of(env, text));
  set("setAutoCancel", "(Z)Landroid/app/Notification$Builder;", JNI_TRUE);
  // Priority and sound, which a channel sets itself from Android 8 on: high,
  // shown at once, with the system's sound.
  constexpr jint priority_high = 1, default_sound = 1;
  set("setPriority", "(I)Landroid/app/Notification$Builder;", priority_high);
  set("setDefaults", "(I)Landroid/app/Notification$Builder;", default_sound);
  if (jobject pressed = opening(env, activity, sdk))
    set("setContentIntent",
        "(Landroid/app/PendingIntent;)Landroid/app/Notification$Builder;",
        pressed);
  jobject notification = env->CallObjectMethod(
      builder,
      env->GetMethodID(builder_class, "build", "()Landroid/app/Notification;"));
  if (!notification) return false;
  env->CallVoidMethod(manager,
                      env->GetMethodID(env->GetObjectClass(manager), "notify",
                                       "(ILandroid/app/Notification;)V"),
                      1, notification);
  return true;
}

}  // namespace pchat::platform::notified::backend
