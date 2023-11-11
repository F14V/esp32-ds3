#ifndef DS3_INT_H
#define DS3_INT_H

#include "sdkconfig.h"
#include "ds3.h"

/** Check if the project is configured properly */
#ifndef CONFIG_BT_ENABLED
#error "The ESP32-DS3 component requires the Bluetooth component to be enabled in the project's menuconfig"
#endif

#ifndef CONFIG_BLUEDROID_ENABLED
#error "The ESP32-DS3 component requires Bluedroid to be enabled in the project's menuconfig"
#endif

#ifndef CONFIG_CLASSIC_BT_ENABLED
#error "The ESP32-DS3 component requires Classic Bluetooth to be enabled in the project's menuconfig"
#endif

#ifndef CONFIG_BT_L2CAP_ENABLED
#error "The ESP32-DS3 component requires Classic Bluetooth's L2CAP to be enabled in the project's menuconfig"
#endif

/** Check the configured blueooth mode */
#ifdef CONFIG_BTDM_CONTROLLER_MODE_BTDM
#define BT_MODE ESP_BT_MODE_BTDM
#elif defined CONFIG_BTDM_CONTROLLER_MODE_BR_EDR_ONLY
#define BT_MODE ESP_BT_MODE_CLASSIC_BT
#else
#error "The selected Bluetooth controller mode is not supported by the ESP32-DS3 component"
#endif


/** Size of the output report buffer for the Dualshock and Navigation controllers */
#define DS3_REPORT_BUFFER_SIZE 48
#define DS3_HID_BUFFER_SIZE    50

/********************************************************************************/
/*                         S H A R E D   T Y P E S                              */
/********************************************************************************/

enum hid_cmd_code {
    /* Report */
    hid_cmd_code_get_report   = 0x40,
    hid_cmd_code_set_report   = 0x50,
    /* Protocol */
    hid_cmd_code_get_protocol = 0x60,
    hid_cmd_code_set_protocol = 0x70,
    /* Idle */
    hid_cmd_code_get_idle     = 0x80,
    hid_cmd_code_set_idle     = 0x90,
    /* Data */
    hid_cmd_code_data         = 0xA0,
    hid_cmd_code_datc         = 0xB0,
    /* Type (for report and data) */
    hid_cmd_code_type_input   = 0x01,
    hid_cmd_code_type_output  = 0x02,
    hid_cmd_code_type_feature = 0x03,
};

enum hid_cmd_identifier {
    hid_cmd_identifier_ds3_enable  = 0xF4,
    hid_cmd_identifier_ds3_control = 0x01,
};

typedef struct {
    uint8_t code;
    uint8_t identifier;
    uint8_t data[DS3_REPORT_BUFFER_SIZE];
} hid_cmd_t;


/********************************************************************************/
/*                           B T   F U N C T I O N S                            */
/********************************************************************************/

void ds3_bt_init();
void ds3_bt_deinit();


/********************************************************************************/
/*                        L 2 C A P   F U N C T I O N S                         */
/********************************************************************************/

void ds3_l2cap_init_services();
void ds3_l2cap_deinit_services();
void ds3_l2cap_send_data(uint8_t p_data[const], uint16_t len);


/********************************************************************************/
/*                      P A R S E R   F U N C T I O N S                         */
/********************************************************************************/

void ds3_parse_input(uint8_t p_packet[const], ds3_input_data_t *const p_data);
void ds3_parse_output(ds3_output_data_t *const p_data, uint8_t p_packet[const]);
void ds3_parse_event(ds3_input_data_t *const p_prev, ds3_input_data_t *const p_data, ds3_event_t *const p_event);

#endif
