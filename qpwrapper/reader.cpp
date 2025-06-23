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
        int bits_per_pixel{};
        std::vector<std::optional<std::array<int, 4>>> channel_colors{};
        std::vector<std::string> channel_names{};
        int optimal_tile_width{};
        int optimal_tile_height{};
        int level_count{};
        std::vector<std::pair<int, int>> level_dimensions;
        std::vector<double> level_downsamples;
        std::string xml;

        void PrintSelf() const;
    };
    meta m_meta{};

    void open();
    void close();
    std::string getXML();
    int getSizeX();
    int getSizeY();
    int getSizeZ();
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
    int getBitsPerPixel();
    // ARGB
    std::optional<std::array<int, 4>> getChannelColor(int channel);
    std::string getChannelName(int channel);

    int getOptimalTileWidth();
    int getOptimalTileHeight();

    int getLevelCount();
    std::vector<std::pair<int, int>> getLevelDimensions();
    std::vector<double> getLevelDownsamples();
    int getPreferredResolutionLevel(double downsample);
    double getPreferredDownsampleFactor(double downsample);
    // PNG bytes
    std::vector<unsigned char> readRegion(double downsample, int x, int y, int w, int h, int z, int t);
    std::vector<unsigned char> readRegion(int level, int x, int y, int w, int h, int z, int t);
    std::vector<unsigned char> readTile(int level, int x, int y, int w, int h, int z, int t);
    std::vector<unsigned char> getDefaultThumbnail(int z, int t);
    std::vector<std::string> getAssociatedImageNames();
    std::vector<unsigned char> getAssociatedImage(std::string const& name);

    void force_gc();
};

Reader::Reader(std::string filePath)
{
    pimpl = std::make_unique<impl>();
    pimpl->jvm_wrapper = JVMWrapper::getInstance();
    pimpl->jvm_env = pimpl->jvm_wrapper->getJNIEnv();
    pimpl->wrapper_cls = pimpl->jvm_wrapper->findClass("qpwrapper");
    if (pimpl->wrapper_cls == nullptr)
    {
        std::cerr << "Error: bfwrapper Class not found." << std::endl;
        pimpl->jvm_wrapper->destroyJVM();
        pimpl = nullptr;
        return;
    }
    pimpl->system_cls = pimpl->jvm_wrapper->findClass("java/lang/System");
    jstring filepath = pimpl->jvm_env->NewStringUTF(filePath.c_str());
    if (auto local_ref = pimpl->jvm_env->NewObject(
            pimpl->wrapper_cls, pimpl->jvm_wrapper->getMethodID(pimpl->wrapper_cls, "<init>", "(Ljava/lang/String;)V"),
            filepath);
        local_ref)
    {
        pimpl->wrapper_instance = pimpl->jvm_env->NewGlobalRef(local_ref);
        pimpl->jvm_env->DeleteLocalRef(local_ref);
    }
    else
    {
        std::cerr << "Error: qpwrapper Class instance can not be created." << std::endl;
        pimpl->jvm_wrapper->destroyJVM();
        pimpl = nullptr;
    }
    pimpl->jvm_env->DeleteLocalRef(filepath);
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

void Reader::open()
{
    if (pimpl) pimpl->open();
}

void Reader::close()
{
    if (pimpl) pimpl->close();
}

std::string Reader::getMetaXML() const
{
    return pimpl->m_meta.xml;
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

int Reader::getBitsPerPixel() const
{
    return pimpl->m_meta.bits_per_pixel;
}

int Reader::getBytesPerPixel() const
{
    return getBytesPerPixel(getPixelType());
}

std::optional<std::array<int, 4>> Reader::getChannelColor(int channel) const
{
    return pimpl->m_meta.channel_colors[channel];
}

std::string Reader::getChannelName(int channel) const
{
    return pimpl->m_meta.channel_names[channel];
}

int Reader::getOptimalTileWidth() const
{
    return pimpl->m_meta.optimal_tile_width;
}

int Reader::getOptimalTileHeight() const
{
    return pimpl->m_meta.optimal_tile_height;
}

std::string Reader::pixelTypeStr(PixelType pt)
{
    static std::string typeStr[8]{"uint8", "int8", "uint16", "int16", "uint32", "int32", "float", "double"};

    int pixelType = static_cast<int>(pt);
    assert(0 <= pixelType && pixelType <= 7);

    return typeStr[pixelType];
}

int Reader::getBytesPerPixel(PixelType pixelType)
{
    switch (pixelType)
    {
    case PixelType::INT8:
        [[fallthrough]];
    case PixelType::UINT8:
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

int Reader::getLevelCount() const
{
    return pimpl->m_meta.level_count;
}

std::vector<std::pair<int, int>> Reader::getLevelDimensions() const
{
    return pimpl->m_meta.level_dimensions;
}

std::vector<double> Reader::getLevelDownsamples() const
{
    return pimpl->m_meta.level_downsamples;
}

int Reader::getPreferredResolutionLevel(double downsample) const
{
    return pimpl->getPreferredResolutionLevel(downsample);
}

double Reader::getPreferredDownsampleFactor(double downsample) const
{
    return pimpl->getPreferredDownsampleFactor(downsample);
}

std::vector<unsigned char> Reader::readRegion(double downsample, int x, int y, int w, int h, int z, int t) const
{
    return pimpl->readRegion(downsample, x, y, w, h, z, t);
}

std::vector<unsigned char> Reader::readRegion(int level, int x, int y, int w, int h, int z, int t) const
{
    return pimpl->readRegion(level, x, y, w, h, z, t);
}

std::vector<unsigned char> Reader::readTile(int level, int x, int y, int w, int h, int z, int t) const
{
    return pimpl->readTile(level, x, y, w, h, z, t);
}

std::vector<unsigned char> Reader::getDefaultThumbnail(int z, int t) const
{
    return pimpl->getDefaultThumbnail(z, t);
}

std::vector<std::string> Reader::getAssociatedImageNames() const
{
    return pimpl->getAssociatedImageNames();
}

std::vector<unsigned char> Reader::getAssociatedImage(std::string const& name) const
{
    return pimpl->getAssociatedImage(name);
}

void Reader::impl::meta::PrintSelf() const
{
    std::string level_dimensions_str = "[";
    for (auto const& level_dimension : level_dimensions)
        level_dimensions_str +=
            std::to_string(level_dimension.first) + " x " + std::to_string(level_dimension.second) + ", ";
    level_dimensions_str.pop_back();
    level_dimensions_str.back() = ']';
    std::cout << "level_count: " << level_count << "\nlevel_dimensions: " << level_dimensions_str
              << "\ntile_width: " << optimal_tile_width << "\ntile_height: " << optimal_tile_height
              << "\nsize_x: " << size_x << "\nsize_y: " << size_y << "\nsize_z: " << size_z << "\nsize_c: " << size_c
              << "\nsize_t: " << size_t << "\nphysical_size_x: " << physical_size_x
              << "\nphysical_size_y: " << physical_size_y << "\nphysical_size_z: " << physical_size_z
              << "\nphysical_size_t: " << physical_size_t << "\npixel_type: " << pixelTypeStr(pixel_type) << "\n";
}

void Reader::impl::open()
{
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
    m_meta.bits_per_pixel = getBitsPerPixel();
    m_meta.channel_colors.resize(m_meta.size_c);
    m_meta.channel_names.resize(m_meta.size_c);
    for (auto c = 0; c < m_meta.size_c; c++)
    {
        m_meta.channel_colors[c] = getChannelColor(c);
        m_meta.channel_names[c] = getChannelName(c);
    }
    m_meta.optimal_tile_width = getOptimalTileWidth();
    m_meta.optimal_tile_height = getOptimalTileHeight();
    m_meta.level_count = getLevelCount();
    m_meta.level_dimensions = getLevelDimensions();
    m_meta.level_downsamples = getLevelDownsamples();
    m_meta.xml = getXML();
}

void Reader::impl::close()
{
    jvm_env->CallVoidMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "close", "()V"));
}

std::string Reader::impl::getXML()
{
    jstring xmldata = (jstring)jvm_env->CallObjectMethod(
        wrapper_instance, jvm_env->GetMethodID(wrapper_cls, "getOMEXML", "()Ljava/lang/String;"));
    if (xmldata != nullptr)
    {
        const char* xmldataChars = jvm_env->GetStringUTFChars(xmldata, nullptr);
        std::string xml = std::string(xmldataChars);
        jvm_env->ReleaseStringUTFChars(xmldata, xmldataChars);
        return xml;
    }
    else
        std::cerr << "Error retrieving xmldata" << std::endl;
    jvm_env->DeleteLocalRef(xmldata);
    return {};
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
    return jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getSizeC", "()I"));
}

int Reader::impl::getSizeT()
{
    return jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getSizeT", "()I"));
}

double Reader::impl::getPhysSizeX()
{
    return jvm_env->CallDoubleMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getPhysSizeX", "()D"));
}

double Reader::impl::getPhysSizeY()
{
    return jvm_env->CallDoubleMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getPhysSizeY", "()D"));
}

double Reader::impl::getPhysSizeZ()
{
    return jvm_env->CallDoubleMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getPhysSizeZ", "()D"));
}

double Reader::impl::getPhysSizeT()
{
    return jvm_env->CallDoubleMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getPhysSizeT", "()D"));
}

Reader::PixelType Reader::impl::getPixelType()
{
    return static_cast<Reader::PixelType>(
        jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getPixelType", "()I")));
}

std::optional<std::array<int, 4>> Reader::impl::getChannelColor(int channel)
{
    assert(channel >= 0 && channel < getSizeC());

    std::optional<std::array<int, 4>> res;

    jint channelColor = jvm_env->CallIntMethod(
        wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getChannelColor", "(I)I"), channel);
    if (channelColor != -1)
        // https://github.com/qupath/qupath/blob/main/qupath-core/src/main/java/qupath/lib/common/ColorTools.java#L321
        // RGBA
        res = std::array<int, 4>{(channelColor >> 16) & 0xff, (channelColor >> 8) & 0xff, channelColor & 0xff,
                                 (channelColor >> 24) & 0xff};
    return res;
}

std::string Reader::impl::getChannelName(int channel)
{
    assert(channel >= 0 && channel < getSizeC());

    std::string channelName;
    jstring name = (jstring)jvm_env->CallObjectMethod(
        wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getChannelName", "(I)Ljava/lang/String;"), channel);
    if (name != nullptr)
    {
        const char* nameChars = jvm_env->GetStringUTFChars(name, nullptr);
        channelName = std::string(nameChars);
        jvm_env->ReleaseStringUTFChars(name, nameChars);
    }
    jvm_env->DeleteLocalRef(name);
    return channelName;
}

int Reader::impl::getBitsPerPixel()
{
    return jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getBitsPerPixel", "()I"));
}

int Reader::impl::getOptimalTileWidth()
{
    return jvm_env->CallIntMethod(wrapper_instance,
                                  jvm_wrapper->getMethodID(wrapper_cls, "getPreferredTileWidth", "()I"));
}

int Reader::impl::getOptimalTileHeight()
{
    return jvm_env->CallIntMethod(wrapper_instance,
                                  jvm_wrapper->getMethodID(wrapper_cls, "getPreferredTileHeight", "()I"));
}

int Reader::impl::getLevelCount()
{
    return jvm_env->CallIntMethod(wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "nResolutions", "()I"));
}

std::vector<std::pair<int, int>> Reader::impl::getLevelDimensions()
{
    std::vector<std::pair<int, int>> res;
    for (auto i = 0; i < getLevelCount(); i++)
    {
        jintArray dims = (jintArray)jvm_env->CallObjectMethod(
            wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getSizeForResolution", "(I)[I"), i);
        if (dims)
        {
            jint* dims_ptr = jvm_env->GetIntArrayElements(dims, nullptr);
            res.push_back(std::make_pair(dims_ptr[0], dims_ptr[1]));
            jvm_env->ReleaseIntArrayElements(dims, dims_ptr, 0);
            jvm_env->DeleteLocalRef(dims);
        }
        else
            res.push_back(std::make_pair(-1, -1));
    }
    return res;
}

std::vector<double> Reader::impl::getLevelDownsamples()
{
    std::vector<double> res;
    jdoubleArray downsamples = (jdoubleArray)jvm_env->CallObjectMethod(
        wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getPreferredDownsamples", "()[D"));
    if (downsamples)
    {
        jsize length = jvm_env->GetArrayLength(downsamples);
        res.resize(length);
        jvm_env->GetDoubleArrayRegion(downsamples, 0, length, (jdouble*)res.data());
    }
    jvm_env->DeleteLocalRef(downsamples);
    return res;
}

int Reader::impl::getPreferredResolutionLevel(double downsample)
{
    return jvm_env->CallIntMethod(
        wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getPreferredResolutionLevel", "(D)I"), downsample);
}
double Reader::impl::getPreferredDownsampleFactor(double downsample)
{
    return jvm_env->CallDoubleMethod(
        wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getPreferredDownsampleFactor", "(D)D"), downsample);
}

std::vector<unsigned char> Reader::impl::readRegion(double downsample, int x, int y, int w, int h, int z, int t)
{
    std::vector<unsigned char> bytes;

    jbyteArray byteArray = (jbyteArray)jvm_env->CallObjectMethod(
        wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "readRegion", "(DIIIIII)[B"), downsample, x, y, w, h, z,
        t);
    if (byteArray != nullptr)
    {
        jsize len = jvm_env->GetArrayLength(byteArray);
        bytes.resize(len);
        jvm_env->GetByteArrayRegion(byteArray, 0, len, (jbyte*)bytes.data());
    }
    jvm_env->DeleteLocalRef(byteArray);

    return bytes;
}

std::vector<unsigned char> Reader::impl::readRegion(int level, int x, int y, int w, int h, int z, int t)
{
    std::vector<unsigned char> bytes;

    jbyteArray byteArray = (jbyteArray)jvm_env->CallObjectMethod(
        wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "readRegion", "(IIIIIII)[B"), level, x, y, w, h, z, t);
    if (byteArray != nullptr)
    {
        jsize len = jvm_env->GetArrayLength(byteArray);
        bytes.resize(len);
        jvm_env->GetByteArrayRegion(byteArray, 0, len, (jbyte*)bytes.data());
    }
    jvm_env->DeleteLocalRef(byteArray);

    return bytes;
}

std::vector<unsigned char> Reader::impl::readTile(int level, int x, int y, int w, int h, int z, int t)
{
    std::vector<unsigned char> bytes;

    jbyteArray byteArray = (jbyteArray)jvm_env->CallObjectMethod(
        wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "readTile", "(IIIIIII)[B"), level, x, y, w, h, z, t);
    if (byteArray != nullptr)
    {
        jsize len = jvm_env->GetArrayLength(byteArray);
        bytes.resize(len);
        jvm_env->GetByteArrayRegion(byteArray, 0, len, (jbyte*)bytes.data());
    }
    jvm_env->DeleteLocalRef(byteArray);

    return bytes;
}

std::vector<unsigned char> Reader::impl::getDefaultThumbnail(int z, int t)
{
    std::vector<unsigned char> bytes;

    jbyteArray byteArray = (jbyteArray)jvm_env->CallObjectMethod(
        wrapper_instance, jvm_wrapper->getMethodID(wrapper_cls, "getDefaultThumbnail", "(II)[B"), z, t);
    if (byteArray != nullptr)
    {
        jsize len = jvm_env->GetArrayLength(byteArray);
        bytes.resize(len);
        jvm_env->GetByteArrayRegion(byteArray, 0, len, (jbyte*)bytes.data());
    }
    jvm_env->DeleteLocalRef(byteArray);

    return bytes;
}

std::vector<std::string> Reader::impl::getAssociatedImageNames()
{
    std::vector<std::string> associatedImageNamesVec;

    jobjectArray associatedImageNames = (jobjectArray)jvm_env->CallObjectMethod(
        wrapper_instance, jvm_env->GetMethodID(wrapper_cls, "getAssociatedImageNames", "()[Ljava/lang/String;"));
    if (associatedImageNames != nullptr)
    {
        jsize length = jvm_env->GetArrayLength(associatedImageNames);
        associatedImageNamesVec.reserve(length);
        for (jsize i = 0; i < length; i++)
        {
            jstring name = (jstring)jvm_env->GetObjectArrayElement(associatedImageNames, i);
            if (auto* nameChars = jvm_env->GetStringUTFChars(name, nullptr); nameChars)
            {
                associatedImageNamesVec.emplace_back(nameChars);
                jvm_env->ReleaseStringUTFChars(name, nameChars);
                jvm_env->DeleteLocalRef(name);
            }
        }
    }
    jvm_env->DeleteLocalRef(associatedImageNames);

    return associatedImageNamesVec;
}

std::vector<unsigned char> Reader::impl::getAssociatedImage(std::string const& name)
{
    std::vector<unsigned char> bytes;

    jstring nameStr = jvm_env->NewStringUTF(name.c_str());
    jbyteArray byteArray = (jbyteArray)jvm_env->CallObjectMethod(
        wrapper_instance, jvm_env->GetMethodID(wrapper_cls, "getAssociatedImage", "(Ljava/lang/String;)[B"), nameStr);
    if (byteArray != nullptr)
    {
        jsize length = jvm_env->GetArrayLength(byteArray);
        bytes.resize(length);
        jvm_env->GetByteArrayRegion(byteArray, 0, length, (jbyte*)bytes.data());
    }
    jvm_env->DeleteLocalRef(byteArray);
    jvm_env->DeleteLocalRef(nameStr);

    return bytes;
}

void Reader::impl::force_gc()
{
    jvm_env->CallStaticVoidMethod(system_cls, jvm_env->GetStaticMethodID(system_cls, "gc", "()V"));
}
