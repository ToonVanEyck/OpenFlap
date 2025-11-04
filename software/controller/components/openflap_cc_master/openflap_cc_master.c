#include "openflap_cc_master.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"

#include <string.h>

//======================================================================================================================
//                                                   MACROS a DEFINES
//======================================================================================================================

#define TAG "OF_CC_MASTER"

#define OF_CC_MASTER_TASK_SIZE 6000
#define OF_CC_MASTER_TASK_PRIO 5

/** Indicates that the model and actual modules are no longer in sync. */
#define OF_CC_MASTER_MODEL_EVENT_DESYNCHRONIZED (1u << 0)
/** Indicates that the model and actual modules are back in sync. */
#define OF_CC_MASTER_MODEL_EVENT_SYNCHRONIZED (1u << 1)

//======================================================================================================================
//                                                   FUNCTION PROTOTYPES
//======================================================================================================================

static void of_cc_master_task(void *arg);

//======================================================================================================================
//                                                   PUBLIC FUNCTIONS
//======================================================================================================================

esp_err_t of_cc_master_init(of_cc_master_ctx_t *ctx, void *model_userdata, of_cc_master_cb_cfg_t *of_master_cb_cfg,
                            const uint16_t *node_cnt_ref)
{
    ESP_RETURN_ON_FALSE(ctx != NULL, ESP_ERR_INVALID_ARG, TAG, "cc_master context is NULL");
    ESP_RETURN_ON_FALSE(model_userdata != NULL, ESP_ERR_INVALID_ARG, TAG, "Model user data is NULL");
    ESP_RETURN_ON_FALSE(of_master_cb_cfg != NULL, ESP_ERR_INVALID_ARG, TAG, "Master callback configuration is NULL");
    ESP_RETURN_ON_FALSE(of_master_cb_cfg->node_cnt_update != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "node_cnt_update callback is NULL");
    ESP_RETURN_ON_FALSE(of_master_cb_cfg->node_exists_and_must_be_written != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "node_exists_and_must_be_written callback is NULL");
    ESP_RETURN_ON_FALSE(of_master_cb_cfg->node_error_set != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "node_error_set callback is NULL");
    ESP_RETURN_ON_FALSE(node_cnt_ref != NULL, ESP_ERR_INVALID_ARG, TAG, "Node count reference is NULL");

    ctx->model_userdata      = model_userdata;
    ctx->model_sync_required = of_master_cb_cfg->model_sync_required;
    ctx->model_sync_done     = of_master_cb_cfg->model_sync_done;

    cc_master_cb_cfg_t master_cb_cfg = {
        .node_cnt_update                 = of_master_cb_cfg->node_cnt_update,
        .node_exists_and_must_be_written = of_master_cb_cfg->node_exists_and_must_be_written,
        .node_error_set                  = of_master_cb_cfg->node_error_set,
    };

    ctx->node_cnt_ref = node_cnt_ref;

    /* Configure UART for chain communication. */
    cc_master_uart_cb_cfg_t uart_cb = {0};
    ESP_RETURN_ON_ERROR(of_cc_master_uart_init(&ctx->uart_ctx, &uart_cb), TAG,
                        "Failed to initialize UART for chain communication");

    /* Initialize chain communication master context. */
    cc_master_init(&ctx->cc_master, &uart_cb, &ctx->uart_ctx, &master_cb_cfg, cc_prop_list, OF_CC_PROP_CNT,
                   model_userdata);

    /* Create event group for context. */
    ctx->event_handle = xEventGroupCreate();
    ESP_RETURN_ON_FALSE(ctx->event_handle != NULL, ESP_FAIL, TAG, "Failed to create event group for context");

    /* Start the task. */
    ESP_RETURN_ON_FALSE(xTaskCreate(of_cc_master_task, "of_cc_master_task", OF_CC_MASTER_TASK_SIZE, ctx,
                                    OF_CC_MASTER_TASK_PRIO, &ctx->task),
                        ESP_FAIL, TAG, "Failed to create chain comm task");

    return ESP_OK;
}

//----------------------------------------------------------------------------------------------------------------------

esp_err_t chain_comm_destroy(of_cc_master_ctx_t *ctx)
{
    ESP_RETURN_ON_FALSE(ctx != NULL, ESP_ERR_INVALID_ARG, TAG, "Chain-comm context is NULL");

    of_cc_master_uart_deinit(&ctx->uart_ctx);

    vEventGroupDelete(ctx->event_handle);
    ctx->event_handle = NULL;
    vTaskDelete(ctx->task);

    return ESP_OK;
}

//----------------------------------------------------------------------------------------------------------------------

esp_err_t of_cc_master_synchronize(of_cc_master_ctx_t *ctx, uint32_t timeout_ms)
{

    xEventGroupClearBits(ctx->event_handle, OF_CC_MASTER_MODEL_EVENT_SYNCHRONIZED);
    xEventGroupSetBits(ctx->event_handle, OF_CC_MASTER_MODEL_EVENT_DESYNCHRONIZED);
    EventBits_t bits = xEventGroupWaitBits(ctx->event_handle, OF_CC_MASTER_MODEL_EVENT_SYNCHRONIZED, pdTRUE, pdFALSE,
                                           pdMS_TO_TICKS(timeout_ms));

    if (bits & OF_CC_MASTER_MODEL_EVENT_SYNCHRONIZED || timeout_ms == 0) {
        return ESP_OK;
    } else {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

//======================================================================================================================
//                                                         PRIVATE FUNCTIONS
//======================================================================================================================

static void of_cc_master_task(void *arg)
{
    of_cc_master_ctx_t *ctx = (of_cc_master_ctx_t *)arg;

    bool controller_is_col_start_prev = !gpio_get_level(COL_START_PIN);
    bool controller_is_row_start_prev = !gpio_get_level(ROW_START_PIN);
    ESP_ERROR_CHECK(of_cc_master_uart_reconfigure(controller_is_col_start_prev, controller_is_row_start_prev));

    while (1) {
        /* Wait here until we receive a desynchronization event. */
        xEventGroupWaitBits(ctx->event_handle, OF_CC_MASTER_MODEL_EVENT_DESYNCHRONIZED, pdTRUE, pdFALSE, portMAX_DELAY);

        /* Read a byte, we are not expecting data so data would indicate an error. */
        uint8_t data = {0};
        if (uart_read_bytes(ctx->uart_ctx.uart_num, &data, 1, 0)) {
            ESP_LOGW(TAG, "Unexpected data received on chain-comm UART before synchronization.");
            vTaskDelay(pdMS_TO_TICKS(CC_NODE_TIMEOUT_MS * 1.1));
        }

        ESP_LOGI(TAG, "Chain Comm Master Synchronization Starting...");

        /* Reconfigure IOs if needed. */
        bool controller_is_col_start = !gpio_get_level(COL_START_PIN);
        bool controller_is_row_start = !gpio_get_level(ROW_START_PIN);
        if (controller_is_col_start != controller_is_col_start_prev ||
            controller_is_row_start != controller_is_row_start_prev) {
            ESP_LOGI(TAG, "Reconfiguring chain-comm IOs");
            ESP_ERROR_CHECK(of_cc_master_uart_reconfigure(controller_is_col_start, controller_is_row_start));
            controller_is_col_start_prev = controller_is_col_start;
            controller_is_row_start_prev = controller_is_row_start;
        }

        /* Synchronize the model. */
        for (cc_prop_id_t prop_id = 0; prop_id < OF_CC_PROP_CNT; prop_id++) {
            cc_action_t required_action = ctx->model_sync_required(ctx->model_userdata, prop_id);

            if (required_action == CC_ACTION_READ) {
                ESP_LOGI(TAG, "Model requires READ of property %s", of_cc_prop_name_by_id(prop_id));
                cc_master_queue_prop_read(&ctx->cc_master, prop_id);
            } else if (required_action == CC_ACTION_WRITE || required_action == CC_ACTION_BROADCAST) {
                bool broadcast = (required_action == CC_ACTION_BROADCAST);
                ESP_LOGI(TAG, "Model requires %s of property %s", broadcast ? "BROADCAST" : "WRITE",
                         of_cc_prop_name_by_id(prop_id));
                cc_master_queue_prop_write(&ctx->cc_master, prop_id, *ctx->node_cnt_ref, broadcast);
            } else {
                continue;
            }

            cc_master_err_t err = CC_MASTER_ERR_FAIL;
            uint8_t attempt_cnt = 0;
            uint32_t delay_ms   = 0;
            do {
                ESP_LOGI(TAG, "Attempt %d for property %s", attempt_cnt + 1, of_cc_prop_name_by_id(prop_id));
                err = cc_master_communication_handler(&ctx->cc_master, &delay_ms);
                vTaskDelay(pdMS_TO_TICKS(delay_ms));
            } while (err != CC_MASTER_OK && attempt_cnt++ < 3);

            if (err == CC_MASTER_OK) {
                ESP_LOGI(TAG, "Synchronized property %s successfully", of_cc_prop_name_by_id(prop_id));
                ctx->model_sync_done(ctx->model_userdata, prop_id);
            } else {
                ESP_LOGE(TAG, "Failed to synchronize property %s after %d attempts", of_cc_prop_name_by_id(prop_id),
                         attempt_cnt);
                break;
            }
        }

        ESP_LOGI(TAG, "Chain Comm Master Synchronization Completed!");
        /* Indicate synchronization is done. */
        xEventGroupSetBits(ctx->event_handle, OF_CC_MASTER_MODEL_EVENT_SYNCHRONIZED);
    }
}

//----------------------------------------------------------------------------------------------------------------------
