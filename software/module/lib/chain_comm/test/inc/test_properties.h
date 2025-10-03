#pragma once

#include "chain_comm_shared.h"

#define TEST_SIZE 4

/* Define 6 example properties*/

#define PROP_STATIC_RW  (cc_prop_id_t)(1)
#define PROP_STATIC_RO  (cc_prop_id_t)(2)
#define PROP_STATIC_WO  (cc_prop_id_t)(3)
#define PROP_DYNAMIC_RW (cc_prop_id_t)(4)
#define PROP_DYNAMIC_RO (cc_prop_id_t)(5)
#define PROP_DYNAMIC_WO (cc_prop_id_t)(6)

#define PROPERTY_CNT (6) /* Update this when adding new properties. */

/* Extern list of property attributes used by both the master and the nodes. */
extern const cc_prop_attr_t cc_property_attribute_list[PROPERTY_CNT];