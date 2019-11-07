/**
 * Copyright (c) [2019] maminjie <canpool@163.com>
 *
 * vlink is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *    http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 */

#ifndef __JFFS2_KERNEL_H__
#define __JFFS2_KERNEL_H__

#include "jffs2_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define jiffies 100

#define ERR_PTR(err) ((void*)(err))
#define PTR_ERR(err) ((unsigned long)(err))
#define IS_ERR(err) ((unsigned long)err > (unsigned long)-1000L)

#define CURRENT_TIME jffs2_get_timestamp()

#define	KERN_EMERG              "<0>" // system is unusable
#define	KERN_ALERT              "<1>" // action must be taken immediately
#define	KERN_CRIT               "<2>" // critical conditions
#define	KERN_ERR                "<3>" // error conditions
#define	KERN_WARNING            "<4>" // warning conditions
#define	KERN_NOTICE             "<5>" // normal but significant condition
#define	KERN_INFO               "<6>" // informational
#define	KERN_DEBUG              "<7>" // debug-level messages

#define printk                  printf

#ifndef min
#define min(x,y) (x<y?x:y)
#endif
#ifndef max
#define max(x,y) (x<y?y:x)
#endif
#define min_t(t, x,y) ((t)x<(t)y?(t)x:(t)y)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __JFFS2_KERNEL_H__ */

