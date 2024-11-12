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

    static JNIEnv* getJNIEnv();
    //\note: global reference
    static jclass findClass(const char* className);
    static jmethodID getMethodID(jclass javaClass, const char* methodName, const char* signature,
                                 bool isStatic = false);
    static jfieldID getFieldID(jclass javaClass, const char* fieldName, const char* signature);
    static jstring getClassName(jclass javaClass);
    static jobjectArray newObjectArray(int length, jobject initial = nullptr);

    static void throwException(jclass clazz, const char* message);
    static void throwException(const char* className, const char* message);

    static void throwOutOfMemoryError(const char* message);
    static void throwNullPointerException(const char* message);

    static void initCache();

private:
    JVMWrapper() = default;
    ~JVMWrapper() = default;
    JVMWrapper(JVMWrapper const&) = delete;
    JVMWrapper& operator=(JVMWrapper const&) = delete;

    static bool createJVM(std::vector<std::string> args);
    static void checkException();

private:
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

    // cache
    // Java classes
    static jclass classClass;
    static jclass objectClass;
    static jclass stringClass;
    static jclass numberClass;
    static jclass shortClass;
    static jclass integerClass;
    static jclass longClass;
    static jclass floatClass;
    static jclass doubleClass;
    static jclass booleanClass;
    static jclass objectArrayClass;
    static jclass shortArrayClass;
    static jclass intArrayClass;
    static jclass longArrayClass;
    static jclass floatArrayClass;
    static jclass doubleArrayClass;
    static jclass booleanArrayClass;
    static jclass arrayListClass;
    static jclass hashMapClass;
    static jclass dateClass;
    static jclass setClass;
    static jclass outOfMemoryError;
    static jclass throwableClass;
    static jclass nullPointerException;

    // Java methods
    static jmethodID classGetNameMethod;
    static jmethodID arrayListInitMethod;
    static jmethodID arrayListAddMethod;
    static jmethodID arrayListGetMethod;
    static jmethodID arrayListRemoveMethod;
    static jmethodID hashMapInitMethod;
    static jmethodID hashMapGetMethod;
    static jmethodID hashMapPutMethod;
    static jmethodID hashMapKeySetMethod;
    static jmethodID hashMapRemoveMethod;
    static jmethodID setToArrayMethod;
    static jmethodID dateInitMethod;
    static jmethodID dateGetTimeMethod;
    static jmethodID doubleInitMethod;
    static jmethodID booleanInitMethod;
    static jmethodID booleanBooleanValueMethod;
    static jmethodID longInitMethod;
    static jmethodID numberDoubleValueMethod;
    static jmethodID throwableGetMessageMethod;
};