#pragma once

#include "chain_comm_shared.h"

#define OF_CC_PROP_UNDEFINED (cc_prop_id_t)(-1) /**< Undefined property ID. */

#define OF_CC_PROP_FIRMWARE_VERSION (cc_prop_id_t)(0)
#define OF_CC_PROP_FIRMWARE_UPDATE  (cc_prop_id_t)(1)
#define OF_CC_PROP_COMMAND          (cc_prop_id_t)(2)
#define OF_CC_PROP_MODULE_INFO      (cc_prop_id_t)(3)
#define OF_CC_PROP_CHARACTER_SET    (cc_prop_id_t)(4)
#define OF_CC_PROP_CHARACTER        (cc_prop_id_t)(5)
#define OF_CC_PROP_OFFSET           (cc_prop_id_t)(6)
#define OF_CC_PROP_COLOR            (cc_prop_id_t)(7)
#define OF_CC_PROP_MOTION           (cc_prop_id_t)(8)
#define OF_CC_PROP_MINIMUM_ROTATION (cc_prop_id_t)(9)
#define OF_CC_PROP_IR_THRESHOLD     (cc_prop_id_t)(10)

#define OF_CC_PROP_CNT (11) /** Total number of properties. */

#define OF_FIRMWARE_UPDATE_PAGE_SIZE 128 /**< Size of a firmware update page in bytes. */

/**
 * \brief Command property commands.
 */
typedef enum {
    CMD_REBOOT       = 0x01, /** Reboot the module. */
    CMD_MOTOR_UNLOCK = 0x02, /** Unlock the motor. */
} command_property_cmd_t;

/**
 * \brief Module types.
 */
typedef enum {
    MODULE_TYPE_UNKNOWN   = 0x00, /**< Unknown module type. */
    MODULE_TYPE_SPLITFLAP = 0x01, /**< Split-flap module type. */
} module_type_t;

/**
 * \brief Module info.
 */
typedef struct __attribute__((__packed__)) {
    union {
        struct {
            bool column_end : 1;    /**< Indicates that this module is the last one in a column. */
            module_type_t type : 4; /**< The type of the module. */
            uint8_t reserved : 3;   /**< Reserved for future use. */
        };
        uint8_t raw; /**< The raw value of the module info. */
    };
} module_info_property_t;

/* Extern list of property used by both the master and the nodes. */
extern cc_prop_t cc_prop_list[OF_CC_PROP_CNT];

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Get the property ID by its name.
 *
 * \param[in] name The name of the property.
 *
 * \return The property ID or -1 if the property does not exist.
 */
cc_prop_id_t cc_prop_id_by_name(const char *name);