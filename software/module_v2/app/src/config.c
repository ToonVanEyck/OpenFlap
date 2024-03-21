#include "config.h"
#include "flash.h"

void configLoad(openflap_config_t *config)
{
    config->ir_limits[0] = (ir_sens_threshold_t){.ir_low = 500, .ir_high = 1200};
    config->ir_limits[1] = (ir_sens_threshold_t){.ir_low = 500, .ir_high = 1200};
    config->ir_limits[2] = (ir_sens_threshold_t){.ir_low = 500, .ir_high = 1200};
    config->ir_limits[3] = (ir_sens_threshold_t){.ir_low = 500, .ir_high = 1200};
    config->ir_limits[4] = (ir_sens_threshold_t){.ir_low = 500, .ir_high = 1200};
    config->ir_limits[5] = (ir_sens_threshold_t){.ir_low = 500, .ir_high = 1200};
}

void configStore(openflap_config_t *config)
{
}