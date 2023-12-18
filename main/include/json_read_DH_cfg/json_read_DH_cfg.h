#ifndef __JSON_READ_DH_CFG__
#define __JSON_READ_DH_CFG__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

#ifdef __cplusplus
extern "C"
{
#endif


#define JSON_BAD_CFG_FILE_NAME -1
#define JSON_BAD_CFG_BACKUP_FILE_NAME -2
#define JSON_BAD_DEEP_SLEEP_BOOLEAN -3
#define JSON_BAD_CRITICAL_BATTERY_VALUE -4
#define JSON_BAD_WIFI_AP_SSID -5
#define JSON_BAD_WIFI_AP_PASSPHRASE -6
#define JSON_BAD_WIFI_AP_AUTH -7
#define JSON_BAD_WIFI_LIST -8
#define JSON_BAD_WIFI_LIST_MEMBER_SSID -9
#define JSON_BAD_WIFI_LIST_MEMBER_PASSPHRASE -10
#define JSON_BAD_WIFI_LIST_MEMBER_AUTH -11
#define JSON_BAD_THINGSBOARD_TOKEN -12
#define JSON_BAD_THINGSBOARD_MAIN_SERVER -13
#define JSON_BAD_SOLAR_PANEL_DIMENSION_LIST -14
#define JSON_BAD_SOLAR_PANEL_DIMENSION_LIST_MEMBER_X -15
#define JSON_BAD_SOLAR_PANEL_DIMENSION_LIST_MEMBER_Y -16
#define JSON_BAD_SOLAR_PANEL_DIMENSION_LIST_MEMBER_Z -17
#define JSON_BAD_LDR_LOCATION_LIST -18
#define JSON_BAD_LDR_LOCATION_LIST_NO_LOCATION_TAG -19
#define JSON_BAD_LDR_LOCATION_LIST_MEMBER_X -20
#define JSON_BAD_LDR_LOCATION_LIST_MEMBER_Y -21
#define JSON_BAD_LDR_LOCATION_LIST_MEMBER_Z -22
#define JSON_BAD_SERVO_LIST -23
#define JSON_BAD_SERVO_LIST_MEMBER_MAX_ROTATION -24
#define JSON_BAD_SERVO_LIST_MEMBER_CAN_REVERSE_LOGIC -25

struct WIFI {
  char ssid[65];
  char passphrase[65];
  char auth[15];
};
struct Vector3 {
  float x, y, z;
};

struct DH_config {
  char system_base_config_file[128];
  char system_base_config_file_backup[128];
  char system_base_allow_deep_sleep;
  char system_base_critical_battery;
  struct WIFI system_WIFI_AP;
  struct WIFI system_WIFI[5];
  unsigned WIFI_count;
  char system_thingsboard_token[65];
  char system_thingsboard_main_server[65];
  struct Vector3 abstract_solar_sys_panel_dimensions;
  struct Vector3 abstract_solar_sys_ldr_location[5];
  unsigned LDR_count;
  float abstract_solar_sys_servo_max_rotation[4];
  char abstract_solar_sys_servo_can_reverse_logic[4];
  unsigned Servo_count;
};

void print_DH_config(struct DH_config*);
int check_and_dump_cfg(cJSON *, struct DH_config *);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* !__JSON_READ_DH_CFG__ */