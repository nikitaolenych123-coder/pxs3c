#include <jni.h>
#include <memory>
#include "core/Emulator.h"
#include <android/native_window_jni.h>

static std::unique_ptr<pxs3c::Emulator> g_emu;

extern "C" JNIEXPORT jboolean JNICALL
Java_com_pxs3c_MainActivity_nativeInit(JNIEnv* env, jobject thiz) {
    g_emu = std::make_unique<pxs3c::Emulator>();
    return g_emu->init() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_pxs3c_MainActivity_nativeLoadGame(JNIEnv* env, jobject thiz, jstring jpath) {
    const char* path = env->GetStringUTFChars(jpath, nullptr);
    bool ok = g_emu && g_emu->loadGame(path);
    env->ReleaseStringUTFChars(jpath, path);
    return ok ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_pxs3c_MainActivity_nativeRunFrame(JNIEnv* env, jobject thiz) {
    if (g_emu) g_emu->runFrame();
}

extern "C" JNIEXPORT void JNICALL
Java_com_pxs3c_MainActivity_nativeShutdown(JNIEnv* env, jobject thiz) {
    if (g_emu) g_emu->shutdown();
    g_emu.reset();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_pxs3c_MainActivity_nativeAttachSurface(JNIEnv* env, jobject thiz, jobject surface) {
#ifdef __ANDROID__
    if (!g_emu) {
        g_emu = std::make_unique<pxs3c::Emulator>();
        if (!g_emu->init()) return JNI_FALSE;
    }
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    bool ok = g_emu->attachAndroidWindow(window);
    ANativeWindow_release(window);
    return ok ? JNI_TRUE : JNI_FALSE;
#else
    (void)env; (void)thiz; (void)surface;
    return JNI_FALSE;
#endif
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_pxs3c_MainActivity_nativeResize(JNIEnv* env, jobject thiz, jint width, jint height) {
#ifdef __ANDROID__
    if (!g_emu) return JNI_FALSE;
    // Renderer is internal; expose resize via emulator by adding a forwarding method or access.
    // For simplicity, call tick once after resize in this prototype.
    // Proper API would add Emulator::resize() — skipping to keep changes focused.
    return JNI_TRUE;
#else
    (void)env; (void)thiz; (void)width; (void)height;
    return JNI_FALSE;
#endif
}

extern "C" JNIEXPORT void JNICALL
Java_com_pxs3c_MainActivity_nativeSetTargetFps(JNIEnv* env, jobject thiz, jint fps) {
    if (!g_emu) return;
    g_emu->setTargetFps(static_cast<int>(fps));
}

extern "C" JNIEXPORT jint JNICALL
Java_com_pxs3c_MainActivity_nativeTickFrame(JNIEnv* env, jobject thiz) {
    if (!g_emu) return 16;
    return g_emu->tickFrameAndGetDelayMs();
}

extern "C" JNIEXPORT void JNICALL
Java_com_pxs3c_MainActivity_nativeSetClearColor(JNIEnv* env, jobject thiz, jfloat r, jfloat g, jfloat b) {
    if (!g_emu) return;
    g_emu->setClearColor(r, g, b);
}

extern "C" JNIEXPORT void JNICALL
Java_com_pxs3c_MainActivity_nativeSetVsync(JNIEnv* env, jobject thiz, jboolean enabled) {
    if (!g_emu) return;
    g_emu->setVsync(enabled == JNI_TRUE);
}
