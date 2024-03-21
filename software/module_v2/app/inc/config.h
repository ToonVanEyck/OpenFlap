#pragma once

#include "openflap.h"

/** Load the config form NVM. */
void configLoad(openflap_config_t *config);

/** Store the config in NVM. */
void configStore(openflap_config_t *config);