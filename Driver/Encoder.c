#include "Encoder.h"

/*
 * 编码器硬件映射（GPIOA/B only）
 * ───────────────────────────────────────────
 * 编码器A（左轮）→ TIM3 Default → PA6(CH1) / PA7(CH2)
 * 编码器B（右轮）→ TIM4 Default → PB6(CH1) / PB7(CH2)
 *
 * TIM 编码器模式自动处理方向：
 *   正转 → CNT 递增，反转 → CNT 递减
 *   4倍频（TI1+TI2 双边沿）→ 11线编码器每转 44 个计数
 *
 * M/T 法测速：DWT 周期计数器提供 ~14ns 精度的时间戳
 *   滑动窗口（8样本 × 10ms ≈ 80ms）消除低速量化抖动
 */

/* DWT / CoreDebug 寄存器（Cortex-M3 内建，直接地址绕过 CMSIS 兼容问题） */
#define DWT_BASE        0xE0001000UL
#define DWT_CTRL        (*(volatile u32*)(DWT_BASE + 0x00))
#define DWT_CYCCNT      (*(volatile u32*)(DWT_BASE + 0x04))
#define COREDBG_BASE    0xE000EDF0UL
#define COREDBG_DEMCR   (*(volatile u32*)(COREDBG_BASE + 0x00))

#define ENC_A_TIM  TIM4  /* 左轮 (PB6/PB7) */
#define ENC_B_TIM  TIM3  /* 右轮 (PA6/PA7) */

/* ---- 硬件计数器：32位软件扩展 ---- */
static volatile s32 enc_cnt[2];   /* 32位软件扩展计数器 */
static s16 enc_last[2];           /* 上一次硬件值（溢出检测） */
static s16 enc_prev_raw[2];       /* Encoder_GetDelta 用的上一次累计值 */

/* ---- M/T 测速：滑动窗口 ---- */
#define ENC_WIN_SIZE  8
typedef struct {
	u32 cycles;   /* DWT->CYCCNT 采样时刻 */
	s32 pos;      /* 累计位置 */
} EncSample_t;

static EncSample_t enc_win[2][ENC_WIN_SIZE];
static u8  enc_win_idx[2];   /* 环形写入位置 */
static u8  enc_win_cnt[2];   /* 已填充样本数 */
static s32 enc_prev_mt[2];   /* Encoder_Update 用的上一次累计值 */

/* 初始化 */
void Encoder_Init(void)
{
	/* --- DWT 周期计数器（Cortex-M3 内建，14ns 分辨率 @72MHz）--- */
	COREDBG_DEMCR |= (1u << 24);   /* TRCENA */
	DWT_CYCCNT = 0;
	DWT_CTRL |= 1;                 /* CYCCNTENA */

	/* --- TIM3: 编码器A（默认引脚 PA6/PA7）--- */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	TIM_TimeBaseInitTypeDef tim_base;
	tim_base.TIM_Prescaler = 0;
	tim_base.TIM_Period = 0xFFFF;
	tim_base.TIM_ClockDivision = TIM_CKD_DIV1;
	tim_base.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(ENC_A_TIM, &tim_base);

	TIM_EncoderInterfaceConfig(ENC_A_TIM,
		TIM_EncoderMode_TI12,
		TIM_ICPolarity_Rising,
		TIM_ICPolarity_Rising);

	TIM_SetCounter(ENC_A_TIM, 0);
	TIM_Cmd(ENC_A_TIM, ENABLE);

	/* --- TIM4: 编码器B（默认引脚 PB6/PB7）--- */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	tim_base.TIM_Period = 0xFFFF;
	TIM_TimeBaseInit(ENC_B_TIM, &tim_base);

	TIM_EncoderInterfaceConfig(ENC_B_TIM,
		TIM_EncoderMode_TI12,
		TIM_ICPolarity_Rising,
		TIM_ICPolarity_Rising);

	TIM_SetCounter(ENC_B_TIM, 0);
	TIM_Cmd(ENC_B_TIM, ENABLE);

	/* 清零全部状态 */
	enc_cnt[0] = 0; enc_cnt[1] = 0;
	enc_last[0] = 0; enc_last[1] = 0;
	enc_prev_raw[0] = 0; enc_prev_raw[1] = 0;
	enc_prev_mt[0] = 0; enc_prev_mt[1] = 0;
	enc_win_idx[0] = 0; enc_win_idx[1] = 0;
	enc_win_cnt[0] = 0; enc_win_cnt[1] = 0;
}

/* ================================================================
   获取累计脉冲数（32位，处理16位硬件溢出）
   ================================================================ */
s32 Encoder_GetCount(Encoder_ID_t id)
{
	TIM_TypeDef* tim = (id == ENC_LEFT) ? ENC_A_TIM : ENC_B_TIM;
	s16 now = (s16)TIM_GetCounter(tim);
	s16 diff = now - enc_last[id];
	enc_cnt[id] += diff;
	enc_last[id] = now;
	return enc_cnt[id];
}

/* ================================================================
   清零累计计数
   ================================================================ */
void Encoder_ResetCount(Encoder_ID_t id)
{
	TIM_TypeDef* tim = (id == ENC_LEFT) ? ENC_A_TIM : ENC_B_TIM;
	TIM_SetCounter(tim, 0);
	enc_cnt[id] = 0;
	enc_last[id] = 0;
	enc_prev_raw[id] = 0;
	enc_prev_mt[id] = 0;
	enc_win_cnt[id] = 0;
}

/* ================================================================
   原始脉冲增量（保留兼容，不再被 CarControl 调用）
   ================================================================ */
s16 Encoder_GetDelta(Encoder_ID_t id)
{
	s32 cur = Encoder_GetCount(id);
	s16 delta = (s16)(cur - enc_prev_raw[id]);
	enc_prev_raw[id] = cur;
	return delta;
}

/* ================================================================
   M/T 法测速（每10ms 调用一次）
   计算过去 ENC_WIN_SIZE 个样本窗口内的平均速度
   CPU 周期 → 时间换算：72,000,000 cycles/s
   speed (pulses/10ms) = delta_pos * 720,000 / delta_cycles
   ================================================================ */
float Encoder_Update(Encoder_ID_t id, s16* out_raw)
{
	s32 cur = Encoder_GetCount(id);
	u32 now = DWT_CYCCNT;

	/* 原始增量（VOFA+ 显示用） */
	s16 raw = (s16)(cur - enc_prev_mt[id]);
	enc_prev_mt[id] = cur;
	if (out_raw) *out_raw = raw;

	/* 写入滑动窗口 */
	u8 idx = enc_win_idx[id];
	enc_win[id][idx].cycles = now;
	enc_win[id][idx].pos    = cur;
	enc_win_idx[id] = (idx + 1) % ENC_WIN_SIZE;
	if (enc_win_cnt[id] < ENC_WIN_SIZE)
		enc_win_cnt[id]++;

	if (enc_win_cnt[id] < 2)
		return (float)raw;   /* 窗口未满，退化为原始 delta */

	/* 取最旧样本 */
	u8 oldest = (enc_win_idx[id] - enc_win_cnt[id] + ENC_WIN_SIZE) % ENC_WIN_SIZE;
	s32 d_pos = cur - enc_win[id][oldest].pos;
	u32 d_cyc = now - enc_win[id][oldest].cycles;

	if (d_cyc == 0) return (float)raw;

	/* d_pos pulses / d_cyc cycles * 72e6 cycles/s * 0.01 s/10ms
	   = d_pos * 720,000 / d_cyc  (pulses/10ms) */
	return (float)d_pos * 720000.0f / (float)d_cyc;
}
