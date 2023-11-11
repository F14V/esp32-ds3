#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <esp_mac.h>
#include "include/ds3.h"
#include "include/ds3_int.h"


/********************************************************************************/
/*                              C O N S T A N T S                               */
/********************************************************************************/

static const uint8_t hid_cmd_payload_report_enable[] = { 0x42, 0x03, 0x00, 0x00 };
// static const uint8_t hid_cmd_payload_led_arguments[] = { 0xFF, 0x27, 0x10, 0x00, 0x32 };


/********************************************************************************/
/*                         L O C A L    V A R I A B L E S                       */
/********************************************************************************/

/* Connection callbacks */
static ds3_connection_callback_t ds3_connection_cb = NULL;

/* Event callbacks */
static ds3_event_callback_t ds3_event_cb = NULL;

/* Status flags */
static bool ds3_is_connected = false;
static bool ds3_is_active = false;

/* Input and output data */
static ds3_input_data_t ds3_input_data;
static ds3_output_data_t ds3_output_data;


/********************************************************************************/
/*              L O C A L    F U N C T I O N     P R O T O T Y P E S            */
/********************************************************************************/

static void ds3_handle_connect_event(uint8_t is_connected);
static void ds3_handle_data_event(ds3_input_data_t *const p_data, ds3_event_t *const p_event);


/********************************************************************************/
/*                      P U B L I C    F U N C T I O N S                        */
/********************************************************************************/

/*******************************************************************************
**
** Function         ds3Init
**
** Description      This initializes the bluetooth services to listen
**                  for an incoming ds3 controller connection.
**
**
** Returns          void
**
*******************************************************************************/
void ds3Init()
{
    ds3_bt_init();
    ds3_l2cap_init_services();
}

/*******************************************************************************
**
** Function         ds3Deinit
**
** Description      This deinitializes the bluetooth services to stop
**                  listening for incoming connections.
**
**
** Returns          void
**
*******************************************************************************/
void ds3Deinit()
{
    ds3_l2cap_deinit_services();
    ds3_bt_deinit();
}

/*******************************************************************************
**
** Function         ds3IsConnected
**
** Description      This returns whether a DS3 controller is connected, based
**                  on whether a successful handshake has taken place.
**
**
** Returns          bool
**
*******************************************************************************/
bool ds3IsConnected()
{
    return ds3_is_active;
}

/*******************************************************************************
**
** Function         ds3HandleConnection
**
** Description      Handle the connection of the DS3 controller.
**
**
** Returns          void
**
*******************************************************************************/
void ds3HandleConnection(bool is_connected)
{
    ds3_is_connected = is_connected;

    /* Process the connection event */
    ds3_handle_connect_event(ds3_is_connected);
}

/*******************************************************************************
**
** Function         ds3EnableReport
**
** Description      This triggers the DS3 controller to start continually
**                  sending its data.
**
**
** Returns          void
**
*******************************************************************************/
void ds3EnableReport()
{
    hid_cmd_t hid_cmd = {
        .code = hid_cmd_code_set_report | hid_cmd_code_type_feature,
        .identifier = hid_cmd_identifier_ds3_enable,
    };
    uint16_t len = sizeof(hid_cmd_payload_report_enable);

    memcpy(hid_cmd.data, hid_cmd_payload_report_enable, len);

    ds3_l2cap_send_data((uint8_t *)&hid_cmd, len + 2U);
}

/*******************************************************************************
**
** Function         ds3SendCommand
**
** Description      Send a command to the DS3 controller.
**
**
** Returns          void
**
*******************************************************************************/
void ds3SendCommand()
{
    hid_cmd_t hid_cmd = {
        .code = hid_cmd_code_set_report | hid_cmd_code_type_output,
        .identifier = hid_cmd_identifier_ds3_control,
        .data = {0},
    };
    uint16_t len = sizeof(hid_cmd.data);

    /* Parse the output data */
    ds3_parse_output(&ds3_output_data, hid_cmd.data);

    /* Send the hid command */
    ds3_l2cap_send_data((uint8_t *)&hid_cmd, len + 2U);
}

/*******************************************************************************
**
** Function         ds3ReceiveData
**
** Description      Process the incoming data from the DS3 controller.
**
**
** Returns          void
**
*******************************************************************************/
void ds3ReceiveData(uint8_t p_data[const])
{
    hid_cmd_t *p_hid_cmd = (hid_cmd_t *)p_data;
    if (p_hid_cmd->code == (hid_cmd_code_data | hid_cmd_code_type_input))
    {
        // if (hid_cmd->identifier == hid_cmd_identifier_ds3_control)
        static ds3_event_t ds3_event;

        /* Save the previous data */
        ds3_input_data_t prev_data = ds3_input_data;

        /* Parse the input data */
        ds3_parse_input(p_hid_cmd->data, &ds3_input_data);

        /* Parse the event */
        ds3_parse_event(&prev_data, &ds3_input_data, &ds3_event);

        /* Process the data event */
        ds3_handle_data_event(&ds3_input_data, &ds3_event);
    }
}

/*******************************************************************************
**
** Function         ds3SetLed
**
** Description      Sets the LEDs on the DS3 controller
**
**
** Returns          void
**
*******************************************************************************/
void ds3SetLed(uint8_t num, bool val)
{
    switch (num)
    {
    case 0:
        ds3SetLeds(val, val, val, val);
        break;
    case 1:
        ds3_output_data.led.led1 = val;
        break;
    case 2:
        ds3_output_data.led.led2 = val;
        break;
    case 3:
        ds3_output_data.led.led3 = val;
        break;
    case 4:
        ds3_output_data.led.led4 = val;
        break;
    
    default:
        break;
    }
    ds3SendCommand();
}
void ds3SetLeds(bool led1, bool led2, bool led3, bool led4)
{
    ds3_output_data.led = (ds3_led_t){led1, led2, led3, led4};
    ds3SendCommand();
}

/*******************************************************************************
**
** Function         ds3SetRumble
**
** Description      Sets the Rumble on the DS3 controller
**
**
** Returns          void
**
*******************************************************************************/
void ds3SetRumble(uint8_t right_duration, uint8_t right_intensity, uint8_t left_duration, uint8_t left_intensity)
{
    ds3_output_data.rumble = (ds3_rumble_t){right_duration, right_intensity, left_duration, left_intensity};
    ds3SendCommand();
}

/*******************************************************************************
**
** Function         ds3SetConnectionCallback
**
** Description      Registers a callback for receiving DS3 controller
**                  connection notifications
**
**
** Returns          void
**
*******************************************************************************/
void ds3SetConnectionCallback(ds3_connection_callback_t cb)
{
    ds3_connection_cb = cb;
}

/*******************************************************************************
**
** Function         ds3SetEventCallback
**
** Description      Registers a callback for receiving DS3 controller events
**
**
** Returns          void
**
*******************************************************************************/
void ds3SetEventCallback(ds3_event_callback_t cb)
{
    ds3_event_cb = cb;
}

/*******************************************************************************
**
** Function         ds3SetBluetoothMacAddress
**
** Description      Writes a Registers a callback for receiving DS3 controller events
**
**
** Returns          void
**
*******************************************************************************/
void ds3SetBluetoothMacAddress(const uint8_t *mac)
{
    // The bluetooth MAC address is derived from the base MAC address
    // https://docs.espressif.com/projects/esp-idf/en/stable/api-reference/system/system.html#mac-address
    uint8_t base_mac[6];
    memcpy(base_mac, mac, 6);
    base_mac[5] -= 2;
    esp_base_mac_addr_set(base_mac);
}

/********************************************************************************/
/*                      L O C A L    F U N C T I O N S                          */
/********************************************************************************/

static void ds3_handle_connect_event(uint8_t is_connected)
{
    if (is_connected) {
        if (!ds3_is_active) {
            ds3EnableReport();
        }
    }
    else {
        ds3_is_active = false;
    }
}

static void ds3_handle_data_event(ds3_input_data_t *const p_data, ds3_event_t *const p_event)
{
    // Trigger packet event, but if this is the very first packet after connecting, trigger a connection event instead
    if (ds3_is_active) {
        /* Call the provided event callback */
        if (ds3_event_cb != NULL) {
            ds3_event_cb(p_data, p_event);
        }
    }
    else {
        ds3_is_active = true;
        /* Call the provided connection callback */
        if (ds3_connection_cb != NULL) {
            ds3_connection_cb(ds3_is_active);
        }
    }
}
