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

// clang-format off
#define OF_PROP_CMD_GENERATOR(GENERATOR)                                                                               \
GENERATOR(CMD_UNDEFINED    = 0, "undefined"   )                                                                        \
GENERATOR(CMD_REBOOT       = 1, "reboot"      )                                                                        \
GENERATOR(CMD_MOTOR_UNLOCK = 2, "motor_unlock")
// clang-format on

/**
 * \brief Command property commands.
 */
typedef enum { OF_PROP_CMD_GENERATOR(GENERATE_1ST_FIELD) CMD_MAX } command_property_cmd_t;

/* Extern list of property used by both the master and the nodes. */
extern cc_prop_t cc_prop_list[OF_CC_PROP_CNT];

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Get the property ID by it's name.
 *
 * \param[in] name The name of the property.
 *
 * \return The property ID or -1 if the property does not exist.
 */
cc_prop_id_t of_cc_prop_id_by_name(const char *name);

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Get the string representation of the property ID.
 *
 * \param[in] id The property ID.
 * \return The string representation of the property ID.
 */
const char *of_cc_prop_name_by_id(cc_prop_id_t id);

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Get the command id by it's name.
 *
 * \param[in] name The name of the command.
 *
 * \return The property ID or -1 if the property does not exist.
 */
command_property_cmd_t of_cmd_id_by_name(const char *name);

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Get the string representation of the command id.
 *
 * \param[in] id The command id.
 * \return The string representation of the command id.
 */
const char *of_cmd_name_by_id(command_property_cmd_t id);

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Get the command id by it's name.
 *
 * \param[in] name The name of the command.
 *
 * \return The property ID or -1 if the property does not exist.
 */
command_property_cmd_t of_cmd_id_by_name(const char *name);

//----------------------------------------------------------------------------------------------------------------------

/**
 * \brief Get the string representation of the command id.
 *
 * \param[in] id The command id.
 * \return The string representation of the command id.
 */
const char *of_cmd_name_by_id(command_property_cmd_t id);

//----------------------------------------------------------------------------------------------------------------------