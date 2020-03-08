#pragma once

#include "Network/Socket/Socket.h"

namespace Public{
namespace Network{
namespace FTP{
	
class NETWORK_API Client
{
public:
	Client(const std::string& ip,int port = 21);
	virtual ~Client();

	bool connect(const std::string& user, const std::string& pass,uint32_t timeout_ms = 5000);

	bool isConnected();

	bool mkdir(const std::string& path);
	bool cddir(const std::string& path);
	bool cdup();
	bool rmdir(const std::string& path);
	std::list<Directory::Dirent> dir(const std::string& path);

	//return -1 error,>= 0 success
    int64_t size(const std::string& path);

	bool get(const std::string& localfile, const std::string& remoteFile);
	bool put(const std::string& localfile, const std::string& remoteFile);
	bool rename(const std::string& src, const std::string& dst);
	bool rmfile(const std::string& filename);
	
private:
	 struct ClientInternal;
	 ClientInternal* internal;
};

}
}
}
	
	