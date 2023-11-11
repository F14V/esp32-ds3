#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "include/ds3.h"
#include "include/ds3_int.h"
#include "esp_log.h"

#define  DS3_TAG "DS3_PARSER"


/********************************************************************************/
/*                            L O C A L    T Y P E S                            */
/********************************************************************************/

/* Input report struct */
typedef struct __attribute__((__packed__)) {
    /* Unknown */
    uint8_t unk0[1];
    /* Button */
    ds3_button_t button;
    /* Unknown */
    uint8_t unk1[1];
    /* Stick */
    ds3_stick_t stick;
    /* Unknown */
    uint8_t unk2[4];
    /* Analog */
    ds3_analog_t analog;
    /* Unknown */
    uint8_t unk3[3];
    /* Status */
    ds3_status_t status;
    /* Unknown */
    uint8_t unk4[9];
    /* Sensor */
    ds3_sensor_t sensor;
} ds3_input_report_t;

/* Led payload struct */
#define DS3_LED_PAYLOAD {0xFF, 0x27, 0x10, 0x00, 0x32}
typedef struct {
    uint8_t led4[5];
    uint8_t led3[5];
    uint8_t led2[5];
    uint8_t led1[5];
} ds3_led_payload_t;

/* Output report struct */
typedef struct __attribute__((__packed__)) {
    uint8_t unk0[1];
    ds3_rumble_t rumble;
    uint8_t unk1[4];
    ds3_led_t led;
    ds3_led_payload_t payload;
    uint8_t unk2[5];
} ds3_output_report_t;


/********************************************************************************/
/*                      P U B L I C    F U N C T I O N S                        */
/********************************************************************************/

/*******************************************************************************
**
** Function         ds3_parse_input
**
** Description      Parse the input packet into input data
**
** Returns          void
**
*******************************************************************************/
void ds3_parse_input(uint8_t p_packet[const], ds3_input_data_t *const p_data)
{
    /* Cast input report pointer */
    ds3_input_report_t *p_report = (ds3_input_report_t *)p_packet;

    /* Parse input data */
    p_data->button = p_report->button;
    p_data->stick = p_report->stick;
    /* Remove offset */
    p_data->stick.lx -= (INT8_MAX + 1);
    p_data->stick.ly -= (INT8_MAX + 1);
    p_data->stick.rx -= (INT8_MAX + 1);
    p_data->stick.ry -= (INT8_MAX + 1);
#ifndef DS3_PARSE_SKIP_ANALOG
    p_data->analog = p_report->analog;
#endif
    p_data->status = p_report->status;
#ifndef DS3_PARSE_SKIP_SENSOR
    p_data->sensor = p_report->sensor;
    /* Remove offset */
    p_data->sensor.ax -= (INT16_MAX + 1);
    p_data->sensor.ay -= (INT16_MAX + 1);
    p_data->sensor.az -= (INT16_MAX + 1);
    p_data->sensor.gz -= (INT16_MAX + 1);
#endif
}

/*******************************************************************************
**
** Function         ds3_parse_output
**
** Description      Parse the output data into output packet
**
** Returns          void
**
*******************************************************************************/
void ds3_parse_output(ds3_output_data_t *const p_data, uint8_t p_packet[const])
{
    /* Cast output report pointer */
    ds3_output_report_t *p_report = (ds3_output_report_t *)p_packet;

    /* Parse output data */
    p_report->rumble = p_data->rumble;
    p_report->led = p_data->led;
    p_report->payload = (ds3_led_payload_t){DS3_LED_PAYLOAD, DS3_LED_PAYLOAD, DS3_LED_PAYLOAD, DS3_LED_PAYLOAD};
}

/*******************************************************************************
**
** Function         ds3_parse_event
**
** Description      Parse the input data and previous data into event report
**
** Returns          void
**
*******************************************************************************/
void ds3_parse_event(ds3_input_data_t *const p_prev, ds3_input_data_t *const p_data, ds3_event_t *const p_event)
{
    /* Button events */
    uint8_t *p_old = (uint8_t *)&p_prev->button;
    uint8_t *p_new = (uint8_t *)&p_data->button;
    uint8_t *p_evt;

    /* Button down */
    p_evt = (uint8_t *)&p_event->button_down;
    p_evt[0] = ~p_old[0] & p_new[0];
    p_evt[1] = ~p_old[1] & p_new[1];
    p_evt[2] = ~p_old[2] & p_new[2];

    /* Button up */
    p_evt = (uint8_t *)&p_event->button_up;
    p_evt[0] = p_old[0] & ~p_new[0];
    p_evt[1] = p_old[1] & ~p_new[1];
    p_evt[2] = p_old[2] & ~p_new[2];

    /* Stick events */
    p_event->stick_changed.lx        = p_data->stick.lx - p_prev->stick.lx;
    p_event->stick_changed.ly        = p_data->stick.ly - p_prev->stick.ly;
    p_event->stick_changed.rx        = p_data->stick.rx - p_prev->stick.rx;
    p_event->stick_changed.ry        = p_data->stick.ry - p_prev->stick.ry;

#ifndef DS3_PARSE_SKIP_ANALOG
#ifndef DS3_PARSE_SKIP_ANALOG_CHANGED
    /* Analog events */
    p_event->analog_changed.up       = p_data->analog.up    - p_prev->analog.up;
    p_event->analog_changed.right    = p_data->analog.right - p_prev->analog.right;
    p_event->analog_changed.down     = p_data->analog.down  - p_prev->analog.down;
    p_event->analog_changed.left     = p_data->analog.left  - p_prev->analog.left;

    p_event->analog_changed.l2       = p_data->analog.l2 - p_prev->analog.l2;
    p_event->analog_changed.r2       = p_data->analog.r2 - p_prev->analog.r2;
    p_event->analog_changed.l1       = p_data->analog.l1 - p_prev->analog.l1;
    p_event->analog_changed.r1       = p_data->analog.r1 - p_prev->analog.r1;

    p_event->analog_changed.triangle = p_data->analog.triangle - p_prev->analog.triangle;
    p_event->analog_changed.circle   = p_data->analog.circle   - p_prev->analog.circle;
    p_event->analog_changed.cross    = p_data->analog.cross    - p_prev->analog.cross;
    p_event->analog_changed.square   = p_data->analog.square   - p_prev->analog.square;
#endif
#endif
}
