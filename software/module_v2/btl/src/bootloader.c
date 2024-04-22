
#include "debug_io.h"
#include "memory_map.h"
#include "py32f0xx_hal.h"
#include <stdint.h>

typedef struct vector_table_tag {
    uint32_t initial_stack_pointer;
    void (*reset_handler)(void);
} vector_table_t;

void jump_to_app(void)
{
    vector_table_t *app_vector_table = (vector_table_t *)&__FLASH_APP_START__;
    __disable_irq();
    __set_CONTROL(0);
    __set_MSP(app_vector_table->initial_stack_pointer);
    SCB->VTOR = app_vector_table->initial_stack_pointer;
    app_vector_table->reset_handler();
}

int main(void)
{
    debug_io_init();
    debug_io_log_info("OpenFlap Bootloader!\n");
    jump_to_app();
    while (1) {
    }
}
