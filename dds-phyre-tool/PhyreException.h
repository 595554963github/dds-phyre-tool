#pragma once
#include <stdexcept>
#include <string>

namespace phyre
{

	class PhyreException : public std::runtime_error
	{
	public:
		PhyreException(const std::wstring& msg = L"");
		virtual std::wstring what();
		virtual ~PhyreException() = default;
	protected:
		std::wstring _msg;
	};

	class PhyreExceptionIO : public PhyreException
	{
	public:
		PhyreExceptionIO(const std::wstring& msg = L"");
		virtual ~PhyreExceptionIO() = default;
	};

	class PhyreExceptionData : public PhyreException
	{
	public:
		PhyreExceptionData(const std::wstring& msg = L"");
		virtual ~PhyreExceptionData() = default;
	};

}

