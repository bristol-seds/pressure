/* Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/* Attention!
 *  To maintain compliance with Nordic Semiconductor ASAs Bluetooth profile
 *  qualification listings, this section of source code must not be modified.
 */

#include "ble_ess.h"
#include <string.h>
#include "nordic_common.h"
#include "ble_l2cap.h"
#include "ble_srv_common.h"
#include "app_util.h"


#define OPCODE_LENGTH 1                                                    /**< Length of opcode inside Heart Rate Measurement packet. */
#define HANDLE_LENGTH 2                                                    /**< Length of handle inside Heart Rate Measurement packet. */


#define BLE_UUID_ENVIROMENTAL_SENSING_SERVICE		0x181A     /**< ES service UUID. */
#define BLE_UUID_PRESSURE_CHAR				0x2A6D     /**< Location and Speed characteristic UUID. */
#define BLE_UUID_TEMPERATURE_CHAR			0x2A6E     /**< LN feature characteristic UUID. */




/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_ess       Heart Rate Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_connect(ble_ess_t * p_ess, ble_evt_t * p_ble_evt)
{
  p_ess->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}


/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_ess       Heart Rate Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_disconnect(ble_ess_t * p_ess, ble_evt_t * p_ble_evt)
{
  UNUSED_PARAMETER(p_ble_evt);
  p_ess->conn_handle = BLE_CONN_HANDLE_INVALID;
}


/**@brief Function for handling write events to the Heart Rate Measurement characteristic.
 *
 * @param[in]   p_ess         Heart Rate Service structure.
 * @param[in]   p_evt_write   Write event received from the BLE stack.
 */
static void on_lsc_cccd_write(ble_ess_t * p_ess, ble_gatts_evt_write_t * p_evt_write)
{
  if (p_evt_write->len == 2)
  {
    // CCCD written, update notification state
    if (p_ess->evt_handler != NULL)
    {
      ble_ess_evt_t evt;

      if (ble_srv_is_notification_enabled(p_evt_write->data))
      {
        evt.evt_type = BLE_ESS_EVT_NOTIFICATION_ENABLED;
      }
      else
      {
        evt.evt_type = BLE_ESS_EVT_NOTIFICATION_DISABLED;
      }

      p_ess->evt_handler(p_ess, &evt);
    }
  }
}


/**@brief Function for handling the Write event.
 *
 * @param[in]   p_ess       Heart Rate Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_write(ble_ess_t * p_ess, ble_evt_t * p_ble_evt)
{
  ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

  if (p_evt_write->handle == p_ess->pc_handles.cccd_handle)
  {
    on_lsc_cccd_write(p_ess, p_evt_write);
  }
  if (p_evt_write->handle == p_ess->tc_handles.cccd_handle)
  {
    on_lsc_cccd_write(p_ess, p_evt_write);
  }
}


void ble_ess_on_ble_evt(ble_ess_t * p_ess, ble_evt_t * p_ble_evt)
{
  switch (p_ble_evt->header.evt_id)
  {
  case BLE_GAP_EVT_CONNECTED:
    on_connect(p_ess, p_ble_evt);
    break;

  case BLE_GAP_EVT_DISCONNECTED:
    on_disconnect(p_ess, p_ble_evt);
    break;

  case BLE_GATTS_EVT_WRITE:
    on_write(p_ess, p_ble_evt);
    break;

  default:
    // No implementation needed.
    break;
  }
}

/**@brief Function for adding the Pressure characteristic.
 *
 * @param[in]   p_ess        Heart Rate Service structure.
 * @param[in]   p_ess_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t pressure_char_add(ble_ess_t            * p_ess,
                                  const ble_ess_init_t * p_ess_init)
{
  ble_gatts_char_md_t char_md;
  ble_gatts_attr_md_t cccd_md;
  ble_gatts_attr_t    attr_char_value;
  ble_uuid_t          ble_uuid;
  ble_gatts_attr_md_t attr_md;
  uint32_t		value = 98;

  memset(&cccd_md, 0, sizeof(cccd_md));

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
  cccd_md.write_perm = p_ess_init->ess_pc_attr_md.cccd_write_perm;
  cccd_md.vloc       = BLE_GATTS_VLOC_STACK;

  memset(&char_md, 0, sizeof(char_md));

  char_md.char_props.notify = 1;
  char_md.char_props.read = 1;
  char_md.p_char_user_desc  = NULL;
  char_md.p_char_pf         = NULL;
  char_md.p_user_desc_md    = NULL;
  char_md.p_cccd_md         = &cccd_md;
  char_md.p_sccd_md         = NULL;

  BLE_UUID_BLE_ASSIGN(ble_uuid, BLE_UUID_PRESSURE_CHAR);

  memset(&attr_md, 0, sizeof(attr_md));

  attr_md.read_perm  = p_ess_init->ess_pc_attr_md.read_perm;
  attr_md.write_perm = p_ess_init->ess_pc_attr_md.write_perm;
  attr_md.vloc       = BLE_GATTS_VLOC_STACK;
  attr_md.rd_auth    = 0;
  attr_md.wr_auth    = 0;
  attr_md.vlen       = 1;

  memset(&attr_char_value, 0, sizeof(attr_char_value));

  attr_char_value.p_uuid    = &ble_uuid;
  attr_char_value.p_attr_md = &attr_md;
  attr_char_value.init_len  = sizeof(uint32_t);
  attr_char_value.init_offs = 0;
  attr_char_value.max_len   = sizeof(uint32_t);
  attr_char_value.p_value   = (uint8_t*)&value;

  return sd_ble_gatts_characteristic_add(p_ess->service_handle,
                                         &char_md,
                                         &attr_char_value,
                                         &p_ess->pc_handles);
}


/**@brief Function for adding the Body Sensor Location characteristic.
 *
 * @param[in]   p_ess        Heart Rate Service structure.
 * @param[in]   p_ess_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t temperature_char_add(ble_ess_t * p_ess, const ble_ess_init_t * p_ess_init)
{
  ble_gatts_char_md_t char_md;
  ble_gatts_attr_md_t cccd_md;
  ble_gatts_attr_t    attr_char_value;
  ble_uuid_t          ble_uuid;
  ble_gatts_attr_md_t attr_md;
  int16_t 		value = 9;

  memset(&cccd_md, 0, sizeof(cccd_md));

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
  cccd_md.write_perm = p_ess_init->ess_tc_attr_md.cccd_write_perm;
  cccd_md.vloc       = BLE_GATTS_VLOC_STACK;

  memset(&char_md, 0, sizeof(char_md));

  char_md.char_props.notify = 1;
  char_md.char_props.read = 1;
  char_md.p_char_user_desc  = NULL;
  char_md.p_char_pf         = NULL;
  char_md.p_user_desc_md    = NULL;
  char_md.p_cccd_md         = &cccd_md;
  char_md.p_sccd_md         = NULL;

  BLE_UUID_BLE_ASSIGN(ble_uuid, BLE_UUID_TEMPERATURE_CHAR);

  memset(&attr_md, 0, sizeof(attr_md));

  attr_md.read_perm  = p_ess_init->ess_tc_attr_md.read_perm;
  attr_md.write_perm = p_ess_init->ess_tc_attr_md.write_perm;
  attr_md.vloc       = BLE_GATTS_VLOC_STACK;
  attr_md.rd_auth    = 0;
  attr_md.wr_auth    = 0;
  attr_md.vlen       = 1;

  memset(&attr_char_value, 0, sizeof(attr_char_value));

  attr_char_value.p_uuid    = &ble_uuid;
  attr_char_value.p_attr_md = &attr_md;
  attr_char_value.init_len  = sizeof(int16_t);
  attr_char_value.init_offs = 0;
  attr_char_value.max_len   = sizeof(int16_t);
  attr_char_value.p_value   = (uint8_t*)&value;

  return sd_ble_gatts_characteristic_add(p_ess->service_handle,
                                         &char_md,
                                         &attr_char_value,
                                         &p_ess->tc_handles);
}


uint32_t ble_ess_init(ble_ess_t * p_ess, const ble_ess_init_t * p_ess_init)
{
  uint32_t   err_code;
  ble_uuid_t ble_uuid;

  // Initialize service structure
  p_ess->evt_handler                 = p_ess_init->evt_handler;
  p_ess->is_sensor_contact_supported = p_ess_init->is_sensor_contact_supported;
  p_ess->conn_handle                 = BLE_CONN_HANDLE_INVALID;
  p_ess->is_sensor_contact_detected  = false;
  p_ess->pressure_last          	= 0;
  p_ess->temperature_last		= -32767;

  // Add service
  BLE_UUID_BLE_ASSIGN(ble_uuid, BLE_UUID_ENVIROMENTAL_SENSING_SERVICE);

  err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                      &ble_uuid,
                                      &p_ess->service_handle);

  if (err_code != NRF_SUCCESS)
  {
    return err_code;
  }

  // Add pressure characteristic
  err_code = pressure_char_add(p_ess, p_ess_init);
  if (err_code != NRF_SUCCESS)
  {
    return err_code;
  }

  // Add temperature characteristic
  err_code = temperature_char_add(p_ess, p_ess_init);
  if (err_code != NRF_SUCCESS)
  {
    return err_code;
  }

  return NRF_SUCCESS;
}


uint32_t ble_ess_pressure_send(ble_ess_t * p_ess, uint32_t pressure)
{
  uint32_t err_code = NRF_SUCCESS;

  if (pressure != p_ess->pressure_last)
  {
    uint16_t len = sizeof(uint32_t);

    // Save new value
    p_ess->pressure_last = pressure;

    // Update database
    err_code = sd_ble_gatts_value_set(p_ess->pc_handles.value_handle,
                                      0,
                                      &len,
                                      (uint8_t*)&pressure);
    if (err_code != NRF_SUCCESS)
    {
      return err_code;
    }

    // Send value if connected and notifying
    if (p_ess->conn_handle != BLE_CONN_HANDLE_INVALID)
    {
      uint16_t		hvx_len;
      ble_gatts_hvx_params_t	hvx_params;

      hvx_len = len;

      memset(&hvx_params, 0, sizeof(hvx_params));

      hvx_params.handle = p_ess->pc_handles.value_handle;
      hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
      hvx_params.offset = 0;
      hvx_params.p_len  = &hvx_len;
      hvx_params.p_data = (uint8_t*)&pressure;

      err_code = sd_ble_gatts_hvx(p_ess->conn_handle, &hvx_params);
      if ((err_code == NRF_SUCCESS) && (hvx_len != len))
      {
        err_code = NRF_ERROR_DATA_SIZE;
      }
    }
    else
    {
      err_code = NRF_ERROR_INVALID_STATE;
    }
  }

  return err_code;
}
uint32_t ble_ess_temperature_send(ble_ess_t * p_ess, uint16_t temperature)
{
  uint32_t err_code = NRF_SUCCESS;

  if (temperature != p_ess->temperature_last)
  {
    uint16_t len = sizeof(uint16_t);

    // Save new value
    p_ess->temperature_last = temperature;

    // Update database
    err_code = sd_ble_gatts_value_set(p_ess->tc_handles.value_handle,
                                      0,
                                      &len,
                                      (uint8_t*)&temperature);
    if (err_code != NRF_SUCCESS)
    {
      return err_code;
    }

    // Send value if connected and notifying
    if (p_ess->conn_handle != BLE_CONN_HANDLE_INVALID)
    {
      uint16_t		hvx_len;
      ble_gatts_hvx_params_t	hvx_params;

      hvx_len = len;

      memset(&hvx_params, 0, sizeof(hvx_params));

      hvx_params.handle = p_ess->tc_handles.value_handle;
      hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
      hvx_params.offset = 0;
      hvx_params.p_len  = &hvx_len;
      hvx_params.p_data = (uint8_t*)&temperature;

      err_code = sd_ble_gatts_hvx(p_ess->conn_handle, &hvx_params);
      if ((err_code == NRF_SUCCESS) && (hvx_len != len))
      {
        err_code = NRF_ERROR_DATA_SIZE;
      }
    }
    else
    {
      err_code = NRF_ERROR_INVALID_STATE;
    }
  }

  return err_code;
}
