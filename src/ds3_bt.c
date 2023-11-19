#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "include/ds3.h"
#include "include/ds3_int.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"

#define DS3_TAG "DS3_BT"
#define DS3_DEVICE_NAME "DS3 Host"


/********************************************************************************/
/*              L O C A L    F U N C T I O N     P R O T O T Y P E S            */
/********************************************************************************/


/********************************************************************************/
/*                      P U B L I C    F U N C T I O N S                        */
/********************************************************************************/

/*******************************************************************************
**
** Function         ds3_bt_init
**
** Description      Initialize the Bluetooth service
**
** Returns          bool
**
*******************************************************************************/
bool ds3_bt_init()
{
    esp_err_t ret;

    /* Initialize the nvs flash */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ret = nvs_flash_erase();
        ESP_ERROR_CHECK(ret);
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

#ifdef CONFIG_BTDM_CONTROLLER_MODE_BR_EDR_ONLY
    /* Release memory used by the BLE stack */
    ret = esp_bt_mem_release(ESP_BT_MODE_BLE);
    ESP_ERROR_CHECK(ret);
#endif

    /* Initialize the Bluetooth controller */
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(DS3_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    /* Enable the Bluetooth controller */
    ret = esp_bt_controller_enable(BT_MODE);
    if (ret != ESP_OK)
    {
        ESP_LOGE(DS3_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    /* Initialize the Bluedroid stack */
    ret = esp_bluedroid_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(DS3_TAG, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    /* Enable the Bluedroid stack */
    ret = esp_bluedroid_enable();
    if (ret != ESP_OK)
    {
        ESP_LOGE(DS3_TAG, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    /* Set the Bluetooth device name */
    ret = esp_bt_dev_set_device_name(DS3_DEVICE_NAME);
    if (ret != ESP_OK)
    {
        ESP_LOGE(DS3_TAG, "%s set device name failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    /* Set the Bluetooth scan mode */
    ret = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
    if (ret != ESP_OK)
    {
        ESP_LOGE(DS3_TAG, "%s set scan mode failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    return true;
}

/*******************************************************************************
**
** Function         ds3_bt_deinit
**
** Description      Deinitialize the Bluetooth service
**
** Returns          bool
**
*******************************************************************************/
bool ds3_bt_deinit()
{
    esp_err_t ret;

    /* Disable the Bluedroid stack */
    ret = esp_bluedroid_disable();
    if (ret != ESP_OK)
    {
        ESP_LOGE(DS3_TAG, "%s disable bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    /* Deinitialize the Bluedroid stack */
    ret = esp_bluedroid_deinit();
    if (ret != ESP_OK)
    {
        ESP_LOGE(DS3_TAG, "%s deinitialize bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    /* Disable the Bluetooth controller */
    ret = esp_bt_controller_disable();
    if (ret != ESP_OK)
    {
        ESP_LOGE(DS3_TAG, "%s disable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    /* Deinitialize the Bluetooth controller */
    ret = esp_bt_controller_deinit();
    if (ret != ESP_OK)
    {
        ESP_LOGE(DS3_TAG, "%s deinitialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    return true;
}
