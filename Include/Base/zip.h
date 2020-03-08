#pragma once

#include "Base/IntTypes.h"
#include "Base/Defs.h"

namespace Public {
namespace Base {

//解压对象
class BASE_API Unzip
{
public:
	struct ZipItem
	{
		int			index;
		std::string	filename;
	};
public:
	Unzip();
	~Unzip();

	bool openFromFile(const std::string& filename, const std::string& passwd = "");
	bool openFromMem(const std::string& data, const std::string& passwd = "");
	bool close();

	bool getItems(std::map<int, ZipItem>& items);
	bool unzipToMem(const ZipItem& item, std::string& data);
	bool unzipToFile(const ZipItem& item, const std::string& filename);
	bool unzipAll(const std::string& dirName);
private:
	struct UnzipInternal;
	UnzipInternal* internal;
};

//压缩对象
class BASE_API Zip
{
public:
	Zip();
	~Zip();

	bool create(const std::string& passwd = "");
	bool saveToMem(std::string& data);
	bool saveToFile(const std::string& filename);

	bool addItemFromMem(const std::string& itemname, const std::string& dat);
	bool addItemFromFile(const std::string& itemname, const std::string& filename);
private:
	struct ZipInternal;
	ZipInternal* internal;
};

}
}