#pragma once

#include "esp_check.h"
#include "properties_common.h"

#define PROPERTY_TAG "CALIBRATION_PROPERTY_HANDLER"

/** Calibration properties */
typedef struct {
    uint8_t offset; /**< Offset between the actual character and the index on the encoder wheel. */
} calibration_property_t;

/**
 * \brief Populate the property from a json object.
 *
 * \param[in] json The json object to convert.
 * \param[out] property The property to populate.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static inline esp_err_t calibration_from_json(void *property, const cJSON *json)
{
    assert(property != NULL);
    assert(json != NULL);

    calibration_property_t *calibration = (calibration_property_t *)property;

    ESP_RETURN_ON_FALSE(json->type != cJSON_Number, ESP_ERR_INVALID_ARG, PROPERTY_TAG, "Expected a number");
    ESP_RETURN_ON_FALSE(json->valueint >= 0 && json->valueint <= 255, ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Value out of range");

    calibration->offset = json->valueint;

    return ESP_OK;
}

/**
 * \brief Convert the property into it's json representation.
 *
 * \param[out] json The json object in which we will store the property.
 * \param[in] property The property to convert.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static inline esp_err_t calibration_to_json(cJSON **json, const void *property)
{
    assert(json != NULL);
    assert(property != NULL);

    const calibration_property_t *calibration = (const calibration_property_t *)property;

    cJSON_AddNumberToObject(*json, "offset", calibration->offset);

    return ESP_OK;
}

/**
 * \brief Deserialize a byte array into a property.
 *
 * \param[out] property The property to populate.
 * \param[in] bin The byte array to deserialize.
 * \param[in] index The index of in case of a multipart property.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static inline esp_err_t calibration_from_binary(void *property, const uint8_t *bin, uint8_t index)
{
    assert(property != NULL);
    assert(bin != NULL);

    calibration_property_t *calibration = (calibration_property_t *)property;

    calibration->offset = bin[0];

    return ESP_OK;
}

/**
 * \brief Serialize the property into a byte array.
 *
 * \param[out] bin The serialized byte array.
 * \param[in] property The property to serialize.
 * \param[in] index The index of in case of a multipart property.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static inline esp_err_t calibration_to_binary(uint8_t *bin, const void *property, uint8_t index)
{
    assert(bin != NULL);
    assert(property != NULL);

    const calibration_property_t *calibration = (const calibration_property_t *)property;

    bin[0] = calibration->offset;

    return ESP_OK;
}

/**
 * The calibration property handler.
 *
 * The calibration property is used to store the calibration data for the modules. The calibration data is used to
 * adjust the offset between the character set and the encoder postion.
 */
static const property_handler_t PROPERTY_HANDLER_CALIBRATION = {
    .id          = PROPERTY_CALIBRATION,
    .from_json   = calibration_from_json,
    .to_json     = calibration_to_json,
    .from_binary = calibration_from_binary,
    .to_binary   = calibration_to_binary,
};

#undef PROPERTY_TAG