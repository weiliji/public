#include "XMLTinyxml.h"
#include <stdarg.h>
#include <stdio.h>
#include <map>
#ifdef WIN32
#define strcasecmp _stricmp
#endif
namespace Public
{
namespace Base
{

// void  printXMLLibVersion()
// {
// 	printf("*************************************************\r\n");
// 	printf("%s Version:%d.%d.%d svn:%s Built in %s\r\n", "XML Lib", r_major, r_minor, r_build, versionalias, __DATE__);
// 	printf("*************************************************\r\n\r\n");
// }
void parseTiXmlElementAttribute(TiXmlElement *pElement, XML::Child &child)
{
	if (pElement == NULL)
	{
		return;
	}

	TiXmlAttribute *attribute = pElement->FirstAttribute();
	while (attribute != NULL)
	{
		child.addAttribute(attribute->Name(), attribute->Value());

		attribute = attribute->Next();
	}
}

void parseTiXmlElementAndAddChild(TiXmlNode *pNode, XML::Child &child)
{
	if (pNode == NULL)
	{
		return;
	}

	TiXmlNode *pChildNode = pNode->FirstChild();
	while (pChildNode != NULL)
	{
		if (pChildNode->Type() == TiXmlNode::TEXT)
		{
			child.data(pChildNode->Value());
		}
		else if (pChildNode->Type() == TiXmlNode::ELEMENT)
		{
			XML::Child &subchild = child.addChild(XML::Child(pChildNode->Value()));
			parseTiXmlElementAttribute(pChildNode->ToElement(), subchild);
			parseTiXmlElementAndAddChild(pChildNode, subchild);
		}
		pChildNode = pChildNode->NextSibling();
	}
}

void buildTiXmlElementFormChild(const XML::Child &child, TiXmlElement *pElement, CharSetEncoding old, CharSetEncoding encode)
{
	if (child.isEmpty())
	{
		return;
	}
	if (!child.data().empty())
	{
		TiXmlText *childElement = new TiXmlText(child.data().readString().c_str());

		pElement->LinkEndChild(childElement);
	}

	for (XML::AttributeIterator iter = child.attribute(); iter; iter++)
	{
		pElement->SetAttribute(buildVaildXmlString(iter->name(), old, encode).c_str(), buildVaildXmlString(iter->value().readString(), old, encode).c_str());
	}

	for (XML::ChildIterator iter = child.child(); iter; iter++)
	{
		TiXmlElement *childElement = new TiXmlElement(buildVaildXmlString(iter->name(), old, encode).c_str());
		if (childElement == NULL)
		{
			return;
		}

		buildTiXmlElementFormChild(*iter, childElement, old, encode);

		pElement->LinkEndChild(childElement);
	}
}

void findVersionAndEncoding(const std::string &xml, std::string &ver, CharSetEncoding &encode)
{
	const char *tmpbuf = xml.c_str();
	do
	{
		const char *tmp = strchr(tmpbuf, '\r');
		if (tmp == NULL)
		{
			break;
		}

		std::string buffer(tmpbuf, tmp - tmpbuf);
		const char *tmpstart = strstr(buffer.c_str(), "<?");
		if (tmpstart == NULL)
		{
			tmpbuf = tmp + 1;
			continue;
		}
		const char *tmpver = strstr(buffer.c_str(), "version");
		if (tmpver == NULL)
		{
			break;
		}
		const char *verstart = strchr(tmpver, '"');
		if (verstart == NULL)
		{
			break;
		}
		verstart += 1;
		char verbuf[256];
		int index = 0;
		while (*verstart != '"' && index < 256)
		{
			verbuf[index] = *verstart;
			index++;
			verstart++;
		}
		verbuf[index] = 0;
		ver = verbuf;

		const char *tmpcoding = strstr(buffer.c_str(), "encoding");
		if (tmpcoding == NULL)
		{
			break;
		}
		const char *codingstart = strchr(tmpcoding, '"');
		if (codingstart == NULL)
		{
			break;
		}
		codingstart += 1;
		char codingbuf[256];
		int codingindex = 0;
		while (*codingstart != '"' && codingindex < 256)
		{
			codingbuf[codingindex] = *codingstart;
			codingindex++;
			codingstart++;
		}
		codingbuf[codingindex] = 0;
		if (strcasecmp(codingbuf, "gb2312") == 0 || strcasecmp(codingbuf, "gbk") == 0)
		{
			encode = CharSetEncoding_GBK;
		}
		else if (strcasecmp(codingbuf, "utf-8") == 0)
		{
			encode = CharSetEncoding_UTF8;
		}
		break;
	} while (1);
}

std::string buildVaildXmlString(const std::string &name, CharSetEncoding old, CharSetEncoding encode)
{
	if (old == CharSetEncoding_Unknown)
	{
		old = CharSetEncoding_GBK;
	}
	if (encode == CharSetEncoding_Unknown)
	{
		encode = old;
	}

	std::string key = name;

	if (old == encode)
	{
		return key;
	}

	if (old == CharSetEncoding_UTF8)
	{
		return String::utf82ansi(key);
	}
	else
	{
		return String::ansi2utf8(key);
	}
}

} // namespace Base
} // namespace Public
