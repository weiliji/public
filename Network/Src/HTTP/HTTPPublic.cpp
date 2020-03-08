#include "Network/HTTP/Public.h"
#include "HTTPDefine.h"
#include "HTTPCache.h"
namespace Public {
namespace Network {
namespace HTTP {

struct ReadContent::ReadContentInternal
{
	shared_ptr<HTTPCache> cache;
	std::string			  filename;
	DataCalback			  callback;
	shared_ptr<ChunkData> chunk;

	WriteContenNotify*	  notify;


	uint32_t chunkReadCallback(const char* buffer, uint32_t size)
	{
		if (callback)
			callback(buffer, size);
		else
			cache->write(buffer, size);

		return size;
	}
};
ReadContent::ReadContent(const shared_ptr<Header>& header, WriteContenNotify* notify, CacheType type, const std::string& filename)
{
	internal = new ReadContentInternal;
	internal->notify = notify;
	internal->filename = filename;

	if (internal->filename.length() > 0)
	{
		internal->cache = make_shared<HTTPCacheFile>(internal->filename, false, false);
	}
	else if (type == CacheType_File)
	{
		internal->filename = ServerCacheFilePath::instance()->cachefilename();
		internal->cache = make_shared<HTTPCacheFile>(internal->filename, true, false);
	}
	else
	{
		internal->cache = make_shared<HTTPCacheMem>();
	}

	if (header)
	{
		Value chunkval = header->header(Transfer_Encoding);
		if (!chunkval.empty() && String::strcasecmp(chunkval.readString().c_str(), CHUNKED) == 0)
		{
			if (internal->cache)
			{
				internal->chunk = make_shared<ChunkData>();
				internal->chunk->setReadCallback(ChunkData::ReadCallback(&ReadContentInternal::chunkReadCallback, internal));

				if (internal->notify) internal->notify->ReadReady();
			}
		}
	}
}
ReadContent::~ReadContent()
{
	internal->cache = NULL;
	SAFE_DELETE(internal);
}
uint32_t ReadContent::size()
{
	return internal->cache->totalsize();
}

std::string ReadContent::cacheFileName() const
{
	return internal->filename;
}
uint32_t ReadContent::read(char* buffer, uint32_t maxlen, uint32_t pos) const
{
	return internal->cache->read(pos, buffer, maxlen);
}
std::string ReadContent::read()
{
	std::string bufferstr;
	char buffer[1024];
	while (1)
	{
		int readlen = internal->cache->read((uint32_t)bufferstr.length(), buffer, 1024);
		if (readlen <= 0) break;

		bufferstr += std::string(buffer, readlen);
	}

	return bufferstr;
}
bool ReadContent::readToFile(const std::string& filename) const
{
	FILE* fd = fopen(filename.c_str(), "wb+");
	if (fd == NULL) return false;

	char buffer[1024];
	int readpos = 0;
	while (1)
	{
		int readlen = internal->cache->read(readpos, buffer, 1024);
		if (readlen <= 0) break;

		readpos += readlen;

		fwrite(buffer, 1, readlen, fd);
	}

	fclose(fd);

	return true;
}

void ReadContent::setDataCallback(const DataCalback& callback)
{
	if (internal->chunk)
	{
		internal->callback = callback;
		if (internal->notify)
		{
			internal->notify->ReadReady();
		}
	}
}

uint32_t ReadContent::append(const char* buffer, uint32_t len, bool & endoffile)
{
	if (internal->chunk) return internal->chunk->append(buffer, len, endoffile);
	else if (internal->cache)
	{
		endoffile = false;
		return internal->cache->write(buffer, len);
	}
	return len;
}

struct WriteContent::WriteContentInternal
{
	shared_ptr<Header> header;
	WriteContenNotify* notify;
	shared_ptr<HTTPCache> cache;

	shared_ptr<ChunkData> chunk;

	virtual ~WriteContentInternal() {}
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

		header->headers[Content_Type] = contenttype;
	}

	uint32_t write(const char* buffer, uint32_t len)
	{
		while (len > 0)
		{
			int writelen = cache->write(buffer, len);
			if (writelen <= 0) return false;

			len -= writelen;
			buffer += writelen;
		}

		if (notify) notify->WriteNotify();

		return len;
	}
};

WriteContent::WriteContent(const shared_ptr<Header>& header, WriteContenNotify* notify, CacheType type)
{
	internal = new WriteContentInternal;
	internal->header = header;
	internal->notify = NULL;

	if (type == CacheType_Mem) internal->cache = make_shared<HTTPCacheMem>();
	else
	{
		std::string filename = ServerCacheFilePath::instance()->cachefilename();
		internal->cache = make_shared<HTTPCacheFile>(filename, true, true);
	}
}
WriteContent::~WriteContent()
{
	SAFE_DELETE(internal);
}

uint32_t WriteContent::write(const char* buffer, uint32_t len)
{
	return internal->write(buffer, len);
}
uint32_t WriteContent::write(const std::string& buffer)
{
	return internal->write(buffer.c_str(), (int)buffer.length());
}
bool WriteContent::writeFromFile(const std::string& filename, bool needdeletefile)
{
	internal->cache = make_shared<HTTPCacheFile>(filename, needdeletefile, true);
	internal->setWriteFileName(filename);

	if (internal->notify) internal->notify->WriteNotify();

	return true;
}
uint32_t WriteContent::size()
{
	return internal->cache->totalsize();
}
uint32_t WriteContent::append(const char* buffer, uint32_t len, bool & endoffile)
{
	endoffile = false;
	return 0;
}
std::string WriteContent::read()
{
	std::string data;
	char buffer[1024];

	while (1)
	{
		int readlen = internal->cache->read((uint32_t)data.length(), buffer, 1024);
		if (readlen <= 0) break;

		data += string(buffer, readlen);
	}

	return data;
}

void WriteContent::writeChunk(const char* buffer, uint32_t len)
{
	if (!internal->chunk)
	{
		internal->header->headers[Transfer_Encoding] = CHUNKED;

		internal->chunk = make_shared<ChunkData>(ChunkData::WriteCallback(&WriteContentInternal::write, internal));
	}
	bool endoffalse = false;
	if (internal->chunk)
		internal->chunk->append(buffer, len, endoffalse);
}
void WriteContent::writeChunkEnd()
{
	writeChunk(NULL, 0);
}

}
}
}