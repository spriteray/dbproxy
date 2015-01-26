
#
#
# BIN				- 可执行文件
# LIBA				- 静态库
# LIBSO				- 动态库
#
# PRODUCT 			- 产品名称
# VERSION			- 版本号
#
# DEPEND_LIBS		- 依赖的其他第三方库
# DEPEND_MODULES	- 依赖的项目的其他模块
#
#

LIBSO 			= libdbclient

PRODUCT 		= DBClientLibrary
VERSION			= 0.1.2

DEPEND_LIBS		= libevlite.so \
				  libprotobuf.so \
				  libpthread.so libm.so

DEPEND_MODULES 	= $(ROOT)/src/message \
				  $(ROOT)/src/io \
				  $(ROOT)/src/utils

include $(ROOT)/Makefile.proto
include $(ROOT)/Makefile.rules
