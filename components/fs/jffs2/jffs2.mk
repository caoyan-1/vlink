#/**
# * Copyright (c) [2019] maminjie <canpool@163.com>
# *
# * vlink is licensed under the Mulan PSL v1.
# * You can use this software according to the terms and conditions of the Mulan PSL v1.
# * You may obtain a copy of Mulan PSL v1 at:
# *
# *    http://license.coscl.org.cn/MulanPSL
# *
# * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# * FIT FOR A PARTICULAR PURPOSE.
# * See the Mulan PSL v1 for more details.
# */

##
# this compat/dirent.h is used to replace dirent.h in Linux/macos
##
ifeq ($(CONFIG_OS_TYPE), $(filter $(CONFIG_OS_TYPE), linux macos))
	C_INCLUDES += -I $(FS_DIR)/include/compat
endif

JFFS2_BASE_DIR 		= $(JFFS2_DIR)

JFFS2_SRC = \
		${wildcard $(JFFS2_BASE_DIR)/cyg/compress/*.c} \
		${wildcard $(JFFS2_BASE_DIR)/cyg/crc/*.c} \
		${wildcard $(JFFS2_BASE_DIR)/port/*.c} \
		${wildcard $(JFFS2_BASE_DIR)/src/*.c} \
		${wildcard $(JFFS2_BASE_DIR)/*.c}

C_SOURCES += $(JFFS2_SRC)

JFFS2_INC = \
		-I $(JFFS2_BASE_DIR)/cyg/compress \
		-I $(JFFS2_BASE_DIR)/cyg/crc \
		-I $(JFFS2_BASE_DIR)/cyg/hal \
		-I $(JFFS2_BASE_DIR)/cyg/infra \
		-I $(JFFS2_BASE_DIR)/port \
		-I $(JFFS2_BASE_DIR)/src \
		-I $(JFFS2_BASE_DIR)
C_INCLUDES += $(JFFS2_INC)

C_DEFS += -D CONFIG_JFFS2=1
