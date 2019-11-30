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

# mbedtls
MBEDTLS_DIR = $(SECURITY_DIR)/mbedtls
include $(MBEDTLS_DIR)/mbedtls.mk

VSL_DIR = $(SECURITY_DIR)/vsl
include $(VSL_DIR)/vsl.mk

# secure c library
ifeq ($(CONFIG_SECURE_CLIB), y)
	SECURE_CLIB_DIR = $(SECURITY_DIR)/huawei_secure_c
	include $(SECURE_CLIB_DIR)/secure_c.mk
endif