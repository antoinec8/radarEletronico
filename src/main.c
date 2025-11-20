/**
 * @file main.c
 * @brief Thread principal do radar eletrônico
 * 
 * Orquestra todo o sistema:
 * - Recebe dados dos sensores
 * - Calcula velocidade
 * - Detecta infrações
 * - Aciona câmera
 * - Valida placas
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include "types.h"
#include "utils/calculations.h"
#include "utils/plate_validator.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* Filas de mensagens */
K_MSGQ_DEFINE(sensor_msgq, sizeof(sensor_data_msg_t), 10, 4);
K_MSGQ_DEFINE(display_msgq, sizeof(display_data_msg_t), 10, 4);

/* Canais ZBUS */
ZBUS_CHAN_DEFINE(camera_trigger_chan,
                 camera_trigger_event_t,
                 NULL,
                 NULL,
                 ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(0));

ZBUS_CHAN_DEFINE(camera_result_chan,
                 camera_result_event_t,
                 NULL,
                 NULL,
                 ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(0));

/* Subscriber para resultado da câmera */
ZBUS_SUBSCRIBER_DEFINE(camera_result_sub, 4);

/**
 * @brief Processa dados do sensor e detecta infrações
 */
static void process_vehicle_detection(const sensor_data_msg_t *sensor_data)
{
    /* Calcula velocidade */
    uint32_t speed = calculate_speed_kmh(sensor_data->time_delta_ms, 
                                          CONFIG_RADAR_SENSOR_DISTANCE_MM);
    
    /* Determina limite aplicável */
    uint32_t limit = get_speed_limit(sensor_data->vehicle_type,
                                      CONFIG_RADAR_SPEED_LIMIT_LIGHT_KMH,
                                      CONFIG_RADAR_SPEED_LIMIT_HEAVY_KMH);
    
    /* Determina status */
    speed_status_t status = determine_speed_status(speed, limit, 
                                                     CONFIG_RADAR_WARNING_THRESHOLD_PERCENT);
    
    LOG_INF("=== PROCESSAMENTO ===");
    LOG_INF("Velocidade calculada: %u km/h", speed);
    LOG_INF("Limite aplicável: %u km/h", limit);
    LOG_INF("Status: %d", status);
    
    /* Envia para display */
    display_data_msg_t display_msg = {
        .speed_kmh = speed,
        .vehicle_type = sensor_data->vehicle_type,
        .status = status,
        .speed_limit = limit
    };
    
    k_msgq_put(&display_msgq, &display_msg, K_NO_WAIT);
    
    /* Se infracao, aciona camera */
    if (status == SPEED_STATUS_VIOLATION) {
        LOG_WRN("*** INFRACAO DETECTADA! Acionando camera... ***");
        
        camera_trigger_event_t trigger = {
            .speed_kmh = speed,
            .vehicle_type = sensor_data->vehicle_type
        };
        
        /* Subscreve ao canal de resultado antes de publicar trigger */
        zbus_chan_add_obs(&camera_result_chan, &camera_result_sub, K_NO_WAIT);
        
        /* Publica evento de trigger */
        if (zbus_chan_pub(&camera_trigger_chan, &trigger, K_MSEC(100)) == 0) {
            /* Aguarda resultado da camera (com timeout) */
            const struct zbus_channel *chan;
            
            if (zbus_sub_wait(&camera_result_sub, &chan, K_SECONDS(2)) == 0) {
                camera_result_event_t result;
                
                if (zbus_chan_read(chan, &result, K_MSEC(100)) == 0) {
                    if (result.valid) {
                        LOG_INF("OK Placa registrada: %s", result.plate);
                        LOG_INF("OK Infracao arquivada com sucesso!");
                    } else {
                        LOG_ERR("X Falha na captura da placa: %s", result.plate);
                        LOG_ERR("X Placa invalida - infracao nao registrada");
                    }
                }
            } else {
                LOG_ERR("Timeout aguardando resultado da camera");
            }
        }
    }
}

int main(void)
{
    sensor_data_msg_t sensor_msg;
    
    LOG_INF("╔════════════════════════════════════════╗");
    LOG_INF("║   RADAR ELETRONICO - INICIALIZANDO    ║");
    LOG_INF("╚════════════════════════════════════════╝");
    
    LOG_INF("Configuracoes:");
    LOG_INF("  - Distancia entre sensores: %d mm", CONFIG_RADAR_SENSOR_DISTANCE_MM);
    LOG_INF("  - Limite veiculos leves: %d km/h", CONFIG_RADAR_SPEED_LIMIT_LIGHT_KMH);
    LOG_INF("  - Limite veiculos pesados: %d km/h", CONFIG_RADAR_SPEED_LIMIT_HEAVY_KMH);
    LOG_INF("  - Limiar de alerta: %d%%", CONFIG_RADAR_WARNING_THRESHOLD_PERCENT);
    LOG_INF("  - Taxa de falha da camera: %d%%", CONFIG_RADAR_CAMERA_FAILURE_RATE_PERCENT);
    
    LOG_INF("\nSistema operacional - aguardando deteccoes...\n");
    
    /* Loop principal */
    while (1) {
        /* Aguarda dados dos sensores */
        if (k_msgq_get(&sensor_msgq, &sensor_msg, K_FOREVER) == 0) {
            process_vehicle_detection(&sensor_msg);
        }
    }

    return 0;
}
