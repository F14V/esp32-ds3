#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "include/ps3.h"
#include "include/ps3_int.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "stack/bt_types.h"
#include "stack/btm_api.h"
#include "stack/l2c_api.h"
#include "osi/allocator.h"

#define PS3_TAG "PS3_L2CAP"

#define PS3_TAG_HIDC "PS3-HIDC"
#define PS3_TAG_HIDI "PS3-HIDI"

#define PS3_L2CAP_ID_HIDC 0x40
#define PS3_L2CAP_ID_HIDI 0x41


/********************************************************************************/
/*              L O C A L    F U N C T I O N     P R O T O T Y P E S            */
/********************************************************************************/

static void ps3_l2cap_init_service(const char *name, uint16_t psm, uint8_t service_id);
static void ps3_l2cap_deinit_service(const char *name, uint16_t psm);
static void ps3_l2cap_connect_ind_cb(BD_ADDR bd_addr, uint16_t l2cap_cid, uint16_t psm, uint8_t l2cap_id);
static void ps3_l2cap_connect_cfm_cb(uint16_t l2cap_cid, uint16_t result);
static void ps3_l2cap_config_ind_cb(uint16_t l2cap_cid, tL2CAP_CFG_INFO *p_cfg);
static void ps3_l2cap_config_cfm_cb(uint16_t l2cap_cid, tL2CAP_CFG_INFO *p_cfg);
static void ps3_l2cap_disconnect_ind_cb(uint16_t l2cap_cid, bool ack_needed);
static void ps3_l2cap_disconnect_cfm_cb(uint16_t l2cap_cid, uint16_t result);
static void ps3_l2cap_data_ind_cb(uint16_t l2cap_cid, BT_HDR *p_msg);
static void ps3_l2cap_congest_cb(uint16_t cid, bool congested);


/********************************************************************************/
/*                         L O C A L    V A R I A B L E S                       */
/********************************************************************************/

static tL2CAP_APPL_INFO ps3_appl_info = {
    ps3_l2cap_connect_ind_cb,    // Connection indication
    ps3_l2cap_connect_cfm_cb,    // Connection confirmation
    NULL,                        // Connection pending
    ps3_l2cap_config_ind_cb,     // Configuration indication
    ps3_l2cap_config_cfm_cb,     // Configuration confirmation
    ps3_l2cap_disconnect_ind_cb, // Disconnect indication
    ps3_l2cap_disconnect_cfm_cb, // Disconnect confirmation
    NULL,                        // QOS violation
    ps3_l2cap_data_ind_cb,       // Data received indication
    ps3_l2cap_congest_cb,        // Congestion status
    NULL                         // Transmit complete
};

static tL2CAP_CFG_INFO ps3_l2cap_cfg_info;
static bool ps3_l2cap_hidc_connected = false;
static bool ps3_l2cap_hidi_connected = false;


/********************************************************************************/
/*                      P U B L I C    F U N C T I O N S                        */
/********************************************************************************/

/*******************************************************************************
**
** Function         ps3_l2cap_init_services
**
** Description      This function initialises the required L2CAP services.
**
** Returns          void
**
*******************************************************************************/
void ps3_l2cap_init_services()
{
    ps3_l2cap_init_service(PS3_TAG_HIDC, BT_PSM_HIDC, BTM_SEC_SERVICE_FIRST_EMPTY + 0);
    ps3_l2cap_init_service(PS3_TAG_HIDI, BT_PSM_HIDI, BTM_SEC_SERVICE_FIRST_EMPTY + 1);
}

/*******************************************************************************
**
** Function         ps3_l2cap_deinit_services
**
** Description      This function deinitialises the required L2CAP services.
**
** Returns          void
**
*******************************************************************************/
void ps3_l2cap_deinit_services()
{
    ps3_l2cap_deinit_service(PS3_TAG_HIDC, BT_PSM_HIDC);
    ps3_l2cap_deinit_service(PS3_TAG_HIDI, BT_PSM_HIDI);
    ps3_l2cap_hidc_connected = false;
    ps3_l2cap_hidi_connected = false;
}

/*******************************************************************************
**
** Function         ps3_l2cap_send_data
**
** Description      This function sends the HID command using the L2CAP service.
**
** Returns          void
**
*******************************************************************************/
void ps3_l2cap_send_data(uint8_t p_data[const], uint16_t len)
{
    uint8_t result;
    BT_HDR *p_buf;

    p_buf = (BT_HDR *)osi_malloc(BT_SMALL_BUFFER_SIZE);

    if (!p_buf)
        ESP_LOGE(PS3_TAG, "[%s] allocating buffer for sending the command failed", __func__);

    p_buf->len = len;
    p_buf->offset = L2CAP_MIN_OFFSET;

    memcpy(&p_buf->data[p_buf->offset], p_data, len);

    result = L2CA_DataWrite(PS3_L2CAP_ID_HIDC, p_buf);

    switch (result)
    {
    case L2CAP_DW_SUCCESS:
        ESP_LOGI(PS3_TAG, "[%s] sending command: success", __func__);
        break;
    case L2CAP_DW_CONGESTED:
        ESP_LOGW(PS3_TAG, "[%s] sending command: congested", __func__);
        break;
    case L2CAP_DW_FAILED:
        ESP_LOGE(PS3_TAG, "[%s] sending command: failed", __func__);
        break;
    default:
        break;
    }
}


/********************************************************************************/
/*                      L O C A L    F U N C T I O N S                          */
/********************************************************************************/

/*******************************************************************************
**
** Function         ps3_l2cap_init_service
**
** Description      This registers the specified bluetooth service in order
**                  to listen for incoming connections.
**
** Returns          void
**
*******************************************************************************/
static void ps3_l2cap_init_service(const char *name, uint16_t psm, uint8_t service_id)
{
    /* Register the PSM for incoming connections */
    if (!L2CA_Register(psm, &ps3_appl_info)) {
        ESP_LOGE(PS3_TAG, "%s Registering service %s failed", __func__, name);
        return;
    }

    /* Register with the Security Manager for our specific security level (none) */
    if (!BTM_SetSecurityLevel(false, name, service_id, 0, psm, 0, 0)) {
        ESP_LOGE(PS3_TAG, "%s Registering security service %s failed", __func__, name);
        return;
    }

    ESP_LOGI(PS3_TAG, "[%s] Service %s Initialized", __func__, name);
}

/*******************************************************************************
**
** Function         ps3_l2cap_deinit_service
**
** Description      This deregisters the specified bluetooth service.
**
** Returns          void
**
*******************************************************************************/
static void ps3_l2cap_deinit_service(const char *name, uint16_t psm)
{
    /* Deregister the PSM from incoming connections */
    L2CA_Deregister(psm);
    ESP_LOGI(PS3_TAG, "[%s] Service %s Deinitialized", __func__, name);
}

/*******************************************************************************
**
** Function         ps3_l2cap_connect_ind_cb
**
** Description      This the L2CAP inbound connection indication callback function.
**
** Returns          void
**
*******************************************************************************/
static void ps3_l2cap_connect_ind_cb(BD_ADDR bd_addr, uint16_t l2cap_cid, uint16_t psm, uint8_t l2cap_id)
{
    ESP_LOGI(PS3_TAG, "[%s] bd_addr: %s\n  l2cap_cid: 0x%02x\n  psm: %d\n  id: %d", __func__, bd_addr, l2cap_cid, psm, l2cap_id);

    /* Send a Connection pending response to the L2CAP layer. */
    L2CA_ConnectRsp(bd_addr, l2cap_id, l2cap_cid, L2CAP_CONN_PENDING, L2CAP_CONN_PENDING);

    /* Send a Connection ok response to the L2CAP layer. */
    L2CA_ConnectRsp(bd_addr, l2cap_id, l2cap_cid, L2CAP_CONN_OK, L2CAP_CONN_OK);

    /* Send a Configuration Request. */
    L2CA_ConfigReq(l2cap_cid, &ps3_l2cap_cfg_info);
}

/*******************************************************************************
**
** Function         ps3_l2cap_connect_cfm_cb
**
** Description      This is the L2CAP connect confirmation callback function.
**
**
** Returns          void
**
*******************************************************************************/
static void ps3_l2cap_connect_cfm_cb(uint16_t l2cap_cid, uint16_t result)
{
    ESP_LOGI(PS3_TAG, "[%s] l2cap_cid: 0x%02x\n  result: %d", __func__, l2cap_cid, result);
}

/*******************************************************************************
**
** Function         ps3_l2cap_config_ind_cb
**
** Description      This is the L2CAP config indication callback function.
**
**
** Returns          void
**
*******************************************************************************/
static void ps3_l2cap_config_ind_cb(uint16_t l2cap_cid, tL2CAP_CFG_INFO *p_cfg)
{
    ESP_LOGI(PS3_TAG, "[%s] l2cap_cid: 0x%02x\n  p_cfg->result: %d\n  p_cfg->mtu_present: %d\n  p_cfg->mtu: %d", __func__, l2cap_cid, p_cfg->result, p_cfg->mtu_present, p_cfg->mtu);

    p_cfg->result = L2CAP_CFG_OK;

    /* Send a Config response */
    L2CA_ConfigRsp(l2cap_cid, p_cfg);
}

/*******************************************************************************
**
** Function         ps3_l2cap_config_cfm_cb
**
** Description      This is the L2CAP config confirmation callback function.
**
**
** Returns          void
**
*******************************************************************************/
static void ps3_l2cap_config_cfm_cb(uint16_t l2cap_cid, tL2CAP_CFG_INFO *p_cfg)
{
    ESP_LOGI(PS3_TAG, "[%s] l2cap_cid: 0x%02x\n  p_cfg->result: %d", __func__, l2cap_cid, p_cfg->result);

    if (p_cfg->result == L2CAP_CFG_OK) {
        if (l2cap_cid == PS3_L2CAP_ID_HIDC) {
            ps3_l2cap_hidc_connected = true;
        }
        if (l2cap_cid == PS3_L2CAP_ID_HIDI) {
            ps3_l2cap_hidi_connected = true;
        }
        /* The PS3 controller is connected after receiving both config confirmation */
        if (ps3_l2cap_hidc_connected && ps3_l2cap_hidi_connected) {
            ps3HandleConnection(true);
        }
    }
}

/*******************************************************************************
**
** Function         ps3_l2cap_disconnect_ind_cb
**
** Description      This is the L2CAP disconnect indication callback function.
**
**
** Returns          void
**
*******************************************************************************/
static void ps3_l2cap_disconnect_ind_cb(uint16_t l2cap_cid, bool ack_needed)
{
    ESP_LOGI(PS3_TAG, "[%s] l2cap_cid: 0x%02x\n  ack_needed: %d", __func__, l2cap_cid, ack_needed);

    if (ack_needed) {
        /* Send a Disconnect response */
        L2CA_DisconnectRsp(l2cap_cid);
    }
    if (l2cap_cid == PS3_L2CAP_ID_HIDC) {
        ps3_l2cap_hidc_connected = false;
    }
    if (l2cap_cid == PS3_L2CAP_ID_HIDI) {
        ps3_l2cap_hidi_connected = false;
    }
    /* The device requests disconnect */
    if (!ps3_l2cap_hidc_connected || !ps3_l2cap_hidi_connected) {
        ps3HandleConnection(false);
    }
}

/*******************************************************************************
**
** Function         ps3_l2cap_disconnect_cfm_cb
**
** Description      This is the L2CAP disconnect confirm callback function.
**
**
** Returns          void
**
*******************************************************************************/
static void ps3_l2cap_disconnect_cfm_cb(uint16_t l2cap_cid, uint16_t result)
{
    ESP_LOGI(PS3_TAG, "[%s] l2cap_cid: 0x%02x\n  result: %d", __func__, l2cap_cid, result);

    if (result == L2CAP_CONN_OK) {
        if (l2cap_cid == PS3_L2CAP_ID_HIDC) {
            ps3_l2cap_hidc_connected = false;
        }
        if (l2cap_cid == PS3_L2CAP_ID_HIDI) {
            ps3_l2cap_hidi_connected = false;
        }
        /* The device acknowledges disconnect */
        if (!ps3_l2cap_hidc_connected || !ps3_l2cap_hidi_connected) {
            ps3HandleConnection(false);
        }
    };
}

/*******************************************************************************
**
** Function         ps3_l2cap_data_ind_cb
**
** Description      This is the L2CAP data indication callback function.
**
**
** Returns          void
**
*******************************************************************************/
static void ps3_l2cap_data_ind_cb(uint16_t l2cap_cid, BT_HDR *p_buf)
{
    /* Check if data is received via the HID interrupt channel */
    if (l2cap_cid == PS3_L2CAP_ID_HIDI) {
        if (p_buf->len > 2) {
            ps3ReceiveData(&p_buf->data[p_buf->offset]);
        }
    }

    osi_free(p_buf);
}

/*******************************************************************************
**
** Function         ps3_l2cap_congest_cb
**
** Description      This is the L2CAP congestion callback function.
**
** Returns          void
**
*******************************************************************************/
static void ps3_l2cap_congest_cb(uint16_t l2cap_cid, bool congested)
{
    ESP_LOGI(PS3_TAG, "[%s] l2cap_cid: 0x%02x\n  congested: %d", __func__, l2cap_cid, congested);
}
