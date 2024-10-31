#pragma once

#include "properties_common.h"

typedef enum {
    MODULE_TYPE_UNDEFINED = 0,
    MODULE_TYPE_SPLITFLAP = 1,
} module_type_t;

/**
 * \brief Module info.
 *
 * The 7 most significant bits are reserved for the module type. The least significant bit is used to indicate if the
 * module position is a column end.
 */
typedef uint8_t module_info_property_t;

#define MODULE_INFO_COLUMN_END_MASK  0x01
#define MODULE_INFO_TYPE_MASK        0xFE
#define MODULE_INFO_TYPE_SHIFT       1
#define MODULE_INFO_TYPE(info)       ((info & MODULE_INFO_TYPE_MASK) >> MODULE_INFO_TYPE_SHIFT)
#define MODULE_INFO_COLUMN_END(info) (info & MODULE_INFO_COLUMN_END_MASK)
#define MODULE_INFO(type, column_end)                                                                                  \
    (((type << MODULE_INFO_TYPE_SHIFT) & MODULE_INFO_TYPE_MASK) | (column_end & MODULE_INFO_COLUMN_END_MASK))

/**
 * \brief Convert the property into it's json representation.
 *
 * \param[out] json The json object in which we will store the property.
 * \param[in] property The property to convert.
 *
 * \return true if the conversion was successful, false otherwise.
 */
static inline bool module_info_to_json(cJSON **json, const void *property)
{
    ESP_LOGI("PROPERTY", "placeholder for: %s", __func__);
    return true;
}

/**
 * \brief Deserialize a byte array into a property.
 *
 * \param[out] property The property to populate.
 * \param[in] bin The byte array to deserialize.
 * \param[in] index The index of in case of a multipart property.
 *
 * \return true if the conversion was successful, false otherwise.
 */
static inline bool module_info_from_binary(void *property, const uint8_t *bin, uint8_t index)
{
    ESP_LOGI("PROPERTY", "placeholder for: %s", __func__);
    return true;
}

/**
 * The module_info property handler.
 *
 * The module_info property is used to send module_infos to the modules. Commands are executed in the modules and do not
 * store any data, as such the property is write-only.
 */
static const property_handler_t PROPERTY_HANDLER_MODULE_INFO = {
    .id                     = PROPERTY_MODULE_INFO,
    .name                   = "module_info",
    .to_json                = module_info_to_json,
    .from_binary            = module_info_from_binary,
    .from_binary_attributes = {.static_size = 1},
};