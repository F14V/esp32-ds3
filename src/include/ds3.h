#ifndef DS3_H
#define DS3_H

#include <stdint.h>

/* CONFIG */
/* Flags that can be defined prior to including this file to skip certain parsing functionality */
// Skip parsing accelerometer and gyroscope
// #define DS3_PARSE_SKIP_SENSOR
// Skip parsing analog functionality of buttons
// #define DS3_PARSE_SKIP_ANALOG
// Skip parsing analog changed events
// #define DS3_PARSE_SKIP_ANALOG_CHANGED

/********************************************************************************/
/*                                  T Y P E S                                   */
/********************************************************************************/

enum ds3_status_cable {
    ds3_status_cable_plugged   = 0x02,
    ds3_status_cable_unplugged = 0x03,
};

enum ds3_status_battery {
    ds3_status_battery_shutdown     = 0x01,
    ds3_status_battery_dying        = 0x02,
    ds3_status_battery_low          = 0x03,
    ds3_status_battery_high         = 0x04,
    ds3_status_battery_full         = 0x05,
    ds3_status_battery_charging     = 0xEE,
    ds3_status_battery_not_charging = 0xF1,
};

enum ds3_status_connection {
    ds3_status_connection_usb,
    ds3_status_connection_bluetooth,
};

enum ds3_status_rumble {
    ds3_status_rumble_on,
    ds3_status_rumble_off,
};


/********************/
/*     I N P U T    */
/********************/

/* Button */
typedef struct {
    uint8_t select   : 1;
    uint8_t l3       : 1;
    uint8_t r3       : 1;
    uint8_t start    : 1;

    uint8_t up       : 1;
    uint8_t right    : 1;
    uint8_t down     : 1;
    uint8_t left     : 1;

    uint8_t l2       : 1;
    uint8_t r2       : 1;
    uint8_t l1       : 1;
    uint8_t r1       : 1;

    uint8_t triangle : 1;
    uint8_t circle   : 1;
    uint8_t cross    : 1;
    uint8_t square   : 1;

    uint8_t ps       : 1;

    uint8_t          : 0;
} ds3_button_t;

/* Stick */
typedef struct {
    int8_t lx;
    int8_t ly;
    int8_t rx;
    int8_t ry;
} ds3_stick_t;

/* Analog */
typedef struct {
    uint8_t up;
    uint8_t right;
    uint8_t down;
    uint8_t left;

    uint8_t l2;
    uint8_t r2;
    uint8_t l1;
    uint8_t r1;

    uint8_t triangle;
    uint8_t circle;
    uint8_t cross;
    uint8_t square;
} ds3_analog_t;

/* Status */
typedef struct {
    uint8_t cable;      /* Plugged / Unplugged */
    uint8_t battery;    /* Charging / Not Charging / Level */
    uint8_t connection; /* Connection: USB / BT & Rumble: On / Off */
} ds3_status_t;

/* Sensor */
typedef struct {
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t gz;
} ds3_sensor_t;


/********************/
/*    O U T P U T   */
/********************/

/* Rumble */
typedef struct {
    uint8_t right_duration;
    uint8_t right_intensity;
    uint8_t left_duration;
    uint8_t left_intensity;
} ds3_rumble_t;

/* LED */
typedef struct {
    uint8_t      : 1;
    uint8_t led1 : 1;
    uint8_t led2 : 1;
    uint8_t led3 : 1;
    uint8_t led4 : 1;
    uint8_t      : 0;
} ds3_led_t;


/*******************/
/*    O T H E R    */
/*******************/

/* Input data struct */
typedef struct {
    ds3_button_t button;
    ds3_stick_t stick;
#ifndef DS3_PARSE_SKIP_ANALOG
    ds3_analog_t analog;
#endif
    ds3_status_t status;
#ifndef DS3_PARSE_SKIP_SENSOR
    ds3_sensor_t sensor;
#endif
} ds3_input_data_t;

/* Output data struct */
typedef struct {
    ds3_rumble_t rumble;
    ds3_led_t led;
} ds3_output_data_t;

/* Event struct */
typedef struct {
    ds3_button_t button_down;
    ds3_button_t button_up;
    ds3_stick_t stick_changed;
#ifndef DS3_PARSE_SKIP_ANALOG
#ifndef DS3_PARSE_SKIP_ANALOG_CHANGED
    ds3_analog_t analog_changed;
#endif
#endif
} ds3_event_t;


/***************************/
/*    C A L L B A C K S    */
/***************************/

typedef void (*ds3_connection_callback_t)(uint8_t is_connected);
typedef void (*ds3_event_callback_t)(ds3_input_data_t *const p_data, ds3_event_t *const p_event);


/********************************************************************************/
/*                             F U N C T I O N S                                */
/********************************************************************************/

bool ds3IsConnected();
void ds3Init();
void ds3Deinit();
void ds3HandleConnection(bool);
void ds3EnableReport();
void ds3SendCommand();
void ds3ReceiveData(uint8_t *const);
void ds3SetLed(uint8_t, bool);
void ds3SetLeds(bool, bool, bool, bool);
void ds3SetRumble(uint8_t, uint8_t, uint8_t, uint8_t);
void ds3SetConnectionCallback(ds3_connection_callback_t);
void ds3SetEventCallback(ds3_event_callback_t);
void ds3SetBluetoothMacAddress(const uint8_t *);

#endif
