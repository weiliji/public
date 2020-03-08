#pragma  once
#include "Base/IntTypes.h"

namespace Public {
namespace Base {

#define HTTPHEADER_WWWAUTHENTICATE "WWW-Authenticate"
#define HTTPHEADER_AUTHENTICATE "Authorization"

struct BASE_API WWW_Authenticate
{
public:
	typedef enum {
		Authenticate_Type_Basic,
		Authenticate_Type_Digest,
	}Authenticate_Type;
public:
	//检测认证信息
	static bool checkAuthenticate(const std::string& method, const std::string& username, const std::string& password, const std::string& pAuthorization);

	//构造一个认证请求信息
	static std::string buildWWWAuthenticate(const std::string& method, const std::string& username, const std::string& password, Authenticate_Type type);

	//构造一个认证回复信息
	static std::string buildAuthorization(const std::string& method, const std::string& username, const std::string& password, const std::string& uri, const std::string& wwwauthen);

	static std::string getAuthorizationUsreName(const std::string& wwwauthen);
};

}
}
