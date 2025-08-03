#include <fstream>
#include <iostream>
#include <memory>
#include <algorithm>
#include <vector>

#include "PhyrePlatformDX11.h"
#include "PhyreException.h"

namespace phyre
{
    PhyrePlatform::_tTextureInfo PhyrePlatformDX11::_getTextureInfo(const _tNamespaceHeader* header, const _tNamespaceClassDescriptor* classes, const _tNamespaceDataMember* members, const char* stringTable, std::fstream& phyreFile, const size_t textureInfoStart)
    {
        _tTextureInfo textureInfo{};
        textureInfo.textureMembers = _getTextureMembers(header, classes, members, stringTable);

        phyreFile.seekg(textureInfoStart + textureInfo.textureMembers.widthOffset, std::ios::beg);
        phyreFile.read(reinterpret_cast<char*>(&textureInfo.width), sizeof(textureInfo.width));

        phyreFile.seekg(textureInfoStart + textureInfo.textureMembers.heightOffset, std::ios::beg);
        phyreFile.read(reinterpret_cast<char*>(&textureInfo.height), sizeof(textureInfo.height));

        phyreFile.seekg(textureInfoStart + textureInfo.textureMembers.mipmapCountOffset, std::ios::beg);
        phyreFile.read(reinterpret_cast<char*>(&textureInfo.mipmapCount), sizeof(textureInfo.mipmapCount));

        phyreFile.seekg(textureInfoStart + textureInfo.textureMembers.maxMipmapLeveOffset, std::ios::beg);
        phyreFile.read(reinterpret_cast<char*>(&textureInfo.maxMipmapLevel), sizeof(textureInfo.maxMipmapLevel));

        phyreFile.seekg(textureInfoStart + textureInfo.textureMembers.textureFlagsOffset, std::ios::beg);
        phyreFile.read(reinterpret_cast<char*>(&textureInfo.textureFlags), sizeof(textureInfo.textureFlags));

        return textureInfo;
    }

    void PhyrePlatformDX11::_setTextureInfo(const _tTextureInfo& textureInfo, std::fstream& phyreFile, const size_t textureInfoStart)
    {
        phyreFile.seekp(textureInfoStart + textureInfo.textureMembers.widthOffset, std::ios::beg);
        phyreFile.write(reinterpret_cast<const char*>(&textureInfo.width), sizeof(textureInfo.width));

        phyreFile.seekp(textureInfoStart + textureInfo.textureMembers.heightOffset, std::ios::beg);
        phyreFile.write(reinterpret_cast<const char*>(&textureInfo.height), sizeof(textureInfo.height));

        phyreFile.seekp(textureInfoStart + textureInfo.textureMembers.mipmapCountOffset, std::ios::beg);
        phyreFile.write(reinterpret_cast<const char*>(&textureInfo.mipmapCount), sizeof(textureInfo.mipmapCount));

        phyreFile.seekp(textureInfoStart + textureInfo.textureMembers.maxMipmapLeveOffset, std::ios::beg);
        phyreFile.write(reinterpret_cast<const char*>(&textureInfo.maxMipmapLevel), sizeof(textureInfo.maxMipmapLevel));

        phyreFile.seekp(textureInfoStart + textureInfo.textureMembers.textureFlagsOffset, std::ios::beg);
        phyreFile.write(reinterpret_cast<const char*>(&textureInfo.textureFlags), sizeof(textureInfo.textureFlags));
    }

    PhyrePlatform::_tTextureInfo PhyrePlatformDX11::_setTextureFormat(const _tTextureInfo& textureInfo, std::fstream& phyreFile, const std::string& newFormat)
    {
        _tTextureInfo ret = textureInfo;
        _tDX11Header dx11Header{};
        phyreFile.seekg(0, std::ios::beg);
        phyreFile.read(reinterpret_cast<char*>(&dx11Header), sizeof(dx11Header));

        size_t remainingDataOffset = textureInfo.fixupDataOffset + dx11Header.userFixupDataSize + sizeof(_tUserFixup) * dx11Header.userFixupCount;
        size_t remainingDataSize = textureInfo.dataOffset - remainingDataOffset;
        std::unique_ptr<char[]> remainingDataBuffer(new char[remainingDataSize]);
        phyreFile.seekg(remainingDataOffset, std::ios::beg);
        phyreFile.read(remainingDataBuffer.get(), remainingDataSize);

        std::unique_ptr<char[]> userFixupBuffer(new char[dx11Header.userFixupDataSize]);
        phyreFile.seekg(textureInfo.fixupDataOffset, std::ios::beg);
        phyreFile.read(userFixupBuffer.get(), dx11Header.userFixupDataSize);

        std::vector<_tUserFixup> fixupEntries(dx11Header.userFixupCount);
        phyreFile.seekg(textureInfo.fixupOffset, std::ios::beg);

        for (auto& fixupEntry : fixupEntries)
            phyreFile.read(reinterpret_cast<char*>(&fixupEntry), sizeof(fixupEntry));

        uint32_t totalFixupDataSize = 0;
        size_t entryCounter = 0;
        phyreFile.seekp(textureInfo.fixupDataOffset, std::ios::beg);
        for (auto& fixupEntry : fixupEntries)
        {
            if (entryCounter == 1)
            {
                phyreFile.write(newFormat.c_str(), newFormat.size() + 1);
                fixupEntry.offset = totalFixupDataSize;
                fixupEntry.size = static_cast<uint32_t>(newFormat.size()) + 1;
                totalFixupDataSize += fixupEntry.size;
            }
            else
            {
                phyreFile.write(&userFixupBuffer[fixupEntry.offset], fixupEntry.size);
                fixupEntry.offset = totalFixupDataSize;
                totalFixupDataSize += fixupEntry.size;
            }
            entryCounter++;
        }

        for (const auto& fixupEntry : fixupEntries)
            phyreFile.write(reinterpret_cast<const char*>(&fixupEntry), sizeof(fixupEntry));

        phyreFile.write(remainingDataBuffer.get(), remainingDataSize);

        int delta = totalFixupDataSize - dx11Header.userFixupDataSize;
        dx11Header.userFixupDataSize = totalFixupDataSize;
        phyreFile.seekp(0, std::ios::beg);
        phyreFile.write(reinterpret_cast<char*>(&dx11Header), sizeof(dx11Header));

        ret.fixupOffset += delta;
        ret.dataOffset += delta;

        return ret;
    }

    PhyrePlatform::_tTextureInfo PhyrePlatformDX11::_getPhyreInfo(std::fstream& phyreFile, const size_t filesize)
    {
        if (filesize < sizeof(_tDX11Header))
            throw PhyreExceptionData(L"File too small to be dx11 platform type");

        _tDX11Header dx11Header{};
        phyreFile.read(reinterpret_cast<char*>(&dx11Header), sizeof(dx11Header));

        if (
            dx11Header.platformId != PLATFORMID || dx11Header.size != sizeof(dx11Header) ||
            filesize < 0ULL + dx11Header.size + dx11Header.namespaceSize
            )
            throw PhyreExceptionData(L"Size too small to fit namespace");

        std::unique_ptr<char[]> namespaceBuffer(new char[dx11Header.namespaceSize]);

        phyreFile.seekg(dx11Header.size, std::ios::beg);
        phyreFile.read(namespaceBuffer.get(), dx11Header.namespaceSize);

        const _tNamespaceHeader* namespaceHeader = reinterpret_cast<_tNamespaceHeader*>(namespaceBuffer.get());

        const size_t stringTableStart = static_cast<size_t>(namespaceHeader->size) - namespaceHeader->stringTableSize - static_cast<size_t>(namespaceHeader->defaultBufferCount) * namespaceHeader->defaultBufferSize;
        const char* stringTable = &namespaceBuffer[stringTableStart];

        const _tNamespaceClassDescriptor* classDescriptors = reinterpret_cast<_tNamespaceClassDescriptor*>(&namespaceBuffer[sizeof(_tNamespaceHeader) + sizeof(uint32_t) * namespaceHeader->typeCount]);

        const _tNamespaceDataMember* memberDescriptors = reinterpret_cast<_tNamespaceDataMember*>(&namespaceBuffer[sizeof(_tNamespaceHeader) + sizeof(uint32_t) * namespaceHeader->typeCount] + sizeof(_tNamespaceClassDescriptor) * namespaceHeader->classCount);

        const uint32_t* typeDescriptors = reinterpret_cast<uint32_t*>(&namespaceBuffer[sizeof(_tNamespaceHeader)]);

        size_t textureInstanceStart = _getInstanceStartRelative(phyreFile, 0ULL + dx11Header.size + namespaceHeader->size, classDescriptors, stringTable, dx11Header.instanceListCount, "PTexture2D");

        const size_t textureInfoStart = 0ULL + dx11Header.size + namespaceHeader->size + dx11Header.instanceListCount * sizeof(_tInstanceDescriptor) + textureInstanceStart;
        auto textureInfo = _getTextureInfo(namespaceHeader, classDescriptors, memberDescriptors, stringTable, phyreFile, textureInfoStart);
        textureInfo.textureInfoOffset = textureInfoStart;

        std::unique_ptr<char[]> userFixupBuffer(new char[dx11Header.userFixupDataSize]);
        textureInfo.fixupDataOffset = 0ULL + dx11Header.size + namespaceHeader->size + dx11Header.instanceListCount * sizeof(_tInstanceDescriptor) + dx11Header.totalDataSize;
        phyreFile.seekg(textureInfo.fixupDataOffset, std::ios::beg);
        phyreFile.read(userFixupBuffer.get(), dx11Header.userFixupDataSize);

        _tUserFixup userFixup{};
        textureInfo.fixupOffset = 0ULL + dx11Header.size + namespaceHeader->size + dx11Header.instanceListCount * sizeof(_tInstanceDescriptor) + dx11Header.totalDataSize + dx11Header.userFixupDataSize;
        phyreFile.seekg(textureInfo.fixupOffset + sizeof(_tUserFixup), std::ios::beg);
        phyreFile.read(reinterpret_cast<char*>(&userFixup), sizeof(userFixup));

        if (userFixup.typeId >= namespaceHeader->typeCount || std::string(&stringTable[typeDescriptors[userFixup.typeId]]) != "PTextureFormatBase")
            throw PhyreExceptionData(L"Texture format not found");

        textureInfo.textureFormat = &userFixupBuffer[userFixup.offset];

        size_t dataOffset = 0ULL + dx11Header.size + namespaceHeader->size + dx11Header.instanceListCount * sizeof(_tInstanceDescriptor)
            + dx11Header.totalDataSize
            + dx11Header.userFixupDataSize + sizeof(_tUserFixup) * dx11Header.userFixupCount
            + dx11Header.pointerArrayFixupSize + dx11Header.pointerFixupSize + dx11Header.arrayFixupSize;

        textureInfo.dataOffset = dataOffset;

        return textureInfo;
    }

    bool PhyrePlatformDX11::isFormatSupported(const std::filesystem::path& phyrePath)
    {
        std::ifstream phyreFile(phyrePath, std::ios::in | std::ios::binary);
        if (!phyreFile)
            throw PhyreExceptionIO(L"Cannot open binary file: " + phyrePath.wstring());

        const size_t filesize = std::filesystem::file_size(phyrePath);

        if (filesize < sizeof(_tDX11Header))
            return false;

        _tDX11Header dx11Header{};
        phyreFile.read(reinterpret_cast<char*>(&dx11Header), sizeof(dx11Header));

        if (
            dx11Header.platformId != PLATFORMID || dx11Header.size != sizeof(dx11Header) ||
            filesize < 0ULL + dx11Header.size + dx11Header.namespaceSize
            )
            return false;

        return true;
    }

    void PhyrePlatformDX11::convertPhyre2DDS(const std::filesystem::path& phyrePath, const std::filesystem::path& ddsPath)
    {
        std::fstream phyreFile(phyrePath, std::ios::in | std::ios::binary);
        if (!phyreFile)
            throw PhyreExceptionIO(L"Cannot open binary file: " + phyrePath.wstring());

        const size_t filesize = std::filesystem::file_size(phyrePath);
        auto textureInfo = _getPhyreInfo(phyreFile, filesize);

        if (textureInfo.dataOffset >= filesize)
            throw PhyreExceptionData(L"There is no DDS data in the phyre file");

        bool useDX10 = (textureInfo.textureFormat == "BC7");
        auto ddsHeader = prepareDDSHeader(textureInfo.textureFormat, textureInfo.width, textureInfo.height, textureInfo.mipmapCount, useDX10);

        std::ofstream ddsFile(ddsPath, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!ddsFile)
            throw PhyreExceptionIO(L"Cannot write file: " + ddsPath.wstring());

        size_t dataSize = filesize - textureInfo.dataOffset;
        std::vector<char> dataBuffer(dataSize);
        phyreFile.seekg(textureInfo.dataOffset, std::ios::beg);
        phyreFile.read(dataBuffer.data(), dataSize);

        if (useDX10)
        {
            _tDDS_HEADER_DXT10 dx10Header{};
            dx10Header.dxgiFormat = DXGI_FORMAT_BC7_UNORM;
            dx10Header.resourceDimension = 3; 
            dx10Header.arraySize = 1;

            ddsFile.write(reinterpret_cast<char*>(&ddsHeader), sizeof(ddsHeader));
            ddsFile.write(reinterpret_cast<char*>(&dx10Header), sizeof(dx10Header));
        }
        else
        {
            ddsFile.write(reinterpret_cast<char*>(&ddsHeader), sizeof(ddsHeader));
        }

        ddsFile.write(dataBuffer.data(), dataSize);
    }

    void PhyrePlatformDX11::convertDDS2Phyre(const std::filesystem::path& ddsPath, const std::filesystem::path& phyrePath)
    {
        std::fstream phyreFile(phyrePath, std::ios::in | std::ios::out | std::ios::binary);
        if (!phyreFile)
            throw PhyreExceptionIO(L"Cannot open binary file for writing: " + phyrePath.wstring());

        const size_t filesize = std::filesystem::file_size(phyrePath);
        auto textureInfo = _getPhyreInfo(phyreFile, filesize);

        if (textureInfo.dataOffset >= filesize)
            throw PhyreExceptionData(L"There is no DDS data in the phyre file");

        std::wcout << L"Found texture" << std::endl;
        std::wcout << L"format:                " << textureInfo.textureFormat.c_str() << std::endl;
        std::wcout << L"width:                " << textureInfo.width << std::endl;
        std::wcout << L"height:                " << textureInfo.height << std::endl;
        std::wcout << L"mipmaps:            " << textureInfo.mipmapCount << std::endl;
        std::wcout << L"max mipmap level:        " << textureInfo.maxMipmapLevel << std::endl;

        std::fstream ddsFile(ddsPath, std::ios::in | std::ios::binary);
        if (!ddsFile)
            throw PhyreExceptionIO(L"Cannot open binary file: " + ddsPath.wstring());

        const size_t ddsfilesize = std::filesystem::file_size(ddsPath);

        if (ddsfilesize < sizeof(_tDDS_HEADER))
            throw PhyreExceptionData(L"File too small to be a proper DDS file");

        _tDDS_HEADER ddsHeader{};
        ddsFile.read(reinterpret_cast<char*>(&ddsHeader), sizeof(ddsHeader));

        const std::string ddsTextureFormat = getDDSFormat(ddsHeader);
        std::wcout << L"Replacing with texture" << std::endl;
        std::wcout << L"format:                " << ddsTextureFormat.c_str() << std::endl;
        std::wcout << L"width:                " << ddsHeader.dwWidth << std::endl;
        std::wcout << L"height:                " << ddsHeader.dwHeight << std::endl;
        std::wcout << L"mipmaps:            " << ddsHeader.dwMipMapCount << std::endl;

        if (textureInfo.textureFormat != ddsTextureFormat)
            textureInfo = _setTextureFormat(textureInfo, phyreFile, ddsTextureFormat);

        size_t dataSize = ddsfilesize - sizeof(ddsHeader);
        std::vector<char> dataBuffer(dataSize);
        ddsFile.seekg(sizeof(ddsHeader), std::ios::beg);
        ddsFile.read(dataBuffer.data(), dataSize);

        uint32_t rowPitch = 0;
        if (ddsHeader.dwFlags & DDSD_PITCH) {
            rowPitch = ddsHeader.dwPitchOrLinearSize;
        }
        else if (ddsHeader.dwFlags & DDSD_LINEARSIZE) {
            rowPitch = ddsHeader.dwPitchOrLinearSize / ddsHeader.dwHeight;
        }

        if (rowPitch > 0) {
            uint32_t height = ddsHeader.dwHeight;
            std::vector<char> rowBuffer(rowPitch);
            for (uint32_t y = 0; y < height / 2; y++) {
                char* topRow = dataBuffer.data() + y * rowPitch;
                char* bottomRow = dataBuffer.data() + (height - 1 - y) * rowPitch;
                std::memcpy(rowBuffer.data(), topRow, rowPitch);
                std::memcpy(topRow, bottomRow, rowPitch);
                std::memcpy(bottomRow, rowBuffer.data(), rowPitch);
            }
        }

        phyreFile.seekp(textureInfo.dataOffset, std::ios::beg);
        phyreFile.write(dataBuffer.data(), dataSize);

        const auto phyreEnd = phyreFile.tellp();

        _tDX11Header dx11Header{};
        phyreFile.seekg(0, std::ios::beg);
        phyreFile.read(reinterpret_cast<char*>(&dx11Header), sizeof(dx11Header));

        dx11Header.maxTextureBufferSize = getBufferSizeByFormat(ddsTextureFormat, ddsHeader.dwWidth, ddsHeader.dwHeight);
        phyreFile.seekp(0, std::ios::beg);
        phyreFile.write(reinterpret_cast<char*>(&dx11Header), sizeof(dx11Header));

        uint32_t fixedMipmapCount = ddsHeader.dwMipMapCount > 1 ? ddsHeader.dwMipMapCount - 1 : 0;
        auto newTextureInfo = textureInfo;
        newTextureInfo.width = ddsHeader.dwWidth;
        newTextureInfo.height = ddsHeader.dwHeight;
        newTextureInfo.mipmapCount = fixedMipmapCount;
        newTextureInfo.maxMipmapLevel = fixedMipmapCount;
        _setTextureInfo(newTextureInfo, phyreFile, textureInfo.textureInfoOffset);

        phyreFile.close();
        std::filesystem::resize_file(phyrePath, phyreEnd);
    }
}