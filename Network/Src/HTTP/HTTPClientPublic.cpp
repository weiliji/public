#include "Network/HTTP/Client.h"
#include "HTTPDefine.h"
#include "HTTPCache.h"
#include "HTTPCommunication.h"
namespace Public {
namespace Network {
namespace HTTP {

struct  ClientResponse::ClientResponseInternal :public WriteContenNotify
{
	shared_ptr<Communication> commu;
	shared_ptr<Header>	header;
	shared_ptr<ReadContent>	content;

	virtual void WriteNotify()
	{
		commu->start();
	}
	virtual void setWriteFileName(const std::string& filename) {}
	virtual void ReadReady() {}
};

ClientResponse::ClientResponse(const shared_ptr<Communication>& commu, CacheType type, const std::string& filename)
{
	internal = new ClientResponseInternal;

	if (commu)
		internal->header = commu->recvHeader;
	if (internal->header == NULL)
		internal->header = make_shared<Header>();

	internal->commu = commu;
	internal->content = make_shared<ReadContent>(internal->header, internal, type, filename);
}
ClientResponse::~ClientResponse()
{
	SAFE_DELETE(internal);
}

shared_ptr<ReadContent> ClientResponse::content() const
{
	return internal->content;
}

shared_ptr<Header>  ClientResponse::header() const
{
	return internal->header;
}

struct ClientRequest::ClientRequestInternal :public WriteContenNotify
{
	ClientRequest::DisconnectCallback	disconnected;
    ClientRequest::RecvCallback         recvcallback;

	shared_ptr<Header> header;
	shared_ptr<WriteContent> content;

	weak_ptr<Communication> commu;

	uint32_t	timeout;

	virtual void WriteNotify()
	{
	}
	virtual void ReadReady() {}
	virtual void setWriteFileName(const std::string& filename)
	{
		std::string contenttype = "application/octet-stream";

		do
		{
			int pos = (int)String::lastIndexOf(filename, ".");
			if (pos == -1) break;

			std::string pres = filename.c_str() + pos + 1;

			uint32_t mimetypeslen = 0;
			ContentInfo* mimetypes = MediaType::info(mimetypeslen);

			for (uint32_t i = 0; i < mimetypeslen; i++)
			{
				if (String::strcasecmp(pres.c_str(), mimetypes[i].filetype) == 0)
				{
					contenttype = mimetypes[i].contentType;
					break;
				}
			}

		} while (0);

		header->push(Content_Type, contenttype);
	}
};

ClientRequest::ClientRequest(const std::string& method, const std::string& url, CacheType type)
{
	internal = new ClientRequestInternal;

	internal->header = make_shared<Header>();
	internal->content = make_shared<WriteContent>(internal->header, internal, type);
	internal->header->method = method;
	internal->header->url = url;

	internal->timeout = 10000;
}
ClientRequest::~ClientRequest()
{
	SAFE_DELETE(internal);
}

shared_ptr<WriteContent> ClientRequest::content()
{
	return internal->content;
}

uint32_t& ClientRequest::timeout()
{
	return internal->timeout;
}

ClientRequest::DisconnectCallback&	ClientRequest::discallback()
{
	return internal->disconnected;
}

ClientRequest::RecvCallback& ClientRequest::recvcallback()
{
    return internal->recvcallback;
}

shared_ptr<Header> ClientRequest::header()
{
	return internal->header;
}


}
}
}