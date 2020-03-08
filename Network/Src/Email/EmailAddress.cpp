#include "Network/Email/Address.h"

namespace Public{
namespace Network{
namespace Email{
struct Address::AddressInternal
{
	std::string		addr;
	std::string		name;
};
Address::Address()
{
	internal = new AddressInternal;
}
Address::Address(const std::string& emailaddr, const std::string& emailname)
{
	internal = new AddressInternal;
	internal->addr = emailaddr;
	internal->name = emailname;
}
Address::Address(const Address& eaddr)
{
	internal = new AddressInternal;
	*internal = *eaddr.internal;
}
Address::~Address()
{
	SAFE_DELETE(internal);
}

const std::string Address::emailAddr() const
{
	return internal->addr;
}
const std::string Address::emailName() const
{
	return internal->name;
}
bool Address::emtpy() const
{
	return internal->addr.length() <= 0;
}
std::string Address::toString() const
{
	return internal->addr;
}
}
};
};


