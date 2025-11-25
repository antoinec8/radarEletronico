/**
 * @file types.h
 * @brief Definições de tipos e estruturas de dados do radar
 * 
 * Este arquivo centraliza todos os tipos, enums e structs usados
 * para comunicação entre threads e representação de dados do sistema.
 */

#ifndef RADAR_TYPES_H
#define RADAR_TYPES_H

#include <zephyr/kernel.h>
#include <stdint.h>

/**
 * @brief Tipos de veículos detectados
 */
typedef enum {
    VEHICLE_TYPE_LIGHT = 0,  /**< Veículo leve (2 eixos) */
    VEHICLE_TYPE_HEAVY = 1,  /**< Veículo pesado (3+ eixos) */
} vehicle_type_t;

/**
 * @brief Status de velocidade para exibição
 */
typedef enum {
    SPEED_STATUS_NORMAL = 0,   /**< Velocidade normal (verde) */
    SPEED_STATUS_WARNING = 1,  /**< Velocidade próxima ao limite (amarelo) */
    SPEED_STATUS_VIOLATION = 2 /**< Infração de velocidade (vermelho) */
} speed_status_t;

/**
 * @brief Estados da máquina de estados de detecção
 */
typedef enum {
    SENSOR_STATE_IDLE = 0,           /**< Aguardando passagem */
    SENSOR_STATE_COUNTING_AXLES = 1, /**< Contando eixos no sensor 1 */
    SENSOR_STATE_MEASURING_SPEED = 2,/**< Medindo tempo entre sensores */
    SENSOR_STATE_COMPLETE = 3        /**< Detecção completa */
} sensor_state_t;

/**
 * @brief Mensagem de dados do sensor para thread principal
 * 
 * Enviada pela thread de sensores via fila quando uma
 * detecção completa é realizada.
 */
typedef struct {
    uint32_t time_delta_ms;      /**< Tempo entre sensores (ms) */
    vehicle_type_t vehicle_type; /**< Tipo de veículo detectado */
    uint8_t axle_count;          /**< Número de eixos contados */
} sensor_data_msg_t;

/**
 * @brief Mensagem para atualização do display
 * 
 * Enviada pela thread principal para thread de display
 */
typedef struct {
    uint32_t speed_kmh;           /**< Velocidade calculada (km/h) */
    vehicle_type_t vehicle_type;  /**< Tipo de veículo */
    speed_status_t status;        /**< Status da velocidade */
    uint32_t speed_limit;         /**< Limite aplicável */
    char plate[8];                /**< Placa capturada (vazio se não for infração) */
} display_data_msg_t;

/**
 * @brief Evento ZBUS para trigger da câmera
 * 
 * Publicado quando há infração
 */
typedef struct {
    uint32_t speed_kmh;           /**< Velocidade da infração */
    vehicle_type_t vehicle_type;  /**< Tipo de veículo */
} camera_trigger_event_t;

/**
 * @brief Resultado da captura de placa
 * 
 * Publicado pela câmera simulada via ZBUS
 */
typedef struct {
    char plate[8];      /**< Placa Mercosul (7 chars + \0) */
    bool valid;         /**< Se a captura foi bem-sucedida */
    uint64_t timestamp; /**< Timestamp da captura */
} camera_result_event_t;

#endif /* RADAR_TYPES_H */
