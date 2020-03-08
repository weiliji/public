#ifndef __Public_EXCEL_H__
#define __Public_EXCEL_H__
#include "Base/Defs.h"
#include "Base/IntTypes.h"
#include "Base/Shared_ptr.h"
#include "Base/Value.h"
#include "Base/BaseTemplate.h"

namespace Public
{
namespace Base
{

//内容显示格式
typedef enum
{
	FomatType_GENERAL = 0,
	FomatType_NUMBER1,	 // 0
	FomatType_NUMBER2,	 // 0.00
	FomatType_NUMBER3,	 // #,##0
	FomatType_NUMBER4,	 // #,##0.00
	FomatType_CURRENCY1,   // "$"#,##0_);("$"#,##0)
	FomatType_CURRENCY2,   // "$"#,##0_);[Red]("$"#,##0)
	FomatType_CURRENCY3,   // "$"#,##0.00_);("$"#,##0.00)
	FomatType_CURRENCY4,   // "$"#,##0.00_);[Red]("$"#,##0.00)
	FomatType_PERCENT1,	// 0%
	FomatType_PERCENT2,	// 0.00%
	FomatType_SCIENTIFIC1, // 0.00E+00
	FomatType_FRACTION1,   // # ?/?
	FomatType_FRACTION2,   // # ??/??
	FomatType_DATE1,	   // M/D/YY
	FomatType_DATE2,	   // D-MMM-YY
	FomatType_DATE3,	   // D-MMM
	FomatType_DATE4,	   // MMM-YY
	FomatType_TIME1,	   // h:mm AM/PM
	FomatType_TIME2,	   // h:mm:ss AM/PM
	FomatType_TIME3,	   // h:mm
	FomatType_TIME4,	   // h:mm:ss
	FomatType_DATETIME,	// M/D/YY h:mm
	FomatType_ACCOUNTING1, // _(#,##0_);(#,##0)
	FomatType_ACCOUNTING2, // _(#,##0_);[Red](#,##0)
	FomatType_ACCOUNTING3, // _(#,##0.00_);(#,##0.00)
	FomatType_ACCOUNTING4, // _(#,##0.00_);[Red](#,##0.00)
	FomatType_CURRENCY5,   // _("$"* #,##0_);_("$"* (#,##0);_("$"* "-"_);_(@_)
	FomatType_CURRENCY6,   // _(* #,##0_);_(* (#,##0);_(* "-"_);_(@_)
	FomatType_CURRENCY7,   // _("$"* #,##0.00_);_("$"* (#,##0.00);_("$"* "-"??_);_(@_)
	FomatType_CURRENCY8,   // _(* #,##0.00_);_(* (#,##0.00);_(* "-"??_);_(@_)
	FomatType_TIME5,	   // mm:ss
	FomatType_TIME6,	   // [h]:mm:ss
	FomatType_TIME7,	   // mm:ss.0
	FomatType_SCIENTIFIC2, // ##0.0E+0
	FomatType_TEXT		   // @
} FomatType;

//定义对齐方式,X和Y可以用于|
typedef enum
{
	ALIGN_X_NONE = 0, //初始化
	ALIGN_X_LEFT,	 //左对齐
	ALIGN_X_CENTER,   //居中对齐
	ALIGN_X_RIGHT,	//右对齐
} ALIGN_X_Type;

typedef enum
{
	ALIGN_Y_TOP,	//向上对齐
	ALIGN_Y_CENTER, //居中对齐
	ALIGN_Y_BOTTOM, //向下对齐
} ALIGN_Y_Type;

//字体显示方式
typedef enum
{
	TxtoriType_NONE = 0,	 //正常模式
	TxtoriType_TOPBOTTOMTXT, //竖行，从上到下显示
	TxtoriType_90NOCLOCKTXT, //竖行，向右显示
	TxtoriType_90CLOCKTXT	//竖行，向左显示
} TxtoriType;

//边框显示方式，可以|的方式
enum
{
	SIDE_BOTTOM = 1, //底部边框
	SIDE_TOP = 2,	//上部边框
	SIDE_LEFT = 4,   //左部边框
	SIDE_RIGHT = 8,  //右部边框
};

//excel 的工作簿
class BASE_API WorkBook
{
public:
	//excel中的颜色
	class BASE_API Color
	{
	public:
		Color();
		Color(const Color &color);
		Color(uint8_t _r, uint8_t _g, uint8_t _b);
		virtual ~Color();

		Color &operator=(const Color &val);

	public:
		struct ColorInternal;
		ColorInternal *internal;
	};

	//excel中的字体
	class BASE_API Font
	{
	public:
		Font();
		Font(const Font &font);
		virtual ~Font();

		Font &operator=(const Font &val);

		static shared_ptr<Font> create(const std::string &name);

		//设置字体大小
		bool setSize(uint32_t size);
		//字体加粗
		bool setBold();
		//字体下划线
		bool setUnderline();
		//设置颜色
		bool setColor(const Color &color);
		bool setColor(const shared_ptr<Color> &color);

		std::string name() const;

	public:
		struct FontInternal;
		FontInternal *internal;
	};

	//excel中的显示格式
	class BASE_API Format
	{
	public:
		Format();
		Format(const Format &format);
		Format &operator=(const Format &val);
		virtual ~Format();

		//显示显示格式
		bool setFormat(FomatType type);

		//设置对齐方式
		bool setAlign(ALIGN_X_Type xalign, ALIGN_Y_Type yalign);

		//设置显示方式
		bool setTxtori(TxtoriType type);

	public:
		struct FormatInternal;
		FormatInternal *internal;
	};

	//excel中的单元格线
	class BASE_API Side
	{
	public:
		//设置边框显示类型,参见SIDE_BOTTOM
		Side(int val, const shared_ptr<Color> &sideColor);
		Side(int val, const Color &sideColor);
		Side(const Side &side);
		Side &operator=(const Side &val);
		~Side();
		//设置边框显示类型,参见SIDE_BOTTOM
		//		bool setType(xlslib_core::border_side_t side, xlslib_core::border_style_t style);
		/*virtual bool borderstyle(xlslib_core::border_side_t side, xlslib_core::border_style_t style);
		virtual bool bordercolor(xlslib_core::border_side_t side, unsigned8_t color);
		*/
	public:
		struct LineInternal;
		LineInternal *internal;
	};

	//excel中的某一个单元格
	class BASE_API Cell
	{
	public:
		Cell() {}
		virtual ~Cell() {}

		//--------------数据相关
		///获取数据,内容
		virtual Value data() const { return Value(); };

		//--------------行数相关
		///行号
		virtual unsigned int rowNum() = 0;
		///列号
		virtual unsigned int colNum() = 0;

		//--------------字体相关
		virtual bool font(const shared_ptr<Font> &font) { return false; }

		//--------------填充颜色
		virtual bool fill(const Color &color) { return false; }
		virtual bool fill(const shared_ptr<Color> &color) { return false; }

		//--------------显示格式
		virtual bool format(const Format &fmt) { return false; }
		virtual bool format(const shared_ptr<Format> &fmt) { return false; }

		//--------------单元格边框
		virtual bool side(const Side &sd) { return false; }
		virtual bool side(const shared_ptr<Side> &sd) { return false; }
	};

	///excel中多行多列选择区域
	class BASE_API Range
	{
	public:
		Range() {}
		virtual ~Range() {}

		//--------------字体相关
		virtual bool font(const shared_ptr<Font> &font) { return false; }

		//--------------填充颜色
		virtual bool fill(const Color &color) { return false; }
		virtual bool fill(const shared_ptr<Color> &color) { return false; }

		//--------------显示格式
		virtual bool format(const Format &fmt) { return false; }
		virtual bool format(const shared_ptr<Format> &fmt) { return false; }

		//--------------单元格边框
		virtual bool side(const Side &sd) { return false; }
		virtual bool side(const shared_ptr<Side> &sd) { return false; }

		////--------------显示，隐藏,整区域的隐藏显示
		//virtual void hidden(bool hidden_opt) {};

		//--------------合并,指定单元格
		virtual bool merge() { return false; }

	public:
		uint32_t startRowNum;
		uint32_t startColNum;
		uint32_t stopRowNum;
		uint32_t stopColNum;
	};
	//excel中的行,用于按行处理
	class BASE_API Row
	{
	public:
		Row() {}
		virtual ~Row() {}

		//--------------数据相关
		///获取数据,内容
		virtual Value data(uint32_t colNum) const { return Value(); };
		///设置数据，内容
		virtual shared_ptr<Cell> setData(uint32_t colNum, const Value &val) { return shared_ptr<Cell>(); };

		//--------------字体相关
		virtual bool font(const shared_ptr<Font> &font) { return false; }

		//--------------填充颜色
		virtual bool fill(const Color &color) { return false; }
		virtual bool fill(const shared_ptr<Color> &color) { return false; }

		//--------------显示格式
		virtual bool format(const Format &fmt) { return false; }
		virtual bool format(const shared_ptr<Format> &fmt) { return false; }

		//--------------区域，单行的指定列合并, 开始的列号，结束的列号
		virtual shared_ptr<Range> range(uint32_t startColNum, uint32_t stopColNum) { return shared_ptr<Range>(); }

		//--------------显示，隐藏,整行的隐藏显示
		//	virtual bool show(bool showflag) { return false; }

		//--------------当前行最大的列数
		virtual uint32_t maxColNum() = 0;

		//--------------当前行的行号
		virtual uint32_t rowNum() = 0;

		//行高
		virtual uint32_t height() { return 0; }
		virtual bool setHeight(uint32_t height) { return false; }
	};

	//excel中的列，用于按列处理
	class BASE_API Col
	{
	public:
		Col() {}
		virtual ~Col() {}

		//--------------数据相关
		///获取数据,内容
		virtual Value data(uint32_t rowNum) const { return Value(); };
		///设置数据，内容
		virtual shared_ptr<Cell> setData(uint32_t rowNum, const Value &val) { return shared_ptr<Cell>(); };

		//--------------字体相关
		virtual bool font(const shared_ptr<Font> &font) { return false; }

		//--------------填充颜色
		virtual bool fill(const Color &color) { return false; }
		virtual bool fill(const shared_ptr<Color> &color) { return false; }

		//--------------显示格式
		virtual bool format(const Format &fmt) { return false; }
		virtual bool format(const shared_ptr<Format> &fmt) { return false; }

		//--------------区域，单行的指定行合并, 开始的行号，结束的行号
		virtual shared_ptr<Range> range(uint32_t startRowNum, uint32_t stopRowNum) { return shared_ptr<Range>(); }

		//--------------显示，隐藏,整行的隐藏显示
		//	virtual bool show(bool showflag) { return false; }

		//--------------当前列的列号
		virtual uint32_t colNum() = 0;

		//--------------当前列的最大的行数
		virtual uint32_t maxRowNum() = 0;

		//列宽
		//virtual uint32_t width() { return 0; }
		//virtual bool setWidth(uint32_t height) { return false; }
	};

	//excel中的sheet
	class BASE_API Sheet
	{
	public:
		Sheet() {}
		virtual ~Sheet() {}

		//sheet名称
		virtual std::string name() { return ""; }

		//--------------最大的行数
		virtual uint32_t maxRowNum() = 0;
		//--------------最大的列数
		virtual uint32_t maxColNum() = 0;

		//获取某一行
		virtual shared_ptr<Row> row(uint32_t rowNum) = 0;
		//获取某一列
		virtual shared_ptr<Col> col(uint32_t colNum) = 0;

		virtual shared_ptr<Cell> cell(uint32_t rowNum, uint32_t colNum) = 0;

		//--------------数据相关
		///获取数据,内容
		virtual Value data(uint32_t rowNum, uint32_t colNum) const { return Value(); };
		///设置数据，内容
		virtual shared_ptr<Cell> setData(uint32_t rowNum, uint32_t colNum, const Value &val) { return shared_ptr<Cell>(); };

		//--------------字体相关
		virtual bool font(const shared_ptr<Font> &font) { return false; }

		//--------------填充颜色
		virtual bool fill(const Color &color) { return false; }
		virtual bool fill(const shared_ptr<Color> &color) { return false; }

		//--------------显示格式
		virtual bool format(const Format &fmt) { return false; }
		virtual bool format(const shared_ptr<Format> &fmt) { return false; }

		//--------------合并
		virtual shared_ptr<Range> range(uint32_t startRowNum, uint32_t startColNum, uint32_t stopRowNum, uint32_t stopColNum) { return shared_ptr<Range>(); }

		//--------------默认宽、高
		virtual bool defaultRowHeight(uint16_t height) { return false; } // sets column widths to 1/256 x width of "0"
		virtual bool defaultColwidth(uint16_t width) { return false; }   // in points (Excel uses twips, 1/20th of a point, but xlslib didn't)
	};

public:
	WorkBook() {}
	virtual ~WorkBook() {}

	//创建一个sheet
	virtual shared_ptr<Sheet> addSheet(const std::string &name) { return shared_ptr<Sheet>(); }

	//获取一个sheet,num从0开始
	virtual shared_ptr<Sheet> getSheet(uint32_t num) { return shared_ptr<Sheet>(); }

	virtual uint32_t sheetCount() { return 0; }
};

class BASE_API Excel
{
public:
	///读取一个xls文件
	static shared_ptr<WorkBook> read(const std::string &xlsfile);

	//创建一个xls文件
	static shared_ptr<WorkBook> create(const std::string &xlsfile);
};

} // namespace Base
} // namespace Public

#endif //__Public_EXCEL_H__
