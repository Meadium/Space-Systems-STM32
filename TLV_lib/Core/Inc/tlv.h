#ifndef TLV_H
#define TLV_H

#include <stdint.h>
#include <stddef.h>

#ifndef TLV_MAX_PAYLOAD
#define TLV_MAX_PAYLOAD 8U //Maksymalnie 8 bajtów na wiadomość
#endif

#define TLV_HEADER_SIZE 2U // 1 bajt na type, 1 na lenght

typedef enum
{
    TLV_OK = 0,
    TLV_ERROR_ARGUMENT,
    TLV_ERROR_LENGTH,
    TLV_ERROR_TRANSPORT // Błąd wysyłania
} tlv_status_t;

typedef void (*tlv_rx_callback_t)( // funkcja do obioru
    void *context,
    uint8_t type,
    uint8_t length,
    const uint8_t *payload
);

typedef int (*tlv_tx_callback_t)( // funkcja do wysyłania
    void *context,
	size_t length,
    const uint8_t *data
);


typedef enum
{
    TLV_PARSE_TYPE = 0,
    TLV_PARSE_LENGTH,
    TLV_PARSE_PAYLOAD
} tlv_parse_state_t;



typedef struct
{
    tlv_rx_callback_t on_frame;
    tlv_tx_callback_t transmit;
    void *context; // kontekst dla callbacków

    uint8_t max_payload;
    uint8_t type;
    uint8_t length;
    uint8_t payload[TLV_MAX_PAYLOAD]; //bufor na odbierane dane

    uint16_t payload_index; //pozycja na następny bajt
    tlv_parse_state_t state; //aktualny stan parsera
} tlv_t;  // Można stworzyć kilka niezależnych instancji parsera, żeby odbierać dane z kilku połączeń na raz

tlv_status_t tlv_init( // Funkcja do inicjazlizacji parsera
    tlv_t *tlv,
    uint8_t max_payload,
    tlv_rx_callback_t on_frame,
    tlv_tx_callback_t transmit,
    void *context
);

void tlv_feed(tlv_t *tlv, uint8_t byte); //Przekazuje jeden bajt do parsera

tlv_status_t tlv_send( //Przekazuje ramkę do funkcji transmisyjnej
    tlv_t *tlv,
    uint8_t type,
    const uint8_t *payload,
    uint8_t length
);

#endif
