#include "Base/Excel.h"
#include "Read_Workbook.h"
#include "Write_WorkBook.h"

namespace Public {
namespace Base {

shared_ptr<WorkBook> Excel::read(const std::string& xlsfile)
{
	return Read_WorkBook::read(xlsfile);
}

shared_ptr<WorkBook> Excel::create(const std::string& xlsfile)
{
	return Write_WorkBook::create(xlsfile);
}


}
}

