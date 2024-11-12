#include <jni.h>
#include <iostream>

#include "jvmwrapper.hpp"
#include "stopwatch.hpp"

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <path_to_image_file>" << std::endl;
        return 1;
    }

    const char* filePath = argv[1];

    auto* jvm_wrapper = JVMWrapper::getInstance();
    auto* env = jvm_wrapper->getJNIEnv();

    // set log level
    // https://bio-formats.readthedocs.io/en/v8.0.0/developers/logging.html
    jclass log_cls = env->FindClass("org/slf4j/Logger");
    if (log_cls == nullptr)
    {
        std::cerr << "Error: bioformats package not loaded." << std::endl;
        jvm_wrapper->destroyJVM();
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
        jvm_wrapper->destroyJVM();
        exit(EXIT_FAILURE);
    }

    jmethodID constructor = env->GetMethodID(wrapper_cls, "<init>", "()V");
    jobject wrapper_instance = env->NewObject(wrapper_cls, constructor);

    // setId
    jmethodID setIdMethod = env->GetMethodID(wrapper_cls, "setId", "(Ljava/lang/String;)Z");
    jstring filePathJava = env->NewStringUTF(filePath);
    jboolean succeed = (jboolean)env->CallBooleanMethod(wrapper_instance, setIdMethod, filePathJava);
    if (!succeed)
    {
        std::cerr << "Error: bfwrapper setId failed." << std::endl;
        jvm_wrapper->destroyJVM();
        exit(EXIT_FAILURE);
    }

    // getMetadata
    jmethodID getMetadataMethod = env->GetMethodID(wrapper_cls, "getMetadata", "()Ljava/lang/String;");
    jstring metadata = (jstring)env->CallObjectMethod(wrapper_instance, getMetadataMethod);
    if (metadata != nullptr)
    {
        const char* metadataChars = env->GetStringUTFChars(metadata, nullptr);
        std::cout << metadataChars << std::endl;
        env->ReleaseStringUTFChars(metadata, metadataChars);
    }
    else
        std::cerr << "Error retrieving metadata" << std::endl;

    // getOMEXML
    jmethodID getOMEXMLMethod = env->GetMethodID(wrapper_cls, "getOMEXML", "()Ljava/lang/String;");
    jstring xmldata = (jstring)env->CallObjectMethod(wrapper_instance, getOMEXMLMethod);
    if (xmldata != nullptr)
    {
        const char* xmldataChars = env->GetStringUTFChars(xmldata, nullptr);
        std::cout << xmldataChars << std::endl;
        env->ReleaseStringUTFChars(xmldata, xmldataChars);
    }
    else
        std::cerr << "Error retrieving xmldata" << std::endl;

    // getSeriesCount
    jmethodID getSeriesCountMethod = env->GetMethodID(wrapper_cls, "getSeriesCount", "()I");
    jint series_cnt = env->CallIntMethod(wrapper_instance, getSeriesCountMethod);
    std::cout << "getSeriesCount: " << series_cnt << std::endl;

    // setSeries
    jmethodID setSeriesMethod = env->GetMethodID(wrapper_cls, "setSeries", "(I)V");
    jint series = 0;
    env->CallVoidMethod(wrapper_instance, setSeriesMethod, series);

    // getSeries
    jmethodID getSeriesMethod = env->GetMethodID(wrapper_cls, "getSeries", "()I");
    jint series_ = env->CallIntMethod(wrapper_instance, getSeriesMethod);
    if (series_ != series)
    {
        std::cerr << "Error: bfwrapper setSeries(" << series << ") but getSeries " << series_ << std::endl;
        jvm_wrapper->destroyJVM();
        exit(EXIT_FAILURE);
    }

    // getImageCount
    jmethodID getImageCountMethod = env->GetMethodID(wrapper_cls, "getImageCount", "()I");
    jint planes = env->CallIntMethod(wrapper_instance, getImageCountMethod);
    std::cout << "getImageCount: " << planes << std::endl;

    // openBytes
    {
        TIME_BLOCK("openBytes");

        size_t total_bytes{};

        jmethodID openBytesMethod = env->GetMethodID(wrapper_cls, "openBytes", "(I)[B");
        for (jint plane = 0; plane < planes; plane++)
        {
            jbyteArray byteArray = (jbyteArray)env->CallObjectMethod(wrapper_instance, openBytesMethod, plane);
            if (byteArray != nullptr)
            {
                // TODO: direct buffer to avoid copy
                jsize len = env->GetArrayLength(byteArray);
                jbyte* body = env->GetByteArrayElements(byteArray, nullptr);
                auto* bytes = new unsigned char[len];
                std::memcpy(bytes, body, sizeof(jbyte) * len);

                total_bytes += len;

                delete[] bytes;
                env->ReleaseByteArrayElements(byteArray, body, JNI_ABORT);
            }
            else
            {
                std::cerr << "Error retrieving bytesdata at plane: " << plane << std::endl;
                break;
            }
        }
        std::cout << "read " << total_bytes << " bytes with openBytes" << std::endl;
    }

    jvm_wrapper->destroyJVM();
    return 0;
}