#include "esp_check.h"
#include "property_handler_common.h"
#include <string.h>

#define PROPERTY_TAG "COLOR_PROPERTY_HANDLER"

static esp_err_t json_to_color(color_t *color, const cJSON *json);

/**
 * \brief Populate the property from a json object.
 *
 * \param[in] json The json object to convert.
 * \param[out] module The module containing the property.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t color_from_json(module_t *module, const cJSON *json)
{
    assert(module != NULL);
    assert(json != NULL);

    cJSON *foreground_json = cJSON_GetObjectItem(json, "foreground");
    cJSON *background_json = cJSON_GetObjectItem(json, "background");

    color_t foreground = {0};
    color_t background = {0};

    ESP_RETURN_ON_ERROR(json_to_color(&foreground, foreground_json), PROPERTY_TAG,
                        "Failed to convert foreground color");
    ESP_RETURN_ON_ERROR(json_to_color(&background, background_json), PROPERTY_TAG,
                        "Failed to convert background color");

    module->color.foreground = foreground;
    module->color.background = background;

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
static esp_err_t color_to_json(cJSON **json, const module_t *module)
{
    assert(json != NULL);
    assert(module != NULL);

    char fg_color_str[8] = {0};
    char bg_color_str[8] = {0};

    snprintf(fg_color_str, sizeof(fg_color_str), "#%06X",
             module->color.foreground.red << 16 | module->color.foreground.green << 8 | module->color.foreground.blue);
    snprintf(bg_color_str, sizeof(bg_color_str), "#%06X",
             module->color.background.red << 16 | module->color.background.green << 8 | module->color.background.blue);

    cJSON_AddStringToObject(*json, "foreground", fg_color_str);
    cJSON_AddStringToObject(*json, "background", bg_color_str);

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
static esp_err_t color_from_binary(module_t *module, const uint8_t *bin, uint16_t bin_size)
{
    assert(module != NULL);
    assert(bin != NULL);

    module->color.foreground.red   = bin[0];
    module->color.foreground.green = bin[1];
    module->color.foreground.blue  = bin[2];
    module->color.background.red   = bin[3];
    module->color.background.green = bin[4];
    module->color.background.blue  = bin[5];

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
static esp_err_t color_to_binary(uint8_t **bin, uint16_t *bin_size, const module_t *module)
{
    assert(bin != NULL);
    assert(bin_size != NULL);
    assert(module != NULL);

    *bin_size = chain_comm_property_write_attributes_get(PROPERTY_COLOR)->static_property_size;

    *bin = malloc(sizeof(*bin_size));
    ESP_RETURN_ON_FALSE(*bin != NULL, ESP_ERR_NO_MEM, PROPERTY_TAG, "Failed to allocate memory");

    (*bin)[0] = module->color.foreground.red;
    (*bin)[1] = module->color.foreground.green;
    (*bin)[2] = module->color.foreground.blue;
    (*bin)[3] = module->color.background.red;
    (*bin)[4] = module->color.background.green;
    (*bin)[5] = module->color.background.blue;

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
static bool color_compare(const module_t *module_a, const module_t *module_b)
{
    assert(module_a != NULL);
    assert(module_b != NULL);

    /* Compare */
    return memcmp(&module_a->color, &module_b->color, sizeof(color_property_t)) == 0;
}

/**
 * The color property handler.
 *
 * The color property is used to store the color data for the modules. The color data
 * is used to adjust the color between the character set and the encoder postion.
 */
const property_handler_t PROPERTY_HANDLER_COLOR = {
    .id          = PROPERTY_COLOR,
    .from_json   = color_from_json,
    .to_json     = color_to_json,
    .from_binary = color_from_binary,
    .to_binary   = color_to_binary,
    .compare     = color_compare,
};

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Convert a json object into a color.
 *
 * \param[out] color The color to convert to.
 * \param[in] json The json object to convert.
 *
 * \return ESP_OK if the conversion was successful, ESP_FAIL otherwise.
 */
static esp_err_t json_to_color(color_t *color, const cJSON *json)
{
    assert(color != NULL);
    assert(json != NULL);

    ESP_RETURN_ON_FALSE(cJSON_IsString(json), ESP_ERR_INVALID_ARG, PROPERTY_TAG, "Expected a string for color");

    const char *color_str = json->valuestring;

    ESP_RETURN_ON_FALSE(strlen(color_str) == 7, ESP_ERR_INVALID_ARG, PROPERTY_TAG, "Invalid color string length");

    ESP_RETURN_ON_FALSE(color_str[0] == '#', ESP_ERR_INVALID_ARG, PROPERTY_TAG, "Invalid color string format");

    uint32_t color_value = 0;
    ESP_RETURN_ON_FALSE(sscanf(color_str + 1, "%lx", &color_value), ESP_ERR_INVALID_ARG, PROPERTY_TAG,
                        "Failed to convert color string");

    color->red   = (color_value >> 16) & 0xFF;
    color->green = (color_value >> 8) & 0xFF;
    color->blue  = color_value & 0xFF;

    return ESP_OK;
}

#undef PROPERTY_TAG