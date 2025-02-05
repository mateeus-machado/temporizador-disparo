#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"

// Definição dos pinos dos LEDs
#define LED_AZUL 11
#define LED_VERMELHO 12
#define LED_VERDE 13

// Definição do pino do botão
#define BOTAO 5

// Tempo de espera entre cada transição de LED (em ms)
#define INTERVALO_MS 3000 

// Variável para controlar se o processo está ativo
volatile bool processo_ativo = false;

// Protótipos das funções
void configurar_hardware();
void acionar_leds();
int64_t desligar_azul_callback(alarm_id_t id, void *user_data);
int64_t desligar_vermelho_callback(alarm_id_t id, void *user_data);
int64_t desligar_verde_callback(alarm_id_t id, void *user_data);
void botao_interrupcao(uint gpio, uint32_t events);

int main() {
    stdio_init_all();
    
    // Configuração inicial do hardware
    configurar_hardware();

    printf("Sistema pronto. Pressione o botão para iniciar a sequência de LEDs.\n");

    // Loop principal (mantém o programa rodando)
    while (true) {
        sleep_ms(1000);
    }
}

// Inicializa LEDs e botão
void configurar_hardware() {
    const int leds[] = {LED_AZUL, LED_VERMELHO, LED_VERDE};

    for (int i = 0; i < 3; i++) {
        gpio_init(leds[i]);
        gpio_set_dir(leds[i], GPIO_OUT);
        gpio_put(leds[i], 0);  // Garante que os LEDs iniciam apagados
    }

    gpio_init(BOTAO);
    gpio_set_dir(BOTAO, GPIO_IN);
    gpio_pull_up(BOTAO);  // Ativa resistor de pull-up interno

    // Configura interrupção no botão (detecção de borda de descida)
    gpio_set_irq_enabled_with_callback(BOTAO, GPIO_IRQ_EDGE_FALL, true, &botao_interrupcao);
}

// Callback acionado ao pressionar o botão (com debounce)
void botao_interrupcao(uint gpio, uint32_t events) {
    static uint32_t ultimo_acionamento = 0;
    uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());

    // Previne múltiplos acionamentos devido ao bouncing
    if (tempo_atual - ultimo_acionamento > 200) {
        if (!processo_ativo) {
            processo_ativo = true;
            printf("Botão pressionado! Iniciando sequência...\n");
            acionar_leds();
        }
    }
    ultimo_acionamento = tempo_atual;
}

// Liga todos os LEDs e inicia o timer para desligar o LED azul
void acionar_leds() {
    gpio_put(LED_AZUL, 1);
    gpio_put(LED_VERMELHO, 1);
    gpio_put(LED_VERDE, 1);

    add_alarm_in_ms(INTERVALO_MS, desligar_azul_callback, NULL, false);
}

// Callback para desligar o LED azul e agendar o desligamento do vermelho
int64_t desligar_azul_callback(alarm_id_t id, void *user_data) {
    gpio_put(LED_AZUL, 0);
    printf("LED azul apagado.\n");

    add_alarm_in_ms(INTERVALO_MS, desligar_vermelho_callback, NULL, false);
    return 0;
}

// Callback para desligar o LED vermelho e agendar o desligamento do verde
int64_t desligar_vermelho_callback(alarm_id_t id, void *user_data) {
    gpio_put(LED_VERMELHO, 0);
    printf("LED vermelho apagado.\n");

    add_alarm_in_ms(INTERVALO_MS, desligar_verde_callback, NULL, false);
    return 0;
}

// Callback para desligar o LED verde e permitir nova ativação do botão
int64_t desligar_verde_callback(alarm_id_t id, void *user_data) {
    gpio_put(LED_VERDE, 0);
    printf("LED verde apagado. Sequência concluída.\n");

    // Habilita novamente o acionamento do botão
    processo_ativo = false;
    return 0;
}
