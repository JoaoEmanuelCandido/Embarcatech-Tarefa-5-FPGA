#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include <irq.h>
#include <uart.h>
#include <console.h>
#include <generated/csr.h>

#include "./lib/rfm95.h"

#include "./lib/aht10.h" 
void i2c_init(void);

#define N_ELEMENTS 8

static bool g_lora_ok   = false;
static bool g_sensor_ok = false;

static char *readstr(void)
{
    char c[2];
    static char s[64];
    static int ptr = 0;

    if(readchar_nonblock()) {
        c[0] = readchar();
        c[1] = 0;
        switch(c[0]) {
            case 0x7f:
            case 0x08:
                if(ptr > 0) {
                    ptr--;
                    putsnonl("\x08 \x08");
                }
                break;
            case 0x07:
                break;
            case '\r':
            case '\n':
                s[ptr] = 0x00;
                putsnonl("\n");
                ptr = 0;
                return s;
            default:
                if(ptr >= (sizeof(s) - 1))
                    break;
                putsnonl(c);
                s[ptr] = c[0];
                ptr++;
                break;
        }
    }
    return NULL;
}

static char *get_token(char **str)
{
    char *c, *d;
    c = strchr(*str, ' ');
    if(c == NULL) {
        d = *str;
        *str += strlen(*str);
        return d;
    }
    *c = 0;
    d = *str;
    *str = c + 1;
    return d;
}

static void prompt(void)
{
    printf("RUNTIME>");
}

static void help(void)
{
    puts("Comandos Auxiliares:");
    puts("help                 - Mostra todos os comandos disponiveis");
    puts("reboot               - Reinicia a CPU");
    puts("\nComandos do módulo LoRa:");
    puts("lora_setup           - Realiza o setup do modulo LoRa (freq 915MHz)");
    puts("lora_info            - Lê informacoes do modulo LoRa");
    puts("\nComandos do sensor AHT10:");
    puts("sensor_setup         - Inicializa I2C e o AHT10");
    puts("sensor_send          - Lê o AHT10 e envia via LoRa (temp/umid)\n\n");
}

static void reboot(void)
{
    ctrl_reset_write(1);
}

static void lora_info(void)
{
    printf("Lendo LoRa...\n");
    printf("Ret: %x\n", rfm95_read_reg(0x42));
}

static void lora_setup(void)
{
    printf("Configurando LoRa (915 MHz)...\n");
    if (!rfm95_init()) {
        printf("Falha ao inicializar LoRa.\n");
        return;
    }
    g_lora_ok = true;
    printf("LoRa pronto.\n");
}

static inline int16_t clamp_to_i16(int32_t v) {
    if (v >  32767) return  32767;
    if (v < -32768) return -32768;
    return (int16_t)v;
}

static bool lora_send_data_i16(int16_t temperatura, int16_t umidade)
{
    uint8_t buf[4];
    buf[0] = (uint8_t)(temperatura & 0xFF);
    buf[1] = (uint8_t)((temperatura >> 8) & 0xFF);
    buf[2] = (uint8_t)(umidade & 0xFF);
    buf[3] = (uint8_t)((umidade >> 8) & 0xFF);

    printf("Enviando (i16): temp=%d (x0.01 C), umid=%d (x0.01 %%)\n", temperatura, umidade);
    return rfm95_send_bytes(buf, sizeof(buf));
}

static bool lora_send_data(float temp_c, float umid_pct)
{
    int32_t t = (int32_t)lroundf(temp_c  * 100.0f);
    int32_t u = (int32_t)lroundf(umid_pct * 100.0f);
    return lora_send_data_i16(clamp_to_i16(t), clamp_to_i16(u));
}

static void sensor_setup(void)
{
    printf("Inicializando I2C e AHT10...\n");
    i2c_init();
    aht10_init();
    g_sensor_ok = true;
    printf("AHT10 pronto.\n");
}

static bool sensor_read_once(float *temp_c, float *umid_pct)
{
    if (!g_sensor_ok) {
        printf("ERRO: Sensor nao inicializado. Rode 'sensor_setup' primeiro.\n");
        return false;
    }

    dados my_data;
    if (!aht10_get_data(&my_data)) {
        printf("ERRO ao ler AHT10.\n");
        return false;
    }

    *temp_c  = (float)my_data.temperatura / 100.0f;
    *umid_pct = (float)my_data.umidade / 100.0f;

    printf("AHT10 -> Temperatura: %d.%02d C, Umidade: %d.%02d %%\n",
           my_data.temperatura/100, abs(my_data.temperatura)%100,
           my_data.umidade/100,     abs(my_data.umidade)%100);

    return true;
}

static void sensor_send(void)
{
    float t, u;
    if (!sensor_read_once(&t, &u)) return;

    if (!lora_send_data(t, u)) {
        printf("ERRO durante envio LoRa.\n");
    }
}

static void console_service(void)
{
    char *str, *token;
    str = readstr();
    if(str == NULL) return;

    token = get_token(&str);

    if(strcmp(token, "help") == 0) {
        help();

    } else if(strcmp(token, "reboot") == 0) {
        reboot();

    } else if(strcmp(token, "lora_info") == 0) {
        lora_info();

    } else if(strcmp(token, "lora_setup") == 0) {
        lora_setup();

    } else if(strcmp(token, "sensor_setup") == 0) {
        sensor_setup();

    } else if(strcmp(token, "sensor_send") == 0) {
        sensor_send();

    } else {
        puts("Comando desconhecido. Digite 'help'.");
    }

    prompt();
}

// ======= main (estrutura preservada) =======
int main(void)
{
#ifdef CONFIG_CPU_HAS_INTERRUPT
    irq_setmask(0);
    irq_setie(1);
#endif
    uart_init();

    printf("Hellorld!\n");
    help();
    prompt();

    while(1)
        console_service();

    return 0;
}
