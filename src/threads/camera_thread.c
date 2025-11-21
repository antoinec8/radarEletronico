/**
 * @file camera_thread.c
 * @brief Thread de simulação de câmera/LPR
 * 
 * Simula captura de placa quando acionada via ZBUS.
 * Pode simular falhas conforme configurado no Kconfig.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <stdlib.h>
#include <stdio.h>
#include "../types.h"
#include "../utils/plate_validator.h"

LOG_MODULE_REGISTER(camera_thread, LOG_LEVEL_INF);

/* Declaração dos canais ZBUS (definidos no main.c) */
ZBUS_CHAN_DECLARE(camera_trigger_chan);
ZBUS_CHAN_DECLARE(camera_result_chan);

/**
 * @brief Gera uma placa Mercosul válida aleatória
 */
static void generate_valid_plate(char *plate_buf)
{
    /* Formato: ABC1D23 */
    plate_buf[0] = 'A' + (rand() % 26);  /* Letra */
    plate_buf[1] = 'A' + (rand() % 26);  /* Letra */
    plate_buf[2] = 'A' + (rand() % 26);  /* Letra */
    plate_buf[3] = '0' + (rand() % 10);  /* Dígito */
    plate_buf[4] = 'A' + (rand() % 26);  /* Letra */
    plate_buf[5] = '0' + (rand() % 10);  /* Dígito */
    plate_buf[6] = '0' + (rand() % 10);  /* Dígito */
    plate_buf[7] = '\0';
}

/**
 * @brief Gera uma placa inválida (simula falha)
 */
static void generate_invalid_plate(char *plate_buf)
{
    /* Formato inválido: todos dígitos ou muito curto */
    if (rand() % 2) {
        /* Placa com formato errado */
        snprintf(plate_buf, 8, "1234567");
    } else {
        /* Placa muito curta */
        snprintf(plate_buf, 8, "ABC12");
    }
}

/**
 * @brief Simula o processamento da câmera
 */
static void process_camera_capture(const camera_trigger_event_t *trigger)
{
    camera_result_event_t result = {0};
    
    LOG_INF("Camera acionada! Capturando placa...");
    
    /* Simula tempo de processamento */
    k_sleep(K_MSEC(100 + (rand() % 200)));
    
    /* Determina se haverá falha baseado na configuração */
    uint32_t random_val = rand() % 100;
    bool will_fail = (random_val < CONFIG_RADAR_CAMERA_FAILURE_RATE_PERCENT);
    
    if (will_fail) {
        /* Simula falha na captura */
        generate_invalid_plate(result.plate);
        result.valid = false;
        LOG_WRN("Falha na captura! Placa invalida: %s", result.plate);
    } else {
        /* Captura bem-sucedida */
        generate_valid_plate(result.plate);
        result.valid = validate_mercosul_plate(result.plate);
        LOG_INF("Placa capturada com sucesso: %s", result.plate);
    }
    
    result.timestamp = k_uptime_get();
    
    /* Publica resultado no ZBUS */
    if (zbus_chan_pub(&camera_result_chan, &result, K_MSEC(100)) != 0) {
        LOG_ERR("Falha ao publicar resultado da câmera");
    }
}

/* Subscriber do ZBUS (nao listener!) */
ZBUS_SUBSCRIBER_DEFINE(camera_sub, 4);

/**
 * @brief Thread de camera - processa triggers via ZBUS
 */
void camera_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    LOG_INF("Thread de camera/LPR iniciada");
    LOG_INF("Taxa de falha configurada: %d%%", CONFIG_RADAR_CAMERA_FAILURE_RATE_PERCENT);
    
    /* Adiciona subscriber ao canal de trigger */
    zbus_chan_add_obs(&camera_trigger_chan, &camera_sub, K_NO_WAIT);
    
    /* Loop principal */
    while (1) {
        const struct zbus_channel *chan;
        
        /* Aguarda mensagens no ZBUS */
        if (zbus_sub_wait(&camera_sub, &chan, K_FOREVER) == 0) {
            camera_trigger_event_t trigger;
            
            if (zbus_chan_read(chan, &trigger, K_MSEC(100)) == 0) {
                LOG_INF("Camera acionada! Capturando placa...");
                process_camera_capture(&trigger);
            } else {
                LOG_ERR("Falha ao ler dados do canal");
            }
        }
    }
}

/* Definição da thread */
#define CAMERA_THREAD_STACK_SIZE 1024
#define CAMERA_THREAD_PRIORITY 6

K_THREAD_DEFINE(camera_thread, CAMERA_THREAD_STACK_SIZE,
                camera_thread_entry, NULL, NULL, NULL,
                CAMERA_THREAD_PRIORITY, 0, 0);
