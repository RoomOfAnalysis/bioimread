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

    jvm_wrapper->destroyJVM();
    return 0;
}