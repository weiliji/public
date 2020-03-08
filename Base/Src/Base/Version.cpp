//
//  Copyright (c)1998-2012,  Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: Version.cpp 31 2013-02-05 04:30:06Z  $
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Base/PrintLog.h"
#include "Base/Version.h"
#include "Base/String.h"
namespace {

static const char* month[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
};

} // namespace noname

namespace Public {
namespace Base {

Version::Version()
{
	Major = Minor = Build = 0;
}
Version::Version(const std::string& verstr)
{
	parseString(verstr);
}
std::string Version::toString() const
{
	return Revision == "" ? versionString() : versionString() + "-" + Revision;
}
std::string Version::versionString() const
{
	char ver[128];
	snprintf_x(ver, 127, "%d.%d.%d.%d", Major, Minor, OEM, Build);

	return ver;
}

bool Version::parseString(const std::string& versionstr)
{
#ifndef WIN32
#define sscanf_s sscanf
#endif
	Major = Minor = OEM = Build = 0;
	Revision = "";

	std::string versontmp = versionstr;
	const char* revsiontmp = strchr(versontmp.c_str(), '-');
	if (revsiontmp != NULL)
	{
		Revision = revsiontmp + 1;
		versontmp = std::string(versontmp.c_str(), revsiontmp - versontmp.c_str());
	}
	if (sscanf_s(versontmp.c_str(), "%d.%d.%d.%d", &Major, &Minor, &OEM ,&Build) == 4)
	{
		return true;
	}
	else if (sscanf_s(versontmp.c_str(), "%d.%d.%d", &Major, &Minor, &OEM) == 3)
	{
		return true;
	}
	else if (sscanf_s(versontmp.c_str(), "%d.%d", &Major, &Minor) == 2)
	{
		return true;
	}
	else if (sscanf_s(versontmp.c_str(), "%d", &Major) == 1)
	{
		return true;
	}
	return false;
}
bool Version::operator < (const Version& version) const
{
	int version1[4] = {Major,Minor,OEM,Build };
	int version2[4] = { version.Major,version.Minor,version.OEM,version.Build };

	for (int i = 0; i < 4; i++)
	{
		if (version1[i] < version2[i]) return true;
		else if (version1[i] > version2[i]) return false;
	}

	return strcmp(Revision.c_str(), version.Revision.c_str()) < 0;
}
bool Version::operator > (const Version& version) const
{
	int version1[4] = { Major,Minor,OEM,Build };
	int version2[4] = { version.Major,version.Minor,version.OEM,version.Build };

	for (int i = 0; i < 4; i++)
	{
		if (version1[i] > version2[i]) return true;
		else if (version1[i] < version2[i]) return false;
	}

	return strcmp(Revision.c_str(), version.Revision.c_str()) > 0;
}
bool Version::operator == (const Version& version) const
{
	int version1[4] = { Major,Minor,OEM,Build };
	int version2[4] = { version.Major,version.Minor,version.OEM,version.Build };

	for (int i = 0; i < 4; i++)
	{
		if (version1[i] != version2[i]) return false;
	}

	return strcmp(Revision.c_str(), version.Revision.c_str()) == 0;
}

SystemTime AppVersion::appDate = Time::getCurrentTime();

AppVersion::AppVersion(const std::string& name, int major, int minor, int build, const std::string& svnString, const std::string& dateString)
{
	this->name = name;
	this->Major = major;
	this->Minor = minor;
	this->Build = build;
	this->Revision = svnString;

	// 得到编译时间
	setAppDate(dateString);
}

AppVersion::AppVersion(const std::string& name, const std::string& versionstr, const std::string& dateString)
{
	this->name = name;
	
	parseString(versionstr);

	// 得到编译时间
	setAppDate(dateString);
}

void AppVersion::print() const
{
	loginfo("*************************************************\r\n");
	loginfo("%s Version:%s Built in %d/%02d/%02d\r\n", name.c_str(), toString().c_str(), date.year, date.month, date.day);
	loginfo("*************************************************\r\n\r\n");
}

void AppVersion::setAppDate(const std::string& dateString)
{
	int i = 0;
	for (i = 0; i < 12; i++) {
		if( strncmp(month[i], dateString.c_str(), 3) == 0 )
			break;
	}
	appDate.month = i+1;
	sscanf(dateString.c_str() + 3, "%d %d", &appDate.day, &appDate.year);
}

} // namespace Base
} // namespace Public

