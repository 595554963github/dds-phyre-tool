#pragma once
#include <cstdint>
#include <filesystem>

#include "PhyreException.h"
#include "PhyrePlatformDX11.h"

namespace phyre
{
	class PhyreContainer
	{
	public:
		PhyreContainer() = delete;
		PhyreContainer(const std::filesystem::path &phyrePath);
		void ConvertPhyre2DDS(const std::filesystem::path& phyrePath, const std::filesystem::path& ddsPath);
		void ConvertDDS2Phyre(const std::filesystem::path& ddsPath, const std::filesystem::path& phyrePath);
		virtual ~PhyreContainer() = default;
	protected:
		static constexpr uint32_t PHYRE_MAGIC = 0x50485952UL;
		static constexpr uint32_t PHYRE_MAGIC_BE = 0x52594850UL;
		struct _tBasicHeader
		{
			uint32_t magic;
			uint32_t size;
			uint32_t namespaceSize;
			uint32_t platformId;
		};
		enum _ePlatformId
		{
			platformDX11 = 0x44583131,
		};

		std::unique_ptr<PhyrePlatform> _phyrePlatform;
	};
}
