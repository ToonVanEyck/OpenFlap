#include "chain_comm_msg.h"
#include "esp_check.h"
#include "esp_log.h"

#include <string.h>

#define TAG "CHAIN_COMM_MSG"

void chain_comm_msg_header_set(chain_comm_msg_t *msg, chain_comm_action_t action, property_id_t property);

//---------------------------------------------------------------------------------------------------------------------

void chain_comm_msg_init_read_all(chain_comm_msg_t *msg, property_id_t property)
{
    assert(msg != NULL);
    chain_comm_msg_init(msg);
    chain_comm_msg_header_set(msg, property_readAll, property);
    chain_comm_msg_data_add(msg, 0); // add module index bytes
    chain_comm_msg_data_add(msg, 0); // add module index bytes
}

//---------------------------------------------------------------------------------------------------------------------

void chain_comm_msg_init_write_seq(chain_comm_msg_t *msg, property_id_t property)
{
    assert(msg != NULL);
    chain_comm_msg_init(msg);
    chain_comm_msg_header_set(msg, property_writeSequential, property);
}

//---------------------------------------------------------------------------------------------------------------------

void chain_comm_msg_init_write_all(chain_comm_msg_t *msg, property_id_t property)
{
    assert(msg != NULL);
    chain_comm_msg_init(msg);
    chain_comm_msg_header_set(msg, property_writeAll, property);
}

//---------------------------------------------------------------------------------------------------------------------

void chain_comm_msg_data_add(chain_comm_msg_t *msg, uint8_t byte)
{
    assert(msg != NULL);
    assert(msg->size < CHAIN_COM_MAX_LEN);

    msg->raw[msg->size++] = byte;
}

//---------------------------------------------------------------------------------------------------------------------

void chain_comm_msg_init(chain_comm_msg_t *msg)
{
    assert(msg != NULL);
    memset(msg, 0, sizeof(chain_comm_msg_t));
}

//---------------------------------------------------------------------------------------------------------------------

/**
 * \brief Add a header to the message.
 *
 * \param[in] msg The message to add the header to.
 * \param[in] action The action to add.
 * \param[in] property The property to add.
 */
void chain_comm_msg_header_set(chain_comm_msg_t *msg, chain_comm_action_t action, property_id_t property)
{
    assert(msg != NULL);
    assert(msg->size == 0);

    msg->structured.header.field.action   = action;
    msg->structured.header.field.property = property;
    msg->size++;
}