#include <esp_netif.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <esp_random.h>
#include <cstdio>
#include <atomic>
#include "driver/gpio.h"

// ThingsBoard includes
#include "thingsboard/Espressif_MQTT_Client.h"
#include "thingsboard/ThingsBoard.h"

// DomoHelios-system base includes
#include "adc_read/adc_read.h"
#include "server/as_client_server.h"
#include "server/as_server_server.h"
#include "storage/storage.h"
#include "wifi/wifi_def.h"
#include "servo_ctrl/servo_util.hpp"
#include "solar_sys/solar_sys.hpp"
#include "solar_sys/misc.hpp"
#include "json_read_DH_cfg/json_read_DH_cfg.h"


static const char *TAG = "SBC23G10DH_main";

// To be loaded directly from config.json ...
/* ----------------- THINGSBOARD CFG -----------------*/

constexpr char TOKEN[] =                "FiY8zgWzu1laHcYcnEkN";
// Luego con identificar dinamicamente el servidor con arp(red local) o link directo a ...
constexpr char THINGSBOARD_SERVER[] =   "192.168.240.200";
constexpr uint16_t THINGSBOARD_PORT =   1883U;
constexpr uint16_t MAX_MESSAGE_SIZE =   128U;
struct Espressif_MQTT_Client mqttClient;

ThingsBoard* tb;

/* ----------------- END THINGSBOARD CFG -----------------*/

static const char *SYS_CFG_FILE_PATH = "/data/config.json";
static const char *WORKING_PATH = "/data";

static char wifi_ok = 0;

static std::atomic<float> adc_batt(0);
static std::atomic<float> adc_panel(0);
static std::atomic_bool to_sleep(0);

static Virtual_Solar_sys<float> *solar;

static std::atomic<float> adc_ldr_a(0), adc_ldr_b(0), adc_ldr_c(0), adc_ldr_d(0);

static char shutdown_ = 0;
static float v;
// Servo control related variables

static std::atomic<float> servo_a_target(0);
static std::atomic<float> servo_b_target(0);
Servo_comm servo_a(0, 180, 500, 3500 - 500, 500, 20, 1000, 1, 15, LEDC_CHANNEL_0, &servo_a_target);
Servo_comm servo_b(1, 180, 500, 3500 - 500, 500, 20, 1000, 1, 15, LEDC_CHANNEL_1, &servo_b_target);
static std::vector<std::tuple<Servo_comm&, std::thread>> servo_bind;

// Terminate assert variable

static std::atomic_bool terminate(0);
static std::atomic_bool disable_external(0);

// Solar_system & publish thread pointers

std::thread *misc_adc_read_handler_thread;
std::thread *battery_control_handler_thread;
std::thread *solar_sys_handler_thread;
std::thread *publish_handler_thread;
std::thread *switch_child_callbacks_handler_thread;

static struct DH_config sys_conf;

// General use 2K buffer;

static char buffer[2048];

// DOMOHELIOS system base functions

void init_base();
void init_adc_read();
void init_gpio();
void init_battery_control();
void init_config();
void init_wifi();
void init_json_sys_config();
void init_client_server();
void thingboard_rpc_subscribe();
void init_thingsboard();
void deinit_thingsboard();
void init_servo();

void misc_adc_read(useconds_t);
void send_telemetry();


void publish_loop();
void switch_child_callbacks_loop();
void battery_control_loop();
void start_battery_control();
void start_misc_adc_read();
void start_solar_sys();
void start_publish_loop();
void start_switch_child_callbacks_loop();

bool battery_is_critical();

void low_battery_sys_drop();
void memory_and_threads_cleanup();
void enter_sleep();

// Children Callbacks

void dummy();
void toggle_gpio_A();
void toggle_gpio_B();
void do_something();


bool subscribed = false;

/*
   *  This approach abstract structure (independent threads)
   *
   *
   *  RPC "subscribed" handler thread
   *      |
   *  (on message)
   *      |
   *      |-> Callback (message response function/action)
   *              |
   *          (toggle_switch)
   *              |
   *              |-> DO "update inner [[switch]] values"
   * 
   *  Child callbacks loop handler thread
   *      |
   *      |-> last [[switch]] value differs from buffered one
   *              |
   *          if((load ^ target) & next) // bitwise-selection
   *              |
   *              |-> DO "invoke child Callbacks" (switch changes reponse subfunctions / actions)
   */

// Avoid memory overflow with multiple single byte sized values
static std::atomic<char> switch_buff(0x00000000); // up to 8 act
static std::atomic<char> switch_(0x00000000); // up to 8 act
constexpr const char RPC_SWITCH[] = "toggle_switch";
constexpr const char RPC_SWITCH_SELECT[] = "toggle_switch_select";

RPC_Response toggle_switch(const RPC_Data &data)
{
    if(!disable_external.load()) {
        int desp = data[RPC_SWITCH_SELECT];
        switch_.store(switch_.load() ^ (1 << (desp % 8)));
    }
    // Void JSON just for keeping the "toggle_switch RPC_RESPONSE" interface working
    return RPC_Response(StaticJsonDocument<JSON_OBJECT_SIZE(1)>());
}

static const std::array<void (*)(), 4UL> children_callback_functions = {
    &toggle_gpio_A, &toggle_gpio_B, &do_something, &dummy /*, ... (<, N-UL>)*/
    };

extern "C"
void app_main() {
    const useconds_t timeout = 3600000000L;
    init_base();
    init_adc_read();
    init_gpio();
    init_battery_control();
    start_battery_control();
    init_config();
    init_wifi();
    while (!terminate.load()) {        
        if (!wifi_ok) {
            wifi_init_softap();
            start_file_server(WORKING_PATH);
            usleep(timeout);
            continue;
        }
        
        init_json_sys_config();
        init_client_server();
        init_servo();
        init_thingsboard();
        
        start_misc_adc_read();
        start_solar_sys();
        start_publish_loop();
        start_switch_child_callbacks_loop();
        
        while (!battery_is_critical()) {}
        
        low_battery_sys_drop();
        memory_and_threads_cleanup();
        enter_sleep();
    }
}

void init_base()
{
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(mount_spiffs_storage(WORKING_PATH));

    set_wifi_ok_addr(&wifi_ok);
}
void init_adc_read()
{
    adc1_init_all(ADC_ATTEN_DB_0);
}
void init_gpio()
{
    gpio_reset_pin(GPIO_NUM_15);
    gpio_set_direction(GPIO_NUM_15, GPIO_MODE_OUTPUT);
    gpio_reset_pin(GPIO_NUM_2);
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
}
void init_battery_control()
{

}
void init_config()
{
    FILE *etc;
    struct stat st;
    if (stat(SYS_CFG_FILE_PATH, &st) == -1) {
        ESP_LOGI(TAG, "NO SYS CFG!");
    } else if (!S_ISDIR(st.st_mode)) {
        etc = fopen(SYS_CFG_FILE_PATH, "r");
        fread(buffer, 1, sizeof(buffer), etc);
        fclose(etc);
        cJSON *json =  cJSON_Parse(buffer);
        int err = check_and_dump_cfg(json, &sys_conf);
        print_DH_config(&sys_conf);
        cJSON_Delete(json);
        if(!err)
            printf("Loaded config file with no errors (%d)\n", err);
        else
            printf("Loaded config file with errors (%d)\n", err);
    }
    usleep(2000000);       
}

void init_wifi()
{
    int i=0, j;

    for(j = 0; j < 1 /*sys.conf.WIFI_count*/; j++) {
        wifi_init_sta(sys_conf.system_WIFI[i].ssid, sys_conf.system_WIFI[i].passphrase);
        if(wifi_ok)
            return;
    }
}

void init_json_sys_config()
{
    std::atomic<float> *target_ldr[] = {
        &adc_ldr_a,
        &adc_ldr_b,
        &adc_ldr_c,
        &adc_ldr_d
    };
    std::atomic<float> *target_servo[] = {
        &servo_b_target,
        &servo_a_target,
    };
    Panel<float> panel(
        &adc_panel,
        sys_conf.abstract_solar_sys_panel_dimensions.x,
        sys_conf.abstract_solar_sys_panel_dimensions.y
        );
    std::vector<LDR<float>> ldr;
    std::vector<Servo<float>> servo;
    int i;
    for(i = 0; i < sys_conf.LDR_count; i++) {
        auto loc = sys_conf.abstract_solar_sys_ldr_location[i];
        ldr.push_back(LDR<float>(target_ldr[i], Vec3<float>(loc.x, loc.y, loc.z)));
    }

    ESP_LOGI(TAG, "LDR count: %d", sys_conf.LDR_count);
    for(i = 0; i < sys_conf.Servo_count; i++) {
            servo.push_back(Servo<float>(
                target_servo[i],
                target_servo[i],
                0.0f,
                sys_conf.abstract_solar_sys_servo_max_rotation[i],
                false,
                sys_conf.abstract_solar_sys_servo_can_reverse_logic[i]
                ));
    }
    ESP_LOGI(TAG, "Servo count: %d", sys_conf.Servo_count);

    solar = new Virtual_Solar_sys(ldr, servo, panel, 4095.0f);
}

void init_client_server()
{
    set_value_addr(&v);
    init_client_web_server();
}

void thingboard_rpc_subscribe()
{
    const std::array<RPC_Callback, 1> callbacks = {
            RPC_Callback{ RPC_SWITCH, toggle_switch}
            // All custom callbacks
            };
    if (!tb->RPC_Subscribe(callbacks.cbegin(), callbacks.cend())) {
        ESP_LOGW(TAG, "Failed to subscribe RPC");
        return;
    }
}

void init_thingsboard()
{
    tb = new ThingsBoard(mqttClient, MAX_MESSAGE_SIZE);
    tb->connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT);
    thingboard_rpc_subscribe();
}
void deinit_thingsboard()
{
    // thingsboard_rpc_unsubscribe();
    tb->~ThingsBoard();
}
void init_servo()
{
    int bits = 15;
    int minValue = 500;
    uint32_t duty = (1<<bits) * minValue / 20000;
    // Same timer for all servos
    servo_comm_ledc_channel_prepare(duty, 15, 50, LEDC_CHANNEL_0, LEDC_TIMER_0, 16);
    servo_comm_ledc_channel_prepare(duty, 15, 50, LEDC_CHANNEL_1, LEDC_TIMER_0, 17);

    servo_bind = servo_thread_init({&servo_a, &servo_b});
    
    for(int i =0; i < 2; i++) {
    servo_a_target.store(0);
    servo_b_target.store(0);
    usleep(1200000);
    servo_a_target.store(180);
    servo_b_target.store(180);
    usleep(1200000);
    servo_a_target.store(0);
    servo_b_target.store(0);
    usleep(1200000);
    }
}

void battery_control_loop()
{
    // Await voltage readings to normalize
    usleep(100000);
    for(;;) {
        if(adc_batt.load() < 3.2f)
            disable_external.store(1);
        // At least 200 mVolts before peripherial re-activation
        else if(adc_batt.load() > 3.4f)
            disable_external.store(0);
        usleep(500000);
    }    
}

void misc_adc_read(useconds_t period)
{
    while(!to_sleep.load()) {
        adc_ldr_a.store(get_average_read(ADC1_CHANNEL_6, 4));
        adc_ldr_b.store(get_average_read(ADC1_CHANNEL_7, 4) * 1.25);
        adc_ldr_c.store(get_average_read(ADC1_CHANNEL_4, 4) * 1.25);
        
        /*
         * Battery Voltmeter
         * 
         * Voltage divider just with the resistors I had (82k, 15k)
         * Expecting max Voltage of 5 volts targeted at 4.30 v
         * 5 * 82 / 97 = 4.226804 v
         * Now the output Vout should come in parallel with 1k resistor
         * in case of previous resistors malfunction
         * 
         * After some scaling testing with 0db attenuation the resultant factor is 185
         */
        
        adc_batt.store(get_average_read(ADC1_CHANNEL_5, 10) / 185);
        printf("\tA: %.2f\n\n\n", adc_ldr_a.load());
        printf("B: %.2f\t", adc_ldr_b.load());
        printf("C: %.2f\n", adc_ldr_c.load());
        printf("Volt: %.2f\n", (float)adc_batt.load());
        printf("Volt (raw): %.2f\n", (float)adc_ldr_d.load() * 185);
        usleep(period);
    }
}

void send_telemetry()
{
    if (!tb->connected())
        tb->connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT);
    float ldr[] = {
        adc_ldr_a.load(),
        adc_ldr_b.load(),
        adc_ldr_c.load(),
    };
    tb->sendTelemetryData("LDR-AVG", (float)(ldr[0] + ldr[1] + ldr[2])/3);
    tb->sendTelemetryData("BATT", adc_batt.load());
    tb->sendTelemetryData("BATT-IS-LOW",  (float)(adc_batt.load() > 3.2f ? 1:0));
    tb->sendTelemetryData("ON-CHARGE", (float)(adc_batt.load() > 4.0f ? 1:0));
    tb->sendTelemetryData("SPANEL", adc_panel.load());
    tb->sendTelemetryData("LDR-TOP", ldr[0]);
    tb->sendTelemetryData("LDR-L", ldr[1]);
    tb->sendTelemetryData("LDR-R", ldr[2]);
    tb->sendTelemetryData("LDR-AVG", (float)(ldr[0] + ldr[1] + ldr[2])/3);
    tb->sendTelemetryData("Servo-X", servo_b_target.load());
    tb->sendTelemetryData("Servo-Y", servo_a_target.load());
    tb->sendTelemetryData("Inversion-Servo-X", (float)(servo_a_target.load() > 90));
    tb->sendTelemetryData("SW-0", (float)(switch_.load() & 1<<0));
    tb->sendTelemetryData("SW-1", (float)(switch_.load() & 1<<1));
    tb->sendTelemetryData("SW-2", (float)(switch_.load() & 1<<2));
    tb->sendTelemetryData("SW-3", (float)(switch_.load() & 1<<3));
    tb->sendTelemetryData("SW-4", (float)(switch_.load() & 1<<4));
    tb->sendTelemetryData("SW-5", (float)(switch_.load() & 1<<5));
    tb->sendTelemetryData("SW-6", (float)(switch_.load() & 1<<6));
    tb->sendTelemetryData("SW-7", (float)(switch_.load() & 1<<7));

    tb->loop();
}

void gpio_set_state()
{

}

void dummy()
{
    // Nothing
}

void toggle_gpio_A()
{
    gpio_set_level(GPIO_NUM_15, switch_buff.load() & 1);
    printf("toggle_gpio_A\n");
    // ...
}

void toggle_gpio_B()
{
    gpio_set_level(GPIO_NUM_2, switch_buff.load() & 2);
    printf("toggle_gpio_B\n");
    // ...
}

void do_something()
{
    printf("do_something\n");
    // ...
}

void publish_loop()
{
    while (!to_sleep.load()) {
        send_telemetry();
        usleep(250000);
    }
}

void switch_child_callbacks_loop()
{
    char load, target;
    uint8_t next;

    while(!to_sleep.load()) {
        // Turn off after low battery has been triggered
        if(disable_external.load())
            switch_.store(0x00000000);
        load = switch_buff.load();
        target = switch_.load();
        if(load != target) {
            switch_buff.store(target);
            next = 1;
            for(void(*func)() : children_callback_functions) {
                if((load ^ target) & next)
                    func();
                next <<= 1;
            }
        }

        usleep(1000000);
    }
}

void start_battery_control()
{
    battery_control_handler_thread = new std::thread(battery_control_loop);
}

void start_switch_child_callbacks_loop()
{
    switch_child_callbacks_handler_thread = new std::thread(switch_child_callbacks_loop);
}

void start_misc_adc_read()
{
    misc_adc_read_handler_thread = new std::thread(misc_adc_read, (useconds_t)250000);
}

void start_solar_sys()
{
    solar_sys_handler_thread = new std::thread(*solar);
}

void start_publish_loop()
{
    publish_handler_thread = new std::thread(publish_loop);
}


bool battery_is_critical()
{
    usleep(250000);
    return (adc_batt.load() - 3.2) < 1e-2;
}

void low_battery_sys_drop()
{
    //..
    deinit_thingsboard();
    to_sleep.store(1);
}


void memory_and_threads_cleanup()
{
    turn_off_servo_control(servo_bind);
    if (battery_control_handler_thread->joinable())
            battery_control_handler_thread->join();
    delete battery_control_handler_thread;
    if (switch_child_callbacks_handler_thread->joinable())
            switch_child_callbacks_handler_thread->join();
    delete switch_child_callbacks_handler_thread;
    if (misc_adc_read_handler_thread->joinable())
            misc_adc_read_handler_thread->join();
    delete misc_adc_read_handler_thread;

    if (solar_sys_handler_thread->joinable())
            solar_sys_handler_thread->join();
    delete solar_sys_handler_thread;
    
    if (publish_handler_thread->joinable())
            publish_handler_thread->join();
    delete publish_handler_thread;
    
    delete solar;
    delete tb;
}

void enter_sleep()
{

}