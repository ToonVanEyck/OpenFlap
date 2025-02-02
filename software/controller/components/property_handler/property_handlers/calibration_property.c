#include "esp_check.h"
#include "property_handler_common.h"

#define PROPERTY_TAG "CALIBRATION_PROPERTY_HANDLER"

/**
 * \brief Populate the property from a json object.
 *
 * \param[in] json The json object to convert.
 * \param[out] module The module containing the property.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t calibration_from_json(module_t *module, const cJSON *json)
{
    assert(module != NULL);
    assert(json != NULL);

    ESP_RETURN_ON_FALSE(module->module_info.type == MODULE_TYPE_SPLITFLAP, ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Module type is not splitflap");
    calibration_property_t *calibration = &module->splitflap.calibration;

    cJSON *offset_json = cJSON_GetObjectItem(json, "offset");
    if (offset_json != NULL) {
        ESP_RETURN_ON_FALSE(cJSON_IsNumber(offset_json), ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                            "Expected a number for offset");
        ESP_RETURN_ON_FALSE(offset_json->valueint >= 0 && offset_json->valueint <= 255, ESP_ERR_INVALID_ARG,
                            PROPERTY_TAG, "Value out of range");

        calibration->offset = offset_json->valueint;
    }

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
static esp_err_t calibration_to_json(cJSON **json, const module_t *module)
{
    assert(json != NULL);
    assert(module != NULL);

    ESP_RETURN_ON_FALSE(module->module_info.type == MODULE_TYPE_SPLITFLAP, ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Module type is not splitflap");
    const calibration_property_t *calibration = &module->splitflap.calibration;

    cJSON_AddNumberToObject(*json, "offset", calibration->offset);

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
static esp_err_t calibration_from_binary(module_t *module, const uint8_t *bin, uint16_t bin_size)
{
    assert(module != NULL);
    assert(bin != NULL);

    ESP_RETURN_ON_FALSE(module->module_info.type == MODULE_TYPE_SPLITFLAP, ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Module type is not splitflap");
    calibration_property_t *calibration = &module->splitflap.calibration;

    calibration->offset = bin[0];

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
static esp_err_t calibration_to_binary(uint8_t **bin, uint16_t *bin_size, const module_t *module)
{
    assert(bin != NULL);
    assert(bin_size != NULL);
    assert(module != NULL);

    ESP_RETURN_ON_FALSE(module->module_info.type == MODULE_TYPE_SPLITFLAP, ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Module type is not splitflap");
    const calibration_property_t *calibration = &module->splitflap.calibration;

    *bin_size = chain_comm_property_write_attributes_get(PROPERTY_CALIBRATION)->static_property_size;

    *bin = malloc(sizeof(*bin_size));
    ESP_RETURN_ON_FALSE(*bin != NULL, ESP_ERR_NO_MEM, PROPERTY_TAG, "Failed to allocate memory");

    (*bin)[0] = calibration->offset;

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
static bool calibration_compare(const module_t *module_a, const module_t *module_b)
{
    assert(module_a != NULL);
    assert(module_b != NULL);

    ESP_RETURN_ON_FALSE(module_a->module_info.type == MODULE_TYPE_SPLITFLAP, false, PROPERTY_TAG,
                        "Module a type is not splitflap");
    ESP_RETURN_ON_FALSE(module_b->module_info.type == MODULE_TYPE_SPLITFLAP, false, PROPERTY_TAG,
                        "Module b type is not splitflap");

    const calibration_property_t *calibration_a = &module_a->splitflap.calibration;
    const calibration_property_t *calibration_b = &module_b->splitflap.calibration;

    /* Compare */
    return calibration_a->offset == calibration_b->offset;
}

/**
 * The calibration property handler.
 *
 * The calibration property is used to store the calibration data for the modules. The calibration data is used to
 * adjust the offset between the character set and the encoder postion.
 */
const property_handler_t PROPERTY_HANDLER_CALIBRATION = {
    .id          = PROPERTY_CALIBRATION,
    .from_json   = calibration_from_json,
    .to_json     = calibration_to_json,
    .from_binary = calibration_from_binary,
    .to_binary   = calibration_to_binary,
    .compare     = calibration_compare,
};

#undef PROPERTY_TAG