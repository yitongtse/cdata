#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void InterruptHandler(long cause);

#ifdef __cplusplus
}
#endif

void InterruptHandler(long cause)
{
	printf("InterruptHandler: 0x%x exception.\r\n", cause);
}