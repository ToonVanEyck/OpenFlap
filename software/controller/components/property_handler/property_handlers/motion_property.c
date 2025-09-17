#include "esp_check.h"
#include "property_handler_common.h"
#include <string.h>

#define PROPERTY_TAG "MOTION_PROPERTY_HANDLER"

/**
 * \brief Populate the property from a json object.
 *
 * \param[in] json The json object to convert.
 * \param[out] module The module containing the property.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t motion_from_json(module_t *module, const cJSON *json)
{
    assert(module != NULL);
    assert(json != NULL);

    cJSON *speed_min_json  = cJSON_GetObjectItem(json, "speed_min");
    cJSON *speed_max_json  = cJSON_GetObjectItem(json, "speed_max");
    cJSON *ramp_start_json = cJSON_GetObjectItem(json, "ramp_start");
    cJSON *ramp_stop_json  = cJSON_GetObjectItem(json, "ramp_stop");

    ESP_RETURN_ON_FALSE(cJSON_IsNumber(speed_min_json), ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Expected a number for speed_min");
    ESP_RETURN_ON_FALSE(cJSON_IsNumber(speed_max_json), ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Expected a number for speed_max");
    ESP_RETURN_ON_FALSE(cJSON_IsNumber(ramp_start_json), ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Expected a number for ramp_start");
    ESP_RETURN_ON_FALSE(cJSON_IsNumber(ramp_stop_json), ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Expected a number for ramp_stop");

    ESP_RETURN_ON_FALSE(speed_min_json->valueint >= 0 && speed_min_json->valueint <= 255, ESP_ERR_INVALID_ARG,
                        PROPERTY_TAG, "Value speed_min out of range");
    ESP_RETURN_ON_FALSE(speed_max_json->valueint >= 0 && speed_max_json->valueint <= 255, ESP_ERR_INVALID_ARG,
                        PROPERTY_TAG, "Value speed_max out of range");
    ESP_RETURN_ON_FALSE(ramp_start_json->valueint >= 0 && ramp_start_json->valueint <= 255, ESP_ERR_INVALID_ARG,
                        PROPERTY_TAG, "Value ramp_start out of range");
    ESP_RETURN_ON_FALSE(ramp_stop_json->valueint >= 0 && ramp_stop_json->valueint <= 255, ESP_ERR_INVALID_ARG,
                        PROPERTY_TAG, "Value ramp_stop out of range");

    module->motion.speed_min           = speed_min_json->valueint;
    module->motion.speed_max           = speed_max_json->valueint;
    module->motion.distance_ramp_start = ramp_start_json->valueint;
    module->motion.distance_ramp_stop  = ramp_stop_json->valueint;

    return ESP_OK;
}

/**
 * \brief Convert the property into it's json representation.
 *
 * \param[out] json The json object in which we will store the property.
 * \param[in] module The module containing the property.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t motion_to_json(cJSON **json, const module_t *module)
{
    assert(json != NULL);
    assert(module != NULL);

    cJSON_AddNumberToObject(*json, "speed_min", module->motion.speed_min);
    cJSON_AddNumberToObject(*json, "speed_max", module->motion.speed_max);
    cJSON_AddNumberToObject(*json, "ramp_start", module->motion.distance_ramp_start);
    cJSON_AddNumberToObject(*json, "ramp_stop", module->motion.distance_ramp_stop);

    return ESP_OK;
}

/**
 * \brief Deserialize a byte array into a property.
 *
 * \param[out] module The module containing the property.
 * \param[in] bin The byte array to deserialize.
 * \param[in] bin_size The size of the byte array.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t motion_from_binary(module_t *module, const uint8_t *bin, uint16_t bin_size)
{
    assert(module != NULL);
    assert(bin != NULL);

    module->motion.speed_min           = bin[0];
    module->motion.speed_max           = bin[1];
    module->motion.distance_ramp_start = bin[2];
    module->motion.distance_ramp_stop  = bin[3];

    return ESP_OK;
}

/**
 * \brief Serialize the property into a byte array.
 *
 * \param[out] bin The serialized byte array.
 * \param[out] bin_size The size of the byte array.
 * \param[in] module The module containing the property.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t motion_to_binary(uint8_t **bin, uint16_t *bin_size, const module_t *module)
{
    assert(bin != NULL);
    assert(bin_size != NULL);
    assert(module != NULL);

    *bin_size = chain_comm_property_write_attributes_get(PROPERTY_MOTION)->static_property_size;

    *bin = malloc(*bin_size);
    ESP_RETURN_ON_FALSE(*bin != NULL, ESP_ERR_NO_MEM, PROPERTY_TAG, "Failed to allocate memory");

    (*bin)[0] = module->motion.speed_min;
    (*bin)[1] = module->motion.speed_max;
    (*bin)[2] = module->motion.distance_ramp_start;
    (*bin)[3] = module->motion.distance_ramp_stop;

    return ESP_OK;
}

/**
 * \brief Compare the properties of two modules.
 *
 * \param[in] module_a The first module to compare.
 * \param[in] module_b The second module to compare.
 *
 * \return true if the properties are the same, false otherwise.
 */
static bool motion_compare(const module_t *module_a, const module_t *module_b)
{
    assert(module_a != NULL);
    assert(module_b != NULL);

    /* Compare */
    return memcmp(&module_a->motion, &module_b->motion, sizeof(motion_property_t)) == 0;
}

/**
 * The motion property handler.
 *
 * The motion property is used to store the motion data for the modules. The motion data
 * is used to adjust the motion between the character set and the encoder postion.
 */
const property_handler_t PROPERTY_HANDLER_MOTION = {
    .id          = PROPERTY_MOTION,
    .from_json   = motion_from_json,
    .to_json     = motion_to_json,
    .from_binary = motion_from_binary,
    .to_binary   = motion_to_binary,
    .compare     = motion_compare,
};

#undef PROPERTY_TAG