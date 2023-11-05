#ifndef PS3_H
#define PS3_H

#include <stdint.h>

/* CONFIG */
/* Flags that can be defined prior to including this file to skip certain parsing functionality */
// Skip parsing accelerometer and gyroscope
// #define PS3_PARSE_SKIP_SENSOR
// Skip parsing analog functionality of buttons
// #define PS3_PARSE_SKIP_ANALOG
// Skip parsing analog changed events
// #define PS3_PARSE_SKIP_ANALOG_CHANGED

/********************************************************************************/
/*                                  T Y P E S                                   */
/********************************************************************************/

enum ps3_status_cable {
    ps3_status_cable_plugged   = 0x02,
    ps3_status_cable_unplugged = 0x03,
};

enum ps3_status_battery {
    ps3_status_battery_shutdown     = 0x01,
    ps3_status_battery_dying        = 0x02,
    ps3_status_battery_low          = 0x03,
    ps3_status_battery_high         = 0x04,
    ps3_status_battery_full         = 0x05,
    ps3_status_battery_charging     = 0xEE,
    ps3_status_battery_not_charging = 0xF1,
};

enum ps3_status_connection {
    ps3_status_connection_usb,
    ps3_status_connection_bluetooth,
};

enum ps3_status_rumble {
    ps3_status_rumble_on,
    ps3_status_rumble_off,
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
} ps3_button_t;

/* Stick */
typedef struct {
    int8_t lx;
    int8_t ly;
    int8_t rx;
    int8_t ry;
} ps3_stick_t;

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
} ps3_analog_t;

/* Status */
typedef struct {
    uint8_t cable;      /* Plugged / Unplugged */
    uint8_t battery;    /* Charging / Not Charging / Level */
    uint8_t connection; /* Connection: USB / BT & Rumble: On / Off */
} ps3_status_t;

/* Sensor */
typedef struct {
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t gz;
} ps3_sensor_t;


/********************/
/*    O U T P U T   */
/********************/

/* Rumble */
typedef struct {
    uint8_t right_duration;
    uint8_t right_intensity;
    uint8_t left_duration;
    uint8_t left_intensity;
} ps3_rumble_t;

/* LED */
typedef struct {
    uint8_t      : 1;
    uint8_t led1 : 1;
    uint8_t led2 : 1;
    uint8_t led3 : 1;
    uint8_t led4 : 1;
    uint8_t      : 0;
} ps3_led_t;


/*******************/
/*    O T H E R    */
/*******************/

/* Input data struct */
typedef struct {
    ps3_button_t button;
    ps3_stick_t stick;
#ifndef PS3_PARSE_SKIP_ANALOG
    ps3_analog_t analog;
#endif
    ps3_status_t status;
#ifndef PS3_PARSE_SKIP_SENSOR
    ps3_sensor_t sensor;
#endif
} ps3_input_data_t;

/* Output data struct */
typedef struct {
    ps3_rumble_t rumble;
    ps3_led_t led;
} ps3_output_data_t;

/* Event struct */
typedef struct {
    ps3_button_t button_down;
    ps3_button_t button_up;
    ps3_stick_t stick_changed;
#ifndef PS3_PARSE_SKIP_ANALOG
#ifndef PS3_PARSE_SKIP_ANALOG_CHANGED
    ps3_analog_t analog_changed;
#endif
#endif
} ps3_event_t;


/***************************/
/*    C A L L B A C K S    */
/***************************/

typedef void (*ps3_connection_callback_t)(uint8_t is_connected);
typedef void (*ps3_connection_object_callback_t)(void *const p_object, uint8_t is_connected);

typedef void (*ps3_event_callback_t)(ps3_input_data_t *const p_data, ps3_event_t *const p_event);
typedef void (*ps3_event_object_callback_t)(void *const p_object, ps3_input_data_t *const p_data, ps3_event_t *const p_event);


/********************************************************************************/
/*                             F U N C T I O N S                                */
/********************************************************************************/

bool ps3IsConnected();
void ps3Init();
void ps3Deinit();
void ps3HandleConnection(bool);
void ps3EnableReport();
void ps3SendCommand();
void ps3ReceiveData(uint8_t *const);
void ps3SetLed(uint8_t, bool);
void ps3SetLeds(bool, bool, bool, bool);
void ps3SetRumble(uint8_t, uint8_t, uint8_t, uint8_t);
void ps3SetConnectionCallback(ps3_connection_callback_t);
void ps3SetConnectionObjectCallback(void *const, ps3_connection_object_callback_t);
void ps3SetEventCallback(ps3_event_callback_t);
void ps3SetEventObjectCallback(void *const, ps3_event_object_callback_t);
void ps3SetBluetoothMacAddress(const uint8_t *);

#endif
