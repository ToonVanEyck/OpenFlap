#include "openflap_properties.h"

cc_prop_t cc_prop_list[OF_CC_PROP_CNT] = {
    [OF_CC_PROP_FIRMWARE_VERSION] = {.attribute = {.name = "FIRMWARE_VERSION"}},
    [OF_CC_PROP_FIRMWARE_UPDATE]  = {.attribute = {.name = "FIRMWARE_UPDATE"}},
    [OF_CC_PROP_COMMAND]          = {.attribute = {.name = "COMMAND"}},
    [OF_CC_PROP_MODULE_INFO]      = {.attribute = {.name = "MODULE_INFO"}},
    [OF_CC_PROP_CHARACTER_SET]    = {.attribute = {.name = "CHARACTER_SET"}},
    [OF_CC_PROP_CHARACTER]        = {.attribute = {.name = "CHARACTER"}},
    [OF_CC_PROP_OFFSET]           = {.attribute = {.name = "OFFSET"}},
    [OF_CC_PROP_COLOR]            = {.attribute = {.name = "COLOR"}},
    [OF_CC_PROP_MOTION]           = {.attribute = {.name = "MOTION"}},
    [OF_CC_PROP_MINIMUM_ROTATION] = {.attribute = {.name = "MINIMUM_ROTATION"}},
    [OF_CC_PROP_IR_THRESHOLD]     = {.attribute = {.name = "IR_THRESHOLD"}},
};