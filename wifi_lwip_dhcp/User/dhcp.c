#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/dhcp.h"
#include "lwip/inet.h"
#include "dhcp.h"
#include "stdio.h"
#include "wifi_base_config.h"
#include "dhcp.h"
#include "stm32h7xx.h"

#define PRINTF printf
#if LWIP_NETCONN

#define MAX_BUFFER_LEN 256

char sendbuf[MAX_BUFFER_LEN];
extern ip4_addr_t ipaddr;
extern char wiced_if_str[100];
static void dhcp_thread(void *arg)
{
	struct netconn *conn;
	ip4_addr_t ipaddr;
  int ret;
	int strlen = 0;
  
  PRINTF("Ŀ��IP��ַ:%d.%d.%d.%d \t �˿ں�:%d\n\n",      \
          DEST_IP_ADDR0,DEST_IP_ADDR1,DEST_IP_ADDR2,DEST_IP_ADDR3,DEST_PORT);
  
  PRINTF("�뽫������λ������ΪTCP Server.��wiofi_base_config.h�ļ��н�Ŀ��IP��ַ�޸�Ϊ�������ϵ�IP��ַ\n\n");
  
  PRINTF("�޸Ķ�Ӧ�ĺ궨��:DEST_IP_ADDR0,DEST_IP_ADDR1,DEST_IP_ADDR2,DEST_IP_ADDR3,DEST_PORT\n\n");
  
	while(1)
	{
    conn = netconn_new(NETCONN_TCP);
    if (conn == NULL)
    {
      PRINTF("create conn failed!\n");
      vTaskDelay(10);
      continue;
    }

    PRINTF("create conn success...\n");
    
    //���������IP��ַ
    IP4_ADDR(&ipaddr,DEST_IP_ADDR0,DEST_IP_ADDR1,DEST_IP_ADDR2,DEST_IP_ADDR3); 			
    
    ret = netconn_connect(conn,&ipaddr,DEST_PORT,0);	        //���ӷ��������˿ں�5001
    if (ret == -1)
    {
      PRINTF("Connect failed!\n");
      netconn_close(conn);
      vTaskDelay(10);
      continue;
    }
    
    PRINTF("Connect to server successful!\n");

    strlen = sprintf(sendbuf,"A LwIP client Using DHCP Address: %s\n",\
    wiced_if_str);

    while(1)
    {
      PRINTF("%s",sendbuf);
      ret=netconn_write(conn,sendbuf, strlen, NETCONN_NOCOPY);
      if (ret == ERR_OK)
      {
         PRINTF("write success...\n");
       }
	   

      vTaskDelay(1000); 
    }
    
//    printf("Connection failed \n");
//    netconn_close(conn); 						//�ر�����
//    netconn_delete(conn);						//ɾ�����ӽṹ
  }
}


void dhcp_netconn_init()
{
  sys_thread_new("dhcp_thread", dhcp_thread, NULL, 2048, 4);
}

#endif /* LWIP_NETCONN*/
