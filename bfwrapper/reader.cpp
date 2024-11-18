#include "reader.hpp"

#include "jvmwrapper.hpp"

#include <iostream>

struct Reader::impl
{
    JVMWrapper* jvm_wrapper = nullptr;
    JNIEnv* jvm_env = nullptr;
    jclass wrapper_cls = nullptr;       // global reference
    jobject wrapper_instance = nullptr; // global reference
};

Reader::Reader()
{
    pimpl = std::make_unique<impl>();
    pimpl->jvm_wrapper = JVMWrapper::getInstance();
    pimpl->jvm_env = pimpl->jvm_wrapper->getJNIEnv();
    pimpl->wrapper_cls = pimpl->jvm_wrapper->findClass("bfwrapper");
    if (pimpl->wrapper_cls == nullptr)
    {
        std::cerr << "Error: bfwrapper Class not found." << std::endl;
        pimpl->jvm_wrapper->destroyJVM();
        pimpl = nullptr;
        return;
    }
    if (auto local_ref = pimpl->jvm_env->NewObject(
            pimpl->wrapper_cls, pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "<init>", "()V"));
        local_ref)
    {
        pimpl->wrapper_instance = pimpl->jvm_env->NewGlobalRef(local_ref);
        pimpl->jvm_env->DeleteLocalRef(local_ref);
    }
    else
    {
        std::cerr << "Error: bfwrapper Class instance can not be created." << std::endl;
        pimpl->jvm_wrapper->destroyJVM();
        pimpl = nullptr;
        return;
    }
}

Reader::~Reader()
{
    if (pimpl)
    {
        close();
        pimpl->jvm_env->DeleteGlobalRef(pimpl->wrapper_cls);
        pimpl->jvm_env->DeleteGlobalRef(pimpl->wrapper_instance);
        pimpl->wrapper_cls = nullptr;
        pimpl->wrapper_instance = nullptr;
    }
    pimpl = nullptr;
}

bool Reader::open(std::string filePath)
{
    jstring filePathJava = pimpl->jvm_env->NewStringUTF(filePath.c_str());
    return pimpl->jvm_env->CallBooleanMethod(
        pimpl->wrapper_instance, pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "setId", "(Ljava/lang/String;)Z"),
        filePathJava);
}

void Reader::close()
{
    pimpl->jvm_env->CallVoidMethod(pimpl->wrapper_instance,
                                   pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "close", "()V"));
}

void Reader::setSeries(int no)
{
    pimpl->jvm_env->CallVoidMethod(pimpl->wrapper_instance,
                                   pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "setSeries", "(I)V"), no);
}

int Reader::getImageCount()
{
    return pimpl->jvm_env->CallIntMethod(pimpl->wrapper_instance,
                                         pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "getImageCount", "()I"));
}

int Reader::getSeriesCount()
{
    return pimpl->jvm_env->CallIntMethod(pimpl->wrapper_instance,
                                         pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "getSeriesCount", "()I"));
}

int Reader::getSeries()
{
    return pimpl->jvm_env->CallIntMethod(pimpl->wrapper_instance,
                                         pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "getSeries", "()I"));
}

int Reader::getSizeX()
{
    return pimpl->jvm_env->CallIntMethod(pimpl->wrapper_instance,
                                         pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "getSizeX", "()I"));
}

int Reader::getSizeY()
{
    return pimpl->jvm_env->CallIntMethod(pimpl->wrapper_instance,
                                         pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "getSizeY", "()I"));
}

int Reader::getSizeZ()
{
    return pimpl->jvm_env->CallIntMethod(pimpl->wrapper_instance,
                                         pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "getSizeZ", "()I"));
}

int Reader::getSizeC()
{
    return pimpl->jvm_env->CallIntMethod(
        pimpl->wrapper_instance, pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "getEffectiveSizeC", "()I"));
}

int Reader::getSizeT()
{
    return pimpl->jvm_env->CallIntMethod(pimpl->wrapper_instance,
                                         pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "getSizeT", "()I"));
}

double Reader::getPhysSizeX()
{
    return pimpl->jvm_env->CallIntMethod(pimpl->wrapper_instance,
                                         pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "getPhysSizeX", "()D"));
}

double Reader::getPhysSizeY()
{
    return pimpl->jvm_env->CallIntMethod(pimpl->wrapper_instance,
                                         pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "getPhysSizeY", "()D"));
}

double Reader::getPhysSizeZ()
{
    return pimpl->jvm_env->CallIntMethod(pimpl->wrapper_instance,
                                         pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "getPhysSizeZ", "()D"));
}

double Reader::getPhysSizeT()
{
    return pimpl->jvm_env->CallIntMethod(pimpl->wrapper_instance,
                                         pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "getPhysSizeT", "()D"));
}

int Reader::getPixelType()
{
    return pimpl->jvm_env->CallIntMethod(pimpl->wrapper_instance,
                                         pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "getPixelType", "()I"));
}

int Reader::getRGBChannelCount()
{
    return pimpl->jvm_env->CallIntMethod(
        pimpl->wrapper_instance, pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "getRGBChannelCount", "()I"));
}

std::array<int, 4> Reader::getChannelColor(int channel)
{
    assert(channel >= 0 && channel < getSizeC());

    std::array<int, 4> color{};

    jintArray channelColor = (jintArray)pimpl->jvm_env->CallObjectMethod(
        pimpl->wrapper_instance, pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "getChannelColor", "(I)[I"),
        channel);

    assert(channelColor != nullptr);

    jsize len = pimpl->jvm_env->GetArrayLength(channelColor);

    assert(len == 4);

    jint* body = pimpl->jvm_env->GetIntArrayElements(channelColor, nullptr);
    std::memcpy(color.data(), body, sizeof(jint) * len);
    pimpl->jvm_env->ReleaseIntArrayElements(channelColor, body, JNI_ABORT);
    return color;
}

int Reader::getPlaneSize()
{
    return pimpl->jvm_env->CallIntMethod(pimpl->wrapper_instance,
                                         pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "getPlaneSize", "()I"));
}

int Reader::getPlaneIndex(int z, int c, int t)
{
    return pimpl->jvm_env->CallIntMethod(pimpl->wrapper_instance,
                                         pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "getPlaneIndex", "(III)I"),
                                         z, c, t);
}

std::array<int, 3> Reader::getZCTCoords(int index)
{
    assert(index >= 0 && index < getImageCount());

    std::array<int, 3> coord{};

    jintArray zct = (jintArray)pimpl->jvm_env->CallObjectMethod(
        pimpl->wrapper_instance, pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "getZCTCoords", "(I)[I"), index);

    assert(zct != nullptr);

    jsize len = pimpl->jvm_env->GetArrayLength(zct);

    assert(len == 3);

    jint* body = pimpl->jvm_env->GetIntArrayElements(zct, nullptr);
    std::memcpy(coord.data(), body, sizeof(jint) * len);
    pimpl->jvm_env->ReleaseIntArrayElements(zct, body, JNI_ABORT);
    return coord;
}

cv::Mat Reader::getPlane(int no)
{
    jbyteArray byteArray = (jbyteArray)pimpl->jvm_env->CallObjectMethod(
        pimpl->wrapper_instance, pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "openPlane", "(I)[B"), no);

    assert(byteArray != nullptr);

    jsize len = pimpl->jvm_env->GetArrayLength(byteArray);
    jbyte* body = pimpl->jvm_env->GetByteArrayElements(byteArray, nullptr);

    // TODO: pre-allocate buffer and cache sizeX...
    auto* bytes = new char[sizeof(jbyte) * len];

    std::memcpy(bytes, body, sizeof(jbyte) * len);

    pimpl->jvm_env->ReleaseByteArrayElements(byteArray, body, JNI_ABORT);

    auto res = cv::Mat(getSizeX(), getSizeY(), pixelType2CVType(getPixelType()), bytes).clone();
    delete[] bytes;
    return res;
}

std::string Reader::pixelTypeStr(int pixelType)
{
    static std::string typeStr[9]{"int8", "uint8", "int16", "uint16", "int32", "uint32", "float", "double", "bit"};

    assert(0 <= pixelType && pixelType <= 8);

    return typeStr[pixelType];
}

int Reader::pixelType2CVType(int pixelType)
{
    static int type[9]{CV_8S, CV_8U, CV_16S, CV_16U, CV_32S, CV_32S, CV_32F, CV_64F, CV_8S};

    assert(0 <= pixelType && pixelType <= 8);

    return type[pixelType];
}
