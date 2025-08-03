#include "PhyrePlatform.h"
#include "PhyreException.h"
#include <algorithm>

namespace phyre
{
    PhyrePlatform::_tClassData PhyrePlatform::_findClass(const _tNamespaceHeader* header, const _tNamespaceClassDescriptor* classes, const _tNamespaceDataMember* members, const char* stringTable, const std::string& className)
    {
        _tClassData ret{};
        for (size_t i = 0, totalMembers = 0; i < header->classCount; i++)
        {
            if (&stringTable[classes[i].nameOffset] == className)
            {
                ret.classStart = &classes[i];
                ret.memberStart = &members[totalMembers];
                break;
            }
            totalMembers += classes[i].dataMemberCount;
        }
        return ret;
    }

    const PhyrePlatform::_tNamespaceDataMember* PhyrePlatform::_findMember(const _tNamespaceDataMember* membersStart, size_t memberCount, const char* stringTable, const std::string& memberName)
    {
        for (size_t i = 0; i < memberCount; i++)
        {
            if (&stringTable[membersStart[i].nameOffset] == memberName)
                return &membersStart[i];
        }
        return nullptr;
    }

    PhyrePlatform::_tTextureMembers PhyrePlatform::_getTextureMembers(const _tNamespaceHeader* header, const _tNamespaceClassDescriptor* classes, const _tNamespaceDataMember* members, const char* stringTable)
    {
        _tTextureMembers ret;
        auto classBaseData = _findClass(header, classes, members, stringTable, "PTexture2DBase");
        if (!classBaseData.classStart || !classBaseData.memberStart)
            throw PhyreExceptionData(L"Can't find width and height. PTexture2DBase not found.");

        auto classCommonData = _findClass(header, classes, members, stringTable, "PTextureCommonBase");
        if (!classCommonData.classStart || !classCommonData.memberStart)
            throw PhyreExceptionData(L"Can't find mipmap info. PTextureCommonBase not found.");

        auto heightMember = _findMember(classBaseData.memberStart, classBaseData.classStart->dataMemberCount, stringTable, "m_height");
        auto widthMember = _findMember(classBaseData.memberStart, classBaseData.classStart->dataMemberCount, stringTable, "m_width");
        auto mipmapMember = _findMember(classCommonData.memberStart, classCommonData.classStart->dataMemberCount, stringTable, "m_mipmapCount");
        auto mipmapMaxMember = _findMember(classCommonData.memberStart, classCommonData.classStart->dataMemberCount, stringTable, "m_maxMipLevel");
        auto textureFlagsMember = _findMember(classCommonData.memberStart, classCommonData.classStart->dataMemberCount, stringTable, "m_textureFlags");

        if (!heightMember || !widthMember)
            throw PhyreExceptionData(L"Can't find width and height members.");
        if (!mipmapMember || !mipmapMaxMember || !textureFlagsMember)
            throw PhyreExceptionData(L"Can't find mipmap members.");

        ret.heightOffset = heightMember->valueOffset;
        ret.widthOffset = widthMember->valueOffset;
        ret.mipmapCountOffset = mipmapMember->valueOffset;
        ret.maxMipmapLeveOffset = mipmapMaxMember->valueOffset;
        ret.textureFlagsOffset = textureFlagsMember->valueOffset;

        return ret;
    }

    size_t PhyrePlatform::_getInstanceStartRelative(std::fstream& phyreFile, size_t instanceOffset, const _tNamespaceClassDescriptor* classes, const char* stringTable, size_t instanceCount, const std::string& className)
    {
        phyreFile.seekg(instanceOffset, std::ios::beg);
        _tInstanceDescriptor instanceDescriptor{};
        size_t textureInstanceId = std::numeric_limits<size_t>::max();
        size_t textureInstanceStart = 0;

        for (size_t i = 0; i < instanceCount; i++)
        {
            phyreFile.read(reinterpret_cast<char*>(&instanceDescriptor), sizeof(instanceDescriptor));
            if (std::string(&stringTable[classes[instanceDescriptor.classId - 1].nameOffset]) == className)
            {
                textureInstanceId = i;
                break;
            }
            textureInstanceStart += instanceDescriptor.size;
        }

        if (textureInstanceId == std::numeric_limits<size_t>::max())
            throw PhyreExceptionData(std::wstring(className.begin(), className.end()) + L" instance not found");

        return textureInstanceStart;
    }

    PhyrePlatform::_tDDS_HEADER PhyrePlatform::prepareDDSHeader(const std::string& format, uint32_t width, uint32_t height, uint32_t mipmaps, bool useDX10)
    {
        _tDDS_HEADER header{};
        header.dwWidth = width;
        header.dwHeight = height;
        header.dwMipMapCount = mipmaps + 1;
        header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_LINEARSIZE;

        if (mipmaps > 0)
        {
            header.dwFlags |= DDSD_MIPMAPCOUNT;
            header.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
        }

        if (format == "BC7" && useDX10)
        {
            header.ddspf.dwFlags = DDPF_FOURCC;
            header.ddspf.dwFourCC = DDSFCC_DX10;
            header.dwPitchOrLinearSize = std::max(1u, (width + 3) / 4) * std::max(1u, (height + 3) / 4) * 16;
        }
        else if (format == "DXT5" || format == "DXT3" || format == "DXT1" || format == "BC5" || format == "BC7")
        {
            header.ddspf.dwFlags = DDPF_FOURCC;
            header.ddspf.dwRBitMask = 0;
            header.ddspf.dwGBitMask = 0;
            header.ddspf.dwBBitMask = 0;
            header.ddspf.dwABitMask = 0;

            if (format == "DXT5")
            {
                header.ddspf.dwFourCC = DDSFCC_DXT5;
                header.dwPitchOrLinearSize = std::max(1u, (width + 3) / 4) * std::max(1u, (height + 3) / 4) * 16;
            }
            else if (format == "DXT3")
            {
                header.ddspf.dwFourCC = DDSFCC_DXT3;
                header.dwPitchOrLinearSize = std::max(1u, (width + 3) / 4) * std::max(1u, (height + 3) / 4) * 16;
            }
            else if (format == "DXT1")
            {
                header.ddspf.dwFourCC = DDSFCC_DXT1;
                header.dwPitchOrLinearSize = std::max(1u, (width + 3) / 4) * std::max(1u, (height + 3) / 4) * 8;
            }
            else if (format == "BC5")
            {
                header.ddspf.dwFourCC = DDSFCC_BC5U;
                header.dwPitchOrLinearSize = std::max(1u, (width + 3) / 4) * std::max(1u, (height + 3) / 4) * 16;
            }
            else if (format == "BC7")
            {
                header.ddspf.dwFourCC = DDSFCC_BC7;
                header.dwPitchOrLinearSize = std::max(1u, (width + 3) / 4) * std::max(1u, (height + 3) / 4) * 16;
            }
        }
        else if (format == "ARGB8")
        {
            header.dwFlags |= DDSD_PITCH;
            header.ddspf.dwFlags = DDPF_ALPHAPIXELS | DDPF_RGB;
            header.ddspf.dwRGBBitCount = 32;
            header.ddspf.dwRBitMask = 0x00FF0000;
            header.ddspf.dwGBitMask = 0x0000FF00;
            header.ddspf.dwBBitMask = 0x000000FF;
            header.ddspf.dwABitMask = 0xFF000000;
            header.dwPitchOrLinearSize = width * 4;
        }
        else if (format == "RGBA8")
        {
            header.dwFlags |= DDSD_PITCH;
            header.ddspf.dwFlags = DDPF_ALPHAPIXELS | DDPF_RGB;
            header.ddspf.dwRGBBitCount = 32;
            header.ddspf.dwRBitMask = 0x000000FF;
            header.ddspf.dwGBitMask = 0x0000FF00;
            header.ddspf.dwBBitMask = 0x00FF0000;
            header.ddspf.dwABitMask = 0xFF000000;
            header.dwPitchOrLinearSize = width * 4;
        }
        else if (format == "A8")
        {
            header.dwFlags |= DDSD_PITCH;
            header.ddspf.dwFlags = DDPF_ALPHA;
            header.ddspf.dwRGBBitCount = 8;
            header.ddspf.dwABitMask = 0xFF;
            header.dwPitchOrLinearSize = width;
        }
        else if (format == "L8")
        {
            header.dwFlags |= DDSD_PITCH;
            header.ddspf.dwFlags = DDPF_LUMINANCE;
            header.ddspf.dwRGBBitCount = 8;
            header.ddspf.dwRBitMask = 0xFF;
            header.dwPitchOrLinearSize = width;
        }
        else
        {
            throw PhyreException(L"Unsupported format: " + std::wstring(format.begin(), format.end()));
        }
        return header;
    }

    const std::string PhyrePlatform::getDDSFormat(const _tDDS_HEADER& ddsHeader)
    {
        std::string ret;
        if (ddsHeader.ddspf.dwFlags & DDPF_FOURCC)
        {
            switch (ddsHeader.ddspf.dwFourCC)
            {
            case DDSFCC_DXT1: ret = "DXT1"; break;
            case DDSFCC_DXT3: ret = "DXT3"; break;
            case DDSFCC_DXT5: ret = "DXT5"; break;
            case DDSFCC_BC5U: ret = "BC5"; break;
            case DDSFCC_BC7: ret = "BC7"; break;
            case DDSFCC_DX10: ret = "DX10"; break;
            }
        }
        else
        {
            if (ddsHeader.ddspf.dwFlags & DDPF_ALPHAPIXELS && ddsHeader.ddspf.dwFlags & DDPF_RGB)
            {
                if (ddsHeader.ddspf.dwRGBBitCount == 32)
                {
                    if (ddsHeader.ddspf.dwRBitMask == 0x000000FF &&
                        ddsHeader.ddspf.dwGBitMask == 0x0000FF00 &&
                        ddsHeader.ddspf.dwBBitMask == 0x00FF0000 &&
                        ddsHeader.ddspf.dwABitMask == 0xFF000000)
                    {
                        ret = "RGBA8";
                    }
                    else if (ddsHeader.ddspf.dwRBitMask == 0x00FF0000 &&
                        ddsHeader.ddspf.dwGBitMask == 0x0000FF00 &&
                        ddsHeader.ddspf.dwBBitMask == 0x000000FF &&
                        ddsHeader.ddspf.dwABitMask == 0xFF000000)
                    {
                        ret = "ARGB8";
                    }
                }
            }
            else if (ddsHeader.ddspf.dwFlags & DDPF_ALPHA) ret = "A8";
            else if (ddsHeader.ddspf.dwFlags & DDPF_LUMINANCE) ret = "L8";
        }
        if (ret.empty()) throw PhyreExceptionData(L"Unsupported or not recognized DDS format");
        return ret;
    }

    uint32_t PhyrePlatform::getBufferSizeByFormat(const std::string& format, uint32_t width, uint32_t height)
    {
        if (format == "DXT5" || format == "DXT3" || format == "BC5" || format == "BC7")
            return std::max(1u, (width + 3) / 4) * std::max(1u, (height + 3) / 4) * 16;
        else if (format == "DXT1")
            return std::max(1u, (width + 3) / 4) * std::max(1u, (height + 3) / 4) * 8;
        else if (format == "ARGB8" || format == "RGBA8")
            return width * height * 4;
        else if (format == "A8" || format == "L8")
            return width * height;
        return 0;
    }
}