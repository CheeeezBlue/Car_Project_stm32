#include "VL53L0X.h"
#include "../Driver/UART.h"
#include "../Driver/fml_types.h"
#include <string.h>
#include <stdlib.h>

static s16 distance_mm = -1;
static u8  data_valid  = 0;

void VL53L0X_Init(void)
{
	UART_Init(UART_ID_3, 115200);
	UART_EnableIRQ(UART_ID_3);
}

void VL53L0X_Update(void)
{
	char line[UART_LINE_SIZE];
	if (!UART_GetLine(UART_ID_3, line, sizeof(line)))
		return;

	const char* p = strstr(line, "d:");
	if (!p) return;
	p += 2;
	while (*p == ' ') p++;
	if (*p < '0' || *p > '9') return;
	s16 val = (s16)atoi(p);
	if (val > 0 && val < 8191) {
		distance_mm = val;
		data_valid  = 1;
	}
}

s16 VL53L0X_GetDistance(void)
{
	return data_valid ? distance_mm : -1;
}

u8 VL53L0X_IsValid(void)
{
	return data_valid;
}
