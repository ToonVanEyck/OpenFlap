#include "board_io.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_log.h"

static const char *TAG = "[GPIO]";

static QueueHandle_t gpio_evt_queue = NULL;
static TaskHandle_t mainTask;
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    ;
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void *arg)
{
    uint32_t io_num;
    uint16_t cnt = 0;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            vTaskDelay(20 / portTICK_PERIOD_MS);
            if (!gpio_get_level(io_num)) {
                ESP_LOGI(TAG, "cnt = %d", cnt++);
                xTaskNotify(mainTask, 1, eSetValueWithOverwrite);
            }
        }
    }
}

void board_io_init(TaskHandle_t task)
{
    mainTask = task;
    gpio_set_direction(FLAP_ENABLE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(FLAP_ENABLE_PIN, 0);

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_HS_MODE,           // timer mode
        .timer_num = LEDC_HS_TIMER,           // timer index
        .clk_cfg = LEDC_AUTO_CLK,             // Auto select the source clock
    };
    ledc_timer_config(&ledc_timer);
    ledc_channel_config_t ledc_channel[LEDC_RGB_CH_NUM] = {
        {
            .channel = LEDC_RED_CHANNEL,
            .duty = 0,
            .gpio_num = LEDC_RED_GPIO,
            .speed_mode = LEDC_HS_MODE,
            .hpoint = 0,
            .timer_sel = LEDC_HS_TIMER,
            .flags.output_invert = 1,
        },
        {
            .channel = LEDC_GREEN_CHANNEL,
            .duty = 0,
            .gpio_num = LEDC_GREEN_GPIO,
            .speed_mode = LEDC_HS_MODE,
            .hpoint = 0,
            .timer_sel = LEDC_HS_TIMER,
            .flags.output_invert = 1,
        },
        {
            .channel = LEDC_BLUE_CHANNEL,
            .duty = 0,
            .gpio_num = LEDC_BLUE_GPIO,
            .speed_mode = LEDC_HS_MODE,
            .hpoint = 0,
            .timer_sel = LEDC_HS_TIMER,
            .flags.output_invert = 1,
        },
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel[0]));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel[1]));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel[2]));
    setLed(0, 0, 0);

    // zero-initialize the config structure.
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .pin_bit_mask = (1ULL << MODE_BTN_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
    };
    gpio_config(&io_conf);

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(MODE_BTN_PIN, gpio_isr_handler, (void *)MODE_BTN_PIN));
}

void setLed(uint8_t red, uint8_t green, uint8_t blue)
{
    ledc_set_duty(LEDC_HS_MODE, LEDC_RED_CHANNEL, red >> 1);
    ledc_update_duty(LEDC_HS_MODE, LEDC_RED_CHANNEL);
    ledc_set_duty(LEDC_HS_MODE, LEDC_GREEN_CHANNEL, green >> 1);
    ledc_update_duty(LEDC_HS_MODE, LEDC_GREEN_CHANNEL);
    ledc_set_duty(LEDC_HS_MODE, LEDC_BLUE_CHANNEL, blue >> 1);
    ledc_update_duty(LEDC_HS_MODE, LEDC_BLUE_CHANNEL);
}