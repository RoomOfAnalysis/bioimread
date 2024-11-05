#include <QApplication>
#include <QFileInfo>

#include <jni.h>

#include <Windows.h>

#include <iostream>

typedef int(__stdcall* JNI_CreateJavaVMFunc)(JavaVM** pvm, JNIEnv** penv, void* args);

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    auto java_home = qgetenv("JAVA_HOME");
    auto jvm_dll_path = java_home + "/server/jvm.dll";

    // https://github.com/mitchdowd/jnipp/blob/master/jnipp.cpp#L1489
    HMODULE jvmDLL = LoadLibraryA(jvm_dll_path.toStdString().c_str());
    if (jvmDLL == 0)
    {
        std::cerr << "failed to load jvm.dll" << std::endl;
        exit(EXIT_FAILURE);
    }

    JNI_CreateJavaVMFunc Dll_JNI_CreateJavaVM = (JNI_CreateJavaVMFunc)GetProcAddress(jvmDLL, "JNI_CreateJavaVM");
    if (Dll_JNI_CreateJavaVM == 0)
    {
        std::cerr << "failed to get jni create jvm func" << std::endl;
        exit(EXIT_FAILURE);
    }

    JavaVM* jvm = nullptr;
    JNIEnv* env = nullptr;

    JavaVMInitArgs vm_args{};
    JavaVMOption* options = new JavaVMOption[1];
    auto jar_path =
        ("-Djava.class.path=" + QApplication::applicationDirPath() + "/bioformats_package.jar").toStdString();
    options[0].optionString = const_cast<char*>(jar_path.c_str()); // "-verbose:jni"

    vm_args.version = JNI_VERSION_1_8;
    vm_args.nOptions = 1;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = false;

    // Is your debugger catching an error here?  This is normal.  Just continue. The JVM
    // intentionally does this to test how the OS handles memory - reference exceptions.
    jint rc = Dll_JNI_CreateJavaVM(&jvm, &env, &vm_args);
    delete[] options;
    if (rc != JNI_OK)
    {
        qDebug() << "jni failed to create jvm";
        exit(EXIT_FAILURE);
    }
    jint ver = env->GetVersion();
    std::cout << "JVM load succeeded: Version ";
    std::cout << ((ver >> 16) & 0x0f) << "." << (ver & 0x0f) << std::endl;

    if (jclass cls = env->FindClass("loci/formats/tools/CommandLineTools"); cls == nullptr)
        std::cerr << "can NOT find bioformats" << std::endl;
    else if (jmethodID method_handle = env->GetStaticMethodID(cls, "printVersion", "()V"); method_handle == nullptr)
        std::cerr << "can NOT get method" << std::endl;
    else
        env->CallStaticObjectMethodV(cls, method_handle, {});

    jvm->DestroyJavaVM();
    FreeLibrary(jvmDLL);

    return 0;

    // QFileInfo info(argv[1]);
    // if (info.exists())
    // {
    // }
    // else
    //     std::cout << "invalid filepath: " << argv[1] << std::endl;

    // return app.exec();
}