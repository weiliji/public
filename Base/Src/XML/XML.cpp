#include "Base/XML.h"
#include "tinyxml/tinyxml.h"
#include "XMLTinyxml.h"
namespace Public
{
namespace Base
{

struct XML::XMLInternal
{
	XMLInternal() : encode(CharSetEncoding_Unknown), version("1.0") {}

	Child body;
	CharSetEncoding encode;
	std::string version;
};

XML::XML()
{
	internal = new XMLInternal();
}
XML::XML(const XML& xml)
{
    internal = new XMLInternal();

    *internal = *xml.internal;
}
XML::~XML()
{
	delete internal;
}
XML::Child &XML::body()
{
	return internal->body;
}
const XML::Child &XML::body() const
{
	return internal->body;
}

bool XML::isEmpty() const
{
	return internal->body.isEmpty();
}
void XML::clear()
{
	XML::Child root;

	internal->body.clear();
	internal->version = "1.0";
	internal->encode = CharSetEncoding_Unknown;
}
bool XML::parseFile(const std::string &file)
{
	clear();

	FILE *fd = fopen(file.c_str(), "rb");
	if (fd == NULL)
	{
		return false;
	}
	fseek(fd, 0, SEEK_END);
	int totalsize = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	char *buffer = new char[totalsize + 100];
	size_t readlen = fread(buffer, 1, totalsize, fd);
	buffer[readlen] = 0;
	fclose(fd);

	bool ret = parseBuffer(buffer);

	delete[] buffer;

	return ret;
}

bool XML::parseBuffer(const std::string &buf)
{
	clear();

	TiXmlDocument xml;
	xml.Parse(buf.c_str());
	if (xml.Error())
	{
		return false;
	}
	TiXmlNode *pNode = xml.FirstChild();
	while (pNode != NULL)
	{
		if (pNode->Type() == TiXmlNode::DECLARATION)
		{
			TiXmlDeclaration *declaration = pNode->ToDeclaration();
			const char *version = declaration->Version();
			const char *encoding = declaration->Encoding();
			if (version != NULL)
			{
				internal->version = version;
			}
			if (encoding != NULL)
			{
				if (String::strcasecmp(encoding, "gb2312") == 0 || String::strcasecmp(encoding, "gbk") == 0)
				{
					internal->encode = CharSetEncoding_GBK;
				}
				else if (String::strcasecmp(encoding, "utf-8") == 0)
				{
					internal->encode = CharSetEncoding_UTF8;
				}
			}
		}
		else
		{
			Child child(pNode->Value());
			parseTiXmlElementAttribute(xml.FirstChildElement(), child);
			parseTiXmlElementAndAddChild(pNode, child);

			internal->body.addChild(child);
		}
		pNode = pNode->NextSibling();
	}

	return true;
}
std::string XML::toString(CharSetEncoding encode) const
{
	if (isEmpty())
	{
		return "";
	}
	TiXmlDocument *doc = new TiXmlDocument;

	if (encode != CharSetEncoding_Unknown)
	{
		TiXmlDeclaration *declaration = new TiXmlDeclaration(internal->version.c_str(), encode == CharSetEncoding_UTF8 ? "UTF-8" : "gb2312", "");
		doc->LinkEndChild(declaration);
	}

	for (XML::ChildIterator iter = internal->body.child(); iter; iter++)
	{
		TiXmlElement *childelement = new TiXmlElement(buildVaildXmlString(iter->name(), internal->encode, encode).c_str());

		buildTiXmlElementFormChild(*iter, childelement, internal->encode, encode);

		doc->LinkEndChild(childelement);
	}
	TiXmlPrinter printer(true);
	doc->Accept(&printer);

	delete doc;

	return printer.CStr();
}
bool XML::saveAs(const std::string &file, CharSetEncoding encode) const
{
	FILE *fd = fopen(file.c_str(), "wb+");
	if (fd == NULL)
	{
		return false;
	}

	std::string str = toString(encode);

	fwrite(str.c_str(), 1, str.length(), fd);

	fclose(fd);

	return true;
}

} // namespace Base
} // namespace Public
