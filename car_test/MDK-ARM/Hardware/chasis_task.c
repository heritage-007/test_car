#include "FreeRTOS.h"
#include "task.h"
#include "can_receive.h"

#include "chasis_task.h"



void chasis_init(void){}

void chassis_task(void *argument){
	vTaskDelay(CHASSIS_TASK_INIT_TIME);
	chasis_init();










}















