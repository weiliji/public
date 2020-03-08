#ifndef __WRITE_SIDE_H__
#define __WRITE_SIDE_H__
//#include "Write_Workbook.h"
#include "Base/Excel.h"

namespace Public {
namespace Base {

struct WorkBook::Side::LineInternal
{
	LineInternal():side(0) {}
	int				  side;
	shared_ptr<Color> color;
};

}
}
#endif //__WRITE_SIDE_H__