#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <esp_mac.h>
#include "include/ps3.h"
#include "include/ps3_int.h"


/********************************************************************************/
/*                              C O N S T A N T S                               */
/********************************************************************************/

static const uint8_t hid_cmd_payload_report_enable[] = { 0x42, 0x03, 0x00, 0x00 };
// static const uint8_t hid_cmd_payload_led_arguments[] = { 0xFF, 0x27, 0x10, 0x00, 0x32 };


/********************************************************************************/
/*                         L O C A L    V A R I A B L E S                       */
/********************************************************************************/

/* Connection callbacks */
static ps3_connection_callback_t ps3_connection_cb = NULL;
static ps3_connection_object_callback_t ps3_connection_object_cb = NULL;
static void *ps3_connection_object = NULL;

/* Event callbacks */
static ps3_event_callback_t ps3_event_cb = NULL;
static ps3_event_object_callback_t ps3_event_object_cb = NULL;
static void *ps3_event_object = NULL;

/* Status flags */
static bool ps3_is_connected = false;
static bool ps3_is_active = false;

/* Input and output data */
static ps3_input_data_t ps3_input_data;
static ps3_output_data_t ps3_output_data;


/********************************************************************************/
/*              L O C A L    F U N C T I O N     P R O T O T Y P E S            */
/********************************************************************************/

static void ps3_handle_connect_event(uint8_t is_connected);
static void ps3_handle_data_event(ps3_input_data_t *const p_data, ps3_event_t *const p_event);


/********************************************************************************/
/*                      P U B L I C    F U N C T I O N S                        */
/********************************************************************************/

/*******************************************************************************
**
** Function         ps3Init
**
** Description      This initializes the bluetooth services to listen
**                  for an incoming PS3 controller connection.
**
**
** Returns          void
**
*******************************************************************************/
void ps3Init()
{
    ps3_bt_init();
    ps3_l2cap_init_services();
}

/*******************************************************************************
**
** Function         ps3Deinit
**
** Description      This deinitializes the bluetooth services to stop
**                  listening for incoming connections.
**
**
** Returns          void
**
*******************************************************************************/
void ps3Deinit()
{
    ps3_l2cap_deinit_services();
    ps3_bt_deinit();
}

/*******************************************************************************
**
** Function         ps3IsConnected
**
** Description      This returns whether a PS3 controller is connected, based
**                  on whether a successful handshake has taken place.
**
**
** Returns          bool
**
*******************************************************************************/
bool ps3IsConnected()
{
    return ps3_is_active;
}

/*******************************************************************************
**
** Function         ps3HandleConnection
**
** Description      Handle the connection of the PS3 controller.
**
**
** Returns          void
**
*******************************************************************************/
void ps3HandleConnection(bool is_connected)
{
    ps3_is_connected = is_connected;

    /* Process the connection event */
    ps3_handle_connect_event(ps3_is_connected);
}

/*******************************************************************************
**
** Function         ps3EnableReport
**
** Description      This triggers the PS3 controller to start continually
**                  sending its data.
**
**
** Returns          void
**
*******************************************************************************/
void ps3EnableReport()
{
    hid_cmd_t hid_cmd = {
        .code = hid_cmd_code_set_report | hid_cmd_code_type_feature,
        .identifier = hid_cmd_identifier_ps3_enable,
    };
    uint16_t len = sizeof(hid_cmd_payload_report_enable);

    memcpy(hid_cmd.data, hid_cmd_payload_report_enable, len);

    ps3_l2cap_send_data((uint8_t *)&hid_cmd, len + 2U);
}

/*******************************************************************************
**
** Function         ps3SendCommand
**
** Description      Send a command to the PS3 controller.
**
**
** Returns          void
**
*******************************************************************************/
void ps3SendCommand()
{
    hid_cmd_t hid_cmd = {
        .code = hid_cmd_code_set_report | hid_cmd_code_type_output,
        .identifier = hid_cmd_identifier_ps3_control,
        .data = {0},
    };
    uint16_t len = sizeof(hid_cmd.data);

    /* Parse the output data */
    ps3_parse_output(&ps3_output_data, hid_cmd.data);

    /* Send the hid command */
    ps3_l2cap_send_data((uint8_t *)&hid_cmd, len + 2U);
}

/*******************************************************************************
**
** Function         ps3ReceiveData
**
** Description      Process the incoming data from the PS3 controller.
**
**
** Returns          void
**
*******************************************************************************/
void ps3ReceiveData(uint8_t p_data[const])
{
    hid_cmd_t *p_hid_cmd = (hid_cmd_t *)p_data;
    if (p_hid_cmd->code == (hid_cmd_code_data | hid_cmd_code_type_input))
    {
        // if (hid_cmd->identifier == hid_cmd_identifier_ps3_control)
        static ps3_event_t ps3_event;

        /* Save the previous data */
        ps3_input_data_t prev_data = ps3_input_data;

        /* Parse the input data */
        ps3_parse_input(p_hid_cmd->data, &ps3_input_data);

        /* Parse the event */
        ps3_parse_event(&prev_data, &ps3_input_data, &ps3_event);

        /* Process the data event */
        ps3_handle_data_event(&ps3_input_data, &ps3_event);
    }
}

/*******************************************************************************
**
** Function         ps3SetLed
**
** Description      Sets the LEDs on the PS3 controller
**
**
** Returns          void
**
*******************************************************************************/
void ps3SetLed(uint8_t num, bool val)
{
    switch (num)
    {
    case 0:
        ps3SetLeds(val, val, val, val);
        break;
    case 1:
        ps3_output_data.led.led1 = val;
        break;
    case 2:
        ps3_output_data.led.led2 = val;
        break;
    case 3:
        ps3_output_data.led.led3 = val;
        break;
    case 4:
        ps3_output_data.led.led4 = val;
        break;
    
    default:
        break;
    }
    ps3SendCommand();
}
void ps3SetLeds(bool led1, bool led2, bool led3, bool led4)
{
    ps3_output_data.led = (ps3_led_t){led1, led2, led3, led4};
    ps3SendCommand();
}

/*******************************************************************************
**
** Function         ps3SetRumble
**
** Description      Sets the Rumble on the PS3 controller
**
**
** Returns          void
**
*******************************************************************************/
void ps3SetRumble(uint8_t right_duration, uint8_t right_intensity, uint8_t left_duration, uint8_t left_intensity)
{
    ps3_output_data.rumble = (ps3_rumble_t){right_duration, right_intensity, left_duration, left_intensity};
    ps3SendCommand();
}

/*******************************************************************************
**
** Function         ps3SetConnectionCallback
**
** Description      Registers a callback for receiving PS3 controller
**                  connection notifications
**
**
** Returns          void
**
*******************************************************************************/
void ps3SetConnectionCallback(ps3_connection_callback_t cb)
{
    ps3_connection_cb = cb;
}

/*******************************************************************************
**
** Function         ps3SetConnectionObjectCallback
**
** Description      Registers a callback for receiving PS3 controller
**                  connection notifications
**
**
** Returns          void
**
*******************************************************************************/
void ps3SetConnectionObjectCallback(void *const p_object, ps3_connection_object_callback_t cb)
{
    ps3_connection_object_cb = cb;
    ps3_connection_object = p_object;
}

/*******************************************************************************
**
** Function         ps3SetEventCallback
**
** Description      Registers a callback for receiving PS3 controller events
**
**
** Returns          void
**
*******************************************************************************/
void ps3SetEventCallback(ps3_event_callback_t cb)
{
    ps3_event_cb = cb;
}

/*******************************************************************************
**
** Function         ps3SetEventObjectCallback
**
** Description      Registers a callback for receiving PS3 controller events
**
**
** Returns          void
**
*******************************************************************************/
void ps3SetEventObjectCallback(void *const p_object, ps3_event_object_callback_t cb)
{
    ps3_event_object_cb = cb;
    ps3_event_object = p_object;
}

/*******************************************************************************
**
** Function         ps3SetBluetoothMacAddress
**
** Description      Writes a Registers a callback for receiving PS3 controller events
**
**
** Returns          void
**
*******************************************************************************/
void ps3SetBluetoothMacAddress(const uint8_t *mac)
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

static void ps3_handle_connect_event(uint8_t is_connected)
{
    if (is_connected) {
        if (!ps3_is_active) {
            ps3EnableReport();
        }
    }
    else {
        ps3_is_active = false;
    }
}

static void ps3_handle_data_event(ps3_input_data_t *const p_data, ps3_event_t *const p_event)
{
    // Trigger packet event, but if this is the very first packet after connecting, trigger a connection event instead
    if (ps3_is_active) {
        if (ps3_event_cb != NULL) {
            ps3_event_cb(p_data, p_event);
        }

        if (ps3_event_object_cb != NULL && ps3_event_object != NULL) {
            ps3_event_object_cb(ps3_event_object, p_data, p_event);
        }
    }
    else {
        ps3_is_active = true;

        if (ps3_connection_cb != NULL) {
            ps3_connection_cb(ps3_is_active);
        }

        if (ps3_connection_object_cb != NULL && ps3_connection_object != NULL) {
            ps3_connection_object_cb(ps3_connection_object, ps3_is_active);
        }
    }
}
