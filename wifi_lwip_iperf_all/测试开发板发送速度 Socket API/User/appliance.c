#include "wifi_base_config.h"
#include "appliance.h"
#include "stm32H7xx.h"
/* FreeRTOSͷ�ļ� */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "iperf_client.h"

#define PRINTF printf

/**
 * @brief app_main
 *
 */
void app_main( void )
{
	
		/*����wifi lwip��Ϣ*/
		Config_WIFI_LwIP_Info();
	
		iperf_client_init();
}
