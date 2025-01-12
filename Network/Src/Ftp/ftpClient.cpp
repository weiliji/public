
#define CUSTOM_MEM_ALLOCATION_IMPL 

#include "Network/Ftp/Client.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include "ftplib.h"
#ifdef __cplusplus
}
#endif

namespace Public{
namespace Network{
namespace FTP{

static bool FtpIsInit = false;

struct Client::ClientInternal
{
	netbuf *			_pConnect;
	std::string			_ftpServerIP;
	int					_ftpServerPort;
	bool				_bConnect;

	ClientInternal() : _pConnect(NULL), _bConnect(false)
	{
		if (!FtpIsInit)
		{
			FtpInit();
			FtpIsInit = true;
		}
	}
};

Client::Client(const std::string& ip, int port)
{
	internal = new ClientInternal();
	internal->_ftpServerIP = ip;
	internal->_ftpServerPort = port;
}

Client::~Client()
{
	if (internal->_pConnect)
	{
		FtpClose(internal->_pConnect);
		FtpQuit(internal->_pConnect);
		internal->_pConnect = NULL;
	}
	SAFE_DELETE(internal);
}

bool Client::connect(const std::string& user, const std::string& pass, uint32_t timeout_ms)
{
	if (internal->_bConnect)
		return true;

	int iRet = FtpConnect((internal->_ftpServerIP + ":" + Value(internal->_ftpServerPort).readString()).c_str(), &internal->_pConnect);
	if (!iRet)
		return false;

	FtpOptions(FTPLIB_CONNMODE, FTPLIB_PASSIVE, internal->_pConnect);
	iRet = FtpLogin(user.c_str(), pass.c_str(), internal->_pConnect);
	if (!iRet)
		return false;

	internal->_bConnect = true;
	return true;
}

bool Client::isConnected()
{
	return internal->_bConnect;
}

bool Client::mkdir(const std::string& path)
{
	if (!isConnected())
		return false;

	return FtpMkdir(path.c_str(), internal->_pConnect) == 1;
}

bool Client::cddir(const std::string & path)
{
	if (!isConnected())
		return false;

	return FtpChdir(path.c_str(), internal->_pConnect) == 1;;
}

bool Client::cdup()
{
	if (!isConnected())
		return false;

	return FtpCDUp(internal->_pConnect) == 1;;
}

bool Client::rmfile(const std::string& filename)
{
	if (!isConnected())
		return false;

	return FtpDelete(filename.c_str(), internal->_pConnect) == 1;;
}

bool Client::rmdir(const std::string& path)
{
	if (!isConnected())
		return false;

	return FtpRmdir(path.c_str(), internal->_pConnect) == 1;;
}

std::list<Directory::Dirent> Client::dir(const std::string& path)
{
	if (!isConnected())
		return std::list<Directory::Dirent>();

	std::string filename = Value(Time::getCurrentMilliSecond()).readString() + "_" + Value((uint64_t)path.c_str()).readString() + ".txt";

	std::list<Directory::Dirent> dirs;

	//internal->filezilla->ListFile(path.c_str(), CServerPath());
	//
	//FILE* fd = fopen(filename.c_str(), "r");
	//if (fd != NULL)
	//{
	//	char buffer[512] = { 0 };
	//	while (fgets(buffer, 511, fd) != NULL)
	//	{
	//		std::vector<std::string> args = String::split(buffer, " ");
	//		if (args.size() != 9) continue;
	//		if (args[8] == "." || args[8] == "..") continue;
	//
	//		Directory::Dirent dir;
	//		dir.Type = args[0][0] == 'd' ? Directory::Dirent::DirentType_Dir : Directory::Dirent::DirentType_File;
	//		dir.FileSize = Value(args[4]).readUint64();
	//		dir.Name = args[8];
	//		dir.Path = path;
	//		dir.CompletePath = path + "/" + dir.Name;
	//		
	//		{
	//			const char* tmp = dir.Name.c_str();
	//			const char* tmp1 = strrchr(tmp, '.');
	//			if (tmp1 != NULL)
	//			{
	//				dir.SuffixName = tmp1 + 1;
	//			}
	//		}
	//
	//		dirs.push_back(dir);
	//	}
	//	fclose(fd);
	//}
	//File::remove(filename);

	return dirs;
}

int64_t Client::size(const std::string & path)
{
	if (!isConnected())
		return false;

	unsigned int size = 0;

	int iRet = FtpSize(path.c_str(), &size, FTPLIB_BINARY, internal->_pConnect);

	return (iRet ? size : -1);
}

bool Client::get(const std::string& localfile, const std::string& remoteFile)
{
	if (!isConnected())
		return false;

	// ...

	return false;
}
bool Client::put(const std::string & localfile, const std::string & remoteFile)
{
	if (!isConnected())
		return false;

	netbuf * pTransChn = NULL;
	int iRet = FtpAccess(remoteFile.c_str(), FTPLIB_FILE_WRITE, FTPLIB_BINARY, internal->_pConnect, &pTransChn);
	if (!iRet)
		return false;

	FILE * pf = NULL;
	pf = fopen(localfile.c_str(), "rb");
	if (!pf)
		return false;

	char * pSndBuf = new char[1024];

	while (true)
	{
		iRet = (int)fread(pSndBuf, 1, 1024, pf);
		iRet = FtpWrite(pSndBuf, iRet, pTransChn);
		if (feof(pf))
			break;
	}
	fclose(pf);
	pf = NULL;
	FtpClose(pTransChn);
	pTransChn = NULL;
	delete[] pSndBuf;
	pSndBuf = NULL;

	return true;
}


bool Client::rename(const std::string& src, const std::string& dst)
{
	if (!isConnected())
		return false;

	return FtpRename(src.c_str(), dst.c_str(), internal->_pConnect) == 1;;
}

}
}
}
	
	