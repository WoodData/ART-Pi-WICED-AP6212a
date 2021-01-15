#include "wifi_base_config.h"
#include "appliance.h"
#include "stm32H7xx.h"
/* FreeRTOSͷ�ļ� */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "httpserver-netconn.h"

#define PRINTF printf
/**
 * @brief ����ʵ��˵��
 *
 */
void ExperimentalDescription()
{
	PRINTF("��������ʾ��������http������\n\n");

	PRINTF("ʵ����ʹ��TCPЭ�鴫�����ݣ������������ΪTCP Client ����������ΪTCP Server\n\n");

	PRINTF("�ȵ��IP��ַ���˿ں���wifi_base_config.h�ļ����޸�\n\n");

	PRINTF("�����̲ο�lwip���ֿ��Բο� <<LwIPӦ��ʵս����ָ��>>��20�� HTTP ������\n\n");
	
	PRINTF("��������������IP��ַΪ -> Network ready IP: *.*.*.*  \n\n");
	
	PRINTF("�򿪵�������������뿪�����IP��ַ�����»س�����\n\n");
	
}
/**
 * @brief app_main
 *
 */
void app_main( void )
{
	
		/*����wifi lwip��Ϣ*/
		Config_WIFI_LwIP_Info();
	
		ExperimentalDescription();
	
		httpserver_init();

}
