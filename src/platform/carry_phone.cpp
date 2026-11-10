// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.carry_phone -- the asking itself: one thread of its own,
// asking the machine's own stack and leaving the bytes where the pump takes
// them.
//
// Asked through JNI of java.net.URL and java.net.HttpURLConnection: pure
// platform classes, found through the bootstrap loader from whichever thread
// is attached (SDL attaches them -- src/platform/jni_android.cc says so of
// whichever thread asks), and carrying nothing of this program's own beyond
// what any Java runtime has. What comes of them -- TLS, certificates, proxy
// settings -- is the machine's, and stays there.
#include "carry_phone.h"

#include <SDL3/SDL.h>
#include <jni.h>

#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace pchat::platform {

// What one asking keeps while it is in the air: the thread that asks, what
// it arrives with, and where it stands -- the connection itself held in a
// global reference, so that a carrying closed while the transfer is still
// coming can hang up on it from the thread that closes.
struct phone_ask {
  std::thread worker;
  std::mutex guard;
  std::string arrived;
  std::string trouble;
  phone_end stood = phone_end::working;
  jobject connection = nullptr;
  std::atomic<bool> stopped{false};
};

namespace {

// The JNI environment of this thread, with a frame the local references made
// inside it are let go with, and whatever exception a call threw cleared as
// it goes: an exception left standing throws again at the next call, which
// is not where it came from. The same thing the module-unit helper does
// (pchat.platform.jni), kept whole here because a classic unit may not
// import a module.
class frame {
 public:
  frame() : env_(static_cast<JNIEnv *>(SDL_GetAndroidJNIEnv())) {
    ready_ = env_ != nullptr && env_->PushLocalFrame(16) == JNI_OK;
  }
  frame(const frame &) = delete;
  frame &operator=(const frame &) = delete;
  ~frame() {
    if (env_ && env_->ExceptionCheck()) env_->ExceptionClear();
    if (ready_) env_->PopLocalFrame(nullptr);
  }
  [[nodiscard]] JNIEnv *env() const { return ready_ ? env_ : nullptr; }

 private:
  JNIEnv *env_ = nullptr;
  bool ready_ = false;
};

// Text for what is a place or a header: those are ASCII, by what a URL and a
// header ever are, and NewStringUTF is whole enough a one for them. Bodies
// never go this way -- they are bytes, and go in as bytes.
[[nodiscard]] jstring ascii_of(JNIEnv *env, std::string_view text) {
  const std::string whole(text);
  return env->NewStringUTF(whole.c_str());
}

// Whether the call just made threw: an exception standing throws again at
// the next one, so it is cleared where it is found -- which is also where
// the asking learns that the machine would not answer it.
[[nodiscard]] bool threw(JNIEnv *env) {
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    return true;
  }
  return false;
}

// Said the same way wherever a thread gives up: what a carrying reads as its
// own words when nothing else says why (the page says "the page could not
// ask for it", and a window "this machine could not ask for it").
constexpr const char *could_not_ask = "the phone could not ask for it";

// Where a thread went, and why it went there -- under the one lock the pump
// takes, since these are what it reads.
void went(phone_ask *one, phone_end stood, std::string why = {}) {
  std::lock_guard held(one->guard);
  one->stood = stood;
  if (!why.empty())
    one->trouble = std::move(why);
}

}  // namespace

phone_ask *phone_open() { return new phone_ask{}; }

void phone_close(phone_ask **one) {
  if (one == nullptr || *one == nullptr)
    return;
  phone_ask *ask = *one;
  ask->stopped = true;
  // Hung up on where a transfer is in the air: the thread's own read fails
  // and it lets go with it -- nothing here waits on a socket, which is what
  // the program's loop has never done.
  {
    const frame here;
    if (JNIEnv *env = here.env()) {
      std::lock_guard held(ask->guard);
      if (ask->connection != nullptr) {
        jclass connection_class = env->GetObjectClass(ask->connection);
        env->CallVoidMethod(ask->connection,
                            env->GetMethodID(connection_class, "disconnect",
                                             "()V"));
        if (env->ExceptionCheck()) env->ExceptionClear();
        env->DeleteGlobalRef(ask->connection);
        ask->connection = nullptr;
      }
    }
  }
  if (ask->worker.joinable())
    ask->worker.join();
  delete ask;
  *one = nullptr;
}

bool phone_ask_at(phone_ask *one, std::string_view url,
                  std::span<const std::string> fields, std::string_view body) {
  // The asking before this one, finished: a carrying asks again only where
  // the connection went before an answer (carry.cc, pump_phone), and never
  // in two threads at once.
  if (one->worker.joinable())
    one->worker.join();
  {
    std::lock_guard held(one->guard);
    one->arrived.clear();
    one->trouble.clear();
    one->stood = phone_end::working;
  }
  one->stopped = false;

  std::string place(url);
  std::string wrote(body);
  std::vector<std::string> flat(fields.begin(), fields.end());
  one->worker =
      std::thread([one, place = std::move(place), flat = std::move(flat),
                   wrote = std::move(wrote)] {
      // The asking itself, whole, in one place: whichever way it ends, the
      // thread lets go of the virtual machine below -- a thread that exits
      // while it is still attached is one the JVM goes on reading after its
      // stack is gone, and the next asking made anywhere is where that
      // shows.
      const auto asking = [&] {
        const frame in_thread;
        JNIEnv *env = in_thread.env();
        if (env == nullptr) {
          went(one, phone_end::lost, could_not_ask);
          return;
        }

        // The connection itself: a URL, what it opens, and the HTTP words
        // it is spoken with. All three are platform classes -- found
        // through the bootstrap loader, attached by SDL like everything
        // that asks Java of any thread (src/platform/jni_android.cc).
        jclass url_class = env->FindClass("java/net/URL");
        jstring where = url_class ? ascii_of(env, place) : nullptr;
        jobject url =
            (url_class && where)
                ? env->NewObject(
                      url_class,
                      env->GetMethodID(url_class, "<init>",
                                       "(Ljava/lang/String;)V"),
                      where)
                : nullptr;
        jobject connection =
            url ? env->CallObjectMethod(
                      url, env->GetMethodID(url_class, "openConnection",
                                            "()Ljava/net/URLConnection;"))
                : nullptr;
        if (threw(env) || connection == nullptr) {
          went(one, phone_end::lost, could_not_ask);
          return;
        }
        jclass http_class = env->GetObjectClass(connection);
        // POST, as every wire here is said: what goes out is a body of an
        // asking and never a pull of something.
        env->CallVoidMethod(connection,
                            env->GetMethodID(http_class, "setRequestMethod",
                                             "(Ljava/lang/String;)V"),
                            env->NewStringUTF("POST"));
        env->CallVoidMethod(connection,
                            env->GetMethodID(http_class, "setDoOutput", "(Z)V"),
                            JNI_TRUE);
        // Long enough for the slowest answer this program waits for, and
        // the same numbers a window asks with (client policy: connect
        // within 30 seconds, read within 300 -- provider/http/wire.h).
        env->CallVoidMethod(connection,
                            env->GetMethodID(http_class, "setConnectTimeout",
                                             "(I)V"),
                            30000);
        env->CallVoidMethod(
            connection,
            env->GetMethodID(http_class, "setReadTimeout", "(I)V"), 300000);
        // The headers, one name and one value at a time, exactly as they
        // were written into this carrying.
        jmethodID set_header = env->GetMethodID(
            http_class, "setRequestProperty",
            "(Ljava/lang/String;Ljava/lang/String;)V");
        if (set_header != nullptr)
          for (std::size_t at = 0; at + 1 < flat.size(); at += 2)
            env->CallVoidMethod(connection, set_header,
                                ascii_of(env, flat[at]),
                                ascii_of(env, flat[at + 1]));
        if (threw(env)) {
          went(one, phone_end::lost, could_not_ask);
          return;
        }
        // Held whole in a global reference, so that phone_close may hang up
        // on this from the thread that closes while the read is in the air.
        {
          std::lock_guard held(one->guard);
          one->connection = env->NewGlobalRef(connection);
        }
        // The body, as bytes: never through a string, which is not what a
        // body of any shape is.
        jobject out = env->CallObjectMethod(
            connection,
            env->GetMethodID(http_class, "getOutputStream",
                             "()Ljava/io/OutputStream;"));
        if (out != nullptr && !wrote.empty()) {
          jbyteArray bytes =
              env->NewByteArray(static_cast<jsize>(wrote.size()));
          env->SetByteArrayRegion(bytes, 0, static_cast<jsize>(wrote.size()),
                                  reinterpret_cast<const jbyte *>(wrote.data()));
          env->CallVoidMethod(out,
                              env->GetMethodID(env->GetObjectClass(out),
                                               "write", "([B)V"),
                              bytes);
          env->CallVoidMethod(out,
                              env->GetMethodID(env->GetObjectClass(out),
                                               "close", "()V"));
        }
        if (threw(env)) {
          went(one, phone_end::lost, could_not_ask);
          return;
        }

        // The status, which comes before a byte of the answer is read: a
        // refusal says so here, in words, and none of its body is taken --
        // a status that says no is never read as an answer (the page says
        // the same of itself, carry.cc, went_on).
        const jint code = env->CallIntMethod(
            connection, env->GetMethodID(http_class, "getResponseCode", "()I"));
        if (threw(env)) {
          went(one, phone_end::lost, could_not_ask);
          return;
        }
        if (code >= 400) {
          std::string why = "the server answered " + std::to_string(code);
          // What the status says beside its number, asked of the platform by
          // the name it has had since java.net began (`getResponseMessage`,
          // and not a name in any other language's) -- looked up first, and
          // never called through an id that is not there: a null id into a
          // call is where the machine stops, and not an error to read.
          const jmethodID phrase = env->GetMethodID(
              http_class, "getResponseMessage", "()Ljava/lang/String;");
          if (phrase == nullptr) {
            if (env->ExceptionCheck()) env->ExceptionClear();
          }
          jstring reason = phrase != nullptr
                               ? static_cast<jstring>(env->CallObjectMethod(
                                     connection, phrase))
                               : nullptr;
          if (reason != nullptr && !threw(env)) {
            const char *said = env->GetStringUTFChars(reason, nullptr);
            if (said != nullptr) {
              why += ": ";
              why += said;
              env->ReleaseStringUTFChars(reason, said);
            }
          }
          went(one, phone_end::refused, std::move(why));
          return;
        }

        // The answer, in the pieces it arrives in: a stream read whole by
        // the loop's own standards -- every piece under the lock the pump
        // takes, and none of it twice.
        jobject in = env->CallObjectMethod(
            connection,
            env->GetMethodID(http_class, "getInputStream",
                             "()Ljava/io/InputStream;"));
        if (threw(env) || in == nullptr) {
          went(one, phone_end::lost, could_not_ask);
          return;
        }
        constexpr jsize room = 8192;
        jbyteArray buffer = env->NewByteArray(room);
        jclass input_class = env->GetObjectClass(in);
        jmethodID read_one = env->GetMethodID(input_class, "read", "([BII)I");
        bool broke = false;
        while (buffer != nullptr && read_one != nullptr && !one->stopped) {
          const jint got = env->CallIntMethod(in, read_one, buffer, 0, room);
          if (threw(env)) {
            broke = true;  // what came whole before it went stays
            break;
          }
          if (got < 0)
            break;  // the end of the answer
          if (got == 0)
            continue;
          std::string piece(static_cast<std::size_t>(got), '\0');
          env->GetByteArrayRegion(buffer, 0, got,
                                  reinterpret_cast<jbyte *>(piece.data()));
          std::lock_guard held(one->guard);
          one->arrived += piece;
          if (one->stopped)
            break;
        }
        // The end of the answer, or where it went: a transfer that broke
        // before it was over is no answer, whole or not -- what came stays,
        // and the carrying above decides whether to ask again (pump_phone).
        const bool interrupted = one->stopped || broke;
        went(one, interrupted ? phone_end::lost : phone_end::answered,
             interrupted ? std::string(could_not_ask) : std::string{});
        env->CallVoidMethod(connection,
                            env->GetMethodID(http_class, "disconnect", "()V"));
        if (env->ExceptionCheck()) env->ExceptionClear();
      };
      asking();
      if (JNIEnv *detach = static_cast<JNIEnv *>(SDL_GetAndroidJNIEnv())) {
        JavaVM *machine = nullptr;
        if (detach->GetJavaVM(&machine) == JNI_OK && machine != nullptr)
          machine->DetachCurrentThread();
      }
    });
  return true;
}

phone_end phone_pump(phone_ask *one, std::string &piece, std::string &why) {
  std::lock_guard held(one->guard);
  // What has arrived since the last pump, whole and once: a refusal never
  // puts a byte here, and what a lost transfer did put stays an answer in
  // the making -- taken, and then judged (carry.cc, pump_phone).
  piece.swap(one->arrived);
  if (one->stood == phone_end::refused || one->stood == phone_end::lost)
    why = one->trouble;
  return one->stood;
}

}  // namespace pchat::platform
