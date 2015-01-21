
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
# DEPEND_MODULES	- 依赖的项目的其他模块, LINK时必须手动添加/
#
#

BIN				= dbproxy

PRODUCT 		= DBProxy
VERSION			= 0.1.1

DEPEND_LIBS		= libevlite.so \
				  libprotobuf.so \
				  /usr/lib64/mysql/libmysqlclient_r.so \
				  libpthread.so libm.so \
				  libtcmalloc_minimal.so

DEPEND_MODULES 	= $(ROOT)/src/message \
				 	$(ROOT)/src/io \
				 	$(ROOT)/src/utils

include $(ROOT)/Makefile.proto
include $(ROOT)/Makefile.rules
