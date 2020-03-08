#
#  Copyright (c)1998-2012, Chongqing Public Technology
#  All Rights Reserved.
#
#	 Description:
#  $Id: Base.mk 2055 2014-07-31 03:58:39Z lixiaoqiang $  
#

#可以自己写一个.platform 用于配置 平台，静态库拷贝的位置, 否则自己指定编译选项，参见.platform
-include .platform

# 设置
# 代码路径
   SRCS_PATH = Src
	
# 库的名称
   SHARDLIB_NAME = 
   STATICLIB_NAME = 


# 应用程序名称
   APP_NAME = test

# 子目录编译需要的目录
   SUB_INCLUDE = 


# 应用程序依赖的库(除了本身的编译的库)
  LIBS = ${PRJ_OUTPUTDIR}/libBase.a ${PRJ_OUTPUTDIR}/libNetwork.a ${PRJ_OUTPUTDIR}/libRTSP.a  -lpthread
  LIBS_DBG = ${PRJ_OUTPUTDIR}/libBase_debug.a ${PRJ_OUTPUTDIR}/libNetwork_debug.a ${PRJ_OUTPUTDIR}/libRTSP_debug.a -lpthread

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



