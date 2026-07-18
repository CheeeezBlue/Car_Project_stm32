#ifndef __JY901S_H__
#define __JY901S_H__

#include "../Driver/fml_types.h"

void  JY901S_Init(void);
void  JY901S_Update(void);
float JY901S_ReadGyroZ(void);   /* Z轴角速度 (°/s) */
float JY901S_ReadYaw(void);     /* 偏航角 (°) */
u8    JY901S_IsValid(void);     /* 数据有效标志 */

#endif
