#pragma once

#include <vector>
#include <string>

#include <jni.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

class JVMWrapper
{
public:
    static JVMWrapper* getInstance(std::vector<std::string> args = {});
    static void destroyJVM();

private:
    JVMWrapper() = default;
    ~JVMWrapper() = default;
    JVMWrapper(JVMWrapper const&) = delete;
    JVMWrapper& operator=(JVMWrapper const&) = delete;

    static bool createJVM(std::vector<std::string> args);

public:
    static JavaVM* m_jvm_ptr;
    static JNIEnv* m_jni_env_ptr;

private:
    static JVMWrapper* m_jvm_wrapper_instance_ptr;

#ifdef _WIN32
    static HMODULE m_jvm_dll;
#else
    static void* m_jvm_dll;
#endif

    typedef jint(JNICALL* JNI_CreateJavaVMFuncPtr)(JavaVM** pvm, void** penv, void* args);

    static JNI_CreateJavaVMFuncPtr m_jni_create_jvm_func_ptr;

    static JavaVMInitArgs m_jvm_init_args;
};