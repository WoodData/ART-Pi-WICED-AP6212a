#include "lwip/sockets.h"  /* equivalent of <sys/socket.h> */
#include <string.h>
#include <stdint.h>
#include "appliance.h"
#include "network/wwd_network_constants.h"
#include "RTOS/wwd_rtos_interface.h"
#include "lwip/sys.h"
#include "stm32h7xx.h"
#include "tcpecho.h"
#define PRINTF printf


/******************************************************
 *                      �궨��
 ******************************************************/

/******************************************************
 *                    ����
 ******************************************************/
#define DHCP_STACK_SIZE               (800)

/* BOOTP operations */
#define BOOTP_OP_REQUEST                (1)
#define BOOTP_OP_REPLY                  (2)

/* DHCP commands */
#define DHCPDISCOVER                    (1)
#define DHCPOFFER                       (2)
#define DHCPREQUEST                     (3)
#define DHCPDECLINE                     (4)
#define DHCPACK                         (5)
#define DHCPNAK                         (6)
#define DHCPRELEASE                     (7)
#define DHCPINFORM                      (8)

/* DHCP�������Ϳͻ��˵�UDP�˿ں�*/
#define IPPORT_DHCPS                   (67)
#define IPPORT_DHCPC                   (68)

/* DHCP�׽��ֳ�ʱֵ���Ժ���Ϊ��λ���� �޸Ĵ�������ʹ�߳��˳�������Ӧ�� */
#define DHCP_SOCKET_TIMEOUT     500



/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/* DHCP���ݽṹ */
typedef struct
{
    uint8_t  opcode;                     /* ������������ */
    uint8_t  hardware_type;              /* Ӳ����ַ����*/
    uint8_t  hardware_addr_len;          /* Ӳ����ַ���� */
    uint8_t  hops;                       /* ���� */
    uint32_t transaction_id;             /* ���ױ�� */
    uint16_t second_elapsed;             /* ������ʼ������� */
    uint16_t flags;
    uint8_t  client_ip_addr[4];          /* �ͻ���IP��ַ */
    uint8_t  your_ip_addr[4];            /* ���IP ��ַ*/
    uint8_t  server_ip_addr[4];          /* ������IP��ַ*/
    uint8_t  gateway_ip_addr[4];         /* ����IP��ַ */
    uint8_t  client_hardware_addr[16];   /* �ͻ���Ӳ����ַ */
    uint8_t  legacy[192];
    uint8_t  magic[4];
    uint8_t  options[275];               /* ѡ���� */
    /* as of RFC2131 it is variable length */
} dhcp_header_t;

/******************************************************
 *             ��̬��������
 ******************************************************/
static unsigned char * find_option( dhcp_header_t* request, unsigned char option_num );
static void dhcp_thread( void * thread_input );

/******************************************************
 *              ��������
 ******************************************************/
static char             new_ip_addr[4]                = { 192, 168, 0, 0 };
static uint16_t         next_available_ip_addr        = ( 1 << 8 ) + 100;
static char             subnet_option_buff[]          = { 1, 4, 255, 255, 0, 0 };
static char             server_ip_addr_option_buff[]  = { 54, 4, 192, 168, 1, 200 };
static char             mtu_option_buff[]             = { 26, 2, WICED_PAYLOAD_MTU>>8, WICED_PAYLOAD_MTU&0xff };
static char             dhcp_offer_option_buff[]      = { 53, 1, DHCPOFFER };
static char             dhcp_ack_option_buff[]        = { 53, 1, DHCPACK };
static char             dhcp_nak_option_buff[]        = { 53, 1, DHCPNAK };
static char             lease_time_option_buff[]      = { 51, 4, 0x00, 0x01, 0x51, 0x80 }; /* 1 day lease */
static char             dhcp_magic_cookie[]           = { 0x63, 0x82, 0x53, 0x63 };
static volatile char    dhcp_quit_flag = 0;
static int              dhcp_socket_handle            = -1;

/*������*/
static xTaskHandle      dhcp_thread_handle;
static dhcp_header_t    dhcp_header_buff;


#define ADDR_ARY_LEN    10
struct sockaddr_in      source_addr_ary[ADDR_ARY_LEN];
int addr_atry_index=0;
/******************************************************
 *              ���ܶ���
 ******************************************************/
extern void tcpecho_thread(void *arg);
/**
 * @brief start_dhcp_server
 *
 * @param local_addr �ȵ��ַ��������IP��
 */
void start_dhcp_server( uint32_t local_addr )
{
	/*************************************************************/
	//lwip ��ص�Ӧ�ÿ��Է�������	
	//tcpecho_init();
	//
	//��
	//��
	//��
	//��
	//��
	//Ӧ
	//��
	//lwip
	//demo
	//���ݴ󲿷ֵ� lwip  demo
	/**************************************************************/	
		dhcp_thread((void*)local_addr);	//�����һֱɨ���Ƿ���wifi����				
}



void quit_dhcp_server( void )
{
    dhcp_quit_flag = 1;
}
#define MAKE_IPV4_ADDRESS(a, b, c, d)          ((((uint32_t) (a)) << 24) | (((uint32_t) (b)) << 16) | (((uint32_t) (c)) << 8) | ((uint32_t) (d)))

/**
 * @brief ��ӡ��ַ��Ϣ
 *
 * @param temp_addr ��ʱ��ַ����
 */
void log_sin_addr_info(struct sockaddr_in  temp_addr)
{
#define LOG_SW  	//��ӡ����
#if defined(LOG_SW)

			PRINTF("%d.%d.%d.%d\n",  \
		((temp_addr.sin_addr.s_addr)&0x000000ff),       \
		(((temp_addr.sin_addr.s_addr)&0x0000ff00)>>8),  \
		(((temp_addr.sin_addr.s_addr)&0x00ff0000)>>16), \
		((temp_addr.sin_addr.s_addr)&0xff000000)>>24); 
	
#endif 
}



/**
 * @brief ��¼�ȵ��IP
 *
 * @param addr_0 IP
 * @param addr_1 IP
 * @param addr_2 IP
 * @param addr_3 IP
 */
void recording_AP_IP(uint8_t addr_0,uint8_t addr_1,uint8_t addr_2,uint8_t addr_3)
{
	int i=0;
	/*��IP��¼��������*/
	source_addr_ary[addr_atry_index].sin_addr.s_addr=MAKE_IPV4_ADDRESS(addr_3, addr_2, addr_1, addr_0);
	addr_atry_index++;
	if(addr_atry_index>ADDR_ARY_LEN)
	{
		addr_atry_index=0;
	}
#define FOR_PRINTF_SW
#if defined(FOR_PRINTF_SW)
	/*��ӡ��ǰ�Ѿ����ӹ���IP*/
	PRINTF("->>>>>>>>IP addr <<<<<<<<<<<-\r\n");
	for(i=0;i<addr_atry_index;i++)
	{
		PRINTF("-%d-> ",i);
		log_sin_addr_info(source_addr_ary[i]);
	}
#endif
	
}

/**
 * @brief dhcp_thread
 * ��������ʼ��ΪDISCOVER�����ṩ��һ�����õ�ַ
 * ��������ʹ��Ҫ����һ�����õ�ַ���κ�REQUEST������Ч
 * ��������ȷ��������һ�����õ�ַ���κ��������Ȼ�������һ�����õ�ַ
 * @param void * thread_input �ȵ��IP��ַ
 */
void dhcp_thread( void * thread_input )
{
    struct sockaddr_in    my_addr;
    struct sockaddr_in    source_addr;
    struct sockaddr_in    destination_addr;
    int                   status;
    char*                 option_ptr;
    socklen_t             source_addr_len;
    static dhcp_header_t* dhcp_header_ptr = &dhcp_header_buff;
    int                   recv_timeout = DHCP_SOCKET_TIMEOUT;
	
    /* ����Ҫ��DHCP���ݰ��з��͵ı���IP��ַ */
    my_addr.sin_addr.s_addr = (u32_t) thread_input;//�ȵ��IP��ַ
    my_addr.sin_family = AF_INET;
		/* ���˿ں���ӵ�IP��ַ */
    my_addr.sin_port = htons( IPPORT_DHCPS );			//�˿ں�
	
    memcpy( &server_ip_addr_option_buff[2], &my_addr.sin_addr.s_addr, 4 );

		/*��ӡ���*/
//		PRINTF("my_addr->> \n");
//		log_sin_addr_info(my_addr);
	
    /* ����DHCP�׽��� */
    dhcp_socket_handle = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt( dhcp_socket_handle, SOL_SOCKET, SO_RCVTIMEO, (char*)&recv_timeout, sizeof( recv_timeout ) );

    /* ���׽��ְ󶨵�����IP��ַ */
    status = bind(dhcp_socket_handle, (struct sockaddr*)&my_addr, sizeof(struct sockaddr_in));

    /*����Ŀ��IP��ַ�Ͷ˿ں� */
    destination_addr.sin_port   = htons( IPPORT_DHCPC );
    destination_addr.sin_family = AF_INET;
		
		/*Ŀ��IPΪ255.255.255.255  �㲥��ַ*/
    memset( &destination_addr.sin_addr.s_addr, 0xff, 4 );
				
		/*��ӡ���*/
//		PRINTF("destination_addr->> \n");
//		log_sin_addr_info(destination_addr);

    /* ����ѭ�� */
    while ( dhcp_quit_flag == 0 )
    {
        /* ����ֱ�����׽��ֽ��յ����ݡ� */
        status = recvfrom( dhcp_socket_handle, (char *)dhcp_header_ptr, sizeof( dhcp_header_buff ), 0 , (struct sockaddr *) &source_addr, &source_addr_len);
        if ( status > 0 )
        {
            /* ���DHCP���� */
            switch ( dhcp_header_ptr->options[2] )
            {
                case DHCPDISCOVER://DHCP  ����  ������
                    {
                        /* Discover����-����OFFER��Ӧ */
                        dhcp_header_ptr->opcode = BOOTP_OP_REPLY;

                        /* ���DHCPѡ���б�*/
                        memset( &dhcp_header_ptr->options, 0, sizeof( dhcp_header_ptr->options ) );

                        /*����ҪԼ��IP��ַ */
                        new_ip_addr[2] = next_available_ip_addr >> 8;
                        new_ip_addr[3] = next_available_ip_addr & 0xff;//���IP��ַ
																						
                        memcpy( &dhcp_header_ptr->your_ip_addr, new_ip_addr, 4 );//�������ӵĴӻ�IP��ַ���Ƶ��ṹ��
																					
                        /* ����ħ��DHCP����*/
                        memcpy( dhcp_header_ptr->magic, dhcp_magic_cookie, 4 );

                        /*����ѡ�� */
                        option_ptr = (char *) &dhcp_header_ptr->options;
                        memcpy( option_ptr, dhcp_offer_option_buff, 3 );       /* DHCP��Ϣ���� */
                        option_ptr += 3;
                        memcpy( option_ptr, server_ip_addr_option_buff, 6 );   /* ��������ʶ�� */
                        option_ptr += 6;
                        memcpy( option_ptr, lease_time_option_buff, 6 );       /* Lease Time */
                        option_ptr += 6;
                        memcpy( option_ptr, subnet_option_buff, 6 );           /* �������� */
                        option_ptr += 6;
                        memcpy( option_ptr, server_ip_addr_option_buff, 6 );   /*·���������أ� */
                        option_ptr[0] = 3; /* Router id */
                        option_ptr += 6;
                        memcpy( option_ptr, server_ip_addr_option_buff, 6 );   /* DNS ������ */
                        option_ptr[0] = 6; /* DNS server id */
                        option_ptr += 6;
                        memcpy( option_ptr, mtu_option_buff, 4 );              /* �ӿ�MTU */
                        option_ptr += 4;
                        option_ptr[0] = 0xff; /* ����ѡ�� */
                        option_ptr++;

												/* ���Ͱ� */
                        sendto( dhcp_socket_handle, (char *)dhcp_header_ptr, (int)(option_ptr - (char*)&dhcp_header_buff), 0 , (struct sockaddr *) &destination_addr, sizeof( destination_addr ));
                    }
                    break;

                case DHCPREQUEST://DHCP����
                    {
                        /* REQUEST command - send back ACK or NAK */
                        unsigned char* requested_address;
                        uint32_t*      server_id_req;
                        uint32_t*      req_addr_ptr;
                        uint32_t*      newip_ptr;

                        /* ��������Ƿ�Ϊ�˷�����ʹ�� */
                        server_id_req = (uint32_t*) find_option( dhcp_header_ptr, 54 );
                        if ( ( server_id_req != NULL ) &&
                             ( my_addr.sin_addr.s_addr != *server_id_req ) )
                        {
                            break; /* ������ID�뱾��IP��ַ��ƥ�� */
                        }

                        dhcp_header_ptr->opcode = BOOTP_OP_REPLY;

                        /* ��ѡ�����ҵ�����ĵ�ַ */
                        requested_address = find_option( dhcp_header_ptr, 50 );

                        /* ����Ҫ��ĵ�ַ */
                        memcpy( &dhcp_header_ptr->your_ip_addr, requested_address, 4 );

                        /* �հ�ѡ���б�*/
                        memset( &dhcp_header_ptr->options, 0, sizeof( dhcp_header_ptr->options ) );

                        /*��DHCP�������Ƶ����ݰ���*/
                        memcpy( dhcp_header_ptr->magic, dhcp_magic_cookie, 4 );

                        option_ptr = (char *) &dhcp_header_ptr->options;

                        /*����Ƿ�������һ������IP��ַ */
                        req_addr_ptr = (uint32_t*) dhcp_header_ptr->your_ip_addr;
                        newip_ptr = (uint32_t*) new_ip_addr;
                        if ( *req_addr_ptr != ( ( *newip_ptr & 0x0000ffff ) | ( ( next_available_ip_addr & 0xff ) << 24 ) | ( ( next_available_ip_addr & 0xff00 ) << 8 ) ) )
                        {
                            /* ��������һ������IP������-ͨ������NAKǿ�ƿͻ��˻�ȡ��һ������IP */
                            /* ����ʵ���ѡ��*/
                            memcpy( option_ptr, dhcp_nak_option_buff, 3 );  /* DHCP��Ϣ���� */
                            option_ptr += 3;
                            memcpy( option_ptr, server_ip_addr_option_buff, 6 ); /* ��������ʶ��*/
                            option_ptr += 6;
                            memset( &dhcp_header_ptr->your_ip_addr, 0, sizeof( dhcp_header_ptr->your_ip_addr ) ); /*��������ĵ�ַ���ֶ�*/
                        }
                        else
                        {
                            /* ��������һ������IP������-ͨ������NAKǿ�ƿͻ��˻�ȡ��һ������IP
                             * ����ʵ���ѡ��
                             */
                            memcpy( option_ptr, dhcp_ack_option_buff, 3 );       /*DHCP��Ϣ���� */
                            option_ptr += 3;
                            memcpy( option_ptr, server_ip_addr_option_buff, 6 ); /* ��������ʶ�� */
                            option_ptr += 6;
                            memcpy( option_ptr, lease_time_option_buff, 6 );     /* ���� */
                            option_ptr += 6;
                            memcpy( option_ptr, subnet_option_buff, 6 );         /*��������*/
                            option_ptr += 6;
                            memcpy( option_ptr, server_ip_addr_option_buff, 6 ); /* ·���������أ�*/
                            option_ptr[0] = 3; /* Router id */
                            option_ptr += 6;
                            memcpy( option_ptr, server_ip_addr_option_buff, 6 ); /* DNS ������ */
                            option_ptr[0] = 6; /* DNS server id */
                            option_ptr += 6;
                            memcpy( option_ptr, mtu_option_buff, 4 );            /* �ӿ�MTU */
                            option_ptr += 4;
														//�����µ�IP��ַ192.168.1.100
                            //WPRINT_APP_INFO(("Assigned new IP address %d.%d.%d.%d\n", (uint8_t)new_ip_addr[0], (uint8_t)new_ip_addr[1], next_available_ip_addr>>8, next_available_ip_addr&0xff )); 
														recording_AP_IP((uint8_t)new_ip_addr[0], (uint8_t)new_ip_addr[1], next_available_ip_addr>>8, next_available_ip_addr&0xff);
														
														tcpecho_init();

                            /* ����IP��ַ*/
                            next_available_ip_addr++;
                            if ( ( next_available_ip_addr & 0xff ) == 0xff ) /* ������ֽڷ�ת */
                            {
                                next_available_ip_addr += 101;
                            }
                            if ( ( next_available_ip_addr >> 8 ) == 0xff ) /* ������ֽڷ�ת*/
                            {
                                next_available_ip_addr += ( 2 << 8 );
                            }
                        }
                        option_ptr[0] = 0xff; /*������*/
                        option_ptr++;

                        /* ���Ͱ� */
                        sendto( dhcp_socket_handle, (char *)&dhcp_header_buff, (int)(option_ptr - (char*)&dhcp_header_buff), 0 , (struct sockaddr *) &destination_addr, sizeof( destination_addr ));
                    }
                    break;

                default:
                    break;
            }
        }
    }
		

    /* ɾ�� DHCP ���� */
    lwip_close( dhcp_socket_handle );
		
		PRINTF("ɾ�� DHCP ����\r\n");
    /* ��������ʼ�߳�*/
    vTaskDelete( dhcp_thread_handle );
}

/**
 * @brief ����ָ����DHCPѡ��
 *
 * @param request DHCP����ṹ
 * @param option_num �����ĸ�DHCPѡ���
 * @return ָ��DHCPѡ�����ݵ�ָ�룬����Ҳ�������ΪNULL
 */
static unsigned char * find_option( dhcp_header_t* request, unsigned char option_num )
{
    unsigned char* option_ptr = (unsigned char*) request->options;
    while ( ( option_ptr[0] != 0xff ) &&
            ( option_ptr[0] != option_num ) &&
            ( option_ptr < ( (unsigned char*) request ) + sizeof( dhcp_header_t ) ) )
    {
        option_ptr += option_ptr[1] + 2;
    }
    if ( option_ptr[0] == option_num )
    {
        return &option_ptr[2];
    }
    return NULL;

}

