#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "json_read_DH_cfg.h"

void print_DH_config(struct DH_config *conf)
{
  int i;
  printf("system_base_config_file: %s\n", conf->system_base_config_file);
  printf("system_base_config_file_backup: %s\n", conf->system_base_config_file_backup);
  printf("system_base_allow_deep_sleep: %d\n", conf->system_base_allow_deep_sleep);
  printf("system_base_critical_battery: %d\n", conf->system_base_critical_battery);
  printf("system_WIFI_AP: %s, %s, %s\n", conf->system_WIFI_AP.ssid, conf->system_WIFI_AP.passphrase, conf->system_WIFI_AP.auth);
  for(i = 0; i < conf->WIFI_count; i++)
    printf("system_WIFI: %s, %s, %s\n", conf->system_WIFI[i].ssid, conf->system_WIFI[i].passphrase, conf->system_WIFI[i].auth);
  printf("system_thingsboard_token: %s\n", conf->system_thingsboard_token);
  printf("system_thingsboard_main_server: %s\n", conf->system_thingsboard_main_server);
  printf("abstract_solar_sys_panel_dimensions: %.2f, %.2f, %.2f\n", conf->abstract_solar_sys_panel_dimensions.x,
      conf->abstract_solar_sys_panel_dimensions.y,
      conf->abstract_solar_sys_panel_dimensions.z);
  for(i = 0; i < conf->LDR_count; i++)
    printf("abstract_solar_sys_ldr_location: %.2f, %.2f, %.2f\n", conf->abstract_solar_sys_ldr_location[i].x,
      conf->abstract_solar_sys_ldr_location[i].y,
      conf->abstract_solar_sys_ldr_location[i].z);
  for(i = 0; i < conf->Servo_count; i++) {
    printf("abstract_solar_sys_servo_max_rotation: %.2f\n", conf->abstract_solar_sys_servo_max_rotation[i]);
    printf("abstract_solar_sys_servo_can_reverse_logic: %d\n", conf->abstract_solar_sys_servo_can_reverse_logic[i]);
  }
}

int check_and_dump_cfg(cJSON *json, struct DH_config *save)
{
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
            printf("Malformed json!!!");
        return 1;
    }

    cJSON *member;
    cJSON *System = cJSON_GetObjectItemCaseSensitive(json, "System");
    cJSON *System_Base = cJSON_GetObjectItemCaseSensitive(System, "Base");
    cJSON *System_WIFI_AP = cJSON_GetObjectItemCaseSensitive(System, "WIFI_AP");
    cJSON *System_WIFI = cJSON_GetObjectItemCaseSensitive(System, "WIFI");
    cJSON *System_ThingsBoard = cJSON_GetObjectItemCaseSensitive(System, "ThingsBoard");
    cJSON *Abstract = cJSON_GetObjectItemCaseSensitive(json, "Abstract");
    cJSON *Abstract_Solar_sys = cJSON_GetObjectItemCaseSensitive(Abstract, "Solar_sys");
    cJSON *Abstract_Solar_sys_Panel = cJSON_GetObjectItemCaseSensitive(Abstract_Solar_sys, "Panel");
    cJSON *Abstract_Solar_sys_LDR = cJSON_GetObjectItemCaseSensitive(Abstract_Solar_sys, "LDR");
    cJSON *Abstract_Solar_sys_Servo = cJSON_GetObjectItemCaseSensitive(Abstract_Solar_sys, "Servo");
    int i;

    if (cJSON_IsObject(System_Base)) {
        member = cJSON_GetObjectItemCaseSensitive(System_Base, "config_file");
        if (cJSON_IsString(member)) {
            strcpy(save->system_base_config_file, member->valuestring);
        } else return JSON_BAD_CFG_FILE_NAME;
        member = cJSON_GetObjectItemCaseSensitive(System_Base, "config_file_backup");
        if (cJSON_IsString(member)) {
            strcpy(save->system_base_config_file_backup, member->valuestring);
        } else return JSON_BAD_CFG_BACKUP_FILE_NAME;
        member = cJSON_GetObjectItemCaseSensitive(System_Base, "allow_deep_sleep");
        if (cJSON_IsTrue(member) || cJSON_IsFalse(member)) {
            save->system_base_allow_deep_sleep = member->valueint;
        } else return JSON_BAD_DEEP_SLEEP_BOOLEAN;
        member = cJSON_GetObjectItemCaseSensitive(System_Base, "critical_battery");
        if (cJSON_IsNumber(member)) {
            save->system_base_critical_battery = member->valueint;
        } else return JSON_BAD_CRITICAL_BATTERY_VALUE;
        member = cJSON_GetObjectItemCaseSensitive(System_WIFI_AP, "ssid");
        if (cJSON_IsString(member)) {
            strcpy(save->system_WIFI_AP.ssid, member->valuestring);
        } else return JSON_BAD_WIFI_AP_SSID;
        member = cJSON_GetObjectItemCaseSensitive(System_WIFI_AP, "passphrase");
        if (cJSON_IsString(member)) {
            strcpy(save->system_WIFI_AP.passphrase, member->valuestring);
        } else return JSON_BAD_WIFI_AP_PASSPHRASE;
        member = cJSON_GetObjectItemCaseSensitive(System_WIFI_AP, "auth");
        if (cJSON_IsString(member)) {
            strcpy(save->system_WIFI_AP.auth, member->valuestring);
        } else return JSON_BAD_WIFI_AP_AUTH;

        if (cJSON_IsArray(System_WIFI)) {
            cJSON *wifi;
            i = 0;
            cJSON_ArrayForEach(wifi, System_WIFI) {
                if(i == 5)
                    break;
                member = cJSON_GetObjectItemCaseSensitive(wifi, "ssid");
                if (cJSON_IsString(member)) {
                    strcpy(save->system_WIFI[i].ssid, member->valuestring);
                } else return JSON_BAD_WIFI_LIST_MEMBER_SSID;
                member = cJSON_GetObjectItemCaseSensitive(wifi, "passphrase");
                if (cJSON_IsString(member)) {
                    strcpy(save->system_WIFI[i].passphrase, member->valuestring);
                } else return JSON_BAD_WIFI_LIST_MEMBER_PASSPHRASE;
                member = cJSON_GetObjectItemCaseSensitive(wifi, "auth");
                if (cJSON_IsString(member)) {
                    strcpy(save->system_WIFI[i].auth, member->valuestring);
                } else return JSON_BAD_WIFI_LIST_MEMBER_AUTH;
                i++;
            }
        } else return JSON_BAD_WIFI_LIST;
        save->WIFI_count = i;
        member = cJSON_GetObjectItemCaseSensitive(System_ThingsBoard, "token");
        if (cJSON_IsString(member)) {
            strcpy(save->system_thingsboard_token, member->valuestring);
        } else return JSON_BAD_THINGSBOARD_TOKEN;
        member = cJSON_GetObjectItemCaseSensitive(System_ThingsBoard, "main_server");
        if (cJSON_IsString(member)) {
            strcpy(save->system_thingsboard_main_server, member->valuestring);
        } else return JSON_BAD_THINGSBOARD_MAIN_SERVER;
        member = cJSON_GetObjectItemCaseSensitive(Abstract_Solar_sys_Panel, "dimensions");
        if (cJSON_IsObject(member)) {
            cJSON *val;
            val = cJSON_GetObjectItemCaseSensitive(member, "x");
            if (cJSON_IsNumber(val)) {
                save->abstract_solar_sys_panel_dimensions.x = val->valuedouble;
            } else return JSON_BAD_SOLAR_PANEL_DIMENSION_LIST_MEMBER_X;
            val = cJSON_GetObjectItemCaseSensitive(member, "y");
            if (cJSON_IsNumber(val)) {
                save->abstract_solar_sys_panel_dimensions.y = val->valuedouble;
            } else return JSON_BAD_SOLAR_PANEL_DIMENSION_LIST_MEMBER_Y;
            val = cJSON_GetObjectItemCaseSensitive(member, "z");
            if (cJSON_IsNumber(val)) {
                save->abstract_solar_sys_panel_dimensions.z = val->valuedouble;
            } else return JSON_BAD_SOLAR_PANEL_DIMENSION_LIST_MEMBER_Z;
        } else return JSON_BAD_SOLAR_PANEL_DIMENSION_LIST;
        if (cJSON_IsArray(Abstract_Solar_sys_LDR)) {
            cJSON *ldr;
            i = 0;
            cJSON_ArrayForEach(ldr, Abstract_Solar_sys_LDR) {
                if(i == 5)
                    break;
                member = cJSON_GetObjectItemCaseSensitive(ldr, "location");
                if (!cJSON_IsObject(member))
                  return JSON_BAD_LDR_LOCATION_LIST_NO_LOCATION_TAG;
                cJSON *val;
                val = cJSON_GetObjectItemCaseSensitive(member, "x");
                if (cJSON_IsNumber(val)) {
                  save->abstract_solar_sys_ldr_location[i].x = val->valuedouble;
                } else return JSON_BAD_LDR_LOCATION_LIST_MEMBER_X;
                val = cJSON_GetObjectItemCaseSensitive(member, "y");
                if (cJSON_IsNumber(val)) {
                  save->abstract_solar_sys_ldr_location[i].y = val->valuedouble;
                } else return JSON_BAD_LDR_LOCATION_LIST_MEMBER_Y;
                val = cJSON_GetObjectItemCaseSensitive(member, "z");
                if (cJSON_IsNumber(val)) {
                  save->abstract_solar_sys_ldr_location[i].z = val->valuedouble;
                } else return JSON_BAD_LDR_LOCATION_LIST_MEMBER_Z;
                i++;
            }
        } else return JSON_BAD_LDR_LOCATION_LIST;
        save->LDR_count = i;
        if (cJSON_IsArray(Abstract_Solar_sys_Servo)) {
          cJSON *servo;
          i = 0;
          cJSON_ArrayForEach(servo, Abstract_Solar_sys_Servo) {
            if(i == 4)
                break;
            member = cJSON_GetObjectItemCaseSensitive(servo, "max_rotation");
            if (cJSON_IsNumber(member)) {
              save->abstract_solar_sys_servo_max_rotation[i] = member->valuedouble;
            } else return JSON_BAD_SERVO_LIST_MEMBER_MAX_ROTATION;
            member = cJSON_GetObjectItemCaseSensitive(servo, "can_reverse_logic");
            if (cJSON_IsTrue(member) || cJSON_IsFalse(member)) {
              save->abstract_solar_sys_servo_can_reverse_logic[i] = member->valueint;
            } else return JSON_BAD_SERVO_LIST_MEMBER_CAN_REVERSE_LOGIC;
            i++;
          }
        } else return JSON_BAD_SERVO_LIST;
        save->Servo_count = i;
    }

    return 0;
}
/*
int main() {
    FILE *fp = fopen("config.json", "r");
    if (fp == NULL) {
        printf("Error: Unable to open the file.\n");
        return 1;
    }
    struct DH_config cfg;
    char buffer[2048];
    fread(buffer, 1, sizeof(buffer), fp);
    fclose(fp);

    cJSON *json =  cJSON_Parse(buffer);

    int err = check_and_dump_cfg(json, &cfg);
    print_DH_config(cfg);

    cJSON_Delete(json);

    return err;
}*/
