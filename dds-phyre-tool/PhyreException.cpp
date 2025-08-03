#include "PhyreException.h"

namespace phyre
{
	PhyreException::PhyreException(const std::wstring& msg)
		: std::runtime_error("")
		, _msg(msg)
	{
	}
	std::wstring PhyreException::what()
	{
		return _msg;
	}
	PhyreExceptionIO::PhyreExceptionIO(const std::wstring& msg)
		: PhyreException(msg)
	{
	}
	PhyreExceptionData::PhyreExceptionData(const std::wstring& msg)
		: PhyreException(msg)
	{
	}
}
