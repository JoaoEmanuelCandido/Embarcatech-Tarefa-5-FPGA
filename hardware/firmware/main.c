#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <irq.h>
#include <uart.h>
#include <console.h>
#include <generated/csr.h>

#include "./lib/rfm9x.h"

#define N_ELEMENTS 8

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
    puts("Available commands:");
    puts("help         - opcoes de comandos");
    puts("reboot       - reboot CPU");
    puts("led          - teste do led");
    puts("lora_info      - Lendo as informacoes do modulo Lora");
    puts("lora_select      - Selecionando o modulo Lora");
    puts("lora_deselect      - Desselecionando o modulo Lora");
    puts("lora_setup      - Setup do modulo Lora");
    puts("lora_click      - Mensagem de teste do modulo Lora");
}

static void reboot(void)
{
    ctrl_reset_write(1);
}

static void toggle_led(void)
{
    int i;
    printf("invertendo led...\n");
    i = leds_out_read();
    leds_out_write(!i);
}

static void lora_info(void)
{
    printf("Lendo Lora...\n");
    printf("Ret: %x\n", rfm9x_read(0x42));
}

static void lora_select(void)
{
    printf("Selecionando Lora...\n");
    rfm9x_select();
}

static void lora_deselect(void)
{
    printf("Desselcionando Lora...\n");
    rfm9x_deselect();
}

static void lora_setup(void)
{
    printf("Configurando Lora...\n");
    rfm9x_setup(915000000);
}

static void lora_click(void)
{
    printf("Enviando mensagem Lora...\n");
    rfm9x_send("Click!");
}

static void console_service(void)
{
    char *str, *token;
    str = readstr();
    if(str == NULL) return;

    token = get_token(&str);
    if(strcmp(token, "help") == 0)
        help();
    else if(strcmp(token, "reboot") == 0)
        reboot();
    else if(strcmp(token, "led") == 0)
        toggle_led();
    else if(strcmp(token, "lora_info") == 0)
        lora_info();
    else if(strcmp(token, "lora_select") == 0)
        lora_select();
    else if(strcmp(token, "lora_deselect") == 0)
        lora_deselect();
    else if(strcmp(token, "lora_setup") == 0)
        lora_setup();
    else if(strcmp(token, "lora_click") == 0)
        lora_click();
    else
        puts("Comando desconhecido. Digite 'help'.");

    prompt();
}

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