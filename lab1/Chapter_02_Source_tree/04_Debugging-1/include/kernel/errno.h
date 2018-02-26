#pragma once

#include <types/errno.h>
#include <kernel/kprint.h>
#include <arch/processor.h>

#ifdef DEBUG

#define LOG(LEVEL, format, ...)	\
kprintf ( "[" #LEVEL ":%s:%d]" format "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define ASSERT(expr)	do if ( !( expr ) ) { LOG ( BUG, ""); halt(); } while(0)

#else /* !DEBUG */

#define ASSERT(expr)
#define LOG(LEVEL, format, ...)

#endif /* DEBUG */

#define log(LEVEL, format, ...)	\
kprintf ( "[" #LEVEL ":%s:%d]" format "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define assert(expr)	do if ( !( expr ) ) { log ( BUG, ""); halt(); } while(0)
