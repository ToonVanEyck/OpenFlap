#include "openflap_properties.h"

#include <string.h>

cc_prop_t cc_prop_list[OF_CC_PROP_CNT] = {
    [OF_CC_PROP_FIRMWARE_VERSION] = {.attribute = {.name = "firmware_version"}},
    [OF_CC_PROP_FIRMWARE_UPDATE]  = {.attribute = {.name = "firmware_update"}},
    [OF_CC_PROP_COMMAND]          = {.attribute = {.name = "command"}},
    [OF_CC_PROP_MODULE_INFO]      = {.attribute = {.name = "module_info"}},
    [OF_CC_PROP_CHARACTER_SET]    = {.attribute = {.name = "character_set"}},
    [OF_CC_PROP_CHARACTER]        = {.attribute = {.name = "character"}},
    [OF_CC_PROP_OFFSET]           = {.attribute = {.name = "offset"}},
    [OF_CC_PROP_COLOR]            = {.attribute = {.name = "color"}},
    [OF_CC_PROP_MOTION]           = {.attribute = {.name = "motion"}},
    [OF_CC_PROP_MINIMUM_ROTATION] = {.attribute = {.name = "minimum_rotation"}},
    [OF_CC_PROP_IR_THRESHOLD]     = {.attribute = {.name = "ir_threshold"}},
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

const char *of_cc_prop_name_by_id(cc_prop_id_t id)
{

    if (id < 0 || id >= OF_CC_PROP_CNT) {
        return "undefined";
    }
    return cc_prop_list[id].attribute.name;
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