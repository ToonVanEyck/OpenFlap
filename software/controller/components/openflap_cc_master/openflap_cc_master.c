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

esp_err_t of_cc_master_init(of_cc_master_ctx_t *ctx, void *model_userdata, of_cc_master_cb_cfg_t *of_master_cb_cfg)
{
    ESP_RETURN_ON_FALSE(ctx != NULL, ESP_ERR_INVALID_ARG, TAG, "cc_master context is NULL");
    ESP_RETURN_ON_FALSE(model_userdata != NULL, ESP_ERR_INVALID_ARG, TAG, "Model user data is NULL");

    ctx->model_userdata      = model_userdata;
    ctx->model_sync_required = of_master_cb_cfg->model_sync_required;
    ctx->model_sync_done     = of_master_cb_cfg->model_sync_done;

    cc_master_cb_cfg_t master_cb_cfg = {
        .node_cnt_update                 = of_master_cb_cfg->node_cnt_update,
        .node_exists_and_must_be_written = of_master_cb_cfg->node_exists_and_must_be_written,
        .node_error_set                  = of_master_cb_cfg->node_error_set,
    };

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
            switch (required_action) {
                case CC_ACTION_READ:
                    ESP_LOGI(TAG, "Model requires READ of property %d", prop_id);
                    if (cc_master_prop_read(&ctx->cc_master, prop_id) == CC_MASTER_OK) {
                        ctx->model_sync_done(ctx->model_userdata, prop_id);
                    } else {
                        ESP_LOGW(TAG, "Failed to read property %d", prop_id);
                    }
                    break;
                case CC_ACTION_WRITE:
                    ESP_LOGI(TAG, "Model requires WRITE of property %d", prop_id);
                    if (cc_master_prop_write(&ctx->cc_master, prop_id, ctx->node_cnt, false, false) == CC_MASTER_OK) {
                        ctx->model_sync_done(ctx->model_userdata, prop_id);
                    } else {
                        ESP_LOGW(TAG, "Failed to write property %d", prop_id);
                    }
                    break;
                case CC_ACTION_BROADCAST:
                    ESP_LOGI(TAG, "Model requires BROADCAST of property %d", prop_id);
                    if (cc_master_prop_write(&ctx->cc_master, prop_id, 0, false, true) == CC_MASTER_OK) {
                        ctx->model_sync_done(ctx->model_userdata, prop_id);
                    } else {
                        ESP_LOGW(TAG, "Failed to broadcast property %d", prop_id);
                    }
                    break;
                default:
                    break; /* No synchronization required. */
            }
        }

        ESP_LOGI(TAG, "Chain Comm Master Synchronization Completed.");
        /* Indicate synchronization is done. */
        xEventGroupSetBits(ctx->event_handle, OF_CC_MASTER_MODEL_EVENT_SYNCHRONIZED);
    }
}

//----------------------------------------------------------------------------------------------------------------------
