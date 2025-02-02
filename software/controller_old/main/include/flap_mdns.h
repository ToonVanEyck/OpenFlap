#ifndef FLAP_MDNS_H
#define FLAP_MDNS_H

#include "esp_system.h"
#include "esp_log.h"
#include "mdns.h"

void flap_mdns_init(char* hostname);

#endif