#include "chain_comm.h"

void writeFlashPage(uint8_t *data);
void do_command(uint8_t *data);

const propertyHandler_t propertyHandlers[end_of_properties] = {
    {.get = NULL, .set = NULL},                 /*no_property*/
    {.get = NULL, .set = writeFlashPage},       /*firmware_property*/
    {.get = NULL, .set = do_command},           /*command_property*/
    {.get = NULL, .set = NULL},                 /*columnEnd_property*/
    {.get = NULL, .set = NULL},                 /*characterMapSize_property*/
    {.get = NULL, .set = NULL},                 /*characterMap_property*/
    {.get = NULL, .set = NULL},                 /*offset_property*/
    {.get = NULL, .set = NULL},                 /*vtrim_property*/
    {.get = NULL, .set = NULL},                 /*character_property*/
    {.get = NULL, .set = NULL},                 /*baseSpeed_property*/
};
