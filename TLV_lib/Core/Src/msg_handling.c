
#include "msg_handling.h"
#include "stm32g4xx_hal.h"

static uint16_t read_u16_le(const uint8_t *p) // odczytuje dwa bajty jako uint (le - little-endian) młodszy bajt pierwszy

{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static int16_t read_i16_le(const uint8_t *p) // odczytuje dwa bity jako int
{
    uint16_t u = read_u16_le(p);

    if (u <= INT16_MAX)
    {
        return (int16_t)u;
    }


    return (int16_t)(-1 - (int32_t)(UINT16_MAX - u));
}

static void write_u16_le(uint8_t *p, uint16_t value) // zapisuje uint jako dwa bajty
{
    p[0] = (uint8_t)(value & 0xFFU); // pobiera 8 najmłodszych bitów
    p[1] = (uint8_t)(value >> 8); // pobiera kolejne 8
}

static void write_i16_le(uint8_t *p, int16_t value) // zapisuje int jako dwa bajty
{
    write_u16_le(p, (uint16_t)value);
}

static tlv_status_t send_message(
    protocol_app_t *app,
    message_id_t id,
    const uint8_t *payload,
    uint8_t length)
{
    return tlv_send(app->tlv, (uint8_t)id, payload, length);
}

static void send_ack( protocol_app_t *app, uint8_t received_id, ack_status_t status)
{
    uint8_t payload[2];

    payload[0] = received_id;
    payload[1] = (uint8_t)status;

    (void)send_message(
        app, MSG_ACK, payload, sizeof(payload)
    );
}

static void send_pong(protocol_app_t *app)
{
    uint8_t payload[1] = { 0U };

    (void)send_message(app, MSG_PONG, payload, sizeof(payload)
    );
}

void protocol_app_init(protocol_app_t *app, tlv_t *tlv)
{
    if ((app == NULL) || (tlv == NULL))
    {
        return;
    }

    app->tlv = tlv;
    app->pong_pending = 0U;
    app->pong_deadline = 0U;
}



void protocol_app_process(protocol_app_t *app, uint32_t now_ms)
{
    if ((app == NULL) || (app->tlv == NULL))
    {
        return;
    }

    if (app->pong_pending != 0U)
    {
        if ((int32_t)(now_ms - app->pong_deadline) >= 0) // Sprawdza, czy nadszedł termin, uwzględniając przepełnienie licznika uint32_t.
        {
            app->pong_pending = 0U;
            send_pong(app);
        }
    }
}

static uint8_t handle_math(protocol_app_t *app, const uint8_t *payload) // Obsuga funkcji matematycznych
{
    uint8_t operation = payload[0]; // znak operacji
    // każda z liczb ma 2 bajty
    int16_t a = read_i16_le(&payload[1]);
    int16_t b = read_i16_le(&payload[3]);

    int32_t wide_result = 0; // tymczasowy wynik, chroniacy przed przepełnieniem
    int16_t result = 0; // wynik końcowy

    uint8_t flags = 0U; // flagi: 01 - overflow, 10 - divide_by_zero
    uint8_t valid = 1U; // wartość zwracana (0 jeżeli operacja się nie powiodła)

    switch (operation)
    {
        case 0x01U: /* ADD */
            wide_result = (int32_t)a + (int32_t)b;
            break;

        case 0x02U: /* SUB */
            wide_result = (int32_t)a - (int32_t)b;
            break;

        case 0x04U: /* MUL */
            wide_result = (int32_t)a * (int32_t)b;
            break;

        case 0x08U: /* DIV */
            if (b == 0)
            {
                flags |= 0x02U; // divide_by_zero
                valid = 0U;
            }
            else if ((a == INT16_MIN) && (b == -1)) // szczególny przypadek dzielenia -32768 / (-1)
            {
                // 32768 nie mieści się w int16_t
                flags |= 0x01U; // overflow
                wide_result = INT16_MAX;
            }
            else
            {
                wide_result = a / b;
            }
            break;

        default:
            // Brak lub więcej niż jedna operacja
            valid = 0U;
            return valid;
    }

    if (valid != 0U)
    {
        if ((wide_result > INT16_MAX) || (wide_result < INT16_MIN))
        {
            flags |= 0x01U; // overflow
            result = (wide_result > 0) ? INT16_MAX : INT16_MIN;
        }
        else
        {
            result = (int16_t)wide_result;
        }
    }

    uint8_t response[3];

    response[0] = flags;
    write_i16_le(&response[1], result); // dopisuje wynik do odpowiedzi

    (void)send_message(app, MSG_MATH_RESP, response, sizeof(response));

    return valid;
}

void protocol_app_on_frame( // Obsługa odpowiedzi na wiadomość
    void *context,
    uint8_t type,
    uint8_t length,
	const uint8_t *payload)
{
    protocol_app_t *app = (protocol_app_t *)context;

    if ((app == NULL) || (app->tlv == NULL))
    {
        return;
    }

    // payload o długości 0 może mieć wskaźnik, którego nie wolno dereferencjonować.
    if ((length > 0U) && (payload == NULL))
    {
        return;
    }

    switch ((message_id_t)type)
    {
        case MSG_ACK:
            if (length != sizeof(msg_ack_t))
            {
                return;
            }

            break;

        case MSG_PING:
            if (length != sizeof(msg_ping_t))
            {
                send_ack(app, type, MSG_STATUS_ERROR);
                return;
            }

            {
                uint16_t delay_ms = read_u16_le(payload);

                //Zastępuje ewentualne wcześniejsze oczekujące PONG nowym terminem
                app->pong_deadline = HAL_GetTick() + delay_ms;
                app->pong_pending = 1U;


                (void)delay_ms;
            }

            send_ack(app, type, MSG_STATUS_OK);
            break;

        case MSG_PONG:
            if (length != sizeof(msg_pong_t))
            {
                send_ack(app, type, MSG_STATUS_ERROR);
                return;
            }

            send_ack(app, type, MSG_STATUS_OK);
            break;

        case MSG_MATH:
            if (length != sizeof(msg_math_t))
            {
                send_ack(app, type, MSG_STATUS_ERROR);
                return;
            }

            uint8_t math_valid = handle_math(app, payload);
            send_ack(app, type, math_valid ? MSG_STATUS_OK : MSG_STATUS_ERROR); // Sprawdza czy operacja się powiodła
            break;

        case MSG_MATH_RESP:
            if (length != sizeof(msg_math_resp_t))
            {
                send_ack(app, type, MSG_STATUS_ERROR);
                return;
            }


            send_ack(app, type, MSG_STATUS_OK);
            break;

        default:
            send_ack(app, type, MSG_STATUS_ERROR);
            break;
    }
}
