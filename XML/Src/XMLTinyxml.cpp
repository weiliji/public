#include "XMLTinyxml.h"
#ifdef WIN32
#include <windows.h>
#else
#include <errno.h>
#include <string.h>
#include <iconv.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <map>
#ifdef WIN32
#define strcasecmp _stricmp
#endif
namespace Public{
namespace XML{

// void  printXMLLibVersion()
// {
// 	printf("*************************************************\r\n");
// 	printf("%s Version:%d.%d.%d svn:%s Built in %s\r\n", "XML Lib", r_major, r_minor, r_build, versionalias, __DATE__);
// 	printf("*************************************************\r\n\r\n");
// }
void parseTiXmlElementAttribute(TiXmlElement* pElement,XMLObject::Child& child)
{
	if(pElement == NULL)
	{
		return;
	}
	
	TiXmlAttribute* attribute = pElement->FirstAttribute();
	while(attribute != NULL)
	{
		child.attribute(attribute->Name(),attribute->Value());

		attribute = attribute->Next();
	}
}

void parseTiXmlElementAndAddChild(TiXmlNode* pNode,XMLObject::Child& child)
{
	if(pNode == NULL)
	{
		return;
	}

	TiXmlNode* pChildNode = pNode->FirstChild();
	while(pChildNode != NULL)
	{
		if(pChildNode->Type() == TiXmlNode::TEXT)
		{
			child.data(pChildNode->Value());
		}
		else if(pChildNode->Type() == TiXmlNode::ELEMENT)
		{
			XMLObject::Child& subchild = child.addChild(XMLObject::Child(pChildNode->Value()));
			parseTiXmlElementAttribute(pChildNode->ToElement(),subchild);
			parseTiXmlElementAndAddChild(pChildNode,subchild);
		}
		pChildNode = pChildNode->NextSibling();
	}
}

void buildTiXmlElementFormChild(XMLObject::Child& child,TiXmlElement* pElement,XMLObject::Encoding old,XMLObject::Encoding encode)
{
	if(child.isEmpty())
	{
		return;
	}
	if(!child.data().empty())
	{
		TiXmlText* childElement = new TiXmlText(child.data().readString().c_str());

		pElement->LinkEndChild(childElement);
	}

	XMLObject::Attribute atti = child.firstAttribute();
	while(!atti.isEmpty())
	{
		pElement->SetAttribute(buildVaildXmlString(atti.name,atti.nametype,old,encode).c_str(),buildVaildXmlString(atti.value.readString(),"",old,encode).c_str());

		atti = child.nextAttribute();
	}

	XMLObject::Child subchild = child.firstChild();
	while(!subchild.isEmpty())
	{
		TiXmlElement* childElement = new TiXmlElement(buildVaildXmlString(subchild.name(),subchild.nametype(),old,encode).c_str());
		if(childElement == NULL)
		{
			return;
		}

		buildTiXmlElementFormChild(subchild,childElement,old,encode);

		pElement->LinkEndChild(childElement);
		subchild = child.nextChild();
	}
}

void findVersionAndEncoding(const std::string& xml,std::string& ver,XMLObject::Encoding& encode)
{
	const char* tmpbuf = xml.c_str();
	do
	{
		const char* tmp = strchr(tmpbuf,'\r');
		if(tmp == NULL)
		{
			break;
		}

		std::string buffer(tmpbuf,tmp-tmpbuf);
		const char* tmpstart = strstr(buffer.c_str(),"<?");
		if(tmpstart == NULL)
		{
			tmpbuf = tmp + 1;
			continue;
		}
		const char*tmpver = strstr(buffer.c_str(),"version");
		if(tmpver == NULL)
		{
			break;
		}
		const char* verstart = strchr(tmpver,'"');
		if(verstart == NULL)
		{
			break;
		}
		verstart += 1;
		char verbuf[256];
		int index = 0;
		while(*verstart != '"' && index < 256)
		{
			verbuf[index] = *verstart;
			index ++;
			verstart ++;
		}
		verbuf[index] = 0;
		ver = verbuf;

		
		const char*tmpcoding = strstr(buffer.c_str(),"encoding");
		if(tmpcoding == NULL)
		{
			break;
		}
		const char* codingstart = strchr(tmpcoding,'"');
		if(codingstart == NULL)
		{
			break;
		}
		codingstart += 1;
		char codingbuf[256];
		int codingindex = 0;
		while(*codingstart != '"' && codingindex < 256)
		{
			codingbuf[codingindex] = *codingstart;
			codingindex ++;
			codingstart ++;
		}
		codingbuf[codingindex] = 0;
		if(strcasecmp(codingbuf,"gb2312") == 0 || strcasecmp(codingbuf,"gbk") == 0)
		{
			encode = XMLObject::Encoding_GBK;
		}
		else if(strcasecmp(codingbuf,"utf-8") == 0)
		{
			encode = XMLObject::Encoding_UTF8;
		}
		break;
	}while(1);
}

std::string buildVaildXmlString(const std::string& name,const std::string& nametype,XMLObject::Encoding old,XMLObject::Encoding encode)
{
	if(old == XMLObject::Encoding_Unknown)
	{
		old = XMLObject::Encoding_GBK;
	}
	if(encode == XMLObject::Encoding_Unknown)
	{
		encode = old;
	}

	std::string key = name;
	if (nametype.length() > 0) key = nametype + ":" + name;

	if(old == encode)
	{
		return key;
	}

	if(old == XMLObject::Encoding_UTF8)
	{
		return String::utf82ansi (key);
	}
	else
	{
		return String::ansi2utf8(key);
	}
}

}
}
