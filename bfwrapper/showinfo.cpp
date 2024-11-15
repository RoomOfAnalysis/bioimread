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

// https://github.com/ome/bioformats/blob/develop/components/formats-api/src/loci/formats/FormatTools.java#L836
int pixelType2CVType(jint pixelType)
{
    static int type[9]{CV_8S, CV_8U, CV_16S, CV_16U, CV_32S, CV_32S, CV_32F, CV_64F, CV_8S};

    assert(0 <= pixelType && pixelType <= 8);
    return type[pixelType];
}

template <typename T> std::ostream& operator<<(std::ostream& os, std::vector<T> const& v)
{
    os << "[ ";
    for (auto const& e : v)
        os << e << " ";
    os << "]\n";
    return os;
}

template <> std::ostream& operator<<(std::ostream& os, std::vector<unsigned char> const& v)
{
    os << "[ ";
    for (int e : v)
        os << e << " ";
    os << "]\n";
    return os;
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

    // getEffectiveSizeC()
    jmethodID getEffectiveSizeCMethod = env->GetMethodID(wrapper_cls, "getEffectiveSizeC", "()I");
    jint sizeC = env->CallIntMethod(wrapper_instance, getEffectiveSizeCMethod);
    std::cout << "getEffectiveSizeC: " << sizeC << std::endl;

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

    std::vector<std::array<int, 4>> cc(sizeC);
    // getChannelColor()
    jmethodID getChannelColorMethod = env->GetMethodID(wrapper_cls, "getChannelColor", "(I)[I");
    for (jint c = 0; c < sizeC; c++)
    {
        jintArray channelColor = (jintArray)env->CallObjectMethod(wrapper_instance, getChannelColorMethod, c);
        if (channelColor != nullptr)
        {
            jsize len = env->GetArrayLength(channelColor);
            assert(len == 4);

            jint* body = env->GetIntArrayElements(channelColor, nullptr);
            std::memcpy(cc[c].data(), body, sizeof(jint) * len);
            env->ReleaseIntArrayElements(channelColor, body, JNI_ABORT);
            std::cout << cc[c][0] << ", " << cc[c][1] << ", " << cc[c][2] << ", " << cc[c][3] << std::endl;
        }
    }

    cv::Mat lut;

    // get8BitLookupTable
    jmethodID get8BitLookupTableMethod = env->GetMethodID(wrapper_cls, "get8BitLookupTable", "()[[B");
    jobjectArray bytesArray = (jobjectArray)env->CallObjectMethod(wrapper_instance, get8BitLookupTableMethod);
    if (bytesArray != nullptr)
    {
        std::vector<std::vector<unsigned char>> lut_v;
        jsize len = env->GetArrayLength(bytesArray);
        jsize len1 = env->GetArrayLength((jbyteArray)env->GetObjectArrayElement(bytesArray, 0));
        lut_v.resize(len, std::vector<unsigned char>(len1));

        for (jsize i = 0; i < len; i++)
        {
            jbyteArray byteArray = (jbyteArray)env->GetObjectArrayElement(bytesArray, i);
            jbyte* body = env->GetByteArrayElements(byteArray, nullptr);

            std::memcpy(lut_v[i].data(), body, sizeof(jbyte) * len1);

            env->ReleaseByteArrayElements(byteArray, body, JNI_ABORT);
        }
        std::cout << lut_v << std::endl;

        assert(len == 3);
        std::vector<unsigned char> lut_1d;
        lut_1d.reserve(len * len1);
        for (jsize i = 0; i < len; i++)
            lut_1d.insert(lut_1d.end(), lut_v[i].cbegin(), lut_v[i].cend());
        lut = cv::Mat(1, len1, CV_8UC3, lut_1d.data());
    }
    env->DeleteLocalRef(bytesArray);

    // get16BitLookupTable
    jmethodID get16BitLookupTableMethod = env->GetMethodID(wrapper_cls, "get16BitLookupTable", "()[[S");
    jobjectArray shortsArray = (jobjectArray)env->CallObjectMethod(wrapper_instance, get16BitLookupTableMethod);
    if (shortsArray != nullptr)
    {
        std::vector<std::vector<short>> lut_v;
        jsize len = env->GetArrayLength(shortsArray);
        jsize len1 = env->GetArrayLength((jshortArray)env->GetObjectArrayElement(shortsArray, 0));
        lut_v.resize(len, std::vector<short>(len1));

        for (jsize i = 0; i < len; i++)
        {
            jshortArray shortArray = (jshortArray)env->GetObjectArrayElement(shortsArray, i);
            jshort* body = env->GetShortArrayElements(shortArray, nullptr);

            std::memcpy(lut_v[i].data(), body, sizeof(jshort) * len1);

            env->ReleaseShortArrayElements(shortArray, body, JNI_ABORT);
        }
        std::cout << lut_v << std::endl;
        // TODO: 16bit lut
    }
    env->DeleteLocalRef(shortsArray);

#if 1
    {
        // single plane image bytes
        auto* bytes = new char[planeSize];

        cv::namedWindow("plane");

        size_t total_bytes{};

        int cv_type = pixelType2CVType(pixelType);

        // use openPlane to eliminate endian problem
        {
            //TIME_BLOCK("openPlane");
            jmethodID openPlaneMethod = env->GetMethodID(wrapper_cls, "openPlane", "(I)[B");
            jmethodID getZCTCoordsMethod = env->GetMethodID(wrapper_cls, "getZCTCoords", "(I)[I");
            for (jint plane = 0; plane < planes; plane++)
            {
                jintArray ZCTCoords = (jintArray)env->CallObjectMethod(wrapper_instance, getZCTCoordsMethod, plane);
                int z = 0, c = 0, t = 0;
                if (ZCTCoords != nullptr)
                {
                    jsize len = env->GetArrayLength(ZCTCoords);
                    assert(len == 3);

                    jint* body = env->GetIntArrayElements(ZCTCoords, nullptr);
                    z = body[0];
                    c = body[1];
                    t = body[2];
                    env->ReleaseIntArrayElements(ZCTCoords, body, JNI_ABORT);
                    std::cout << "z: " << z << ", c: " << c << ", t: " << t << std::endl;
                }

                jbyteArray byteArray = (jbyteArray)env->CallObjectMethod(wrapper_instance, openPlaneMethod, plane);
                if (byteArray != nullptr)
                {
                    jsize len = env->GetArrayLength(byteArray);
                    jbyte* body = env->GetByteArrayElements(byteArray, nullptr);

                    std::memcpy(bytes, body, sizeof(jbyte) * len);

                    total_bytes += sizeof(jbyte) * len;

                    env->ReleaseByteArrayElements(byteArray, body, JNI_ABORT);

                    auto img = cv::Mat(sizeY, sizeX, cv_type, bytes);
                    cv::imshow("plane", img);
                    if (cv::waitKey(0) == 'q') break;
                }
                else
                {
                    std::cerr << "Error retrieving bytesdata at plane: " << plane << std::endl;
                    break;
                }
            }
        }

        delete[] bytes;
        std::cout << "read " << total_bytes << " bytes with openBytes" << std::endl;
    }
#endif

    jvm_wrapper->destroyJVM();
    return 0;
}