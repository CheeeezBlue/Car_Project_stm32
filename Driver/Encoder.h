#ifndef __ENCODER_H__
#define __ENCODER_H__

#include "fml_types.h"

/* 编码器线数（每转脉冲数，根据电机型号修改） */
#define ENC_PPR  11

void  Encoder_Init(void);
s32   Encoder_GetCount(Encoder_ID_t id);    /* 累计脉冲数（32位扩展） */
void  Encoder_ResetCount(Encoder_ID_t id);  /* 清零计数 */
s16   Encoder_GetDelta(Encoder_ID_t id);    /* 距上次调用的脉冲增量（原始值） */

/* M/T 法测速 + 原始增量一并返回
 * out_raw: 可选的原始 delta 输出（用于 VOFA+ 显示），填 NULL 则忽略
 * 返回值: 窗口平均速度（pulses/10ms），float */
float Encoder_Update(Encoder_ID_t id, s16* out_raw);

#endif
