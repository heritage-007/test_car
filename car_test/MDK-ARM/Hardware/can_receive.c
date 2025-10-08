#include "FreeRTOS.h"
#include "can_receive.h"
#include "main.h"
#include "cmsis_os.h"

CAN_TxHeaderTypeDef can_send_pHeader;
static motor_measure_t motor_chassis[4];
extern void detect_hook(uint16_t toe);
static uint8_t  chassis_can_send_data[8];


#define get_motor_measure(ptr, data)                                    \
    {                                                                   \
        (ptr)->last_ecd = (ptr)->ecd;                                   \
        (ptr)->ecd = (uint16_t)((data)[0] << 8 | (data)[1]);            \
        (ptr)->speed_rpm = (uint16_t)((data)[2] << 8 | (data)[3]);      \
        (ptr)->given_current = (uint16_t)((data)[4] << 8 | (data)[5]);  \
        (ptr)->temperate = (data)[6];                                   \
    }

const motor_measure_t *get_chassis_motor_measure_point(uint8_t i)
{
    return &motor_chassis[(i & 0x03)];
}

void can_cmd_chasis(int16_t motor1,int16_t motor2,int16_t motor3,int16_t motor4){
	uint32_t can_mail_box;
	can_send_pHeader.DLC=0x08;
	can_send_pHeader.RTR=CAN_RTR_DATA;
	can_send_pHeader.IDE=CAN_ID_STD;
	can_send_pHeader.StdId=CAN_CHASSIS_ALL_ID;
	chassis_can_send_data[0]=(motor1>>8);
	chassis_can_send_data[1]=motor1;
	chassis_can_send_data[2]=(motor2>>8);
	chassis_can_send_data[3]=motor2;
	chassis_can_send_data[4]=(motor3>>8);
	chassis_can_send_data[5]=motor3;
	chassis_can_send_data[6]=(motor4>>8);
	chassis_can_send_data[7]=motor4;

	HAL_CAN_AddTxMessage(&chasis_can,&can_send_pHeader,chassis_can_send_data,&can_mail_box);

}

void CAN_cmd_chassis_reset_ID(void)
{
    uint32_t send_mail_box;
    can_send_pHeader.StdId = 0x700;
    can_send_pHeader.IDE = CAN_ID_STD;
    can_send_pHeader.RTR = CAN_RTR_DATA;
    can_send_pHeader.DLC = 0x08;
    chassis_can_send_data[0] = 0;
    chassis_can_send_data[1] = 0;
    chassis_can_send_data[2] = 0;
    chassis_can_send_data[3] = 0;
    chassis_can_send_data[4] = 0;
    chassis_can_send_data[5] = 0;
    chassis_can_send_data[6] = 0;
    chassis_can_send_data[7] = 0;

    HAL_CAN_AddTxMessage(&chasis_can, &can_send_pHeader, chassis_can_send_data, &send_mail_box);
}


void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan){
	if(hcan==&hcan1){
		CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];

    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data);

    switch (rx_header.StdId)
    {
        case CAN_3508_M1_ID:
        case CAN_3508_M2_ID:
        case CAN_3508_M3_ID:
        case CAN_3508_M4_ID:
        {
            static uint8_t i = 0;
           
            i = rx_header.StdId - CAN_3508_M1_ID;
            get_motor_measure(&motor_chassis[i], rx_data);
//            detect_hook(CHASSIS_MOTOR1_TOE + i);
            break;
        }

        default:
        {
            break;
        }
    }

	}

}








