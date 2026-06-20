#ifndef __KEY_H
#define __KEY_H
#include "fml_types.h"
#include "GPIO.h"

#define KEY_SPEED_UP    1   /* A3 — 加速 */
#define KEY_SPEED_DOWN  2   /* B11 — 减速 */

void Key_Init(void);
uint8_t Key_GetNum(void);

#endif
