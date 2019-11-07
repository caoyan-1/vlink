/*
 * YAFFS: Yet another FFS. A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2018 Aleph One Ltd.
 *
 * Created by Timothy Manning <timothy@yaffs.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "test_yaffs_chmod_EROFS.h"

int test_yaffs_chmod_EROFS(void)
{
	int error=0;
	int output;
	if (yaffs_close(yaffs_open(FILE_PATH,O_CREAT | O_RDWR, FILE_MODE))==-1){
		print_message("failed to create file\n",1);
		return -1;
	}
	EROFS_setup();

	output = yaffs_chmod(FILE_PATH,S_IREAD|S_IWRITE);

	if (output<0){
		error=yaffs_get_error();
		if (abs(error)==EROFS){
			return 1;
		} else {
			print_message("different error than expected\n",2);
			return -1;
		}
	} else {
		print_message("chmoded with EROFS setup (which is a bad thing)\n",2);
		return -1;
	}

}

int test_yaffs_chmod_EROFS_clean(void)
{
	return  EROFS_clean() | test_yaffs_chmod() ;
}
