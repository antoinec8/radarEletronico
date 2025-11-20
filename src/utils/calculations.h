/**
 * @file calculations.h
 * @brief Funções de cálculo do radar
 * 
 * Funções puras para cálculos de velocidade e classificação.
 * Ideais para testes unitários.
 */

#ifndef RADAR_CALCULATIONS_H
#define RADAR_CALCULATIONS_H

#include <stdint.h>
#include <stdbool.h>
#include "../types.h"

/**
 * @brief Calcula velocidade em km/h a partir do tempo e distância
 * 
 * Fórmula: velocidade = (distância / tempo) * 3600
 * 
 * @param time_delta_ms Tempo entre sensores em milissegundos
 * @param distance_mm Distância entre sensores em milímetros
 * @return Velocidade em km/h
 */
static inline uint32_t calculate_speed_kmh(uint32_t time_delta_ms, uint32_t distance_mm)
{
    if (time_delta_ms == 0) {
        return 0; /* Evita divisão por zero */
    }
    
    /* 
     * Velocidade = distância / tempo
     * km/h = (mm / ms) * (1 km / 1000000 mm) * (3600000 ms / 1 h)
     * km/h = (mm * 3600) / (ms * 1000)
     */
    uint64_t speed = ((uint64_t)distance_mm * 3600) / ((uint64_t)time_delta_ms * 1000);
    return (uint32_t)speed;
}

/**
 * @brief Classifica o veículo baseado no número de eixos
 * 
 * @param axle_count Número de eixos detectados
 * @return Tipo do veículo
 */
static inline vehicle_type_t classify_vehicle(uint8_t axle_count)
{
    return (axle_count <= 2) ? VEHICLE_TYPE_LIGHT : VEHICLE_TYPE_HEAVY;
}

/**
 * @brief Determina o status da velocidade (normal/alerta/infração)
 * 
 * @param speed_kmh Velocidade medida
 * @param speed_limit Limite de velocidade aplicável
 * @param warning_threshold_percent Percentual do limite para alerta (ex: 90)
 * @return Status da velocidade
 */
static inline speed_status_t determine_speed_status(uint32_t speed_kmh, 
                                                     uint32_t speed_limit,
                                                     uint32_t warning_threshold_percent)
{
    if (speed_kmh >= speed_limit) {
        return SPEED_STATUS_VIOLATION;
    }
    
    /* Calcula o limiar de alerta: limite * (threshold / 100) */
    uint32_t warning_threshold = (speed_limit * warning_threshold_percent) / 100;
    
    if (speed_kmh >= warning_threshold) {
        return SPEED_STATUS_WARNING;
    }
    
    return SPEED_STATUS_NORMAL;
}

/**
 * @brief Obtém o limite de velocidade baseado no tipo de veículo
 * 
 * @param vehicle_type Tipo do veículo
 * @param light_limit Limite para veículos leves (do Kconfig)
 * @param heavy_limit Limite para veículos pesados (do Kconfig)
 * @return Limite de velocidade aplicável
 */
static inline uint32_t get_speed_limit(vehicle_type_t vehicle_type,
                                        uint32_t light_limit,
                                        uint32_t heavy_limit)
{
    return (vehicle_type == VEHICLE_TYPE_LIGHT) ? light_limit : heavy_limit;
}

#endif /* RADAR_CALCULATIONS_H */
