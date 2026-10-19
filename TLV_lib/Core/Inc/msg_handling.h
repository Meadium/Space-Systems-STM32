
#ifndef INC_MSG_HANDLING_H_
#define INC_MSG_HANDLING_H_

#include <stdint.h>
#include "tlv.h"
#include "msg.h"

typedef struct
{
	tlv_t *tlv;
	uint8_t pong_pending;
	uint32_t pong_deadline;
} protocol_app_t;



void protocol_app_init(protocol_app_t *app, tlv_t *tlv);

void protocol_app_process(protocol_app_t *app, uint32_t now_ms);

void protocol_app_on_frame(void *context, uint8_t type, uint8_t lenght, const uint8_t *payload);


#endif /* INC_MSG_HANDLING_H_ */
