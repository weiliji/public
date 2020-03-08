#include "Base/wwwAuthenticate.h"
#include "Base/Md5.h"
#include "Base/Base64.h"
#include "Base/Value.h"
#include "Base/Time.h"
#include "Base/Sha1.h"
#include "Base/String.h"
#include "Base/PrintLog.h"

namespace Public {
namespace Base {

static std::string getAuthenAttr(const std::string& pAuthorization, const std::string& key)
{
	const char* tmpstr = pAuthorization.c_str();

	std::string flag = key + "=\"";
	const char* findstr = strstr(tmpstr, flag.c_str());
	if (findstr == NULL)
	{
		return "";
	}
	findstr += flag.length();
	const char* endstr = strchr(findstr, '"');
	if (endstr == NULL)
	{
		return "";
	}
	return std::string(findstr, endstr - findstr);
}
static std::string getGenerateRandom()
{
	char buffer[64];
	char* tmp = buffer;

	srand((unsigned int)Time::getCurrentMilliSecond());
	for (int i = 0; i < 8; i += 2)
	{
		sprintf(tmp + i, "%02x", rand() & 0xff);
	}
	return buffer;
}

static std::string buildBasicAuthorization(const std::string& username, const std::string& password)
{
	std::string authyenstr = username + ":" + password;

	std::string buffer = Base64::encode(authyenstr);

	return buffer;
}

static std::string buildDigestAuthorization_HA1_user_pass_realm(const std::string& algorithm,const std::string& username, const std::string& password, const std::string& realm,const std::string& nonce,const std::string& cnone)
{
	std::string strHash1src = username + ":" + realm + ":" + password;


	Md5 md5;
	md5.init();
	md5.update(strHash1src);
	std::string authen = md5.hex();
	
	if (String::iequals(algorithm, "md5-sess"))
	{
		std::string strsha1nonce = authen + ":" + nonce + ":" + cnone;

		Md5 md5;
		md5.init();
		md5.update(strsha1nonce);
		authen = md5.hex();
	}

	return authen;
}
static std::string buildDigestAuthorization_HA2_method_uri(const std::string& qop,const std::string& method, const std::string& uri,const std::string& entityBody)
{
	std::string strHash2src = method + ":" + uri;

	Md5 md5;
	md5.init();
	md5.update(strHash2src);

	if (String::iequals(qop, "auth-int"))
	{
		md5.update(":");
		md5.update(entityBody);
	}

	std::string authen = md5.hex();

	return authen;
}

static std::string buildDigestAuthorization_Response_HA1_nonceQop_HA2(const std::string& ha1, const std::string& ha2, const std::string& nonce, const std::string& qop, const std::string& cnone,const std::string& nc)
{
	Md5 md5;
	md5.init();

	md5.update(ha1);
	md5.update(":");

	md5.update(nonce);
	md5.update(":");

	if (qop.length() > 0)
	{
		md5.update(nc);
		md5.update(":");

		md5.update(cnone);
		md5.update(":");

		md5.update(qop);
		md5.update(":");
	}

	md5.update(ha2);

	std::string response = md5.hex();

	return response;
}

std::string WWW_Authenticate::buildWWWAuthenticate(const std::string& method, const std::string& username, const std::string& password, Authenticate_Type type)
{
	std::string authstring;
	if (type == Authenticate_Type_Basic)
	{
		std::string wwwauthen = std::string("Basic ") + "realm=\""+username+"\"";

		return wwwauthen;
	}
	else
	{
		std::string realmstr = getGenerateRandom();
		std::string noncestr = getGenerateRandom();

		std::string wwwauthen = std::string("Digest ") + "realm=\"" + realmstr + "\",nonce=\"" + noncestr + "\"";

		return wwwauthen;
	}
}
std::string WWW_Authenticate::buildAuthorization(const std::string& method, const std::string& username, const std::string& password, const std::string& uri, const std::string& wwwauthen)
{
	std::string authstring;

	if (NULL != strstr(wwwauthen.c_str(), "Basic"))
	{
		authstring = std::string("Basic ") + buildBasicAuthorization(username, password);
	}
	else if (NULL != strstr(wwwauthen.c_str(), "Digest"))
	{
		std::string algorithm = getAuthenAttr(wwwauthen, "algorithm");
		std::string realm = getAuthenAttr(wwwauthen, "realm");
		std::string nonce = getAuthenAttr(wwwauthen,"nonce");
		std::string qop = getAuthenAttr(wwwauthen, "qop");
		std::string entityBody;
		std::string cnonce;
		std::string nc;

		if (qop.length() > 0)
		{
			nc = "00000001";
			cnonce = getGenerateRandom();
		}

		std::string ha1 = buildDigestAuthorization_HA1_user_pass_realm(algorithm, username, password, realm, nonce, cnonce);
		std::string ha2 = buildDigestAuthorization_HA2_method_uri(qop, method, uri, entityBody);
		std::string response = buildDigestAuthorization_Response_HA1_nonceQop_HA2(ha1, ha2, nonce, qop, cnonce, nc);

		authstring = std::string("Digest username=\"") + username + "\", realm=\"" + realm + "\", nonce=\"" + nonce + "\", uri=\"" + uri + "\"";

		if (qop.length() > 0)
		{
			authstring += ", cnonce=\""+cnonce+"\", nc=\"" + nc + "\", qop=\"" +qop + "\"";
		}

		authstring += ", response=\"" + response + "\"";
	}

	return authstring;
}
std::string WWW_Authenticate::getAuthorizationUsreName(const std::string& pAuthorization)
{
	const char* authenstart = strstr(pAuthorization.c_str(), "Basic ");
	if (NULL != authenstart)
	{
		std::string authenresult = authenstart + strlen("Basic ");
		std::string authenstr = Base64::decode(authenresult);

		std::vector<std::string> autheninfos = String::split(authenstr, ":");

		return autheninfos.size() <= 0 ? "" : autheninfos[0];
	}
	else if (NULL != strstr(pAuthorization.c_str(), "Digest"))
	{
		std::string username = getAuthenAttr(pAuthorization, "username");

		return username;
	}

	return "";
}
bool WWW_Authenticate::checkAuthenticate(const std::string& method, const std::string& username, const std::string& password, const std::string& pAuthorization)
{
	const char* authenstart = strstr(pAuthorization.c_str(), "Basic ");
	if (NULL != authenstart)
	{
		std::string authenresult = authenstart + strlen("Basic ");

		return buildBasicAuthorization(username, password) == authenresult;
	}
	else if (NULL != strstr(pAuthorization.c_str(), "Digest"))
	{
		std::string algorithm = getAuthenAttr(pAuthorization, "algorithm");
		std::string realm = getAuthenAttr(pAuthorization, "realm");
		std::string nonce = getAuthenAttr(pAuthorization, "nonce");
		std::string qop = getAuthenAttr(pAuthorization, "qop");
	//	std::string username = getAuthenAttr(pAuthorization, "username");
		std::string entityBody;
		std::string cnonce = getAuthenAttr(pAuthorization, "cnonce");
		std::string nc = getAuthenAttr(pAuthorization, "nc");
		std::string url = getAuthenAttr(pAuthorization, "uri");


		std::string szResponse = getAuthenAttr(pAuthorization, "response");

		std::string ha1 = buildDigestAuthorization_HA1_user_pass_realm(algorithm, username, password, realm, nonce, cnonce);
		std::string ha2 = buildDigestAuthorization_HA2_method_uri(qop, method, url, entityBody);
		std::string response = buildDigestAuthorization_Response_HA1_nonceQop_HA2(ha1, ha2, nonce, qop, cnonce, nc);

		return szResponse == response;
	}

	return false;
}

}
}
