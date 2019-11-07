/*
 * YAFFS: Yet another Flash File System . A NAND-flash specific file system. 
 *
 * Copyright (C) 2002-2018 Aleph One Ltd.
 *
 * Created by Timothy Manning <timothy@yaffs.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1 as
 * published by the Free Software Foundation.
 *
 * Note: Only YAFFS headers are LGPL, YAFFS C code is covered by GPL.
 */

#ifndef __TEST_YAFFS_RENAME_ENOTEMPTY_H__
#define __TEST_YAFFS_RENAME_ENOTEMPTY_H__

#include "lib.h"
#include "yaffsfs.h"

int test_yaffs_rename_ENOTEMPTY(void);
int test_yaffs_rename_ENOTEMPTY_clean(void);

#endif
