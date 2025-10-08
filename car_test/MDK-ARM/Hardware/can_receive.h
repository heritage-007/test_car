#ifndef CAN_RECEIVE_H
#define CAN_RECEIVE_H

#include "can.h"
#include "struct_typedef.h"

#define chasis_can hcan1

typedef enum
{
    CAN_CHASSIS_ALL_ID = 0x200,
    CAN_3508_M1_ID = 0x201,
    CAN_3508_M2_ID = 0x202,
    CAN_3508_M3_ID = 0x203,
    CAN_3508_M4_ID = 0x204,
} can_msg_id_e;


typedef struct{
	uint16_t last_ecd;
	uint16_t ecd;
	uint16_t speed_rpm;
	uint16_t given_current;
	uint8_t temperate;
}motor_measure_t;


void can_cmd_chasis(int16_t motor1,int16_t motor2,int16_t motor3,int16_t motor4);
void CAN_cmd_chassis_reset_ID(void);
const motor_measure_t *get_chassis_motor_measure_point(uint8_t i);







#endif

