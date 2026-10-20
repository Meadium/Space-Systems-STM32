
#include "ws2812b.h"
/* Biblioteka służąca do obsugi diód

/* Częstotliwość zegara jest ustawiona na 80MHz/100 = 800000, więc okres to 1,25 micro s.
 * Do wysłania bitu 0 użyjemy 32% wypełnienia, a 1 - 64%. (ARR = 99), więc 32U = 32%
 *  Aby zresetować diody trzeba wysłać 40 zer.
 */

#define BIT_0_TIME		32U
#define BIT_1_TIME		64U

//40*1,25mi s = 50 mi s stan niski = reset
#define RESET_LEN		40U 
#define LED_N			5U

static uint32_t led_buffer[RESET_LEN + 24 * LED_N]; // buffer do wysłania zawierający 8 bitów * 3 kolory * liczba diód + bity do resetu

static void set_byte(uint32_t pos, uint8_t value)
{
  for (int i = 0; i < 8; i++) {
    if (value & 0x80) { // sprawdza czy pierwszy bit jest 1 (0x80 - 1000 0000)
      led_buffer[pos + i] = BIT_1_TIME; //jeżeli tak zapisuje do bufora 1, jeżeli nie 0
    } else {
      led_buffer[pos + i] = BIT_0_TIME;
    }

    value <<= 1; // przesuwa wszystkie bity w lewo (żeby sprawdzić kolejne bity)
  }
}

void ws2812b_update(TIM_HandleTypeDef *tim) // wysyła dane z buforu przez DMA
{
	HAL_TIM_PWM_Start_DMA(tim, TIM_CHANNEL_1, (uint32_t*)led_buffer, sizeof(led_buffer));
}



void ws2812b_init(TIM_HandleTypeDef *tim)
{
	for (int i = 0; i < RESET_LEN; ++i) // ustawia na początku reset
	{
		led_buffer[i] = 0;
	}
	for (int i = 0; i < 24 * LED_N; i++){ // zapełnia resztę buforu zerami
		led_buffer[RESET_LEN + i] = BIT_0_TIME;
	}


	  HAL_TIM_Base_Start(tim);
	  ws2812b_update(tim);
}


void ws2812b_wait(TIM_HandleTypeDef *tim) // czeka na zakończenie transmisji
{
	while (HAL_TIM_GetChannelState(tim, TIM_CHANNEL_1) == HAL_TIM_CHANNEL_STATE_BUSY)
		{}
}

void ws2812b_set_color(uint32_t led, uint8_t red, uint8_t green, uint8_t blue)
{
	if (led < LED_N)
	{
		// pierwsze 8 bitów to kolor zielony, kolejne 8 czerwony i ostatnie 8 niebieski
		set_byte(RESET_LEN + 24 * led, green);
		set_byte(RESET_LEN + 24 * led + 8, red);
		set_byte(RESET_LEN + 24 * led + 16, blue);
	}
}


