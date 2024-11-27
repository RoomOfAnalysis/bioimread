#include <jni.h>
#include <iostream>
#include <cassert>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "jvmwrapper.hpp"
#include "stopwatch.hpp"

//#define SHOW_PLANES
//#define BENCH_ARR
#define BENCH_RGN
//#define BENCH_CRI
//#define BENCH_DBB

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
    env->DeleteLocalRef(log_level_name);
    env->DeleteLocalRef(root_logger_cls);
    env->DeleteLocalRef(log_level_cls);
    env->DeleteLocalRef(root_logger_name);
    env->DeleteLocalRef(root_logger);
    env->DeleteLocalRef(log_factory_cls);
    env->DeleteLocalRef(log_cls);

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
    env->DeleteLocalRef(filePathJava);

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
    env->DeleteLocalRef(metadata);

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
    env->DeleteLocalRef(xmldata);

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

    // getSizeZ()
    jmethodID getSizeZMethod = env->GetMethodID(wrapper_cls, "getSizeZ", "()I");
    jint sizeZ = env->CallIntMethod(wrapper_instance, getSizeZMethod);
    std::cout << "getSizeZ: " << sizeZ << std::endl;

    // getEffectiveSizeC()
    jmethodID getEffectiveSizeCMethod = env->GetMethodID(wrapper_cls, "getEffectiveSizeC", "()I");
    jint sizeC = env->CallIntMethod(wrapper_instance, getEffectiveSizeCMethod);
    std::cout << "getEffectiveSizeC: " << sizeC << std::endl;

    // getSizeT()
    jmethodID getSizeTMethod = env->GetMethodID(wrapper_cls, "getSizeT", "()I");
    jint sizeT = env->CallIntMethod(wrapper_instance, getSizeTMethod);
    std::cout << "getSizeT: " << sizeT << std::endl;

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
        env->DeleteLocalRef(channelColor);
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
            env->DeleteLocalRef(byteArray);
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
            env->DeleteLocalRef(shortArray);
        }
        std::cout << lut_v << std::endl;
        // TODO: 16bit lut
    }
    env->DeleteLocalRef(shortsArray);

#if 1
    {
        // single plane image bytes
        auto* bytes = new char[planeSize];

#ifdef SHOW_PLANES
        cv::namedWindow("plane");
#endif

        size_t total_bytes{};

        int cv_type = pixelType2CVType(pixelType);

        // use openPlane to eliminate endian problem
        {
            TIME_BLOCK("openPlane");
            jmethodID openPlaneMethod = env->GetMethodID(wrapper_cls, "openPlane", "(I)[B");
            jmethodID getPlaneIndexMethod = env->GetMethodID(wrapper_cls, "getPlaneIndex", "(III)I");
#ifdef SHOW_PLANES
            std::vector<cv::Mat> mats(sizeC);
            cv::Mat img;
#endif
#ifdef BENCH_DBB
            // 4.5s for 2.54GB data
            jmethodID openPlaneWithBufferMethod =
                env->GetMethodID(wrapper_cls, "openPlane", "(ILjava/nio/ByteBuffer;)V");
            jobject dbb = env->NewDirectByteBuffer((void*)bytes, planeSize);
#endif
            for (jint t = 0; t < sizeT; t++)
                for (jint z = 0; z < sizeZ; z++)
                {
                    for (jint c = 0; c < sizeC; c++)
                    {
                        jint plane = (jint)env->CallIntMethod(wrapper_instance, getPlaneIndexMethod, z, c, t);
                        //std::cout << "z: " << z << ", c: " << c << ", t: " << t << ", plane: " << plane << std::endl;

#ifdef BENCH_DBB
                        env->CallVoidMethod(wrapper_instance, openPlaneWithBufferMethod, plane, dbb);
                        total_bytes += planeSize;
#else
                        jbyteArray byteArray =
                            (jbyteArray)env->CallObjectMethod(wrapper_instance, openPlaneMethod, plane);
                        if (byteArray != nullptr)
                        {
                            jsize len = env->GetArrayLength(byteArray);
                            // according to https://rocksdb.org/blog/2023/11/06/java-jni-benchmarks.html
                            // and https://developer.android.com/training/articles/perf-jni#region-calls
                            // `GetByteArrayRegion` has similar performance with `GetPrimitiveArrayCritical` but with less restriction
#ifdef BENCH_ARR
                            // 5.00s for 2.54GB data
                            jbyte* body = env->GetByteArrayElements(byteArray, nullptr);
                            std::memcpy(bytes, body, sizeof(jbyte) * len);
                            env->ReleaseByteArrayElements(byteArray, body, JNI_ABORT);
#elif defined(BENCH_RGN)
                            // 4.47s for 2.54GB data
                            // copy but one direction
                            env->GetByteArrayRegion(byteArray, 0, len, (jbyte*)bytes);
#elif defined(BENCH_CRI)
                            // 4.43s for 2.54GB data
                            // https://docs.oracle.com/en/java/javase/23/docs/specs/jni/functions.html#getprimitivearraycritical
                            // this MAY not copy
                            auto* ptr = env->GetPrimitiveArrayCritical(byteArray, nullptr);
                            std::memcpy(bytes, ptr, sizeof(jbyte) * len);
                            env->ReleasePrimitiveArrayCritical(byteArray, ptr, JNI_ABORT);
#endif

                            total_bytes += sizeof(jbyte) * len;
#ifdef SHOW_PLANES
                            mats[c] = cv::Mat(sizeY, sizeX, cv_type, bytes).clone();
#endif
                        }
                        else
                        {
                            std::cerr << "Error retrieving bytesdata at plane: " << plane << std::endl;
                            break;
                        }
                        env->DeleteLocalRef(byteArray);
#endif
                    }
#ifdef SHOW_PLANES
                    cv::merge(mats, img);
                    // lut seems not good
                    // cv::cvtColor(img, img, cv::COLOR_RGBA2RGB);
                    // cv::LUT(img, lut, img);
                    cv::imshow("plane", img);
                    if (cv::waitKey(0) == 'q') break;
#endif
                }
#ifdef BENCH_DBB
            env->DeleteLocalRef(dbb);
#endif
        }

        delete[] bytes;
        std::cout << "read " << total_bytes << " bytes with openBytes" << std::endl;
    }
#endif

#if 0
    {
        // https://docs.oracle.com/en/java/javase/23/docs/specs/jni/functions.html#newdirectbytebuffer
        // capacity can NOT be larger than  Integer.MAX_VALUE (2147483647 B ~ 1.99GB)
        // the byte order of the returned buffer is ALWAYS big-endian, so i have to change it in bfwrapper
        planes = 511;
        auto buffer_size = (jlong)planes * planeSize;
        std::cout << buffer_size << std::endl;  // 2143289344 B ~ 1.996 GB
        auto buffer = std::make_unique<char[]>(buffer_size);
        jobject dbb = env->NewDirectByteBuffer((void*)buffer.get(), buffer_size);
        if (dbb == nullptr) std::cerr << "Error: failed to create direct byte buffer" << std::endl;
        {
            TIME_BLOCK("openPlanes");

            // 3.7s for 1.996 GB data -> 4.7s for 2.54 GB
            // not faster than `GetByteArrayRegion`
            jmethodID openPlanes = env->GetMethodID(wrapper_cls, "openPlanes", "(IILjava/nio/ByteBuffer;)Z");
            auto succeed = env->CallBooleanMethod(wrapper_instance, openPlanes, 0, planes, dbb);
            std::cout << (bool)succeed << std::endl;
        }
#if 0
        int cv_type = pixelType2CVType(pixelType);
        auto ptr = buffer.get();
        for (auto i = 0; i < planes; i++)
        {
            auto img = cv::Mat(sizeY, sizeX, cv_type, ptr).clone();
            cv::imshow("plane", img);
            ptr += planeSize;
            if (cv::waitKey(0) == 'q') break;
        }
#endif
        env->DeleteLocalRef(dbb);
    }
#endif

#if 0
    {
        // 5.3s for 2.54GB data
        // much slower than direct buffer

        // total 648 planes
        auto buffer_size = (jlong)planes * planeSize;
        auto buffer = std::make_unique<char[]>(buffer_size);
        auto offset = (size_t)511 * planeSize;
        {
            TIME_BLOCK("openPlanes");

            // first 511 planes
            jmethodID openPlanes = env->GetMethodID(wrapper_cls, "openPlanes", "(II)Z");
            auto succeed = env->CallBooleanMethod(wrapper_instance, openPlanes, 0, 511);
            //std::cout << (bool)succeed << std::endl;

            jobject mbb = env->GetObjectField(
                wrapper_instance, env->GetFieldID(wrapper_cls, "mapped_buffer", "Ljava/nio/MappedByteBuffer;"));
            if (mbb == nullptr) std::cerr << "Error: failed to create direct byte buffer, mbb" << std::endl;
            void* mbb_address = env->GetDirectBufferAddress(mbb);
            if (mbb_address == nullptr) std::cerr << "Error: failed to create direct byte buffer, mbb_address" << std::endl;
            //jlong mbb_cap = env->GetDirectBufferCapacity(mbb);
            //if (mbb_cap == 0) std::cerr << "Error: failed to create direct byte buffer, mbb_cap" << std::endl;
            std::memcpy(buffer.get(), mbb_address, offset);

            // remaing 137 planes
            succeed = env->CallBooleanMethod(wrapper_instance, openPlanes, 511, 137);
            //std::cout << (bool)succeed << std::endl;
            std::memcpy(buffer.get() + offset, mbb_address, (size_t)137 * planeSize);

            env->DeleteLocalRef(mbb);
        }
#if 0
        int cv_type = pixelType2CVType(pixelType);
        auto ptr = buffer.get() + offset;
        for (auto i = 511; i < planes; i++)
        {
            auto img = cv::Mat(sizeY, sizeX, cv_type, ptr).clone();
            cv::imshow("plane", img);
            ptr += planeSize;
            if (cv::waitKey(0) == 'q') break;
        }
#endif
    }
#endif

    jvm_wrapper->destroyJVM();
    return 0;
}