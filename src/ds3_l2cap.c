#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "include/ds3.h"
#include "include/ds3_int.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "stack/bt_types.h"
#include "stack/btm_api.h"
#include "stack/l2c_api.h"
#include "osi/allocator.h"

#define DS3_TAG "DS3_L2CAP"

#define DS3_TAG_HIDC "DS3-HIDC"
#define DS3_TAG_HIDI "DS3-HIDI"

#define DS3_L2CAP_ID_HIDC 0x40
#define DS3_L2CAP_ID_HIDI 0x41


/********************************************************************************/
/*              L O C A L    F U N C T I O N     P R O T O T Y P E S            */
/********************************************************************************/

static bool ds3_l2cap_init_service(const char *name, uint16_t psm, uint8_t service_id);
static void ds3_l2cap_deinit_service(const char *name, uint16_t psm);
static void ds3_l2cap_connect_ind_cb(BD_ADDR bd_addr, uint16_t l2cap_cid, uint16_t psm, uint8_t l2cap_id);
static void ds3_l2cap_connect_cfm_cb(uint16_t l2cap_cid, uint16_t result);
static void ds3_l2cap_config_ind_cb(uint16_t l2cap_cid, tL2CAP_CFG_INFO *p_cfg);
static void ds3_l2cap_config_cfm_cb(uint16_t l2cap_cid, tL2CAP_CFG_INFO *p_cfg);
static void ds3_l2cap_disconnect_ind_cb(uint16_t l2cap_cid, bool ack_needed);
static void ds3_l2cap_disconnect_cfm_cb(uint16_t l2cap_cid, uint16_t result);
static void ds3_l2cap_data_ind_cb(uint16_t l2cap_cid, BT_HDR *p_msg);
static void ds3_l2cap_congest_cb(uint16_t cid, bool congested);


/********************************************************************************/
/*                         L O C A L    V A R I A B L E S                       */
/********************************************************************************/

static tL2CAP_APPL_INFO ds3_appl_info = {
    ds3_l2cap_connect_ind_cb,    // Connection indication
    ds3_l2cap_connect_cfm_cb,    // Connection confirmation
    NULL,                        // Connection pending
    ds3_l2cap_config_ind_cb,     // Configuration indication
    ds3_l2cap_config_cfm_cb,     // Configuration confirmation
    ds3_l2cap_disconnect_ind_cb, // Disconnect indication
    ds3_l2cap_disconnect_cfm_cb, // Disconnect confirmation
    NULL,                        // QOS violation
    ds3_l2cap_data_ind_cb,       // Data received indication
    ds3_l2cap_congest_cb,        // Congestion status
    NULL                         // Transmit complete
};

static tL2CAP_CFG_INFO ds3_l2cap_cfg_info;
static bool ds3_l2cap_hidc_connected = false;
static bool ds3_l2cap_hidi_connected = false;


/********************************************************************************/
/*                      P U B L I C    F U N C T I O N S                        */
/********************************************************************************/

/*******************************************************************************
**
** Function         ds3_l2cap_init_services
**
** Description      This function initialises the required L2CAP services.
**
** Returns          bool
**
*******************************************************************************/
bool ds3_l2cap_init_services()
{
    bool ok;

    ok = ds3_l2cap_init_service(DS3_TAG_HIDC, BT_PSM_HIDC, BTM_SEC_SERVICE_FIRST_EMPTY + 0);
    if (ok != true)
    {
        return false;
    }
    ok = ds3_l2cap_init_service(DS3_TAG_HIDI, BT_PSM_HIDI, BTM_SEC_SERVICE_FIRST_EMPTY + 1);
    if (ok != true)
    {
        return false;
    }

    return true;
}

/*******************************************************************************
**
** Function         ds3_l2cap_deinit_services
**
** Description      This function deinitialises the required L2CAP services.
**
** Returns          void
**
*******************************************************************************/
void ds3_l2cap_deinit_services()
{
    ds3_l2cap_deinit_service(DS3_TAG_HIDC, BT_PSM_HIDC);
    ds3_l2cap_deinit_service(DS3_TAG_HIDI, BT_PSM_HIDI);
    ds3_l2cap_hidc_connected = false;
    ds3_l2cap_hidi_connected = false;
}

/*******************************************************************************
**
** Function         ds3_l2cap_send_data
**
** Description      This function sends the HID command using the L2CAP service.
**
** Returns          bool
**
*******************************************************************************/
bool ds3_l2cap_send_data(uint8_t p_data[const], uint16_t len)
{
    uint8_t result;
    BT_HDR *p_buf;

    p_buf = (BT_HDR *)osi_malloc(BT_SMALL_BUFFER_SIZE);

    if (!p_buf)
        ESP_LOGE(DS3_TAG, "[%s] allocating buffer for sending the command failed", __func__);

    p_buf->len = len;
    p_buf->offset = L2CAP_MIN_OFFSET;

    memcpy(&p_buf->data[p_buf->offset], p_data, len);

    result = L2CA_DataWrite(DS3_L2CAP_ID_HIDC, p_buf);

    switch (result)
    {
    case L2CAP_DW_SUCCESS:
        ESP_LOGI(DS3_TAG, "[%s] sending command: success", __func__);
        break;
    case L2CAP_DW_CONGESTED:
        ESP_LOGW(DS3_TAG, "[%s] sending command: congested", __func__);
        break;
    case L2CAP_DW_FAILED:
        ESP_LOGE(DS3_TAG, "[%s] sending command: failed", __func__);
        break;
    default:
        break;
    }
    
    return (result == L2CAP_DW_SUCCESS);
}


/********************************************************************************/
/*                      L O C A L    F U N C T I O N S                          */
/********************************************************************************/

/*******************************************************************************
**
** Function         ds3_l2cap_init_service
**
** Description      This registers the specified bluetooth service in order
**                  to listen for incoming connections.
**
** Returns          bool
**
*******************************************************************************/
static bool ds3_l2cap_init_service(const char *name, uint16_t psm, uint8_t service_id)
{
    /* Register the PSM for incoming connections */
    if (!L2CA_Register(psm, &ds3_appl_info)) {
        ESP_LOGE(DS3_TAG, "%s Registering service %s failed", __func__, name);
        return false;
    }

    /* Register with the Security Manager for our specific security level (none) */
    if (!BTM_SetSecurityLevel(false, name, service_id, 0, psm, 0, 0)) {
        ESP_LOGE(DS3_TAG, "%s Registering security service %s failed", __func__, name);
        return false;
    }

    ESP_LOGI(DS3_TAG, "[%s] Service %s Initialized", __func__, name);

    return true;
}

/*******************************************************************************
**
** Function         ds3_l2cap_deinit_service
**
** Description      This deregisters the specified bluetooth service.
**
** Returns          void
**
*******************************************************************************/
static void ds3_l2cap_deinit_service(const char *name, uint16_t psm)
{
    /* Deregister the PSM from incoming connections */
    L2CA_Deregister(psm);
    ESP_LOGI(DS3_TAG, "[%s] Service %s Deinitialized", __func__, name);
}

/*******************************************************************************
**
** Function         ds3_l2cap_connect_ind_cb
**
** Description      This the L2CAP inbound connection indication callback function.
**
** Returns          void
**
*******************************************************************************/
static void ds3_l2cap_connect_ind_cb(BD_ADDR bd_addr, uint16_t l2cap_cid, uint16_t psm, uint8_t l2cap_id)
{
    ESP_LOGI(DS3_TAG, "[%s] bd_addr: %s\n  l2cap_cid: 0x%02x\n  psm: %d\n  id: %d", __func__, bd_addr, l2cap_cid, psm, l2cap_id);

    /* Send a Connection pending response to the L2CAP layer. */
    L2CA_ConnectRsp(bd_addr, l2cap_id, l2cap_cid, L2CAP_CONN_PENDING, L2CAP_CONN_PENDING);

    /* Send a Connection ok response to the L2CAP layer. */
    L2CA_ConnectRsp(bd_addr, l2cap_id, l2cap_cid, L2CAP_CONN_OK, L2CAP_CONN_OK);

    /* Send a Configuration Request. */
    L2CA_ConfigReq(l2cap_cid, &ds3_l2cap_cfg_info);
}

/*******************************************************************************
**
** Function         ds3_l2cap_connect_cfm_cb
**
** Description      This is the L2CAP connect confirmation callback function.
**
**
** Returns          void
**
*******************************************************************************/
static void ds3_l2cap_connect_cfm_cb(uint16_t l2cap_cid, uint16_t result)
{
    ESP_LOGI(DS3_TAG, "[%s] l2cap_cid: 0x%02x\n  result: %d", __func__, l2cap_cid, result);
}

/*******************************************************************************
**
** Function         ds3_l2cap_config_ind_cb
**
** Description      This is the L2CAP config indication callback function.
**
**
** Returns          void
**
*******************************************************************************/
static void ds3_l2cap_config_ind_cb(uint16_t l2cap_cid, tL2CAP_CFG_INFO *p_cfg)
{
    ESP_LOGI(DS3_TAG, "[%s] l2cap_cid: 0x%02x\n  p_cfg->result: %d\n  p_cfg->mtu_present: %d\n  p_cfg->mtu: %d", __func__, l2cap_cid, p_cfg->result, p_cfg->mtu_present, p_cfg->mtu);

    p_cfg->result = L2CAP_CFG_OK;

    /* Send a Config response */
    L2CA_ConfigRsp(l2cap_cid, p_cfg);
}

/*******************************************************************************
**
** Function         ds3_l2cap_config_cfm_cb
**
** Description      This is the L2CAP config confirmation callback function.
**
**
** Returns          void
**
*******************************************************************************/
static void ds3_l2cap_config_cfm_cb(uint16_t l2cap_cid, tL2CAP_CFG_INFO *p_cfg)
{
    ESP_LOGI(DS3_TAG, "[%s] l2cap_cid: 0x%02x\n  p_cfg->result: %d", __func__, l2cap_cid, p_cfg->result);

    if (p_cfg->result == L2CAP_CFG_OK) {
        if (l2cap_cid == DS3_L2CAP_ID_HIDC) {
            ds3_l2cap_hidc_connected = true;
        }
        if (l2cap_cid == DS3_L2CAP_ID_HIDI) {
            ds3_l2cap_hidi_connected = true;
        }
        /* The DS3 controller is connected after receiving both config confirmation */
        if (ds3_l2cap_hidc_connected && ds3_l2cap_hidi_connected) {
            ds3HandleConnection(true);
        }
    }
}

/*******************************************************************************
**
** Function         ds3_l2cap_disconnect_ind_cb
**
** Description      This is the L2CAP disconnect indication callback function.
**
**
** Returns          void
**
*******************************************************************************/
static void ds3_l2cap_disconnect_ind_cb(uint16_t l2cap_cid, bool ack_needed)
{
    ESP_LOGI(DS3_TAG, "[%s] l2cap_cid: 0x%02x\n  ack_needed: %d", __func__, l2cap_cid, ack_needed);

    if (ack_needed) {
        /* Send a Disconnect response */
        L2CA_DisconnectRsp(l2cap_cid);
    }
    if (l2cap_cid == DS3_L2CAP_ID_HIDC) {
        ds3_l2cap_hidc_connected = false;
    }
    if (l2cap_cid == DS3_L2CAP_ID_HIDI) {
        ds3_l2cap_hidi_connected = false;
    }
    /* The device requests disconnect */
    if (!ds3_l2cap_hidc_connected || !ds3_l2cap_hidi_connected) {
        ds3HandleConnection(false);
    }
}

/*******************************************************************************
**
** Function         ds3_l2cap_disconnect_cfm_cb
**
** Description      This is the L2CAP disconnect confirm callback function.
**
**
** Returns          void
**
*******************************************************************************/
static void ds3_l2cap_disconnect_cfm_cb(uint16_t l2cap_cid, uint16_t result)
{
    ESP_LOGI(DS3_TAG, "[%s] l2cap_cid: 0x%02x\n  result: %d", __func__, l2cap_cid, result);

    if (result == L2CAP_CONN_OK) {
        if (l2cap_cid == DS3_L2CAP_ID_HIDC) {
            ds3_l2cap_hidc_connected = false;
        }
        if (l2cap_cid == DS3_L2CAP_ID_HIDI) {
            ds3_l2cap_hidi_connected = false;
        }
        /* The device acknowledges disconnect */
        if (!ds3_l2cap_hidc_connected || !ds3_l2cap_hidi_connected) {
            ds3HandleConnection(false);
        }
    };
}

/*******************************************************************************
**
** Function         ds3_l2cap_data_ind_cb
**
** Description      This is the L2CAP data indication callback function.
**
**
** Returns          void
**
*******************************************************************************/
static void ds3_l2cap_data_ind_cb(uint16_t l2cap_cid, BT_HDR *p_buf)
{
    /* Check if data is received via the HID interrupt channel */
    if (l2cap_cid == DS3_L2CAP_ID_HIDI) {
        if (p_buf->len > 2) {
            ds3ReceiveData(&p_buf->data[p_buf->offset]);
        }
    }

    osi_free(p_buf);
}

/*******************************************************************************
**
** Function         ds3_l2cap_congest_cb
**
** Description      This is the L2CAP congestion callback function.
**
** Returns          void
**
*******************************************************************************/
static void ds3_l2cap_congest_cb(uint16_t l2cap_cid, bool congested)
{
    ESP_LOGI(DS3_TAG, "[%s] l2cap_cid: 0x%02x\n  congested: %d", __func__, l2cap_cid, congested);
}
