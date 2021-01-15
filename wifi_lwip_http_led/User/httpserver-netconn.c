
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"

#include "httpserver-netconn.h"
#include "appliance.h"
#include "wifi_base_config.h"
#include "./led/bsp_led.h"   

#define PRINTF printf

#define bool _Bool
#define true 1
#define false 0

#if LWIP_NETCONN

#ifndef HTTPD_DEBUG
#define HTTPD_DEBUG         LWIP_DBG_OFF
#endif

#define	FALSE  0		/* �� */
#define	TRUE  !FALSE	/* �� */

const static char http_html_hdr[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";

const unsigned char Led1On_Data[] ="<HTML> \
                                    <head><title>HTTP LED Control</title></head> \
                                    <center> \
                                    <p> \
                                    <font size=\"6\">LED<font style = \"color:red\">�Ѵ򿪣�</font> \
                                    <form method=post action=\"off\" name=\"ledform\"> \
                                    <input type=\"submit\" value=\"�ر�\" style=\"width:80px;height:30px;\"></form> \
                                    </center> \
                                    </HTML> ";

	//��LED��ʱ�����������������html��Ϣ����ʾ�������ͼ15-7��ʾ
	const unsigned char Led1Off_Data[] =" <HTML> \
                                        <head><title>HTTP LED Control</title></head> \
                                        <center> \
                                        <p> \
                                        <font size=\"6\">LED<font style = \"color:red\">�ѹرգ�</font> \
                                        <form method=post action=\"on\" name=\"ledform\"> \
                                        <input type=\"submit\" value=\"��\" style=\"width:80px;height:30px;\"></form> \
                                        </center> \
                                        </HTML> ";
  
  static const char http_index_html[] = "<html><head><title>Congrats!</title></head>\
                                         <body><h2 align=\"center\">Welcome to Fire lwIP HTTP Server!</h2>\
                                         <p align=\"center\">This is a small test page : http control led.</p>\
                                         <p align=\"center\"><a href=\"http://www.firebbs.cn/forum.php/\"> <font size=\"6\"> Ұ�������̳ </font> </a></p>\
                                         <a href=\"http://www.firebbs.cn/forum.php/\">\
                                         <img src=\"http://www.firebbs.cn/data/attachment/portal/201806/05/163015rhz7mbgbt0zfujzh.jpg\"/></a>\
                                         </body></html>";
static bool led_on = FALSE;

/*������ҳ����*/
void httpserver_send_html(struct netconn *conn, bool led_status)
{
  /* ��������ͷ */
  netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);
	/* ����LED״̬�����Ͳ�ͬ��LED���� */
	if(led_status == TRUE)
     netconn_write(conn, Led1On_Data, sizeof(Led1On_Data)-1, NETCONN_NOCOPY);
	else
	  netconn_write(conn, Led1Off_Data, sizeof(Led1Off_Data)-1, NETCONN_NOCOPY);
  netconn_write(conn, http_index_html, sizeof(http_index_html)-1, NETCONN_NOCOPY);

}
/** Serve one HTTP connection accepted in the http thread */

static void httpserver_serve(struct netconn *conn)
{
  struct netbuf *inbuf;
  char *buf;
  u16_t buflen;
  err_t err;
  
  /*�ȴ��ͻ��˵���������*/
  err = netconn_recv(conn, &inbuf);
  
  if (err == ERR_OK) {
    netbuf_data(inbuf, (void**)&buf, &buflen);
    /* ��GET������*/
    if (buflen>=5 &&
        buf[0]=='G' &&
        buf[1]=='E' &&
        buf[2]=='T' &&
        buf[3]==' ' &&
        buf[4]=='/' ) {
      
      /* ��POST�� ����*/
      httpserver_send_html(conn, led_on);
    }
	else if(buflen>=8&&buf[0]=='P'&&buf[1]=='O'&&buf[2]=='S'&&buf[3]=='T')
	{
		if(buf[6]=='o'&&buf[7]=='n'){		//�����LED
		    led_on = TRUE;
        LED1_ON;
        PRINTF("LED ON!\n");
		}else if(buf[6]=='o'&&buf[7]=='f'&&buf[8]=='f'){	//����ر�LED
		    led_on = FALSE;
         LED1_OFF;
        PRINTF("LED OFF!\n");
	    }
		//��������
		httpserver_send_html(conn, led_on);
	}

	netbuf_delete(inbuf);
  }
  /*  �ر�*/
  netconn_close(conn);
}

/** The main function, never returns! */
static void
httpserver_thread(void *arg)
{
  struct netconn *conn, *newconn;
  err_t err;
  LWIP_UNUSED_ARG(arg);
  
  /* �������ӽṹ */
  conn = netconn_new(NETCONN_TCP);
  LWIP_ERROR("http_server: invalid conn", (conn != NULL), return;);

  led_on = TRUE;
  LED1_ON;
  
  /* ��IP��ַ��˿ں�  */
  netconn_bind(conn, NULL, LOCAL_PORT);
  
  /* ���� */
  netconn_listen(conn);
  
  do {
    err = netconn_accept(conn, &newconn);
    if (err == ERR_OK) {
      httpserver_serve(newconn);
      netconn_delete(newconn);
    }
  } while(err == ERR_OK);
  LWIP_DEBUGF(HTTPD_DEBUG,
    ("http_server_netconn_thread: netconn_accept received error %d, shutting down",
    err));
  netconn_close(conn);
  netconn_delete(conn);
}

/** ��ʼ��HTTP ������*/
void
httpserver_init()
{
  sys_thread_new("http_server_netconn", httpserver_thread, NULL, 1024, 4);
}

#endif /* LWIP_NETCONN*/
