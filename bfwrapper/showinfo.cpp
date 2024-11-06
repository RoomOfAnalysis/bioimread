#include <jni.h>
#include <iostream>
#include <filesystem>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif // _WIN32

typedef int(__stdcall* JNI_CreateJavaVMFunc)(JavaVM** pvm, JNIEnv** penv, void* args);

std::string getExecutableDir()
{
    char buffer[1024];
#ifdef _WIN32
    GetModuleFileNameA(nullptr, buffer, sizeof(buffer));
#else
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) buffer[len] = '\0';
#endif
    std::filesystem::path exe_path(buffer);
    return exe_path.parent_path().string();
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <path_to_image_file>" << std::endl;
        return 1;
    }

    std::string java_home = std::getenv("JAVA_HOME");
    auto jvm_dll_path = java_home + "/bin/server/jvm.dll";
    HMODULE jvm_dll = LoadLibraryA(jvm_dll_path.c_str());
    if (jvm_dll == nullptr)
    {
        std::cerr << "failed to load jvm.dll at: " << jvm_dll_path << std::endl;
        exit(EXIT_FAILURE);
    }
    JNI_CreateJavaVMFunc Dll_JNI_CreateJavaVM = (JNI_CreateJavaVMFunc)GetProcAddress(jvm_dll, "JNI_CreateJavaVM");
    if (Dll_JNI_CreateJavaVM == 0)
    {
        std::cerr << "failed to get jni create jvm func" << std::endl;
        exit(EXIT_FAILURE);
    }

    const char* filePath = argv[1];

    // Get the directory of the executable
    std::string exe_dir = getExecutableDir();

#ifdef _MSC_VER
    // Construct the base path for the java directory located at one level up for MSVC build
    std::filesystem::path java_dir = std::filesystem::path(exe_dir).parent_path() / "java";
#else
    // If compiling with GCC or another compiler, the java directory is in the same directory as the executable
    std::filesystem::path javaDir = std::filesystem::path(exe_dir) / "java";
#endif

    // Construct the classpath using the java_dir
    std::string java_class_path =
        "-Djava.class.path=" + java_dir.string() + "/bfwrapper.jar;" + java_dir.string() + "/bioformats_package.jar;";

    // JNI initialization
    JavaVMOption options[1];
    options[0].optionString = const_cast<char*>(java_class_path.c_str());

    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_1_8;
    vm_args.nOptions = 1;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = false;

    JavaVM* jvm;
    JNIEnv* env;
    jint rc = Dll_JNI_CreateJavaVM(&jvm, &env, &vm_args);
    if (rc != JNI_OK)
    {
        std::cerr << "Error creating JVM: " << rc << std::endl;
        exit(EXIT_FAILURE);
    }
    else
        std::cout << "JVM succesfully created." << std::endl;

    // set log level
    // https://bio-formats.readthedocs.io/en/v8.0.0/developers/logging.html
    jclass log_cls = env->FindClass("org/slf4j/Logger");
    if (log_cls == nullptr)
    {
        std::cerr << "Error: bioformats package not loaded." << std::endl;
        jvm->DestroyJavaVM();
        exit(EXIT_FAILURE);
    }
    jstring root_logger_name = (jstring)env->GetStaticObjectField(
        log_cls, env->GetStaticFieldID(log_cls, "ROOT_LOGGER_NAME", "Ljava/lang/String;"));
    jclass log_factory_cls = env->FindClass("org/slf4j/LoggerFactory");
    jmethodID getLogger =
        env->GetStaticMethodID(log_factory_cls, "getLogger", "(Ljava/lang/String;)Lorg/slf4j/Logger;");
    jobject root_logger = env->CallStaticObjectMethod(log_factory_cls, getLogger, root_logger_name);
    jclass log_level_cls = env->FindClass("ch/qos/logback/classic/Level");
    jstring log_level_name = (jstring)env->GetStaticObjectField(
        log_cls, env->GetStaticFieldID(log_level_cls, "ERROR", "Lch/qos/logback/classic/Level;"));
    jclass root_logger_cls = env->GetObjectClass(root_logger);
    jmethodID setLogLevel = env->GetMethodID(root_logger_cls, "setLevel", "(Lch/qos/logback/classic/Level;)V");
    env->CallVoidMethod(root_logger, setLogLevel, log_level_name);

    jclass wrapper_cls = env->FindClass("bfwrapper");
    if (wrapper_cls == nullptr)
    {
        std::cerr << "Error: bfwrapper Class not found." << std::endl;
        jvm->DestroyJavaVM();
        exit(EXIT_FAILURE);
    }

    jmethodID constructor = env->GetMethodID(wrapper_cls, "<init>", "()V");
    jobject wrapper_instance = env->NewObject(wrapper_cls, constructor);

    // getMetadata
    jmethodID getMetadataMethod =
        env->GetMethodID(wrapper_cls, "getMetadata", "(Ljava/lang/String;)Ljava/lang/String;");
    jstring filePathJava = env->NewStringUTF(filePath);
    jstring metadata = (jstring)env->CallObjectMethod(wrapper_instance, getMetadataMethod, filePathJava);
    if (metadata != nullptr)
    {
        const char* metadataChars = env->GetStringUTFChars(metadata, nullptr);
        std::cout << metadataChars << std::endl;
        env->ReleaseStringUTFChars(metadata, metadataChars);
    }
    else
        std::cerr << "Error retrieving metadata" << std::endl;

    // getOMEXML
    jmethodID getOMEXMLMethod = env->GetMethodID(wrapper_cls, "getOMEXML", "(Ljava/lang/String;)Ljava/lang/String;");
    jstring xmldata = (jstring)env->CallObjectMethod(wrapper_instance, getOMEXMLMethod, filePathJava);
    if (xmldata != nullptr)
    {
        const char* xmldataChars = env->GetStringUTFChars(xmldata, nullptr);
        std::cout << xmldataChars << std::endl;
        env->ReleaseStringUTFChars(xmldata, xmldataChars);
    }
    else
        std::cerr << "Error retrieving xmldata" << std::endl;

    jvm->DestroyJavaVM();
    return 0;
}