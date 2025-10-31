#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "rfm96.h"
#include "ssd1306.h"


#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19
#define PIN_RST  20
#define PIN_DIO0 8   

#define LORA_FREQUENCY 915E6
#define SEND_INTERVAL_MS 10000

#define SDA_PIN 14
#define SCL_PIN 15
#define I2C_PORT i2c1
#define OLED_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

typedef struct {
    int16_t temperatura;
    int16_t umidade;
} aht10;

struct repeating_timer timer;
int cont = 0;
bool start = true;

uint8_t ssd[ssd1306_buffer_length];
ssd1306_t disp;
struct render_area frame_area;


void limpar_display() {
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
}

// Desenha uma string no buffer e atualiza o display
void mostrar_texto(int x, int y, const char *texto) {
    ssd1306_draw_string(ssd, x, y, (char *)texto);
    render_on_display(ssd, &frame_area);
}

// ----------------------------------------------------------

// Callback para timer repetitivo mostrando "Aguardando..."
bool repeating_timer_callback(struct repeating_timer *t) {
    char msg[32];
    int dots = cont % 4;
    sprintf(msg, "Aguardando %.*s", dots, "...");
    limpar_display();
    ssd1306_draw_string(ssd, 0, 8, msg);
    ssd1306_draw_string(ssd, 0, 24, "dados...");
    render_on_display(ssd, &frame_area);
    cont++;
    return true;
}

// ----------------------------------------------------------

// Função para exibir a tela "Aguardando"
void aguardar() {
    limpar_display();
    ssd1306_draw_string(ssd, 0, 8, "Aguardando");
    ssd1306_draw_string(ssd, 0, 24, "dados...");
    render_on_display(ssd, &frame_area);
    add_repeating_timer_ms(400, repeating_timer_callback, NULL, &timer);
    sleep_ms(10000);
}

// ----------------------------------------------------------

void iniciar() {
    stdio_init_all();


    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);


    ssd1306_init(&disp, OLED_WIDTH, OLED_HEIGHT, false, OLED_ADDR, I2C_PORT);


    frame_area.start_column = 0;
    frame_area.end_column   = OLED_WIDTH - 1;
    frame_area.start_page   = 0;
    frame_area.end_page     = (OLED_HEIGHT / 8) - 1;


    calculate_render_area_buffer_length(&frame_area);

    // Limpa display
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);


    ssd1306_draw_string(ssd, 0, 0, "Inic. LoRa...");
    render_on_display(ssd, &frame_area);
    sleep_ms(500);


    rfm96_config_t lora_cfg = {
        .spi_instance = spi0,
        .pin_miso = PIN_MISO,
        .pin_cs   = PIN_CS,
        .pin_sck  = PIN_SCK,
        .pin_mosi = PIN_MOSI,
        .pin_rst  = PIN_RST,
        .pin_dio0 = PIN_DIO0,
        .frequency = LORA_FREQUENCY
    };

    if (!lora_init(lora_cfg)) {
        memset(ssd, 0, ssd1306_buffer_length);
        ssd1306_draw_string(ssd, 0, 8, "ERRO: LoRa");
        render_on_display(ssd, &frame_area);
        while (1);
    }

    char freq_msg[32];
    sprintf(freq_msg, "Freq: %.1f MHz", LORA_FREQUENCY / 1e6);
    memset(ssd, 0, ssd1306_buffer_length);
    ssd1306_draw_string(ssd, 0, 8, "LoRa OK");
    ssd1306_draw_string(ssd, 0, 24, freq_msg);
    render_on_display(ssd, &frame_area);
    sleep_ms(1000);

    // Limpa display e inicia recepção LoRa contínua
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
    lora_start_rx_continuous();
}


// ----------------------------------------------------------

// Mostra temperatura e umidade no display
void imprimedisplay(float temp, float umid) {
    int temp_int = (int)temp;
    int temp_frac = (int)((fabsf(temp) - abs(temp_int)) * 100 + 0.5f);

    int umid_int = (int)umid;
    int umid_frac = (int)((fabsf(umid) - abs(umid_int)) * 100 + 0.5f);

    char temp_str[32];
    char umid_str[32];

    sprintf(temp_str, "Temp: %d.%02dC", temp_int, temp_frac);
    sprintf(umid_str,  "Umid: %d.%02d%%", umid_int, umid_frac);

    limpar_display();
    ssd1306_draw_string(ssd, 0, 8, temp_str);
    ssd1306_draw_string(ssd, 0, 24, umid_str);
    render_on_display(ssd, &frame_area);
}

// ----------------------------------------------------------

int main() {
    aht10 recebido;

    iniciar();
    aguardar();

    while (true) {
        int len = lora_receive_bytes((uint8_t *)&recebido, sizeof(recebido));
        if (len == sizeof(recebido)) {
            if (start) {
                cancel_repeating_timer(&timer);
                limpar_display();
                start = false;
            }
            imprimedisplay(recebido.temperatura / 100.0f, recebido.umidade / 100.0f);
        }
    }

    return 0;
}