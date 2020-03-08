#include "Network/HTTP/Public.h"

namespace Public {
namespace Network {
namespace HTTP {

struct ChunkData::ChunkDataInternal
{
	WriteCallback writecallback;
	ReadCallback readcallback;

	uint32_t		chunkbodysize;

	ChunkDataInternal() :chunkbodysize(0) {}
};

ChunkData::ChunkData()
{
	internal = new ChunkDataInternal();
}
ChunkData::ChunkData(const WriteCallback& writecallback)
{
	internal = new ChunkDataInternal();
	internal->writecallback = writecallback;
}
ChunkData::~ChunkData()
{
	SAFE_DELETE(internal);
}

void ChunkData::setReadCallback(const ReadCallback& readcallback)
{
	internal->readcallback = readcallback;
}

uint32_t ChunkData::append(const char* bufferptr, uint32_t len, bool & endoffile)
{
	const char* buffer = bufferptr;
	endoffile = false;

	while (len >= (int)strlen(HTTPSEPERATOR))
	{
		//find 
		if (internal->chunkbodysize == 0)
		{
			int pos = (int)String::indexOf(buffer, len, HTTPSEPERATOR);
			if (pos == -1) return (uint32_t)(buffer - bufferptr);

			std::string chunksizestr = std::string(buffer, pos);
			sscanf(String::tolower(chunksizestr).c_str(), "%x", &internal->chunkbodysize);
			//chuned eof
			if (internal->chunkbodysize == 0)
			{
				if ((int)len < (int)(pos + strlen(HTTPSEPERATOR) * 2)) return (uint32_t)(buffer - bufferptr);
				//是否结束标识 2个HTTPHREADERLINEEND 
				if (memcmp(buffer + pos, HTTPSEPERATOR HTTPSEPERATOR, strlen(HTTPSEPERATOR) * 2) != 0) return (uint32_t)(buffer - bufferptr);

				len -= pos + (int)strlen(HTTPSEPERATOR) * 2;
				buffer += pos + (int)strlen(HTTPSEPERATOR) * 2;
				endoffile = true;

				return (uint32_t)(buffer - bufferptr);
			}
			buffer += pos + (int)strlen(HTTPSEPERATOR);
			len -= pos + (int)strlen(HTTPSEPERATOR);

			//数据长度加上结束标识长度
			internal->chunkbodysize += (int)strlen(HTTPSEPERATOR);
		}
		else
		{
			//当数据还不够时，返回数据长度，当数据满了，返回chunk缺少长度
			int datalen = min(len, (uint32_t)(internal->chunkbodysize > strlen(HTTPSEPERATOR) ?
				internal->chunkbodysize - strlen(HTTPSEPERATOR) : internal->chunkbodysize));

			//数据不够时，处理数据
			if (internal->chunkbodysize > strlen(HTTPSEPERATOR))
			{
				internal->readcallback(buffer, datalen);
			}
			buffer += datalen;
			len -= datalen;
			internal->chunkbodysize -= datalen;
		}
	}

	return (uint32_t)(buffer - bufferptr);
}
void ChunkData::write(const char* buffer, uint32_t len)
{
	//write chunk start
	{
		char buffer[32] = { 0 };
		sprintf(buffer, "%x" HTTPSEPERATOR, len);

		internal->writecallback(buffer, (int)strlen(buffer));
	}
	internal->writecallback(buffer, (int)len);

	//write chunk end
	{
		internal->writecallback(HTTPSEPERATOR, (int)strlen(HTTPSEPERATOR));
	}
}


}
}
}
