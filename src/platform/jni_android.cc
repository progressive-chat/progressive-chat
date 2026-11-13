// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.jni -- What an Android backend calls Java through: the JNI
// environment of this thread, and text given to Java as UTF-8. Taken on
// whichever thread asks: SDL attaches it to the virtual machine.
module;
#include <SDL3/SDL.h>
#include <jni.h>
// SPDX-License-Identifier: AGPL-3.0-only
export module pchat.platform.jni;

import std;

export namespace pchat::platform {

// The JNI environment of this thread, with a frame the local references made
// inside it are let go with, and any exception a call threw cleared as it
// goes: an exception left standing throws again at the next call, which is
// not where it came from.
class jni_frame {
 public:
  jni_frame() : env_(static_cast<JNIEnv *>(SDL_GetAndroidJNIEnv())) {
    ready_ = env_ && env_->PushLocalFrame(64) == JNI_OK;
  }
  jni_frame(const jni_frame &) = delete;
  jni_frame &operator=(const jni_frame &) = delete;
  ~jni_frame() {
    if (env_ && env_->ExceptionCheck()) env_->ExceptionClear();
    if (ready_) env_->PopLocalFrame(nullptr);
  }
  [[nodiscard]] JNIEnv *env() const { return ready_ ? env_ : nullptr; }

 private:
  JNIEnv *env_ = nullptr;
  bool ready_ = false;
};

// UTF-8 as a java.lang.String: through new String(bytes, "UTF-8"), not
// through NewStringUTF, whose modified UTF-8 is not UTF-8 past the basic
// plane -- a title or a body with an emoji in it would come out wrong.
[[nodiscard]] inline jstring string_of(JNIEnv *env, std::string_view text) {
  std::vector<jbyte> bytes;
  bytes.reserve(text.size());
  for (const char one : text) bytes.push_back(static_cast<jbyte>(one));
  jbyteArray array = env->NewByteArray(static_cast<jsize>(bytes.size()));
  if (!array) return nullptr;
  env->SetByteArrayRegion(array, 0, static_cast<jsize>(bytes.size()),
                          bytes.data());
  jclass string_class = env->FindClass("java/lang/String");
  return static_cast<jstring>(
      env->NewObject(string_class,
                     env->GetMethodID(string_class, "<init>",
                                      "([BLjava/lang/String;)V"),
                     array, env->NewStringUTF("UTF-8")));
}

}  // namespace pchat::platform
