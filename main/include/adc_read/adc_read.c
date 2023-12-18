#include "adc_read.h"


static const char *TAG_ADC_READ_LOOP = "ADC_READ_LOOP";

#if CONFIG_IDF_TARGET_ESP32
static adc_channel_t channel[2] = {ADC_CHANNEL_6, ADC_CHANNEL_7};
#else
static adc_channel_t channel[2] = {ADC_CHANNEL_2, ADC_CHANNEL_3};
#endif

static TaskHandle_t s_task_handle;

/**
 * Formato salida oled:
 * 
 *  |:: Lectura    ::|
 *  |>    0596     < |
 *  |:: Intensidad ::|
 *  |           0    |
 *  |__minima________|
 * 
 *  |:: Lectura    ::|
 *  |>    3080     < |
 *  |:: Intensidad ::|
 *  |  | | | |  4    |
 *  |__alta__________|
 * 
 *  |:: Lectura    ::|
 *  |>    3612     < |
 *  |:: Intensidad ::|
 *  |  | | | | |5    |
 *  |__maxima________|
 * 
 * */


char text[][6][20] = {
        {
            ":: Lectura    ::"
        },
        //  >  2612  <
        {
            ":: Intensidad ::"
        },
		{
			"      0", " |    1", " ||   2", " |||  3", " |||| 4", " |||||5"
		},
		{
			"_minima_", "_baja___", "_baja___", "_media__", "_alta___", "_maxima_"
		}
	};

// Modificacion de ssd1306_display_text_x3 a x2

void ssd1306_display_text_x2(SSD1306_t * dev, int page, const char * text, int text_len, bool invert)
{
	if (page >= dev->_pages) return;
	int _text_len = text_len;
	if (_text_len > 8) _text_len = 8;

	uint8_t seg = 0;

	for (uint8_t nn = 0; nn < _text_len; nn++) {

		uint8_t const * const in_columns = font8x8_basic_tr[(uint8_t)text[nn]];

		// make the character 2x as high
		out_column_t out_columns[8];
		memset(out_columns, 0, sizeof(out_columns));

		for (uint8_t xx = 0; xx < 8; xx++) { // for each column (x-direction)

			uint32_t in_bitmask = 0b1;
			uint32_t out_bitmask = 0b11;

			for (uint8_t yy = 0; yy < 8; yy++) { // for pixel (y-direction)
				if (in_columns[xx] & in_bitmask) {
					out_columns[xx].u32 |= out_bitmask;
				}
				in_bitmask <<= 1;
				out_bitmask <<= 2;
			}
		}

		// render character in 8 column high pieces, making them 2x as wide
		for (uint8_t yy = 0; yy < 2; yy++)	{ // for each group of 8 pixels high (y-direction)

			uint8_t image[16];
			for (uint8_t xx = 0; xx < 8; xx++) { // for each column (x-direction)
				image[xx*2+0] = 
				image[xx*2+1] = out_columns[xx].u8[yy];
			}
			if (invert) ssd1306_invert(image, 16);
			if (dev->_flip) ssd1306_flip(image, 16);
			if (dev->_address == SPIAddress) {
				spi_display_image(dev, page+yy, seg, image, 16);
			} else {
				i2c_display_image(dev, page+yy, seg, image, 16);
			}
			memcpy(&dev->_page[page+yy]._segs[seg], image, 16);
		}
		seg = seg + 16;
	}
}

// Layout personalizado oled

void disp_upd(SSD1306_t *dev, uint16_t data, const uint16_t max)
{
	if(data >= max)
		data = max - 1;

	char raw[10] = {'\0'};
	int _v_bar = 6 * data / max;

	sprintf(raw, "> %04hu <", data+1);

	ssd1306_display_text   (dev, 0, *text[0], 16, true);
	ssd1306_display_text_x2(dev, 1, raw, 8, false);
	ssd1306_display_text   (dev, 3, *text[1], 16, true);
	ssd1306_display_text_x2(dev, 4, text[2][_v_bar], 9, false);
	ssd1306_display_text_x2(dev, 6, text[3][_v_bar], 8, false);
}

static esp_adc_cal_characteristics_t adc1_c;
void adc1_init_all(adc_atten_t atten)
{
    esp_adc_cal_characterize(ADC_UNIT_1, atten, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_c);

    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_2_5));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_6)); 
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_5, atten)); 
}

float_t get_average_read(adc1_channel_t channel, uint32_t samples)
{
    float_t sum = 0.0f;

    for(uint32_t i = 0; i < samples; i++)
        sum += adc1_get_raw(channel);
    return (float_t)sum / samples;
}