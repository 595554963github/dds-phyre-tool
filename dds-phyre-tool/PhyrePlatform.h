#pragma once
#include <cstdint>
#include <string>
#include <filesystem>
#include <fstream>

namespace phyre
{
    class PhyrePlatform
    {
    public:
        virtual ~PhyrePlatform() = default;
        virtual bool isFormatSupported(const std::filesystem::path& phyrePath) = 0;
        virtual void convertPhyre2DDS(const std::filesystem::path& phyrePath, const std::filesystem::path& ddsPath) = 0;
        virtual void convertDDS2Phyre(const std::filesystem::path& ddsPath, const std::filesystem::path& phyrePath) = 0;

    protected:
        struct _tNamespaceHeader
        {
            uint32_t magic;
            uint32_t size;
            uint32_t typeCount;
            uint32_t classCount;
            uint32_t classDataMemberCount;
            uint32_t stringTableSize;
            uint32_t defaultBufferCount;
            uint32_t defaultBufferSize;
        };

        struct _tNamespaceDataMember
        {
            uint32_t nameOffset;
            uint32_t typeId;
            uint32_t valueOffset;
            uint32_t size;
            uint32_t flags;
            uint32_t fixedArraySize;
        };

        struct _tNamespaceClassDescriptor
        {
            uint32_t baseClassId;
            uint32_t sizeAndAlign;
            uint32_t nameOffset;
            uint32_t dataMemberCount;
            uint32_t offsetFromParent;
            uint32_t offsetToBase;
            uint32_t offsetToBaseInAllocateBlock;
            uint32_t flags;
            uint32_t defaultBufferOffset;
        };

        struct _tInstanceDescriptor
        {
            uint32_t classId;
            uint32_t count;
            uint32_t size;
            uint32_t objectSize;
            uint32_t arraysSize;
            uint32_t pointersInArraysCount;
            uint32_t arrayFixupCount;
            uint32_t pointerFixupCount;
            uint32_t pointerArrayFixupCount;
        };

        struct _tUserFixup
        {
            uint32_t typeId;
            uint32_t size;
            uint32_t offset;
        };

        struct _tTextureMembers
        {
            size_t widthOffset;
            size_t heightOffset;
            size_t mipmapCountOffset;
            size_t maxMipmapLeveOffset;
            size_t textureFlagsOffset;
        };

        struct _tTextureInfo
        {
            uint32_t format;
            uint8_t memoryType;
            uint32_t mipmapCount;
            uint32_t maxMipmapLevel;
            uint32_t textureFlags;
            uint32_t width;
            uint32_t height;
            _tTextureMembers textureMembers;
            std::string textureFormat;
            size_t fixupOffset;
            size_t fixupDataOffset;
            size_t textureInfoOffset;
            size_t dataOffset;
        };

        enum _eDDS_FOURCC
        {
            DDSFCC_DXT5 = 0x35545844,
            DDSFCC_DXT3 = 0x33545844,
            DDSFCC_DXT1 = 0x31545844,
            DDSFCC_BC5U = 0x55354342,
            DDSFCC_ATI2 = 0x32495441,
            DDSFCC_BC7 = 0x20374342,
            DDSFCC_DX10 = 0x30315844
        };

        enum _eDXGI_FORMAT
        {
            DXGI_FORMAT_BC7_UNORM = 98,
            DXGI_FORMAT_BC7_UNORM_SRGB = 99
        };

        enum _eDDSPF_FLAGS
        {
            DDPF_ALPHAPIXELS = 0x1,
            DDPF_ALPHA = 0x2,
            DDPF_FOURCC = 0x4,
            DDPF_RGB = 0x40,
            DDPF_YUV = 0x200,
            DDPF_LUMINANCE = 0x20000
        };

        enum _eDDS_FLAGS
        {
            DDSD_CAPS = 0x1,
            DDSD_HEIGHT = 0x2,
            DDSD_WIDTH = 0x4,
            DDSD_PITCH = 0x8,
            DDSD_PIXELFORMAT = 0x1000,
            DDSD_MIPMAPCOUNT = 0x20000,
            DDSD_LINEARSIZE = 0x80000,
            DDSD_DEPTH = 0x800000
        };

        enum _eDDS_CAPS
        {
            DDSCAPS_COMPLEX = 0x8,
            DDSCAPS_TEXTURE = 0x1000,
            DDSCAPS_MIPMAP = 0x400000
        };

        struct _tDDS_PIXELFORMAT {
            uint32_t dwSize = sizeof(_tDDS_PIXELFORMAT);
            uint32_t dwFlags = DDPF_FOURCC;
            uint32_t dwFourCC = 0;
            uint32_t dwRGBBitCount = 32;
            uint32_t dwRBitMask = 0x000000FF;
            uint32_t dwGBitMask = 0x0000FF00;
            uint32_t dwBBitMask = 0x00FF0000;
            uint32_t dwABitMask = 0xFF000000;
        };

        struct _tDDS_HEADER {
            char magic[4] = { 'D','D','S',' ' };
            uint32_t dwSize = sizeof(_tDDS_HEADER) - 4;
            uint32_t dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
            uint32_t dwHeight = 0;
            uint32_t dwWidth = 0;
            uint32_t dwPitchOrLinearSize = 0;
            uint32_t dwDepth = 1;
            uint32_t dwMipMapCount = 1;
            uint32_t dwReserved1[11] = { 0 };
            _tDDS_PIXELFORMAT ddspf;
            uint32_t dwCaps = DDSCAPS_TEXTURE;
            uint32_t dwCaps2 = 0;
            uint32_t dwCaps3 = 0;
            uint32_t dwCaps4 = 0;
            uint32_t dwReserved2 = 0;
        };

        struct _tDDS_HEADER_DXT10 {
            uint32_t dxgiFormat;
            uint32_t resourceDimension;
            uint32_t miscFlag;
            uint32_t arraySize;
            uint32_t miscFlags2;
        };

        struct _tClassData
        {
            const _tNamespaceClassDescriptor* classStart;
            const _tNamespaceDataMember* memberStart;
        };

        virtual _tClassData _findClass(const _tNamespaceHeader* header,
            const _tNamespaceClassDescriptor* classes,
            const _tNamespaceDataMember* members,
            const char* stringTable,
            const std::string& className);

        virtual const _tNamespaceDataMember* _findMember(const _tNamespaceDataMember* membersStart,
            size_t memberCount,
            const char* stringTable,
            const std::string& memberName);

        virtual _tTextureMembers _getTextureMembers(const _tNamespaceHeader* header,
            const _tNamespaceClassDescriptor* classes,
            const _tNamespaceDataMember* members,
            const char* stringTable);

        virtual size_t _getInstanceStartRelative(std::fstream& phyreFile,
            size_t instanceOffset,
            const _tNamespaceClassDescriptor* classes,
            const char* stringTable,
            size_t instanceCount,
            const std::string& className);

        virtual _tDDS_HEADER prepareDDSHeader(const std::string& format,
            uint32_t width,
            uint32_t height,
            uint32_t mipmaps,
            bool useDX10 = false);

        const std::string getDDSFormat(const _tDDS_HEADER& ddsHeader);
        uint32_t getBufferSizeByFormat(const std::string& format,
            uint32_t width,
            uint32_t height);
    };
}