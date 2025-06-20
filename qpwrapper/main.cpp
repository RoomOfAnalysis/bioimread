#include <jni.h>
#include <iostream>
#include <cassert>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "jvmwrapper.hpp"
#include "stopwatch.hpp"

#define SHOW_IMAGES

std::string pixelTypeStr(jint pixelType)
{
    static std::string typeStr[8]{"uint8", "int8", "uint16", "int16", "uint32", "int32", "float", "double"};

    assert(0 <= pixelType && pixelType <= 7);
    return typeStr[pixelType];
}

// https://github.com/qupath/qupath/blob/main/qupath-core/src/main/java/qupath/lib/images/servers/PixelType.java#L30
int pixelType2CVType(jint pixelType)
{
    static int type[8]{CV_8U, CV_8S, CV_16U, CV_16S, CV_32S, CV_32S, CV_32F, CV_64F};

    assert(0 <= pixelType && pixelType <= 7);
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

    jclass wrapper_cls = env->FindClass("qpwrapper");
    if (wrapper_cls == nullptr)
    {
        std::cerr << "Error: qpwrapper Class not found." << std::endl;
        jvm_wrapper->destroyJVM();
        exit(EXIT_FAILURE);
    }

    jmethodID constructor = env->GetMethodID(wrapper_cls, "<init>", "(Ljava/lang/String;)V");
    jstring filepath = env->NewStringUTF(filePath);
    jobject wrapper_instance = env->NewObject(wrapper_cls, constructor, filepath);
    env->DeleteLocalRef(filepath);
    if (wrapper_instance == nullptr)
    {
        std::cerr << "Error: Failed to create qpwrapper instance." << std::endl;
        jvm_wrapper->destroyJVM();
        exit(EXIT_FAILURE);
    }

    // getMetadata()
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

    // getOMEXML()
    jmethodID getOMEXLMethod = env->GetMethodID(wrapper_cls, "getOMEXML", "()Ljava/lang/String;");
    jstring omexml = (jstring)env->CallObjectMethod(wrapper_instance, getOMEXLMethod);
    if (omexml != nullptr)
    {
        const char* omexmlChars = env->GetStringUTFChars(omexml, nullptr);
        std::cout << omexmlChars << std::endl;
        env->ReleaseStringUTFChars(omexml, omexmlChars);
    }
    else
        std::cerr << "Error retrieving OME-XML" << std::endl;
    env->DeleteLocalRef(omexml);

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

    // getSizeT()
    jmethodID getSizeTMethod = env->GetMethodID(wrapper_cls, "getSizeT", "()I");
    jint sizeT = env->CallIntMethod(wrapper_instance, getSizeTMethod);
    std::cout << "getSizeT: " << sizeT << std::endl;

    // isRGB()
    jmethodID isRGBMethod = env->GetMethodID(wrapper_cls, "isRGB", "()Z");
    jboolean isRGB = (jboolean)env->CallBooleanMethod(wrapper_instance, isRGBMethod);
    std::cout << "isRGB: " << std::boolalpha << (isRGB == JNI_TRUE) << std::endl;

    // getBytesPerPixel()
    jmethodID getBytesPerPixelMethod = env->GetMethodID(wrapper_cls, "getBytesPerPixel", "()I");
    jint bytesPerPixel = env->CallIntMethod(wrapper_instance, getBytesPerPixelMethod);
    std::cout << "getBytesPerPixel: " << bytesPerPixel << std::endl;

    // getAssociatedImageNames()
    jmethodID getAssociatedImageNamesMethod =
        env->GetMethodID(wrapper_cls, "getAssociatedImageNames", "()[Ljava/lang/String;");
    jobjectArray associatedImageNames =
        (jobjectArray)env->CallObjectMethod(wrapper_instance, getAssociatedImageNamesMethod);
    std::vector<std::string> associatedImageNamesVec;
    if (associatedImageNames != nullptr)
    {
        jsize length = env->GetArrayLength(associatedImageNames);
        std::cout << "getAssociatedImageNames: " << length << " images" << std::endl;
        associatedImageNamesVec.reserve(length);
        for (jsize i = 0; i < length; i++)
        {
            jstring name = (jstring)env->GetObjectArrayElement(associatedImageNames, i);
            if (auto* nameChars = env->GetStringUTFChars(name, nullptr); nameChars)
            {
                std::cout << '\t' << nameChars << std::endl;
                associatedImageNamesVec.emplace_back(nameChars);
                env->ReleaseStringUTFChars(name, nameChars);
                env->DeleteLocalRef(name);
            }
        }
    }
    env->DeleteLocalRef(associatedImageNames);

    // getAssociatedImage()
    jmethodID getAssociatedImageMethod = env->GetMethodID(wrapper_cls, "getAssociatedImage", "(Ljava/lang/String;)[B");
    // getAssociatedImageSize()
    jmethodID getAssociatedImageSize =
        env->GetMethodID(wrapper_cls, "getAssociatedImageSize", "(Ljava/lang/String;)[I");
    for (auto const& name : associatedImageNamesVec)
    {
        jstring nameStr = env->NewStringUTF(name.c_str());
        jbyteArray imageBytes = (jbyteArray)env->CallObjectMethod(wrapper_instance, getAssociatedImageMethod, nameStr);
        jintArray imageSize = (jintArray)env->CallObjectMethod(wrapper_instance, getAssociatedImageSize, nameStr);
        int width = 0, height = 0;
        if (imageSize != nullptr)
        {
            jsize length = env->GetArrayLength(imageSize);
            assert(length == 2);

            jint* size = env->GetIntArrayElements(imageSize, nullptr);
            width = size[0];
            height = size[1];
            env->ReleaseIntArrayElements(imageSize, size, 0);
        }
        env->DeleteLocalRef(imageSize);
        std::cout << "getAssociatedImageSize: " << name << " " << width << "x" << height << std::endl;
        if (imageBytes != nullptr)
        {
            jsize length = env->GetArrayLength(imageBytes);
            std::cout << "getAssociatedImage: " << name << " " << length << " bytes" << std::endl;
            std::vector<uchar> bytes(length);
            env->GetByteArrayRegion(imageBytes, 0, length, (jbyte*)bytes.data());

#ifdef SHOW_IMAGES
            cv::Mat img = cv::imdecode(bytes, cv::IMREAD_UNCHANGED);
            cv::imshow(name, img);
            if (cv::waitKey(0) == 'q') break;
#endif
        }
        env->DeleteLocalRef(imageBytes);
        env->DeleteLocalRef(nameStr);
    }

    // nResolutions()
    jmethodID nResolutionsMethod = env->GetMethodID(wrapper_cls, "nResolutions", "()I");
    jint nResolutions = env->CallIntMethod(wrapper_instance, nResolutionsMethod);
    std::cout << "nResolutions: " << nResolutions << std::endl;

    // getDownsampleForResolution()
    jmethodID getDownsampleForResolutionMethod = env->GetMethodID(wrapper_cls, "getDownsampleForResolution", "(I)D");
    jdouble downsample = 0.0;
    for (jint i = 0; i < nResolutions; i++)
    {
        downsample = env->CallDoubleMethod(wrapper_instance, getDownsampleForResolutionMethod, i);
        std::cout << "getDownsampleForResolution: " << i << " " << downsample << std::endl;
    }

    // getSizeForResolution()
    jmethodID getSizeForResolutionMethod = env->GetMethodID(wrapper_cls, "getSizeForResolution", "(I)[I");
    jint size_x = 0, size_y = 0;
    for (jint i = 0; i < nResolutions; i++)
    {
        jintArray sizeArray = (jintArray)env->CallObjectMethod(wrapper_instance, getSizeForResolutionMethod, i);
        if (sizeArray)
        {
            jsize length = env->GetArrayLength(sizeArray);
            assert(length == 2);

            jint* size = env->GetIntArrayElements(sizeArray, nullptr);
            size_x = size[0];
            size_y = size[1];
            std::cout << "getSizeForResolution: " << i << " " << size_x << "x" << size_y << std::endl;

            env->ReleaseIntArrayElements(sizeArray, size, 0);
        }
        env->DeleteLocalRef(sizeArray);
    }

    // readTile()
    jmethodID readTileMethod = env->GetMethodID(wrapper_cls, "readTile", "(IIIIIII)[B");
    jbyteArray tileBytes =
        (jbyteArray)env->CallObjectMethod(wrapper_instance, readTileMethod, nResolutions - 1, 0, 0, 256, 256, 0, 0);
    if (tileBytes != nullptr)
    {
        jsize length = env->GetArrayLength(tileBytes);
        std::cout << "readTile: " << length << " bytes" << std::endl;
        std::vector<uchar> bytes(length);
        env->GetByteArrayRegion(tileBytes, 0, length, (jbyte*)bytes.data());

#ifdef SHOW_IMAGES
        cv::Mat img = cv::imdecode(bytes, cv::IMREAD_UNCHANGED);
        cv::imshow("getTile", img);
        if (cv::waitKey(0) == 'q')
            ;
#endif
    }
    env->DeleteLocalRef(tileBytes);

    // readRegion()
    jmethodID readRegionMethod = env->GetMethodID(wrapper_cls, "readRegion", "(DIIIIII)[B");
    jbyteArray regionBytes = (jbyteArray)env->CallObjectMethod(
        wrapper_instance, readRegionMethod, 1.0, (sizeX - size_x) / 2, (sizeY - size_y) / 2, size_x, size_y, 0, 0);
    if (regionBytes != nullptr)
    {
        jsize length = env->GetArrayLength(regionBytes);
        std::cout << "readRegion: " << length << " bytes" << std::endl;
        std::vector<uchar> bytes(length);
        env->GetByteArrayRegion(regionBytes, 0, length, (jbyte*)bytes.data());

#ifdef SHOW_IMAGES
        cv::Mat img = cv::imdecode(bytes, cv::IMREAD_UNCHANGED);
        cv::imshow("readRegion", img);
        if (cv::waitKey(0) == 'q')
            ;
#endif
    }
    env->DeleteLocalRef(regionBytes);

    jvm_wrapper->destroyJVM();
    return 0;
}