#ifndef __logger_h__
#define __logger_h__

#include <stdio.h>
#include "main.h"

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

typedef void(uart_init_t)(void);

#if (DEBUG_LOG > 0)
#define FERROR(fmt, ...) \
	printf("%s:%d ERROR: " fmt "\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define FERROR(fmt, ...)
#endif

#if (DEBUG_LOG > 1)
#define FWARNING(fmt, ...) \
	printf("%s:%d WARNING: " fmt "\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define FWARNING(fmt, ...)
#endif

#if (DEBUG_LOG > 2)
#define FINFO(fmt, ...) \
	printf("%s:%d INFO: " fmt "\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define FINFO(fmt, ...)
#endif

int retarget_init(UART_HandleTypeDef *huart, uart_init_t *init);

#ifdef __cplusplus
}
#endif /* End of CPP guard */

#endif //__logger_h__
