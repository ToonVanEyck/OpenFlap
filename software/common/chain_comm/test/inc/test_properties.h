#pragma once

#include "chain_comm_shared.h"

#define TEST_PROP_SIZE (10)

/* Define 6 example properties*/

#define PROP_STATIC_RW           (cc_prop_id_t)(0)
#define PROP_STATIC_RO           (cc_prop_id_t)(1)
#define PROP_STATIC_WO           (cc_prop_id_t)(2)
#define PROP_DYNAMIC_RW          (cc_prop_id_t)(3)
#define PROP_DYNAMIC_RO          (cc_prop_id_t)(4)
#define PROP_DYNAMIC_WO          (cc_prop_id_t)(5)
#define PROP_STATIC_RW_HALF_SIZE (cc_prop_id_t)(6)

#define PROPERTY_CNT (7) /* Update this when adding new properties. */

/* Extern list of property used by both the master and the nodes. */
extern cc_prop_t cc_prop_list[PROPERTY_CNT];