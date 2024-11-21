#include "jvmwrapper.hpp"

#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <cassert>

#ifdef _WIN32
#else
#include <dlfcn.h>
#endif // _WIN32

//#define DEBUG_GC

JVMWrapper* JVMWrapper::m_jvm_wrapper_instance_ptr = nullptr;
JVMWrapper::JNI_CreateJavaVMFuncPtr JVMWrapper::m_jni_create_jvm_func_ptr = nullptr;
JavaVM* JVMWrapper::m_jvm_ptr = nullptr;
JNIEnv* JVMWrapper::m_jni_env_ptr = nullptr;
JavaVMInitArgs JVMWrapper::m_jvm_init_args{};

#ifdef _WIN32
HMODULE JVMWrapper::m_jvm_dll = nullptr;
#else
void* JVMWrapper::m_jvm_dll = nullptr;
#endif

jclass JVMWrapper::classClass = nullptr;
jclass JVMWrapper::objectClass = nullptr;
jclass JVMWrapper::stringClass = nullptr;
jclass JVMWrapper::numberClass = nullptr;
jclass JVMWrapper::shortClass = nullptr;
jclass JVMWrapper::integerClass = nullptr;
jclass JVMWrapper::longClass = nullptr;
jclass JVMWrapper::floatClass = nullptr;
jclass JVMWrapper::doubleClass = nullptr;
jclass JVMWrapper::booleanClass = nullptr;
jclass JVMWrapper::objectArrayClass = nullptr;
jclass JVMWrapper::shortArrayClass = nullptr;
jclass JVMWrapper::intArrayClass = nullptr;
jclass JVMWrapper::longArrayClass = nullptr;
jclass JVMWrapper::floatArrayClass = nullptr;
jclass JVMWrapper::doubleArrayClass = nullptr;
jclass JVMWrapper::booleanArrayClass = nullptr;
jclass JVMWrapper::arrayListClass = nullptr;
jclass JVMWrapper::hashMapClass = nullptr;
jclass JVMWrapper::dateClass = nullptr;
jclass JVMWrapper::setClass = nullptr;
jclass JVMWrapper::outOfMemoryError = nullptr;
jclass JVMWrapper::nullPointerException = nullptr;
jclass JVMWrapper::throwableClass = nullptr;

jmethodID JVMWrapper::classGetNameMethod = nullptr;
jmethodID JVMWrapper::arrayListInitMethod = nullptr;
jmethodID JVMWrapper::arrayListAddMethod = nullptr;
jmethodID JVMWrapper::arrayListGetMethod = nullptr;
jmethodID JVMWrapper::arrayListRemoveMethod = nullptr;
jmethodID JVMWrapper::hashMapInitMethod = nullptr;
jmethodID JVMWrapper::hashMapGetMethod = nullptr;
jmethodID JVMWrapper::hashMapPutMethod = nullptr;
jmethodID JVMWrapper::hashMapKeySetMethod = nullptr;
jmethodID JVMWrapper::hashMapRemoveMethod = nullptr;
jmethodID JVMWrapper::setToArrayMethod = nullptr;
jmethodID JVMWrapper::dateInitMethod = nullptr;
jmethodID JVMWrapper::dateGetTimeMethod = nullptr;
jmethodID JVMWrapper::doubleInitMethod = nullptr;
jmethodID JVMWrapper::booleanInitMethod = nullptr;
jmethodID JVMWrapper::booleanBooleanValueMethod = nullptr;
jmethodID JVMWrapper::longInitMethod = nullptr;
jmethodID JVMWrapper::numberDoubleValueMethod = nullptr;
jmethodID JVMWrapper::throwableGetMessageMethod = nullptr;

JVMWrapper* JVMWrapper::getInstance(std::vector<std::string> args)
{
    if (m_jvm_wrapper_instance_ptr == nullptr && createJVM(args)) m_jvm_wrapper_instance_ptr = new JVMWrapper();
    return m_jvm_wrapper_instance_ptr;
}

std::string getExecutableDir()
{
    char buffer[1024];
    uint32_t size = sizeof(buffer);
#ifdef _WIN32
    GetModuleFileNameA(nullptr, buffer, size);
#elif _DARWIN
    _NSGetExecutablePath(buffer, &size);
#else
    ssize_t len = readlink("/proc/self/exe", buffer, size - 1);
    if (len != -1) buffer[len] = '\0';
#endif
    std::filesystem::path exe_path(buffer);
    return exe_path.parent_path().string();
}

bool JVMWrapper::createJVM(std::vector<std::string> args)
{
    std::string jvm_dll_path;
    std::string java_dir_path;

    if (!args.empty())
    {
        jvm_dll_path = args[0];
        java_dir_path = args[1];
    }
    else
    {
        std::string java_home = std::getenv("JAVA_HOME");
        jvm_dll_path = java_home + "/bin/server/";

#ifdef _WIN32
        jvm_dll_path += "jvm.dll";
#elif _DARWIN
        jvm_dll_path += "libjvm.dylib";
#else
        jvm_dll_path += "libjvm.so";
#endif
        // Get the directory of the executable
        std::string exe_dir = getExecutableDir();

#ifdef _MSC_VER
        // Construct the base path for the java directory located at one level up for MSVC build
        java_dir_path = (std::filesystem::path(exe_dir).parent_path() / "java").string();
#else
        // If compiling with GCC or another compiler, the java directory is in the same directory as the executable
        java_dir_path = (std::filesystem::path(exe_dir) / "java").string();
#endif
    }

    // Load jvm.dll
#ifdef _WIN32
    m_jvm_dll = LoadLibraryA(jvm_dll_path.c_str());
#else
    m_jvm_dll = dlopen((const char*)jvm_dll_path.c_str(), RTLD_NOW);
#endif
    if (m_jvm_dll == nullptr)
    {
        std::cerr << "failed to load jvm.dll at: " << jvm_dll_path << std::endl;
        return false;
    }

    m_jni_create_jvm_func_ptr = (JVMWrapper::JNI_CreateJavaVMFuncPtr)GetProcAddress(m_jvm_dll, "JNI_CreateJavaVM");
    if (m_jni_create_jvm_func_ptr == nullptr)
    {
        std::cerr << "failed to get jni create jvm func" << std::endl;
        return false;
    }

    // Construct the classpath
    std::string java_class_path = "-Djava.class.path=";
    for (auto const& entry : std::filesystem::directory_iterator(java_dir_path))
        if (entry.path().extension() == ".jar") java_class_path += "/" + entry.path().string() + ";";

    // JNI initialization
    std::vector<JavaVMOption> options;
    options.push_back(JavaVMOption{.optionString = const_cast<char*>("-Xmx256m")});
    options.push_back(JavaVMOption{.optionString = const_cast<char*>(java_class_path.c_str())});
#ifdef DEBUG_GC
    // https://www.ibm.com/docs/en/sdk-java-technology/8?topic=data-xtgc-tracing
    options.push_back(JavaVMOption{.optionString = const_cast<char*>("-verbose:gc")});
#endif

    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_1_8;
    vm_args.nOptions = options.size();
    vm_args.options = options.data();
    vm_args.ignoreUnrecognized = false;

    jint rc = m_jni_create_jvm_func_ptr(&m_jvm_ptr, (void**)&m_jni_env_ptr, &vm_args);
    if (rc != JNI_OK)
    {
        std::cerr << "Error creating JVM: " << rc << std::endl;
        return false;
    }

    jint ver = m_jni_env_ptr->GetVersion();
    std::cout << "JVM created successfully: Version" << ((ver >> 16) & 0x0f) << "." << (ver & 0x0f);
    return true;
}

void JVMWrapper::destroyJVM()
{
    if (m_jvm_ptr) m_jvm_ptr->DestroyJavaVM();
#ifdef _WIN32
    if (m_jvm_dll) FreeLibrary(m_jvm_dll);
#else
    if (m_jvm_dll) dlclose(m_jvm_dll);
#endif
}

void JVMWrapper::checkException()
{
    if (m_jni_env_ptr->ExceptionCheck())
    {
        m_jni_env_ptr->ExceptionDescribe();
        m_jni_env_ptr->ExceptionClear();
    }
}

JNIEnv* JVMWrapper::getJNIEnv()
{
    return m_jni_env_ptr;
}

jclass JVMWrapper::findClass(const char* className)
{
    assert(m_jni_env_ptr);

    jclass javaClass = m_jni_env_ptr->FindClass(className);
    if (!javaClass)
    {
        std::cerr << "Couldn't find Java class: " << className << std::endl;
        checkException();
        return nullptr;
    }
    else
    {
        jclass globalClass = (jclass)m_jni_env_ptr->NewGlobalRef(javaClass);
        m_jni_env_ptr->DeleteLocalRef(javaClass);
        return globalClass;
    }
}

jmethodID JVMWrapper::getMethodID(jclass javaClass, const char* methodName, const char* signature, bool isStatic)
{
    assert(m_jni_env_ptr);

    jmethodID javaMethodID = isStatic ? m_jni_env_ptr->GetStaticMethodID(javaClass, methodName, signature) :
                                        javaMethodID = m_jni_env_ptr->GetMethodID(javaClass, methodName, signature);
    if (!javaMethodID)
    {
        std::cerr << "Couldn't find Java method ID: " << methodName << " " << signature << std::endl;
        checkException();
    }
    return javaMethodID;
}

jfieldID JVMWrapper::getFieldID(jclass javaClass, const char* fieldName, const char* signature)
{
    assert(m_jni_env_ptr);

    jfieldID javaFieldID = m_jni_env_ptr->GetFieldID(javaClass, fieldName, signature);
    if (!javaFieldID)
    {
        std::cerr << "Couldn't find Java field ID: " << fieldName << " " << signature << std::endl;
        checkException();
    }
    return javaFieldID;
}

jstring JVMWrapper::getClassName(jclass javaClass)
{
    assert(m_jni_env_ptr);

    if (!classClass) classClass = findClass("java/lang/Class");
    return (jstring)m_jni_env_ptr->CallObjectMethod(javaClass,
                                                    getMethodID(classClass, "getName", "()Ljava/lang/String;", false));
}

jobjectArray JVMWrapper::newObjectArray(int length, jobject initial)
{
    assert(m_jni_env_ptr);

    if (!objectClass) objectClass = findClass("java/lang/Object");
    return m_jni_env_ptr->NewObjectArray(length, objectClass, initial);
}

void JVMWrapper::throwException(jclass clazz, const char* message)
{
    assert(m_jni_env_ptr);

    m_jni_env_ptr->ExceptionClear();
    m_jni_env_ptr->ThrowNew(clazz, message);
}

void JVMWrapper::throwException(const char* className, const char* message)
{
    assert(m_jni_env_ptr);

    jclass clazz = findClass(className);
    throwException(clazz, message);
    m_jni_env_ptr->DeleteLocalRef(clazz);
}

void JVMWrapper::initCache()
{
    assert(m_jni_env_ptr);

    classClass = findClass("java/lang/Class");
    objectClass = findClass("java/lang/Object");
    numberClass = findClass("java/lang/Number");
    stringClass = findClass("java/lang/String");
    shortClass = findClass("java/lang/Short");
    integerClass = findClass("java/lang/Integer");
    longClass = findClass("java/lang/Long");
    floatClass = findClass("java/lang/Float");
    doubleClass = findClass("java/lang/Double");
    booleanClass = findClass("java/lang/Boolean");
    shortArrayClass = findClass("[S");
    intArrayClass = findClass("[I");
    longArrayClass = findClass("[J");
    floatArrayClass = findClass("[F");
    doubleArrayClass = findClass("[D");
    booleanArrayClass = findClass("[Z");
    objectArrayClass = findClass("[Ljava/lang/Object;");
    arrayListClass = findClass("java/util/ArrayList");
    hashMapClass = findClass("java/util/HashMap");
    dateClass = findClass("java/util/Date");
    setClass = findClass("java/util/Set");

    outOfMemoryError = findClass("java/lang/OutOfMemoryError");
    nullPointerException = findClass("java/lang/NullPointerException");
    throwableClass = findClass("java/lang/Throwable");

    classGetNameMethod = getMethodID(classClass, "getName", "()Ljava/lang/String;", false);

    arrayListInitMethod = getMethodID(arrayListClass, "<init>", "()V", false);
    arrayListAddMethod = getMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z", false);
    arrayListGetMethod = getMethodID(arrayListClass, "get", "(I)Ljava/lang/Object;", false);
    arrayListRemoveMethod = getMethodID(arrayListClass, "remove", "(I)Ljava/lang/Object;", false);

    hashMapInitMethod = getMethodID(hashMapClass, "<init>", "(I)V", false);
    hashMapGetMethod = getMethodID(hashMapClass, "get", "(Ljava/lang/Object;)Ljava/lang/Object;", false);
    hashMapPutMethod =
        getMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;", false);
    hashMapKeySetMethod = getMethodID(hashMapClass, "keySet", "()Ljava/util/Set;", false);
    hashMapRemoveMethod = getMethodID(hashMapClass, "remove", "(Ljava/lang/Object;)Ljava/lang/Object;", false);

    setToArrayMethod = getMethodID(setClass, "toArray", "()[Ljava/lang/Object;", false);

    dateInitMethod = getMethodID(dateClass, "<init>", "(J)V", false);
    dateGetTimeMethod = getMethodID(dateClass, "getTime", "()J", false);

    doubleInitMethod = getMethodID(doubleClass, "<init>", "(D)V", false);
    booleanInitMethod = getMethodID(booleanClass, "<init>", "(Z)V", false);
    booleanBooleanValueMethod = getMethodID(booleanClass, "booleanValue", "()Z", false);
    longInitMethod = getMethodID(longClass, "<init>", "(J)V", false);
    numberDoubleValueMethod = getMethodID(numberClass, "doubleValue", "()D", false);
    throwableGetMessageMethod = getMethodID(throwableClass, "getMessage", "()Ljava/lang/String;", false);
}

void JVMWrapper::throwOutOfMemoryError(const char* message)
{
    if (!outOfMemoryError) outOfMemoryError = findClass("java/lang/OutOfMemoryError");
    throwException(outOfMemoryError, message);
}

void JVMWrapper::throwNullPointerException(const char* message)
{
    if (!nullPointerException) nullPointerException = findClass("java/lang/NullPointerException");
    throwException(nullPointerException, message);
}
