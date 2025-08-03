#pragma once
#include "PhyrePlatform.h"

namespace phyre
{

    class PhyrePlatformDX11 :
        public PhyrePlatform
    {
    public:
        virtual ~PhyrePlatformDX11() = default;
    private:
		struct _tDX11Header
		{
			uint32_t magic;
			uint32_t size;
			uint32_t namespaceSize;
			uint32_t platformId;
			uint32_t instanceListCount;
			uint32_t arrayFixupSize;
			uint32_t arrayFixupCount;
			uint32_t pointerFixupSize;
			uint32_t pointerFixupCount;
			uint32_t pointerArrayFixupSize;
			uint32_t pointerArrayFixupCount;
			uint32_t pointersInArraysCount;
			uint32_t userFixupCount;
			uint32_t userFixupDataSize;
			uint32_t totalDataSize;
			uint32_t headerClassInstanceCount;
			uint32_t headerClassChildCount;
			uint32_t physicsEngineID;
			uint32_t indexBufferSize;
			uint32_t vertexBufferSize;
			uint32_t maxTextureBufferSize;
		};

		static constexpr uint32_t PLATFORMID = 0x44583131;

		/*
		* Most phyre classes are not binary compatible between versions,
		* sometimes even between formats in the same versions. Thankfully
		* phyre is also self describing, so we can get offset of all
		* members from namespace definition.
		*/
		_tTextureInfo _getTextureInfo (const _tNamespaceHeader* header, const  _tNamespaceClassDescriptor* classes, const _tNamespaceDataMember* members, const char* stringTable, std::fstream& phyreFile, const size_t textureInfoStart);
		void _setTextureInfo(const _tTextureInfo& textureInfo, std::fstream& phyreFile, const size_t textureInfoStart);
		_tTextureInfo _setTextureFormat(const _tTextureInfo& textureInfo, std::fstream& phyreFile, const std::string& newFormat);
		_tTextureInfo _getPhyreInfo(std::fstream& phyreFile, const size_t filesize);

		// Inherited via PhyrePlatform
		virtual bool isFormatSupported(const std::filesystem::path& phyrePath) override;

		// Inherited via PhyrePlatform
		virtual void convertPhyre2DDS(const std::filesystem::path& phyrePath, const std::filesystem::path& ddsPath) override;

		// Inherited via PhyrePlatform
		virtual void convertDDS2Phyre(const std::filesystem::path& ddsPath, const std::filesystem::path& phyrePath) override;
	};

}
