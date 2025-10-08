#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "uart.h"
#include "usart.h"

extern DMA_HandleTypeDef hdma_usart3_rx;
uint8_t uart1_tx_busy;
uint8_t rx1_buf[34];	
uint8_t rx2_buf[34];
RC_ctrl_t rc_ctrl;

void RC_restart(uint16_t dma_buf_num){
	  __HAL_UART_DISABLE(&huart3);
    __HAL_DMA_DISABLE(&hdma_usart3_rx);

    hdma_usart3_rx.Instance->NDTR = dma_buf_num;

    __HAL_DMA_ENABLE(&hdma_usart3_rx);
    __HAL_UART_ENABLE(&huart3);
}


void slove_RC_lost(void)
{
    RC_restart(SBUS_RX_BUF_NUM);
}
void slove_data_error(void)
{
    RC_restart(SBUS_RX_BUF_NUM);
}


//取绝对值
static int16_t RC_abs(int16_t value)
{
    if (value > 0)
    {
        return value;
    }
    else
    {
        return -value;
    }
}

//获取遥控器指针
const RC_ctrl_t *get_remote_control_point(void)
{
    return &rc_ctrl;
}

//检测遥控器输出值是否超过范围
uint8_t RC_data_is_error(void)
{
    
    if (RC_abs(rc_ctrl.rc.ch[0]) > RC_CHANNAL_ERROR_VALUE)
    {
        goto error;
    }
    if (RC_abs(rc_ctrl.rc.ch[1]) > RC_CHANNAL_ERROR_VALUE)
    {
        goto error;
    }
    if (RC_abs(rc_ctrl.rc.ch[2]) > RC_CHANNAL_ERROR_VALUE)
    {
        goto error;
    }
    if (RC_abs(rc_ctrl.rc.ch[3]) > RC_CHANNAL_ERROR_VALUE)
    {
        goto error;
    }
    if (rc_ctrl.rc.s[0] == 0)
    {
        goto error;
    }
    if (rc_ctrl.rc.s[1] == 0)
    {
        goto error;
    }
    return 0;

error:
    rc_ctrl.rc.ch[0] = 0;
    rc_ctrl.rc.ch[1] = 0;
    rc_ctrl.rc.ch[2] = 0;
    rc_ctrl.rc.ch[3] = 0;
    rc_ctrl.rc.ch[4] = 0;
    rc_ctrl.rc.s[0] = RC_SW_DOWN;
    rc_ctrl.rc.s[1] = RC_SW_DOWN;
    rc_ctrl.mouse.x = 0;
    rc_ctrl.mouse.y = 0;
    rc_ctrl.mouse.z = 0;
    rc_ctrl.mouse.press_l = 0;
    rc_ctrl.mouse.press_r = 0;
    rc_ctrl.key.v = 0;
    return 1;
}
//初始化函数
//参数1：dma的第一个缓冲区地址；
//参数2：第二个缓冲区地址
//参数3：DMA一次接收的最大数量
//返回值为空
void RC_init(uint8_t *rx1_buf, uint8_t *rx2_buf, uint16_t dma_buf_num){
	SET_BIT(huart3.Instance->CR3,USART_CR3_DMAR);
	
	__HAL_UART_ENABLE_IT(&huart3,UART_FLAG_IDLE);
	__HAL_DMA_DISABLE(&hdma_usart3_rx);
	
	while(hdma_usart3_rx.Instance->CR&DMA_SxCR_EN){
		__HAL_DMA_DISABLE(&hdma_usart3_rx);
	}
	
	hdma_usart3_rx.Instance->PAR=(uint32_t)&USART3->DR;
	hdma_usart3_rx.Instance->M0AR=(uint32_t)rx1_buf;
	hdma_usart3_rx.Instance->M1AR=(uint32_t)rx2_buf;
	hdma_usart3_rx.Instance->NDTR=dma_buf_num;
	
	SET_BIT(hdma_usart3_rx.Instance->CR,DMA_SxCR_DBM);
	__HAL_DMA_ENABLE(&hdma_usart3_rx);
}
//初始化DMA
void remote_control_init(void){

	RC_init(rx1_buf, rx2_buf, SBUS_RX_BUF_NUM);
}

//解码
static void sbus_to_rc(volatile const uint8_t *sbus_buf, RC_ctrl_t *rc_ctrl)
{
 if (sbus_buf == NULL || rc_ctrl == NULL)
 {
 return;
 }
 rc_ctrl->rc.ch[0] = (sbus_buf[0] | (sbus_buf[1] << 8)) & 0x07ff; //!< Channel 0
 rc_ctrl->rc.ch[1] = ((sbus_buf[1] >> 3) | (sbus_buf[2] << 5)) & 0x07ff; //!< Channel 1
 rc_ctrl->rc.ch[2] = ((sbus_buf[2] >> 6) | (sbus_buf[3] << 2) | //!< Channel 2
 (sbus_buf[4] << 10)) &0x07ff;
 rc_ctrl->rc.ch[3] = ((sbus_buf[4] >> 1) | (sbus_buf[5] << 7)) & 0x07ff; //!< Channel 3
 rc_ctrl->rc.s[0] = ((sbus_buf[5] >> 4) & 0x0003); //!< Switch left
 rc_ctrl->rc.s[1] = ((sbus_buf[5] >> 4) & 0x000C) >> 2; //!< Switch right
 rc_ctrl->mouse.x = sbus_buf[6] | (sbus_buf[7] << 8); //!< Mouse X axis
 rc_ctrl->mouse.y = sbus_buf[8] | (sbus_buf[9] << 8); //!< Mouse Y axis
 rc_ctrl->mouse.z = sbus_buf[10] | (sbus_buf[11] << 8); //!< Mouse Z axis
 rc_ctrl->mouse.press_l = sbus_buf[12]; //!< Mouse Left Is Press ?
 rc_ctrl->mouse.press_r = sbus_buf[13]; //!< Mouse Right Is Press ?
 rc_ctrl->key.v = sbus_buf[14] | (sbus_buf[15] << 8); //!< KeyBoard value
 rc_ctrl->rc.ch[4] = sbus_buf[16] | (sbus_buf[17] << 8); //NULL
 rc_ctrl->rc.ch[0] -= RC_CH_VALUE_OFFSET;
 rc_ctrl->rc.ch[1] -= RC_CH_VALUE_OFFSET;
 rc_ctrl->rc.ch[2] -= RC_CH_VALUE_OFFSET;
 rc_ctrl->rc.ch[3] -= RC_CH_VALUE_OFFSET;
 rc_ctrl->rc.ch[4] -= RC_CH_VALUE_OFFSET;

}

void USART3_IRQHandler(void)
{
			if(huart3.Instance->SR & UART_FLAG_RXNE)//接收到数据
			{
					__HAL_UART_CLEAR_PEFLAG(&huart3);
			}
			else if(USART3->SR & UART_FLAG_IDLE){//检测到空闲中断
			
				__HAL_UART_CLEAR_PEFLAG(&huart3);
				__HAL_DMA_DISABLE(&hdma_usart3_rx);
				
				static uint8_t real_time_length;
				
				if((hdma_usart3_rx.Instance->CR&DMA_SxCR_CT)==RESET){
					
					real_time_length=SBUS_RX_BUF_NUM-hdma_usart3_rx.Instance->NDTR;
					hdma_usart3_rx.Instance->NDTR=SBUS_RX_BUF_NUM;
					hdma_usart3_rx.Instance->CR |= DMA_SxCR_CT;
				
					if(real_time_length==RC_FRAME_LENGTH){
						sbus_to_rc(rx1_buf, &rc_ctrl);
						
					}
					__HAL_DMA_ENABLE(&hdma_usart3_rx);
				}
				else if(hdma_usart3_rx.Instance->CR&DMA_SxCR_CT){
					real_time_length=SBUS_RX_BUF_NUM-hdma_usart3_rx.Instance->NDTR;
					hdma_usart3_rx.Instance->NDTR=SBUS_RX_BUF_NUM;
					hdma_usart3_rx.Instance->CR &= ~(DMA_SxCR_CT);
				
					if(real_time_length==RC_FRAME_LENGTH){
						sbus_to_rc(rx2_buf, &rc_ctrl);
						
					}
					__HAL_DMA_ENABLE(&hdma_usart3_rx);
					
				}
		}
}










