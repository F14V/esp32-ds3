#ifndef PS3_H
#define PS3_H

#include <stdint.h>

/* CONFIG */
// Use accelerometer and gyroscope
#define PS3_PARSE_SENSOR
// Use analog functionality of buttons
#define PS3_PARSE_ANALOG_BUTTON
// Use analog changed events
#define PS3_PARSE_ANALOG_CHANGED

/********************************************************************************/
/*                                  T Y P E S                                   */
/********************************************************************************/

/********************/
/*    A N A L O G   */
/********************/

typedef struct {
    uint8_t lx;
    uint8_t ly;
    uint8_t rx;
    uint8_t ry;
} ps3_analog_stick_t;

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
} ps3_analog_button_t;

typedef struct {
    ps3_analog_stick_t stick;
#ifdef PS3_PARSE_ANALOG_BUTTON
    ps3_analog_button_t button;
#endif
} ps3_analog_t;


/*********************/
/*   B U T T O N S   */
/*********************/

typedef union {
    struct {
        uint32_t select   : 1;
        uint32_t l3       : 1;
        uint32_t r3       : 1;
        uint32_t start    : 1;

        uint32_t up       : 1;
        uint32_t right    : 1;
        uint32_t down     : 1;
        uint32_t left     : 1;

        uint32_t l2       : 1;
        uint32_t r2       : 1;
        uint32_t l1       : 1;
        uint32_t r1       : 1;

        uint32_t triangle : 1;
        uint32_t circle   : 1;
        uint32_t cross    : 1;
        uint32_t square   : 1;

        uint32_t ps       : 1;
    };
    uint32_t fields;
} ps3_button_t;


/*******************************/
/*   S T A T U S   F L A G S   */
/*******************************/

enum ps3_status_battery {
    ps3_status_battery_shutdown = 0x01,
    ps3_status_battery_dying    = 0x02,
    ps3_status_battery_low      = 0x03,
    ps3_status_battery_high     = 0x04,
    ps3_status_battery_full     = 0x05,
    ps3_status_battery_charging = 0xEE
};

enum ps3_status_connection {
    ps3_status_connection_usb,
    ps3_status_connection_bluetooth
};

typedef struct {
    enum ps3_status_battery battery;
    enum ps3_status_connection connection;
    uint8_t charging : 1;
    uint8_t rumbling : 1;
} ps3_status_t;


/********************/
/*   S E N S O R S  */
/********************/

typedef struct {
    int16_t z;
} ps3_sensor_gyroscope_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} ps3_sensor_accelerometer_t;

typedef struct {
    ps3_sensor_accelerometer_t accelerometer;
    ps3_sensor_gyroscope_t gyroscope;
} ps3_sensor_t;


/*******************/
/*    O T H E R    */
/*******************/

typedef struct {
    /* Rumble control */
    uint8_t rumble_right_duration;
    uint8_t rumble_right_intensity;
    uint8_t rumble_left_duration;
    uint8_t rumble_left_intensity;

    /* LED control */
    uint8_t led1 : 1;
    uint8_t led2 : 1;
    uint8_t led3 : 1;
    uint8_t led4 : 1;
} ps3_cmd_t;

typedef struct {
    ps3_button_t button_down;
    ps3_button_t button_up;
#ifdef PS3_PARSE_ANALOG_CHANGED
    ps3_analog_t analog_changed;
#endif
} ps3_event_t;

typedef struct {
    ps3_analog_t analog;
    ps3_button_t button;
    ps3_status_t status;
#ifdef PS3_PARSE_SENSOR
    ps3_sensor_t sensor;
#endif
} ps3_t;


/***************************/
/*    C A L L B A C K S    */
/***************************/

typedef void(*ps3_connection_callback_t)( uint8_t is_connected );
typedef void(*ps3_connection_object_callback_t)( void *object, uint8_t is_connected );

typedef void(*ps3_event_callback_t)( ps3_t ps3, ps3_event_t event );
typedef void(*ps3_event_object_callback_t)( void *object, ps3_t ps3, ps3_event_t event );


/********************************************************************************/
/*                             F U N C T I O N S                                */
/********************************************************************************/

bool ps3IsConnected();
void ps3Init();
void ps3Deinit();
void ps3Enable();
void ps3Cmd( ps3_cmd_t ps3_cmd );
void ps3SetConnectionCallback( ps3_connection_callback_t cb );
void ps3SetConnectionObjectCallback( void *object, ps3_connection_object_callback_t cb );
void ps3SetEventCallback( ps3_event_callback_t cb );
void ps3SetEventObjectCallback( void *object, ps3_event_object_callback_t cb );
void ps3SetLed( uint8_t player );
void ps3SetLedCmd( ps3_cmd_t *cmd, uint8_t player );
void ps3SetBluetoothMacAddress( const uint8_t *mac );


#endif
