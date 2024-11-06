#include <QApplication>
#include <QFileInfo>
#include <QDebug>

#include <jni.h>

#include <Windows.h>

typedef int(__stdcall* JNI_CreateJavaVMFunc)(JavaVM** pvm, JNIEnv** penv, void* args);

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QFileInfo info(argv[1]);
    if (!info.exists()) qDebug() << "image file not exist at:" << info.absoluteFilePath();
    auto img_file_path = info.absoluteFilePath().toStdString();

    auto java_home = qgetenv("JAVA_HOME");
    auto jvm_dll_path = (java_home + "/bin/server/jvm.dll").toStdString();

    // https://github.com/mitchdowd/jnipp/blob/master/jnipp.cpp#L1489
    HMODULE jvmDLL = LoadLibraryA(jvm_dll_path.c_str());
    if (jvmDLL == 0)
    {
        qCritical() << "failed to load jvm.dll at:" << jvm_dll_path;
        exit(EXIT_FAILURE);
    }

    JNI_CreateJavaVMFunc Dll_JNI_CreateJavaVM = (JNI_CreateJavaVMFunc)GetProcAddress(jvmDLL, "JNI_CreateJavaVM");
    if (Dll_JNI_CreateJavaVM == 0)
    {
        qCritical() << "failed to get jni create jvm func";
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
    // https://stackoverflow.com/questions/36250235/exception-0xc0000005-from-jni-createjavavm-jvm-dll/53654317#53654317
    jint rc = Dll_JNI_CreateJavaVM(&jvm, &env, &vm_args);
    delete[] options;
    if (rc != JNI_OK)
    {
        qCritical() << "jni failed to create jvm";
        exit(EXIT_FAILURE);
    }
    jint ver = env->GetVersion();
    qDebug() << "JVM load succeeded: Version" << ((ver >> 16) & 0x0f) << "." << (ver & 0x0f);

    if (jclass cls = env->FindClass("loci/formats/ImageReader"); cls == nullptr)
        qCritical() << "can NOT find bioformats class";
    else
    {
        if (jmethodID constructor = env->GetMethodID(cls, "<init>", "()V"); constructor == nullptr)
            qCritical() << "can NOT get constructor";
        else
        {
            jobject instance = env->NewObject(cls, constructor);

            if (jmethodID method_handle = env->GetMethodID(cls, "getFormat", "(Ljava/lang/String;)Ljava/lang/String;");
                method_handle == nullptr)
                qCritical() << "can NOT get method";
            else
            {
                if (jstring filename = env->NewStringUTF(img_file_path.c_str()); filename == nullptr)
                    qCritical() << "jni bad allc";
                else
                {
                    jstring jstr = static_cast<jstring>(env->CallObjectMethod(instance, method_handle, filename));
                    if (jstr == nullptr) qCritical() << "jstr null";
                    if (env->ExceptionCheck()) env->ExceptionDescribe();
                    char const* str = env->GetStringUTFChars(jstr, nullptr);
                    qDebug() << str;
                    env->ReleaseStringUTFChars(jstr, str);
                    env->DeleteLocalRef(jstr);
                    env->DeleteLocalRef(filename);
                }
            }

            env->DeleteLocalRef(instance);
        }
    }

    jvm->DestroyJavaVM();
    FreeLibrary(jvmDLL);

    return 0; // app.exec();
}