/**
 * @file camera_thread.c
 * @brief Thread de integração com camera_service
 * 
 * Integra com o módulo camera_service do professor via ZBUS.
 * Converte entre as interfaces camera_trigger_chan/camera_result_chan (internas)
 * e chan_camera_evt (do módulo externo).
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "camera_service.h"
#include "../types.h"
#include "../utils/plate_validator.h"

LOG_MODULE_REGISTER(camera_thread, LOG_LEVEL_INF);

/* Declaração dos canais ZBUS internos (definidos no main.c) */
ZBUS_CHAN_DECLARE(camera_trigger_chan);
ZBUS_CHAN_DECLARE(camera_result_chan);

/* Canal do camera_service (externo) */
ZBUS_CHAN_DECLARE(chan_camera_evt);

/**
 * @brief Processa trigger da câmera usando camera_service
 */
static void process_camera_capture(const camera_trigger_event_t *trigger)
{
    int ret;
    
    LOG_INF("=== CAPTURA INICIADA ===");
    LOG_INF("Velocidade: %u km/h, Tipo: %s", 
            trigger->speed_kmh,
            trigger->vehicle_type == VEHICLE_TYPE_LIGHT ? "LEVE" : "PESADO");
    
    /* Chama API do camera_service para iniciar captura */
    ret = camera_api_capture(K_SECONDS(5));
    if (ret != 0) {
        /* Falha ao iniciar captura */
        camera_result_event_t result = {0};
        result.valid = false;
        result.timestamp = k_uptime_get();
        snprintf(result.plate, sizeof(result.plate), "ERROR");
        
        LOG_ERR("Falha ao iniciar captura (erro %d)", ret);
        
        /* Publica resultado de erro */
        zbus_chan_pub(&camera_result_chan, &result, K_MSEC(100));
        return;
    }
    
    LOG_INF("Comando de captura enviado ao camera_service com sucesso!");
    LOG_INF("Aguardando evento chan_camera_evt...");
    /* A resposta virá via chan_camera_evt e será processada no listener */
}

/* Subscriber do ZBUS para camera_trigger_chan */
ZBUS_SUBSCRIBER_DEFINE(camera_sub, 4);

/* MSG_SUBSCRIBER para eventos do camera_service (bloqueante) */
ZBUS_MSG_SUBSCRIBER_DEFINE(camera_evt_sub);

/**
 * @brief Processa eventos do camera_service em uma thread dedicada
 */
static void camera_evt_processor_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    const struct zbus_channel *chan;
    struct msg_camera_evt evt;
    mercosul_country_t country;
    
    LOG_INF("Thread processadora de eventos camera_service iniciada");
    
    /* Adiciona msg_subscriber ao canal de eventos do camera_service */
    zbus_chan_add_obs(&chan_camera_evt, &camera_evt_sub, K_NO_WAIT);
    
    while (1) {
        /* Aguarda eventos do camera_service (bloqueante) */
        if (zbus_sub_wait_msg(&camera_evt_sub, &chan, &evt, K_FOREVER) == 0) {
            LOG_INF(">>> EVENTO RECEBIDO do camera_service!");
            LOG_INF("Tipo: %d", evt.type);
            
            camera_result_event_t result = {0};
            result.timestamp = k_uptime_get();
            
            switch (evt.type) {
            case MSG_CAMERA_EVT_TYPE_DATA:
                /* Captura bem-sucedida */
                if (evt.captured_data != NULL && evt.captured_data->plate != NULL) {
                    /* Copia placa */
                    strncpy(result.plate, evt.captured_data->plate, sizeof(result.plate) - 1);
                    result.plate[sizeof(result.plate) - 1] = '\0';
                    
                    /* Remove espaços (bug do camera_service) */
                    char clean_plate[8] = {0};
                    int clean_idx = 0;
                    for (int i = 0; result.plate[i] != '\0' && clean_idx < 7; i++) {
                        if (result.plate[i] != ' ') {
                            clean_plate[clean_idx++] = result.plate[i];
                        }
                    }
                    clean_plate[clean_idx] = '\0';
                    
                    LOG_INF("Placa recebida: '%s' -> limpa: '%s'", result.plate, clean_plate);
                    
                    /* Copia placa limpa de volta */
                    strncpy(result.plate, clean_plate, sizeof(result.plate));
                    
                    /* Valida placa Mercosul */
                    result.valid = validate_mercosul_plate(result.plate, &country);
                    
                    if (result.valid) {
                        LOG_INF("Placa capturada: %s (%s)", 
                                result.plate, get_country_name(country));
                    } else {
                        LOG_WRN("Placa formato invalido: %s (camera_service)", result.plate);
                    }
                } else {
                    LOG_ERR("Dados NULL do camera_service");
                    result.valid = false;
                    snprintf(result.plate, sizeof(result.plate), "NULL");
                }
                break;
                
            case MSG_CAMERA_EVT_TYPE_ERROR:
                LOG_WRN("Erro na captura (codigo %d)", evt.error_code);
                result.valid = false;
                snprintf(result.plate, sizeof(result.plate), "ERR%03d", evt.error_code);
                break;
                
            default:
                LOG_ERR("Tipo de evento desconhecido: %d", evt.type);
                result.valid = false;
                snprintf(result.plate, sizeof(result.plate), "UNKNOWN");
                break;
            }
            
            /* Publica resultado */
            if (zbus_chan_pub(&camera_result_chan, &result, K_MSEC(100)) == 0) {
                LOG_INF("OK Resultado publicado!");
            } else {
                LOG_ERR("X Falha ao publicar resultado");
            }
        }
    }
}

K_THREAD_DEFINE(camera_evt_processor, 2048, camera_evt_processor_thread, 
                NULL, NULL, NULL, 5, 0, 0);

/**
 * @brief Thread de integração - processa triggers via ZBUS
 */
void camera_integration_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    LOG_INF("Thread de integração camera iniciada");
    
    /* Adiciona subscriber ao canal de trigger interno */
    zbus_chan_add_obs(&camera_trigger_chan, &camera_sub, K_NO_WAIT);
    
    LOG_INF("Aguardando triggers de captura...");
    
    /* Loop principal - aguarda triggers */
    while (1) {
        const struct zbus_channel *chan;
        
        /* Aguarda mensagens no ZBUS */
        if (zbus_sub_wait(&camera_sub, &chan, K_FOREVER) == 0) {
            camera_trigger_event_t trigger;
            
            if (zbus_chan_read(chan, &trigger, K_MSEC(100)) == 0) {
                process_camera_capture(&trigger);
            } else {
                LOG_ERR("Falha ao ler dados do canal de trigger");
            }
        }
    }
}

/* Definição da thread */
#define CAMERA_INTEGRATION_THREAD_STACK_SIZE 2048
#define CAMERA_INTEGRATION_THREAD_PRIORITY 6

K_THREAD_DEFINE(camera_integration_thread, CAMERA_INTEGRATION_THREAD_STACK_SIZE,
                camera_integration_thread_entry, NULL, NULL, NULL,
                CAMERA_INTEGRATION_THREAD_PRIORITY, 0, 0);
