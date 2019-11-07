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

NET_LINUX_SRC = ${wildcard $(NET_LINUX_DIR)/*.c}
C_SOURCES += $(NET_LINUX_SRC)

NET_LINUX_INC = -I $(NET_LINUX_DIR)
C_INCLUDES += $(NET_LINUX_INC)

C_DEFS += -D CONFIG_NET_LINUX=1