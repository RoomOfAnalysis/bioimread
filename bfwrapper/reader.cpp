#include "reader.hpp"

#include "jvmwrapper.hpp"

#include <cassert>
#include <iostream>

struct Reader::impl
{
    JVMWrapper* jvm_wrapper = nullptr;
    JNIEnv* jvm_env = nullptr;
    jclass wrapper_cls = nullptr;       // global reference
    jobject wrapper_instance = nullptr; // global reference
    jclass system_cls = nullptr;        // global reference

    struct meta
    {
        int series_count{};
        int series{-1};
        int image_count{};
        int size_x{};
        int size_y{};
        int size_z{};
        int size_c{};
        int size_t{};
        double physical_size_x{};
        double physical_size_y{};
        double physical_size_z{};
        double physical_size_t{};
        Reader::PixelType pixel_type{};
        int rgb_channel_count{};
        std::vector<std::optional<std::array<int, 4>>> channel_colors{};
        int plane_size{};

        void PrintSelf() const;
    };
    meta m_meta{};

    bool open(std::string filePath);
    void close();
    bool reopen();
    void setSeries(int no);
    int getImageCount();
    int getSeriesCount();
    int getSeries();
    int getSizeX();
    int getSizeY();
    int getSizeZ();
    // effective
    int getSizeC();
    int getSizeT();
    // mm
    double getPhysSizeX();
    // mm
    double getPhysSizeY();
    // mm
    double getPhysSizeZ();
    // s
    double getPhysSizeT();
    PixelType getPixelType();
    int getRGBChannelCount();
    std::optional<std::array<int, 4>> getChannelColor(int channel);
    int getPlaneSize();
    int getPlaneIndex(int z, int c, int t);
    std::array<int, 3> getZCTCoords(int index);
    std::unique_ptr<char[]> getPlane(int no);
    std::unique_ptr<std::vector<std::array<unsigned char, 3>>> get8BitLut();
    std::unique_ptr<std::vector<std::array<short, 3>>> get16BitLut();

    void force_gc();
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
    pimpl->system_cls = pimpl->jvm_wrapper->findClass("java/lang/System");
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
        pimpl->jvm_env->DeleteGlobalRef(pimpl->system_cls);
        pimpl->wrapper_cls = nullptr;
        pimpl->wrapper_instance = nullptr;
        pimpl->system_cls = nullptr;
    }
    pimpl = nullptr;
}

bool Reader::open(std::string filePath)
{
    if (!pimpl) return false;
    return pimpl->open(std::move(filePath));
}

void Reader::close()
{
    if (pimpl) pimpl->close();
}

bool Reader::reopen()
{
    if (!pimpl) return false;
    return pimpl->reopen();
}

void Reader::setSeries(int no)
{
    pimpl->setSeries(no);
}

int Reader::getImageCount() const
{
    return pimpl->m_meta.image_count;
}

int Reader::getSeriesCount() const
{
    return pimpl->m_meta.series_count;
}

int Reader::getSeries() const
{
    return pimpl->m_meta.series;
}

int Reader::getSizeX() const
{
    return pimpl->m_meta.size_x;
}

int Reader::getSizeY() const
{
    return pimpl->m_meta.size_y;
}

int Reader::getSizeZ() const
{
    return pimpl->m_meta.size_z;
}

int Reader::getSizeC() const
{
    return pimpl->m_meta.size_c;
}

int Reader::getSizeT() const
{
    return pimpl->m_meta.size_t;
}

double Reader::getPhysSizeX() const
{
    return pimpl->m_meta.physical_size_x;
}

double Reader::getPhysSizeY() const
{
    return pimpl->m_meta.physical_size_y;
}

double Reader::getPhysSizeZ() const
{
    return pimpl->m_meta.physical_size_z;
}

double Reader::getPhysSizeT() const
{
    return pimpl->m_meta.physical_size_t;
}

Reader::PixelType Reader::getPixelType() const
{
    return pimpl->m_meta.pixel_type;
}

int Reader::getBytesPerPixel() const
{
    return getBytesPerPixel(getPixelType());
}

int Reader::getRGBChannelCount() const
{
    return pimpl->m_meta.rgb_channel_count;
}

std::optional<std::array<int, 4>> Reader::getChannelColor(int channel) const
{
    return pimpl->m_meta.channel_colors[channel];
}

int Reader::getPlaneSize() const
{
    return pimpl->m_meta.plane_size;
}

int Reader::getPlaneIndex(int z, int c, int t) const
{
    return pimpl->getPlaneIndex(z, c, t);
}

std::array<int, 3> Reader::getZCTCoords(int index) const
{
    return pimpl->getZCTCoords(index);
}

std::unique_ptr<char[]> Reader::getPlane(int no) const
{
    return pimpl->getPlane(no);
}

std::unique_ptr<std::vector<std::array<unsigned char, 3>>> Reader::get8BitLut() const
{
    return pimpl->get8BitLut();
}

std::unique_ptr<std::vector<std::array<short, 3>>> Reader::get16BitLut() const
{
    return pimpl->get16BitLut();
}

std::string Reader::pixelTypeStr(PixelType pt)
{
    static std::string typeStr[9]{"int8", "uint8", "int16", "uint16", "int32", "uint32", "float", "double", "bit"};

    int pixelType = static_cast<int>(pt);
    assert(0 <= pixelType && pixelType <= 8);

    return typeStr[pixelType];
}

int Reader::getBytesPerPixel(PixelType pixelType)
{
    switch (pixelType)
    {
    case PixelType::INT8:
        [[fallthrough]];
    case PixelType::UINT8:
        [[fallthrough]];
    case PixelType::BIT:
        return 1;
    case PixelType::INT16:
        [[fallthrough]];
    case PixelType::UINT16:
        return 2;
    case PixelType::INT32:
        [[fallthrough]];
    case PixelType::UINT32:
        [[fallthrough]];
    case PixelType::FLOAT:
        return 4;
    case PixelType::DOUBLE:
        return 8;
    }
    return 1;
}

void Reader::impl::meta::PrintSelf() const
{
    std::cout << "series_count: " << series_count << "\nseries: " << series << "\nimage_count: " << image_count
              << "\nsize_x: " << size_x << "\nsize_y: " << size_y << "\nsize_z: " << size_z << "\nsize_c: " << size_c
              << "\nsize_t: " << size_t << "\nphysical_size_x: " << physical_size_x
              << "\nphysical_size_y: " << physical_size_y << "\nphysical_size_z: " << physical_size_z
              << "\nphysical_size_t: " << physical_size_t << "\npixel_type: " << pixelTypeStr(pixel_type)
              << "\nrgb_channel_count: " << rgb_channel_count << "\nplane_size: " << plane_size << "\n";
}

bool Reader::impl::open(std::string filePath)
{
    jstring filePathJava = jvm_env->NewStringUTF(filePath.c_str());
    auto res = jvm_env->CallBooleanMethod(
        wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "setId", "(Ljava/lang/String;)Z"), filePathJava);
    jvm_env->DeleteLocalRef(filePathJava);
    if (res) m_meta.series_count = getSeriesCount();
    return res;
}

void Reader::impl::close()
{
    jvm_env->CallVoidMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "close", "()V"));
    m_meta.series = -1;
}

bool Reader::impl::reopen()
{
    if (auto res =
            jvm_env->CallBooleanMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "reopenFile", "()Z"));
        !res)
        return false;
    else
    {
        m_meta.series_count = getSeriesCount();
        return true;
    }
}

void Reader::impl::setSeries(int no)
{
    if (no < 0 || no >= m_meta.series_count)
    {
        std::cerr << "Error: series " << no << " not in range of [0, " << m_meta.series_count << "]";
        return;
    }
    if (m_meta.series == no) return;
    jvm_env->CallVoidMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "setSeries", "(I)V"), no);
    m_meta.series = no;
    m_meta.image_count = getImageCount();
    m_meta.size_x = getSizeX();
    m_meta.size_y = getSizeY();
    m_meta.size_z = getSizeZ();
    m_meta.size_t = getSizeT();
    m_meta.size_c = getSizeC();
    m_meta.physical_size_x = getPhysSizeX();
    m_meta.physical_size_y = getPhysSizeY();
    m_meta.physical_size_z = getPhysSizeZ();
    m_meta.physical_size_t = getPhysSizeT();
    m_meta.pixel_type = getPixelType();
    m_meta.rgb_channel_count = getRGBChannelCount();
    m_meta.channel_colors.resize(m_meta.size_c);
    for (auto c = 0; c < m_meta.size_c; c++)
        m_meta.channel_colors[c] = getChannelColor(c);
    m_meta.plane_size = getPlaneSize();
}

int Reader::impl::getImageCount()
{
    return jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getImageCount", "()I"));
}

int Reader::impl::getSeriesCount()
{
    return jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getSeriesCount", "()I"));
}

int Reader::impl::getSeries()
{
    return jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getSeries", "()I"));
}

int Reader::impl::getSizeX()
{
    return jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getSizeX", "()I"));
}

int Reader::impl::getSizeY()
{
    return jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getSizeY", "()I"));
}

int Reader::impl::getSizeZ()
{
    return jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getSizeZ", "()I"));
}

int Reader::impl::getSizeC()
{
    return jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getEffectiveSizeC", "()I"));
}

int Reader::impl::getSizeT()
{
    return jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getSizeT", "()I"));
}

double Reader::impl::getPhysSizeX()
{
    return jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getPhysSizeX", "()D"));
}

double Reader::impl::getPhysSizeY()
{
    return jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getPhysSizeY", "()D"));
}

double Reader::impl::getPhysSizeZ()
{
    return jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getPhysSizeZ", "()D"));
}

double Reader::impl::getPhysSizeT()
{
    return jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getPhysSizeT", "()D"));
}

Reader::PixelType Reader::impl::getPixelType()
{
    return static_cast<Reader::PixelType>(
        jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getPixelType", "()I")));
}

int Reader::impl::getRGBChannelCount()
{
    return jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getRGBChannelCount", "()I"));
}

std::optional<std::array<int, 4>> Reader::impl::getChannelColor(int channel)
{
    assert(channel >= 0 && channel < getSizeC());

    std::optional<std::array<int, 4>> res;

    jintArray channelColor = (jintArray)jvm_env->CallObjectMethod(
        wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getChannelColor", "(I)[I"), channel);
    if (channelColor != nullptr)
    {
        jsize len = jvm_env->GetArrayLength(channelColor);
        if (len == 4)
        {
            std::array<int, 4> color{};
            jvm_env->GetIntArrayRegion(channelColor, 0, len, (jint*)color.data());
            res = color;
        }
    }
    jvm_env->DeleteLocalRef(channelColor);
    return res;
}

int Reader::impl::getPlaneSize()
{
    return jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getPlaneSize", "()I"));
}

int Reader::impl::getPlaneIndex(int z, int c, int t)
{
    return jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getPlaneIndex", "(III)I"), z,
                                  c, t);
}

std::array<int, 3> Reader::impl::getZCTCoords(int index)
{
    assert(index >= 0 && index < getImageCount());

    std::array<int, 3> coord{};

    jintArray zct = (jintArray)jvm_env->CallObjectMethod(
        wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getZCTCoords", "(I)[I"), index);

    assert(zct != nullptr);

    jsize len = jvm_env->GetArrayLength(zct);

    assert(len == 3);

    jvm_env->GetIntArrayRegion(zct, 0, len, (jint*)coord.data());
    jvm_env->DeleteLocalRef(zct);
    return coord;
}

std::unique_ptr<char[]> Reader::impl::getPlane(int no)
{
    jbyteArray byteArray = (jbyteArray)jvm_env->CallObjectMethod(
        wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "openPlane", "(I)[B"), no);

    assert(byteArray != nullptr);

    jsize len = jvm_env->GetArrayLength(byteArray);
    auto bytes = std::make_unique<char[]>(sizeof(jbyte) * len);
    jvm_env->GetByteArrayRegion(byteArray, 0, len, (jbyte*)bytes.get());
    jvm_env->DeleteLocalRef(byteArray);

    //force_gc();  // 340M -> 170M with 11-12ms time cost (non-force: 1-2ms time cost)

    return bytes;
}

std::unique_ptr<std::vector<std::array<unsigned char, 3>>> Reader::impl::get8BitLut()
{
    auto lut = std::make_unique<std::vector<std::array<unsigned char, 3>>>();
    jmethodID get8BitLookupTableMethod = jvm_env->GetMethodID(wrapper_cls, "get8BitLookupTable", "()[[B");
    jobjectArray bytesArray = (jobjectArray)jvm_env->CallObjectMethod(wrapper_instance, get8BitLookupTableMethod);
    if (bytesArray != nullptr)
    {
        assert(jvm_env->GetArrayLength(bytesArray) == 3); // RGB 3 channels

        auto b0 = (jbyteArray)jvm_env->GetObjectArrayElement(bytesArray, 0);
        jsize len = jvm_env->GetArrayLength(b0);
        jvm_env->DeleteLocalRef(b0);
        lut->resize(len);

        std::vector<unsigned char> buffer(len);
        for (jsize i = 0; i < 3; i++)
        {
            jbyteArray b = (jbyteArray)jvm_env->GetObjectArrayElement(bytesArray, i);
            jvm_env->GetByteArrayRegion(b, 0, len, (jbyte*)buffer.data());
            jvm_env->DeleteLocalRef(b);

            for (auto j = 0; j < len; j++)
                (*lut)[j][i] = buffer[j];
        }
    }
    jvm_env->DeleteLocalRef(bytesArray);
    return lut;
}

std::unique_ptr<std::vector<std::array<short, 3>>> Reader::impl::get16BitLut()
{
    auto lut = std::make_unique<std::vector<std::array<short, 3>>>();
    jmethodID get16BitLookupTableMethod = jvm_env->GetMethodID(wrapper_cls, "get16BitLookupTable", "()[[S");
    jobjectArray bytesArray = (jobjectArray)jvm_env->CallObjectMethod(wrapper_instance, get16BitLookupTableMethod);
    if (bytesArray != nullptr)
    {
        assert(jvm_env->GetArrayLength(bytesArray) == 3); // RGB 3 channels

        auto b0 = (jshortArray)jvm_env->GetObjectArrayElement(bytesArray, 0);
        jsize len = jvm_env->GetArrayLength(b0);
        jvm_env->DeleteLocalRef(b0);
        lut->resize(len);

        std::vector<short> buffer(len);
        for (jsize i = 0; i < 3; i++)
        {
            jshortArray b = (jshortArray)jvm_env->GetObjectArrayElement(bytesArray, i);
            jvm_env->GetShortArrayRegion(b, 0, len, (jshort*)buffer.data());
            jvm_env->DeleteLocalRef(b);

            for (auto j = 0; j < len; j++)
                (*lut)[j][i] = buffer[j];
        }
    }
    jvm_env->DeleteLocalRef(bytesArray);
    return lut;
}

void Reader::impl::force_gc()
{
    jvm_env->CallStaticVoidMethod(system_cls, jvm_env->GetStaticMethodID(system_cls, "gc", "()V"));
}
