#include <jni.h>
#include <iostream>
#include <cassert>

#include <opencv2/highgui.hpp>

#include "jvmwrapper.hpp"
#include "stopwatch.hpp"

std::string pixelTypeStr(jint pixelType)
{
    static std::string typeStr[9]{"int8", "uint8", "int16", "uint16", "int32", "uint32", "float", "double", "bit"};

    assert(0 <= pixelType && pixelType <= 8);
    return typeStr[pixelType];
}

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

    // getPixelType()
    jmethodID getPixelTypeMethod = env->GetMethodID(wrapper_cls, "getPixelType", "()I");
    jint pixelType = env->CallIntMethod(wrapper_instance, getPixelTypeMethod);
    std::cout << "getPixelType: " << pixelTypeStr(pixelType) << std::endl;

    // getSizeX()
    jmethodID getSizeXMethod = env->GetMethodID(wrapper_cls, "getSizeX", "()I");
    jint sizeX = env->CallIntMethod(wrapper_instance, getSizeXMethod);
    std::cout << "getSizeX: " << sizeX << std::endl;

    // getSizeY()
    jmethodID getSizeYMethod = env->GetMethodID(wrapper_cls, "getSizeY", "()I");
    jint sizeY = env->CallIntMethod(wrapper_instance, getSizeYMethod);
    std::cout << "getSizeY: " << sizeY << std::endl;

    // getRGBChannelCount()
    jmethodID getRGBChannelCountMethod = env->GetMethodID(wrapper_cls, "getRGBChannelCount", "()I");
    jint channelCount = env->CallIntMethod(wrapper_instance, getRGBChannelCountMethod);
    std::cout << "getRGBChannelCount: " << channelCount << std::endl;

    // getBytesPerPixel()
    jmethodID getBytesPerPixelMethod = env->GetMethodID(wrapper_cls, "getBytesPerPixel", "()I");
    jint bytesPerPixel = env->CallIntMethod(wrapper_instance, getBytesPerPixelMethod);
    std::cout << "getBytesPerPixel: " << bytesPerPixel << std::endl;

    // getPlaneSize()
    jmethodID getPlaneSizeMethod = env->GetMethodID(wrapper_cls, "getPlaneSize", "()I");
    jint planeSize = env->CallIntMethod(wrapper_instance, getPlaneSizeMethod);
    std::cout << "getPlaneSize: " << planeSize << std::endl;

    // getChannelColor()
    jmethodID getChannelColorMethod = env->GetMethodID(wrapper_cls, "getChannelColor", "(I)Ljava/lang/String;");
    jstring channelColor0 = (jstring)env->CallObjectMethod(wrapper_instance, getChannelColorMethod, 0);
    const char* channelColor0Chars = env->GetStringUTFChars(channelColor0, nullptr);
    std::string channelColor0Str = channelColor0Chars;
    std::cout << channelColor0Str << std::endl;
    env->ReleaseStringUTFChars(xmldata, channelColor0Chars);

    // single plane image bytes
    auto* bytes = new char[planeSize];

    cv::namedWindow("plane");
#if 1
    // openBytes
    {
        //TIME_BLOCK("openBytes");

        size_t total_bytes{};

        jmethodID openBytesMethod = env->GetMethodID(wrapper_cls, "openBytes", "(I)[B");
        for (jint plane = 0; plane < planes; plane++)
        {
            jbyteArray byteArray = (jbyteArray)env->CallObjectMethod(wrapper_instance, openBytesMethod, plane);
            if (byteArray != nullptr)
            {
                // direct buffer from C to java caused error at https://github.com/ome/bioformats/blob/develop/components/formats-bsd/src/loci/formats/ImageTools.java#L224
                // java.lang.ArrayStoreException: arraycopy: destination type java.nio.DirectByteBuffer is not an array
                //         at java.base/java.lang.System.arraycopy(Native Method)
                //         at loci.formats.ImageTools.splitChannels(ImageTools.java:224)
                jsize len = env->GetArrayLength(byteArray);
                jbyte* body = env->GetByteArrayElements(byteArray, nullptr);

                std::memcpy(bytes, body, sizeof(jbyte) * len);

                total_bytes += len;

                env->ReleaseByteArrayElements(byteArray, body, JNI_ABORT);

                // show image
                //\note opencv requires little endian... may need swap byte endianess first
                auto img = cv::Mat(sizeY, sizeX, CV_8UC1, bytes);
                cv::imshow("plane", img);
                if (cv::waitKey(0) == 'q') break;
            }
            else
            {
                std::cerr << "Error retrieving bytesdata at plane: " << plane << std::endl;
                break;
            }
        }
        delete[] bytes;
        std::cout << "read " << total_bytes << " bytes with openBytes" << std::endl;
    }
#endif

    jvm_wrapper->destroyJVM();
    return 0;
}