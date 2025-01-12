#ifndef __WRITE_RANGE_H__
#define __WRITE_RANGE_H__
#include "Base/Excel.h"
#include "xlslib/xlslib.h"

namespace Public {
namespace Base {
	
///excel中多行多列选择区域
class Write_Range :public WorkBook::Range
{
public:
	Write_Range(const shared_ptr<WorkBook::Sheet>& _worksheet, uint32_t _startRowNum, uint32_t _startColNum, uint32_t _stopRowNum, uint32_t _stopColNum, xlslib_core::range* _range);
	~Write_Range();

	bool font(const shared_ptr<WorkBook::Font>& _font);

	//--------------填充颜色
	bool fill(const WorkBook::Color& _color);
	bool fill(const shared_ptr<WorkBook::Color>& _color);

	//--------------显示格式
	bool format(const WorkBook::Format& _fmt);
	bool format(const shared_ptr<WorkBook::Format>& _fmt);

	//--------------单元格边框
	bool side(const WorkBook::Side& _sd);
	bool side(const shared_ptr<WorkBook::Side>& _sd);

	////获取某一行
	//shared_ptr<WorkBook::Row> row(uint32_t rowNum);
	////--------------显示，隐藏,整区域的隐藏显示
	void hidden(bool hidden_opt);

	//--------------合并,指定单元格
	bool merge();

public:
	shared_ptr<WorkBook::Sheet> worksheet;
	xlslib_core::range* _range;

	uint32_t startRowNum;
	uint32_t startColNum;
	uint32_t stopRowNum;
	uint32_t stopColNum;
};

}
}

#endif //__WRITE_RANGE_H__