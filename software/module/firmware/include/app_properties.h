#include "chain_comm.h"

void do_command(uint8_t *data);
void get_columnEnd(uint8_t *data);
void get_version(uint8_t *data);
void set_character(uint8_t *data);
void get_character(uint8_t *data);
void get_characterMapSize(uint8_t *data);
void set_characterMap(uint8_t *data);
void get_characterMap(uint8_t *data);
void set_offset(uint8_t *data);
void get_offset(uint8_t *data);
void set_vtrim(uint8_t *data);
void get_vtrim(uint8_t *data);
void set_baseSpeed(uint8_t *data);
void get_baseSpeed(uint8_t *data);

const propertyHandler_t propertyHandlers[end_of_properties] = {
    {.get = NULL, .set = NULL},                         /*no_property*/
    {.get = get_version, .set = NULL},                  /*firmware_property*/
    {.get = NULL, .set = do_command},                   /*command_property*/
    {.get = get_columnEnd, .set = NULL},                /*columnEnd_property*/
    {.get = get_characterMapSize, .set = NULL},         /*characterMapSize_property*/
    {.get = get_characterMap, .set = set_characterMap}, /*characterMap_property*/
    {.get = get_offset, .set = set_offset},             /*offset_property*/
    {.get = get_vtrim, .set = set_vtrim},               /*vtrim_property*/
    {.get = get_character, .set = set_character},       /*character_property*/
    {.get = get_baseSpeed, .set = set_baseSpeed},       /*baseSpeed_property*/
};
