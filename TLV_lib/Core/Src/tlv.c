#include "tlv.h"

static void tlv_reset(tlv_t *tlv)
{
    tlv->type = 0U;
    tlv->length = 0U;
    tlv->payload_index = 0U;
    tlv->state = TLV_PARSE_TYPE;
}

tlv_status_t tlv_init( tlv_t *tlv, uint8_t max_payload, tlv_rx_callback_t on_frame, tlv_tx_callback_t transmit, void *context) // Funkcja do inicjazlizacji parsera
{
    if ((tlv == NULL) ||
        (on_frame == NULL) ||
        (transmit == NULL))
    {
        return TLV_ERROR_ARGUMENT;
    }

    if (max_payload > TLV_MAX_PAYLOAD)
    {
        return TLV_ERROR_LENGTH;
    }

    tlv->on_frame = on_frame;
    tlv->transmit = transmit;
    tlv->context = context;
    tlv->max_payload = max_payload;

    tlv_reset(tlv);

    return TLV_OK;
}


void tlv_feed(tlv_t *tlv, uint8_t byte) //Przekazuje jeden bajt do parsera
{
    if (tlv == NULL)
    {
        return;
    }

    switch (tlv->state)
    {
        case TLV_PARSE_TYPE: // Następny bajt jest interpretowany jako typ
            tlv->type = byte;
            tlv->state = TLV_PARSE_LENGTH; // Teraz parser oczekuje na długość
            break;

        case TLV_PARSE_LENGTH:
        	// Byte jest długością ramki
            if (byte > tlv->max_payload) // Nieprawidłowa długość
            {
                tlv_reset(tlv);
                break;
            }

            tlv->length = byte;
            tlv->payload_index = 0U;

            if (byte == 0U) //Jeżeli długość jest równa 0, przechodzi do obsługi ramki
            {
                tlv->on_frame(tlv->context, tlv->type, 0U, tlv->payload);

                tlv_reset(tlv);
            }
            else
            {
                tlv->state = TLV_PARSE_PAYLOAD;
            }
            break;

        case TLV_PARSE_PAYLOAD: // Przyjmuje zawartość ramki
            tlv->payload[tlv->payload_index] = byte;
            tlv->payload_index++;

            if (tlv->payload_index >= tlv->length)
            {
                tlv->on_frame(tlv->context, tlv->type, tlv->length, tlv->payload);

                tlv_reset(tlv);
            }
            break;

        default:
            tlv_reset(tlv);
            break;
    }
}

tlv_status_t tlv_send(tlv_t *tlv, uint8_t type, const uint8_t *payload, uint8_t length) //Przekazuje ramkę do funkcji transmisyjnej
{
    uint8_t frame[TLV_HEADER_SIZE + TLV_MAX_PAYLOAD];

    if ((tlv == NULL) || (tlv->transmit == NULL))
    {
        return TLV_ERROR_ARGUMENT;
    }

    if ((length > tlv->max_payload) ||
        ((length > 0U) && (payload == NULL)))
    {
        return TLV_ERROR_LENGTH;
    }

    frame[0] = type;
    frame[1] = length;

    for (uint16_t i = 0U; i < length; i++)
    {
        frame[TLV_HEADER_SIZE + i] = payload[i];
    }

    if (tlv->transmit(tlv->context, TLV_HEADER_SIZE + length, frame) != 0)
    {
        return TLV_ERROR_TRANSPORT;
    }

    return TLV_OK;
}
