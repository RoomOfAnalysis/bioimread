#include "jvmwrapper.hpp"

#include <filesystem>
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#else
#include <dlfcn.h>
#endif // _WIN32

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
    auto* options = new JavaVMOption[1];
    options[0].optionString = const_cast<char*>(java_class_path.c_str());

    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_1_8;
    vm_args.nOptions = 1;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = false;

    jint rc = m_jni_create_jvm_func_ptr(&m_jvm_ptr, (void**)&m_jni_env_ptr, &vm_args);
    delete[] options;
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
