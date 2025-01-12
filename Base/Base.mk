#
#  Copyright (c)1998-2012, Chongqing Public Technology
#  All Rights Reserved.
#
#	 Description:
#  $Id: Base.mk 2055 2014-07-31 03:58:39Z lixiaoqiang $  
#

#可以自己写一个.platform 用于配置 平台，静态库拷贝的位置, 否则自己指定编译选项，参见.platform
-include .platform

# 生成库的设置
# 定义库的文件目录
   SRCS_PATH =  Src Src/Base Src/Expression Src/uuid Src/regex Src/Excel Src/Excel/libxls Src/Excel/xlslib Src/Excel/sqlite3 Src/zip Src/zip/miniZip Src/XML Src/XML/tinyxml
	
# 库的名称
   SHARDLIB_NAME = 
   STATICLIB_NAME = Base


# 应用程序名称
   APP_NAME = 

# 子目录编译需要的目录
   SUB_INCLUDE = 


# 应用程序依赖的库(除了本身的编译的库)
#  LIBS = /usr/lib/libiconv.dylib
#  LIBS_DBG = /usr/lib/libiconv.dylib
  LIBS = 
  LIBS_DBG = 

#这个为第三方自动生成的库，不能修改
#AutoAddOtherDefineStart
LDLIBS = 
LDLIBS_DBG = 
#AotoAddOtherDefineEnd


include ${PRJ_PATH}/mk/platform/x86
include ${PRJ_PATH}/mk/builddir_v2.mk


#UserDefined Option
 -include userdefinedOption.mk

include ${PRJ_PATH}/mk/checksvn.mk					
include ${PRJ_PATH}/mk/print.mk
include ${PRJ_PATH}/mk/option.mk
include ${PRJ_PATH}/mk/rules_v2.mk



