#include <fstream>

#include "PhyreContainer.h"
namespace phyre
{
	PhyreContainer::PhyreContainer(const std::filesystem::path &phyrePath)
	{
		std::ifstream phyreFile(phyrePath, std::ios::in | std::ios::binary);
		if (!phyreFile)
			throw PhyreExceptionIO(L"Cannot open binary file: " + phyrePath.wstring());
		
		if (std::filesystem::file_size(phyrePath) < sizeof(_tBasicHeader))
			throw PhyreExceptionData(L"Phyre file too small to be valid");

		_tBasicHeader basicHeader;
		phyreFile.read(reinterpret_cast<char*>(&basicHeader), sizeof(basicHeader));
		phyreFile.close();

		if (basicHeader.magic == PHYRE_MAGIC_BE)
			throw PhyreExceptionData(L"Big Endian files are not yet supported");

		if (basicHeader.magic != PHYRE_MAGIC)
			throw PhyreExceptionData(L"Invalid phyre file header");

		switch (basicHeader.platformId)
		{
			case _ePlatformId::platformDX11:
				_phyrePlatform = std::unique_ptr<PhyrePlatform>(new PhyrePlatformDX11);
				break;
			default:
				throw PhyreExceptionData(L"Unsupported phyre platform");
		}

		if (!_phyrePlatform->isFormatSupported(phyrePath))
			throw PhyreExceptionData(L"Unsupported phyre format");
	}
	void PhyreContainer::ConvertPhyre2DDS(const std::filesystem::path& phyrePath, const std::filesystem::path& ddsPath)
	{
		_phyrePlatform->convertPhyre2DDS(phyrePath, ddsPath);
	}
	void PhyreContainer::ConvertDDS2Phyre(const std::filesystem::path& ddsPath, const std::filesystem::path& phyrePath)
	{
		_phyrePlatform->convertDDS2Phyre(ddsPath, phyrePath);
	}
}