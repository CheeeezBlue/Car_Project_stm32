#ifndef __VL53L0X_H__
#define __VL53L0X_H__

#include "../Driver/fml_types.h"

void VL53L0X_Init(void);
void VL53L0X_Update(void);
s16  VL53L0X_GetDistance(void);   /* 返回最新距离 (mm)，无效时返回 -1 */
u8   VL53L0X_IsValid(void);       /* 数据是否有效 (最后一次解析成功) */

#endif
