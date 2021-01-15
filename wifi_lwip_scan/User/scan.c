#include "scan.h"

#include <stdio.h>
#include <string.h>
#include "wwd_management.h"
#include "wwd_wifi.h"
#include "wwd_debug.h"
#include "wwd_events.h"
#include "wwd_assert.h"
#include "RTOS/wwd_rtos_interface.h"
#include "platform/wwd_platform_interface.h"
#include "lwip/tcpip.h"
#include "wiced_utilities.h"
#include "wwd_buffer_interface.h"
/******************************************************
 *               ��̬��������
 ******************************************************/
static void scan_results_handler( wiced_scan_result_t** result_ptr, void* user_data, wiced_scan_status_t status );

/******************************************************
 *               ��������
 ******************************************************/
static wiced_mac_t             bssid_list[200]; /* ��ɨ���BSSID��AP MAC��ַ���б� */
static host_semaphore_type_t   num_scan_results_semaphore;
static wiced_scan_result_t     result_buff[CIRCULAR_RESULT_BUFF_SIZE];
static uint16_t                result_buff_write_pos = 0;
static uint16_t                result_buff_read_pos  = 0;


/******************************************************
 *              ���ܶ���
 ******************************************************/

/**
 * Main Scan app
 */
void app_main( void )
{
    wiced_scan_result_t * result_ptr = (wiced_scan_result_t *) &result_buff;
    wiced_scan_result_t * record;
    int                   record_number;
    wwd_result_t          result;

    /* ��ʼ�� Wiced */
    WPRINT_APP_INFO(("Starting Wiced v" WICED_VERSION "\n"));

    wwd_buffer_init( NULL );
    result = wwd_management_wifi_on( COUNTRY );
    if ( result != WWD_SUCCESS )
    {
        WPRINT_APP_INFO(("Error %d while starting WICED!\n", (int) result));
    }

    host_rtos_init_semaphore( &num_scan_results_semaphore );

    /*�ظ�ɨ��*/
    while (1)
    {
        record_number = 1;

        /* ����ѽ�����BSSID��ַ�б� */
        memset( &bssid_list, 0, sizeof( bssid_list ) );

        /* ��ʼɨ�� */
        WPRINT_APP_INFO(("Starting Scan\n"));

        /* ɨ�����ֵ��ʾ�� */
#if 0
        wiced_ssid_t ssid = { 12,  "YOUR_AP_SSID" };                     /* ��ɨ����ΪYOUR_AP_SSID������ */
        wiced_mac_t mac = {{0x00,0x1A,0x30,0xB9,0x4E,0x90}};             /*ʹ��ָ����BSSIDɨ������ */
        uint16_t chlist[] = { 6, 0 };                                    /* ��ɨ��ͨ��6 */
        wiced_scan_extended_params_t extparam = { 5, 1000, 1000, 100 };  /* ���Ѹ�����ɨ��ʱ�䣨ÿ��ͨ��1���5��̽������*/
#endif /* if 0 */

        result_buff_read_pos = 0;
        result_buff_write_pos = 0;

        if ( WWD_SUCCESS != wwd_wifi_scan( WICED_SCAN_TYPE_ACTIVE, WICED_BSS_TYPE_ANY, NULL, NULL, NULL, NULL, scan_results_handler, (wiced_scan_result_t **) &result_ptr, NULL, WWD_STA_INTERFACE ) )
        {
            WPRINT_APP_ERROR(("Error starting scan\n"));
            host_rtos_deinit_semaphore( &num_scan_results_semaphore );
            return;
        }
        WPRINT_APP_INFO(("Waiting for scan results...\n"));

        while ( host_rtos_get_semaphore( &num_scan_results_semaphore, NEVER_TIMEOUT, WICED_FALSE ) == WWD_SUCCESS )
        {
            int k;

            record = &result_buff[result_buff_read_pos];

            /*TODO:��0xff����Ϊ�Ѷ���ı�־*/
            if ( record->channel == (uint8_t) 0xff )
            {
                /*ɨ����� */
                break;
            }

            /* ��ӡ SSID */
            WPRINT_APP_INFO(("\n#%03d SSID          : ", record_number ));
            for ( k = 0; k < (int)record->SSID.length; k++ )
            {
                WPRINT_APP_INFO(("%c",(char)record->SSID.value[k]));
            }
            WPRINT_APP_INFO(("\n" ));

            wiced_assert( "error", (record->bss_type==WICED_BSS_TYPE_INFRASTRUCTURE)||(record->bss_type==WICED_BSS_TYPE_ADHOC));

            /* ��ӡ������������ */
            WPRINT_APP_INFO(("     BSSID         : %02X:%02X:%02X:%02X:%02X:%02X\n", (unsigned int) record->BSSID.octet[0],
                                                                                       (unsigned int) record->BSSID.octet[1],
                                                                                       (unsigned int) record->BSSID.octet[2],
                                                                                       (unsigned int) record->BSSID.octet[3],
                                                                                       (unsigned int) record->BSSID.octet[4],
                                                                                       (unsigned int) record->BSSID.octet[5] ));
            WPRINT_APP_INFO(("     RSSI          : %ddBm", (int)record->signal_strength ));
            if ( record->flags & WICED_SCAN_RESULT_FLAG_RSSI_OFF_CHANNEL )
            {
                WPRINT_APP_INFO((" (off-channel)\n"));
            }
            else
            {
                WPRINT_APP_INFO(("\n"));
            }
            WPRINT_APP_INFO(("     Max Data Rate : %.1f Mbits/s\n", (float)record->max_data_rate /1000.0));
            WPRINT_APP_INFO(("     Network Type  : %s\n", (record->bss_type==WICED_BSS_TYPE_INFRASTRUCTURE)?"Infrastructure":(record->bss_type==WICED_BSS_TYPE_ADHOC)?"Ad hoc":"Unknown" ));
            WPRINT_APP_INFO(("     Security      : %s\n", (record->security==WICED_SECURITY_OPEN)?"Open":
                                                      (record->security==WICED_SECURITY_WEP_PSK)?"WEP":
                                                      (record->security==WICED_SECURITY_WPA_TKIP_PSK)?"WPA":
                                                      (record->security==WICED_SECURITY_WPA2_AES_PSK)?"WPA2 AES":
                                                      (record->security==WICED_SECURITY_WPA2_MIXED_PSK)?"WPA2 Mixed":"Unknown" ));
            WPRINT_APP_INFO(("     Radio Band    : %s\n", (record->band==WICED_802_11_BAND_5GHZ)?"5GHz":"2.4GHz" ));
            WPRINT_APP_INFO(("     Channel       : %d\n", (int) record->channel ));
            result_buff_read_pos++;
            if ( result_buff_read_pos >= (uint16_t) CIRCULAR_RESULT_BUFF_SIZE )
            {
                result_buff_read_pos = 0;
            }
            record_number++;

        }

        WPRINT_APP_INFO(("\nEnd of scan results - next scan in 5 seconds\n"));

        /* �ȴ�5���ӣ�ֱ����һ��ɨ�� */
        (void) host_rtos_delay_milliseconds( (uint32_t) 5000 );
    }
}
    /*@+infloops@*/
/**
 *  ɨ�����ص�
 *  ɨ��������ʱ����
 *
 *  @param result_ptr :ָ��洢�����λ�õ�ָ���ָ�롣 �ڲ�ָ����Ը��£��Խ���һ�����������λ�á�
 *  @param user_data : ����
 */
void scan_results_handler( wiced_scan_result_t** result_ptr, void* user_data, wiced_scan_status_t status )
{
    if ( result_ptr == NULL )
    {
        /* ��� */
        result_buff[result_buff_write_pos].channel = 0xff;
        host_rtos_set_semaphore( &num_scan_results_semaphore, WICED_FALSE );
        return;
    }

    wiced_scan_result_t* record = ( *result_ptr );

    /* ����Ѵ�ӡ��BSSIDֵ�б� */
    wiced_mac_t * tmp_mac = bssid_list;
    while ( NULL_MAC( tmp_mac->octet ) == WICED_FALSE )
    {
        if ( CMP_MAC( tmp_mac->octet, record->BSSID.octet ) == WICED_TRUE )
        {
            /* �Ѿ��������BSSID */
            return;
        }
        tmp_mac++;
    }

    /*�µ�BSSID-������ӵ��б��� */
    memcpy( &tmp_mac->octet, record->BSSID.octet, sizeof(wiced_mac_t) );

    /* �������ӵ��б�Ϊ��һ���������ָ�� */
    result_buff_write_pos++;
    if ( result_buff_write_pos >= CIRCULAR_RESULT_BUFF_SIZE )
    {
        result_buff_write_pos = 0;
    }
    *result_ptr = &result_buff[result_buff_write_pos];
    host_rtos_set_semaphore( &num_scan_results_semaphore, WICED_FALSE );

    wiced_assert( "Circular result buffer overflow", result_buff_write_pos != result_buff_read_pos );
}
