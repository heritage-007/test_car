#ifndef UART_H
#define UART_H
#include "stdint.h"
//遥控器接收dma的缓冲区大小
#define SBUS_RX_BUF_NUM 32u
//指定接收一帧数据的大小
#define RC_FRAME_LENGTH 18u
//遥控器接收到信号的偏移值
#define RC_CH_VALUE_OFFSET ((uint16_t)1024)
//遥控器接收错误数据上限(偏移值上限)
#define RC_CHANNAL_ERROR_VALUE 700

#define RC_CH_VALUE_MIN         ((uint16_t)364)
    
#define RC_CH_VALUE_MAX         ((uint16_t)1684)

/* ----------------------- RC Switch Definition----------------------------- */
#define RC_SW_UP                ((uint16_t)1)
#define RC_SW_MID               ((uint16_t)3)
#define RC_SW_DOWN              ((uint16_t)2)
#define switch_is_down(s)       (s == RC_SW_DOWN)
#define switch_is_mid(s)        (s == RC_SW_MID)
#define switch_is_up(s)         (s == RC_SW_UP)
/* ----------------------- PC Key Definition-------------------------------- */
#define KEY_PRESSED_OFFSET_W            ((uint16_t)1 << 0)
#define KEY_PRESSED_OFFSET_S            ((uint16_t)1 << 1)
#define KEY_PRESSED_OFFSET_A            ((uint16_t)1 << 2)
#define KEY_PRESSED_OFFSET_D            ((uint16_t)1 << 3)
#define KEY_PRESSED_OFFSET_SHIFT        ((uint16_t)1 << 4)
#define KEY_PRESSED_OFFSET_CTRL         ((uint16_t)1 << 5)
#define KEY_PRESSED_OFFSET_Q            ((uint16_t)1 << 6)
#define KEY_PRESSED_OFFSET_E            ((uint16_t)1 << 7)
#define KEY_PRESSED_OFFSET_R            ((uint16_t)1 << 8)
#define KEY_PRESSED_OFFSET_F            ((uint16_t)1 << 9)
#define KEY_PRESSED_OFFSET_G            ((uint16_t)1 << 10)
#define KEY_PRESSED_OFFSET_Z            ((uint16_t)1 << 11)
#define KEY_PRESSED_OFFSET_X            ((uint16_t)1 << 12)
#define KEY_PRESSED_OFFSET_C            ((uint16_t)1 << 13)
#define KEY_PRESSED_OFFSET_V            ((uint16_t)1 << 14)
#define KEY_PRESSED_OFFSET_B            ((uint16_t)1 << 15)


void remote_control_init(void);

typedef struct {
    int16_t ch[5];  
    uint8_t s[2];   
} rc_info_t;


typedef struct {
    int16_t x;      
    int16_t y;      
    int16_t z;     
    uint8_t press_l; 
    uint8_t press_r; 
} mouse_info_t;

typedef struct{
		uint16_t v;
}KEY;

typedef struct {
    rc_info_t rc;
    mouse_info_t mouse;
    KEY key;       
} RC_ctrl_t;

























#endif











