#include "wifi_base_config.h"
#include "appliance.h"
#include "stm32h7xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "mqttclient.h"
#include "cJSON_Process.h"
#include "./delay/core_delay.h"    
#include "./dht11/bsp_dht11.h"
#include "./led/bsp_led.h"
#include "debug.h"

#define PRINTF printf
/**************************** ������ ********************************/
/* 
 * ��������һ��ָ�룬����ָ��һ�����񣬵����񴴽���֮�����;�����һ��������
 * �Ժ�����Ҫ��������������Ҫͨ�������������������������������Լ�����ô
 * ����������ΪNULL��
 */
static TaskHandle_t AppTaskCreate_Handle = NULL;/* ���������� */
static TaskHandle_t Test1_Task_Handle = NULL;/* LED������ */
static TaskHandle_t Test2_Task_Handle = NULL;/* KEY������ */

/********************************** �ں˶����� *********************************/
/*
 * �ź�������Ϣ���У��¼���־�飬�����ʱ����Щ�������ں˵Ķ���Ҫ��ʹ����Щ�ں�
 * ���󣬱����ȴ����������ɹ�֮��᷵��һ����Ӧ�ľ����ʵ���Ͼ���һ��ָ�룬������
 * �ǾͿ���ͨ��������������Щ�ں˶���
 *
 * �ں˶���˵���˾���һ��ȫ�ֵ����ݽṹ��ͨ����Щ���ݽṹ���ǿ���ʵ��������ͨ�ţ�
 * �������¼�ͬ���ȸ��ֹ��ܡ�������Щ���ܵ�ʵ��������ͨ��������Щ�ں˶���ĺ���
 * ����ɵ�
 * 
 */
 
QueueHandle_t MQTT_Data_Queue =NULL;

/******************************* ȫ�ֱ������� ************************************/
/*
 * ��������дӦ�ó����ʱ�򣬿�����Ҫ�õ�һЩȫ�ֱ�����
 */
DHT11_Data_TypeDef DHT11_Data;

/******************************* �궨�� ************************************/
/*
 * ��������дӦ�ó����ʱ�򣬿�����Ҫ�õ�һЩ�궨�塣
 */
#define  MQTT_QUEUE_LEN    4   /* ���еĳ��ȣ����ɰ������ٸ���Ϣ */
#define  MQTT_QUEUE_SIZE   4   /* ������ÿ����Ϣ��С���ֽڣ� */

/*
*************************************************************************
*                             ��������
*************************************************************************
*/
static void AppTaskCreate(void);/* ���ڴ������� */

static void Test1_Task(void* pvParameters);/* Test1_Task����ʵ�� */
static void Test2_Task(void* pvParameters);/* Test2_Task����ʵ�� */
/**
 * @brief app_main
 *
 */
void app_main( void )
{
	
		/*����wifi lwip��Ϣ*/
		Config_WIFI_LwIP_Info();
	
		AppTaskCreate();
	
  /* ��������ִ�е����� */    
    while (1)
    {
    }

}

/***********************************************************************
  * @ ������  �� AppTaskCreate
  * @ ����˵���� Ϊ�˷���������е����񴴽����������������������
  * @ ����    �� ��  
  * @ ����ֵ  �� ��
  **********************************************************************/
static void AppTaskCreate(void)
{
  BaseType_t xReturn = pdPASS;/* ����һ��������Ϣ����ֵ��Ĭ��ΪpdPASS */
 
  /* ����Test_Queue */
  MQTT_Data_Queue = xQueueCreate((UBaseType_t ) MQTT_QUEUE_LEN,/* ��Ϣ���еĳ��� */
                                 (UBaseType_t ) MQTT_QUEUE_SIZE);/* ��Ϣ�Ĵ�С */
  if(NULL != MQTT_Data_Queue)
    PRINTF("����MQTT_Data_Queue��Ϣ���гɹ�!\r\n");
    
  PRINTF("������ʹ�ÿ��������ٶ��ƣ������ϱ���ʪ������\n\n");
  
  PRINTF("��������ģ�����£�\n\t ����<--����-->·��<--����-->������\n\n ·�������������ӵ�����(������)\n\n");
  
  PRINTF("ʵ����ʹ��MQTTЭ�鴫������(����TCPЭ��) ����������ΪMQTT Client\n\n");
  
  PRINTF("�����̵�IP��ַ����User/arch/sys_arch.h�ļ����޸�\n\n");
    
  PRINTF("������LWIP���ֿ��Բο�<<LwIPӦ��ʵս����ָ��>>��22�� ���ӵ��ٶ��칤�����\n\n");
   
  PRINTF("�ڿ������dht11�ӿڽ���DHT11��ʪ�ȴ�����\n\n");  
  
  PRINTF("������Ϣ(�����ڰٶ��ƴ������豸�����޸ģ�����ο��鼮��22�� ���ӵ��ٶ��칤�����)\n\n");
  
  PRINTF("�ٶ�������/IP��ַ : %s \t �˿ں� : %d \n\n",HOST_NAME,HOST_PORT);  
  
  PRINTF("�ٶ���CLIENT_ID : %s\n\n",CLIENT_ID); 
  
  PRINTF("�ٶ���USER_NAME : %s\n\n",USER_NAME); 
  
  PRINTF("�ٶ���PASSWORD : %s\n\n",PASSWORD);  
  
  PRINTF("�ٶ���TOPIC : %s\n\n",TOPIC);  
  
  PRINTF("�ٶ���TEST_MESSAGE : %s\n\n",TEST_MESSAGE);    
  
  mqtt_thread_init();

  taskENTER_CRITICAL();           //�����ٽ���
 
  /* ����Test1_Task���� */
  xReturn = xTaskCreate((TaskFunction_t )Test1_Task, /* ������ں��� */
                        (const char*    )"Test1_Task",/* �������� */
                        (uint16_t       )1024,   /* ����ջ��С */
                        (void*          )NULL,	/* ������ں������� */
                        (UBaseType_t    )2,	    /* ��������ȼ� */
                        (TaskHandle_t*  )&Test1_Task_Handle);/* ������ƿ�ָ�� */
  if(pdPASS == xReturn)
    PRINT_DEBUG("Create Test1_Task sucess...\r\n");
  
  /* ����Test2_Task���� */
  xReturn = xTaskCreate((TaskFunction_t )Test2_Task,  /* ������ں��� */
                        (const char*    )"Test2_Task",/* �������� */
                        (uint16_t       )512,  /* ����ջ��С */
                        (void*          )NULL,/* ������ں������� */
                        (UBaseType_t    )1, /* ��������ȼ� */
                        (TaskHandle_t*  )&Test2_Task_Handle);/* ������ƿ�ָ�� */ 
  if(pdPASS == xReturn)
    PRINT_DEBUG("Create Test2_Task sucess...\n\n");
  
  vTaskDelete(AppTaskCreate_Handle); //ɾ��AppTaskCreate����
  
  taskEXIT_CRITICAL();            //�˳��ٽ���
}



/**********************************************************************
  * @ ������  �� Test1_Task
  * @ ����˵���� Test1_Task��������
  * @ ����    ��   
  * @ ����ֵ  �� ��
  ********************************************************************/
static void Test1_Task(void* parameter)
{	
  uint8_t res;
  //DHT11��ʼ��
  DHT11_GPIO_Config();
  DHT11_Data_TypeDef* send_data;
  while (1)
  {
    taskENTER_CRITICAL();           //�����ٽ���
    res = Read_DHT11(&DHT11_Data);
    taskEXIT_CRITICAL();            //�˳��ٽ���
    send_data = &DHT11_Data;
    if(SUCCESS == res)
    {
//      PRINTF("humidity = %f , temperature = %f\n",
//             (float)DHT11_Data.humi_int,(float)DHT11_Data.temp_int);
      xQueueSend( MQTT_Data_Queue, /* ��Ϣ���еľ�� */
                  &send_data,/* ���͵���Ϣ���� */
                  0 );        /* �ȴ�ʱ�� 0 */
    }

    vTaskDelay(1000);/* ��ʱ1000��tick */
  }

}

/**********************************************************************
  * @ ������  �� Test2_Task
  * @ ����˵���� Test2_Task��������
  * @ ����    ��   
  * @ ����ֵ  �� ��
  ********************************************************************/
static void Test2_Task(void* parameter)
{	 
  while (1)
  {
    LED2_TOGGLE;
    vTaskDelay(2000);/* ��ʱ2000��tick */
  }
}

/********************************END OF FILE****************************/





