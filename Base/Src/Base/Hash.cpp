#include "Base/Hash.h"

namespace Public {
namespace Base {

/*
  * BKDR string hash function. Based on the works of Kernighan, Dennis and Pike.
  * addr :http://www.360doc.com/content/14/0610/10/14505022_385328710.shtml
 */
uint64_t Hash::bkdr(const std::string& data)
{
	uint64_t output = 0;

	static int magicnum = 131; // the magic number, 31, 131, 1313, 13131, etc.. orz..
	
	const char* buffertmp = data.c_str();
	size_t bufferlen = data.length();

	for (size_t i = 0; i < bufferlen; i++)
	{
		output = output * magicnum + (buffertmp[i]);
	}

	return output;
}

}
}
