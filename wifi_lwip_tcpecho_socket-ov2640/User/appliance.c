#include "wifi_base_config.h"
#include "appliance.h"
#include "stm32h7xx.h"
#include "debug.h"
/* FreeRTOSͷ�ļ� */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "./sdram/bsp_sdram.h" 
#include "./camera/bsp_ov2640.h"
				
#include "camera_data_queue.h"
#include "tcp_server.h"
#include "stdlib.h"
#include "string.h"

				
				
/**
 * @brief app_main
 *
 */
#define fire_demo_log(M, ...) custom_log("WIFI", M, ##__VA_ARGS__)

extern uint32_t frame_counter;
extern int send_fream;
extern int cbReadFinish_num;
int time_miao=0;
void app_main( void )
{
		host_thread_type_t    wwd_thread;
		camera_data * cambuf;
		int32_t err = kNoErr;

//		/*����wifi lwip��Ϣ*/
		Config_WIFI_LwIP_Info();

		err = camera_queue_init();
	
		cambuf = cbWrite(&cam_circular_buff);
	
		err = open_camera((uint32_t *)cambuf->head, CAMERA_QUEUE_DATA_LEN);
	
		host_rtos_create_thread( &wwd_thread, (void *)tcp_server_thread, "TCP_server", NULL,1024*10, 1);

		frame_counter=0;//֡����������
		send_fream=0;//֡����������
    while(1)
    {
			/*��ʱ*/
			time_miao++;
			vTaskDelay(1000);
			/*���֡��*/
			//printf("-time_miao-%d----->>>>>>>>frame_counter=%d fps/s ,send_fream ->%d fps/s \r\n",time_miao,frame_counter,send_fream);
			frame_counter=0;			
			send_fream=0;
    }

}

