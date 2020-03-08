#ifndef __XMLTINYXML_H__
#define __XMLTINYXML_H__

#include "Base/XML.h"
#include "tinyxml/tinyxml.h"
namespace Public{
namespace Base{


void findVersionAndEncoding(const std::string& xml,std::string& ver, CharSetEncoding& encode);
void buildTiXmlElementFormChild(const XML::Child& child,TiXmlElement* pElement, CharSetEncoding old, CharSetEncoding encode);
void parseTiXmlElementAttribute(TiXmlElement* pElement,XML::Child& child);
void parseTiXmlElementAndAddChild(TiXmlNode* pElement,XML::Child& child);
std::string buildVaildXmlString(const std::string& name,  CharSetEncoding old, CharSetEncoding encode);

}
}

#endif //__XMLTINYXML_H__
