#include "openflap_properties.h"

#include <string.h>

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

static const char *of_cmd_prop_cmd_names[CMD_MAX] = {OF_PROP_CMD_GENERATOR(GENERATE_2ND_FIELD)};

//----------------------------------------------------------------------------------------------------------------------

cc_prop_id_t of_cc_prop_id_by_name(const char *name)
{
    for (cc_prop_id_t i = 0; i < OF_CC_PROP_CNT; i++) {
        if (strcmp(cc_prop_list[i].attribute.name, name) == 0) {
            return i;
        }
    }
    return OF_CC_PROP_UNDEFINED;
}

//----------------------------------------------------------------------------------------------------------------------

command_property_cmd_t of_cmd_id_by_name(const char *name)
{
    for (command_property_cmd_t cmd = CMD_UNDEFINED + 1; cmd < CMD_MAX; cmd++) {
        if (strcmp(name, of_cmd_prop_cmd_names[cmd]) == 0) {
            return cmd;
        }
    }
    return CMD_UNDEFINED;
}

//----------------------------------------------------------------------------------------------------------------------

const char *of_cmd_name_by_id(command_property_cmd_t id)
{

    id = (id < CMD_UNDEFINED) ? CMD_UNDEFINED : id;
    id = (id >= CMD_MAX) ? CMD_UNDEFINED : id;
    return of_cmd_prop_cmd_names[id];
}

//----------------------------------------------------------------------------------------------------------------------