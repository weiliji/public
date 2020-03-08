#include "Base/zip.h"
#include "Base/File.h"
#include "Base/Guid.h"
#include "Base/Time.h"
#include "Base/String.h"
#include "Base/Guard.h"
#include "miniZip/unzip.h"

#define READBUFFERSIZE 4096
namespace Public {
namespace Base {

struct Unzip::UnzipInternal
{
public:
	UnzipInternal()
	{
		m_zFile = NULL;
	}

	~UnzipInternal() 
	{
		close();
	}

	bool openFromFile(const std::string& filename, const std::string& passwd)
	{
		m_zFile = unzOpen64(filename.c_str());
		if (m_zFile == NULL)
		{
			return false;
		}

		unz_global_info64 ginfo;
		memset(&ginfo, 0, sizeof(ginfo));
		int err = unzGetGlobalInfo64(m_zFile, &ginfo);
		if (err != UNZ_OK)
		{
			return false;
		}

		std::vector<std::string> tmp_fileNameList;
		for (uint64_t i = 0; i < ginfo.number_entry; i++)
		{
			unz_file_info64 finfo;
			char filename[256] = {};
			unzGetCurrentFileInfo64(m_zFile, &finfo, filename, 256, NULL, 0, NULL, 0);
			size_t pos = String::lastIndexOf(filename, "/");
			if ((size_t)pos == strlen(filename) - 1)
			{
				//目录，目录结尾带"/"
			}
			else
			{
				tmp_fileNameList.push_back(filename);
			}

			if (i < ginfo.number_entry && unzGoToNextFile(m_zFile) != UNZ_OK)
			{
				break;
			}
		}

		unzGoToFirstFile(m_zFile);

		Guard locker(m_mutex);
		{
			for (size_t i = 0; i < tmp_fileNameList.size(); i++)
			{
				Unzip::ZipItem item;
				item.index = (int)i;
				item.filename = tmp_fileNameList[i];
				m_fileNameList[(int)i] = item;
			}
		}

		m_password = passwd;

		return true;

	}
	bool openFromMem(const std::string& data, const std::string& passwd)
	{
		return false;
	}

	bool close()
	{
		if (m_zFile != NULL)
		{
			unzCloseCurrentFile(m_zFile);
			unzClose(m_zFile);
		}
		m_zFile = NULL;

		return true;
	}

	bool getItem(std::map<int, Unzip::ZipItem>& item)
	{
		Guard locker(m_mutex);
		item = m_fileNameList;
		return true;
	}

	bool unzipToMem(const Unzip::ZipItem& item, std::string& data)
	{
		{
			Guard locker(m_mutex);

			if (m_zFile == NULL)
			{
				return false;
			}

			std::map<int, Unzip::ZipItem>::iterator iter = m_fileNameList.find(item.index);
			if (iter == m_fileNameList.end() || item.filename != iter->second.filename)
			{
				return false;
			}
		}

		int err = unzLocateFile(m_zFile, item.filename.c_str(), 1);
		if (err != UNZ_OK)
		{
			return false;
		}

		err = unzOpenCurrentFilePassword(m_zFile, m_password.c_str());
		if (err != UNZ_OK)
		{
			return false;
		}

		bool result = false;
		char readBuf[READBUFFERSIZE] = {};
		while (1)
		{
			memset(readBuf, 0, READBUFFERSIZE);
			int readSize = unzReadCurrentFile(m_zFile, readBuf, READBUFFERSIZE);
			if (readSize < 0)
			{
				//error
				break;
			}
			else
			{
				if (readSize == 0)
				{
					//read eof
					result = true;
					break;
				}

				data += string(readBuf, readSize);
			}
		}

		unzCloseCurrentFile(m_zFile);

		return result;
	}
	bool unzipToFile(const Unzip::ZipItem& item, const std::string& filename)
	{
		{
			Guard locker(m_mutex);

			if (m_zFile == NULL)
			{
				return false;
			}

			std::map<int, Unzip::ZipItem>::iterator iter = m_fileNameList.find(item.index);
			if (iter == m_fileNameList.end() || item.filename != iter->second.filename)
			{
				return false;
			}
		}

		int err = unzLocateFile(m_zFile, item.filename.c_str(), 1);
		if (err != UNZ_OK)
		{
			return false;
		}

		err = unzOpenCurrentFilePassword(m_zFile, m_password.c_str());
		if (err != UNZ_OK)
		{
			return false;
		}

		if (!_writeToFile(m_zFile, filename))
		{
			return false;
		}

		unzCloseCurrentFile(m_zFile);

		return true;;
	}

	bool unzipAll(const std::string& dirName)
	{
		{
			Guard locker(m_mutex);
			if (m_zFile == NULL)
			{
				return false;
			}
		}

		std::string tmp_name = dirName;
		size_t pos = String::lastIndexOf(dirName, "/");
		if ((size_t)pos != dirName.length() - 1)
		{
			//没找到"/",加上
			tmp_name += PATH_SEPARATOR;
		}

		int err = unzGoToFirstFile(m_zFile);
		if (err != UNZ_OK)
		{
			return false;
		}

		unz_global_info64 ginfo = {};
		unzGetGlobalInfo64(m_zFile, &ginfo);

		for (uint64_t i = 0; i < ginfo.number_entry; i++)
		{
			unz_file_info64 finfo;
			char filename[256] = {};
			err = unzGetCurrentFileInfo64(m_zFile, &finfo, filename, 256, NULL, 0, NULL, 0);
			if (err != UNZ_OK)
			{
				continue;
			}

			size_t pos = String::lastIndexOf(filename, "/");
			if ((size_t)pos == strlen(filename) - 1)
			{
				//目录
				std::vector<string> vec = String::split(filename, "/");
				std::string tmpDirPath = tmp_name;
				for (uint32_t i = 0; i < vec.size(); i++)
				{
					tmpDirPath += vec[i] + "/";
					if (!File::access(tmpDirPath, File::accessExist))
					{
						File::makeDirectory(tmpDirPath);
					}
				}
			}
			else
			{
				//文件
				int err = unzOpenCurrentFilePassword(m_zFile, m_password.c_str());
				if (err != UNZ_OK)
				{
					return false;
				}

				std::string writeFileName = tmp_name + string(filename);

				if (!_writeToFile(m_zFile, writeFileName))
				{
					//logdebug("unzip %s fail", filename);
				}

				unzCloseCurrentFile(m_zFile);
			}

			if (i < ginfo.number_entry && unzGoToNextFile(m_zFile) != UNZ_OK)
			{
				break;
			}
		}

		return true;
	}
private:
	bool _writeToFile(unzFile zfile, const std::string& filename)
	{
		FILE* pfile = fopen(filename.c_str(), "wb+");
		if (pfile == NULL)
		{
			return false;
		}
		bool result = false;
		char readBuf[READBUFFERSIZE] = {};
		while (1)
		{
			int readSize = unzReadCurrentFile(m_zFile, readBuf, READBUFFERSIZE);
			if (readSize < 0)
			{
				//error
				fclose(pfile);
				pfile = NULL;
				File::remove(filename);
				break;
			}
			else
			{
				if (readSize == 0)
				{
					//read eof
					result = true;
					break;
				}

				int writeSize = (int)fwrite(readBuf, 1, readSize, pfile);
				if (writeSize != readSize)
				{
					//write error
					break;
				}
			}
		}

		if (pfile != NULL)
		{
			fclose(pfile);
		}

		return result;
	}
private:
	unzFile								 m_zFile;
	std::string							 m_password;
	Mutex								 m_mutex;
	std::map<int, Unzip::ZipItem>  m_fileNameList;
};

Unzip::Unzip()
{
	internal = new UnzipInternal();
}
Unzip::~Unzip()
{
	SAFE_DELETE(internal);
}
bool Unzip::openFromFile(const std::string& filename, const std::string& passwd) 
{ 
	return internal->openFromFile(filename, passwd); 
}
bool Unzip::openFromMem(const std::string& data, const std::string& passwd) 
{
	return internal->openFromMem(data, passwd);
}
bool Unzip::close()
{
	return internal->close();
}
bool Unzip::getItems(std::map<int, ZipItem>& items)
{
	return internal->getItem(items);
}
bool Unzip::unzipToMem(const ZipItem& item, std::string& data)
{
	return internal->unzipToMem(item, data);
}
bool Unzip::unzipToFile( const ZipItem& item, const std::string& filename)
{
	return internal->unzipToFile(item, filename);
}
bool Unzip::unzipAll(const std::string& dirName)
{
	return internal->unzipAll(dirName);
}

}
}