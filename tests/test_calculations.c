/**
 * @file test_calculations.c
 * @brief Testes unitários para funções de cálculo
 * 
 * Testa as funções:
 * - calculate_speed_kmh
 * - classify_vehicle
 * - determine_speed_status
 * - get_speed_limit
 */

#include <zephyr/ztest.h>
#include "../src/utils/calculations.h"

/**
 * @brief Testa cálculo de velocidade
 */
ZTEST(calculations_tests, test_calculate_speed_basic)
{
    /* Caso 1: 1000mm em 1000ms = 3.6 km/h */
    uint32_t speed = calculate_speed_kmh(1000, 1000);
    zassert_equal(speed, 3, "Velocidade deveria ser ~3 km/h");
    
    /* Caso 2: 1000mm em 100ms = 36 km/h */
    speed = calculate_speed_kmh(100, 1000);
    zassert_equal(speed, 36, "Velocidade deveria ser 36 km/h");
    
    /* Caso 3: 1000mm em 60ms ≈ 60 km/h */
    speed = calculate_speed_kmh(60, 1000);
    zassert_equal(speed, 60, "Velocidade deveria ser 60 km/h");
}

ZTEST(calculations_tests, test_calculate_speed_edge_cases)
{
    /* Caso de divisão por zero */
    uint32_t speed = calculate_speed_kmh(0, 1000);
    zassert_equal(speed, 0, "Divisão por zero deve retornar 0");
    
    /* Velocidade muito alta (fisicamente improvável, mas testa overflow) */
    speed = calculate_speed_kmh(10, 10000);
    zassert_true(speed > 0, "Velocidade alta deve ser calculada");
}

/**
 * @brief Testa classificação de veículos
 */
ZTEST(calculations_tests, test_classify_vehicle)
{
    /* Veículos leves: 2 eixos ou menos */
    zassert_equal(classify_vehicle(1), VEHICLE_TYPE_LIGHT, 
                  "1 eixo = veículo leve");
    zassert_equal(classify_vehicle(2), VEHICLE_TYPE_LIGHT, 
                  "2 eixos = veículo leve");
    
    /* Veículos pesados: 3 ou mais eixos */
    zassert_equal(classify_vehicle(3), VEHICLE_TYPE_HEAVY, 
                  "3 eixos = veículo pesado");
    zassert_equal(classify_vehicle(4), VEHICLE_TYPE_HEAVY, 
                  "4 eixos = veículo pesado");
    zassert_equal(classify_vehicle(10), VEHICLE_TYPE_HEAVY, 
                  "10 eixos = veículo pesado");
}

/**
 * @brief Testa determinação do status de velocidade
 */
ZTEST(calculations_tests, test_determine_speed_status)
{
    uint32_t limit = 60;
    uint32_t warning_threshold = 90; /* 90% de 60 = 54 km/h */
    
    /* Velocidade normal (abaixo do alerta) */
    speed_status_t status = determine_speed_status(50, limit, warning_threshold);
    zassert_equal(status, SPEED_STATUS_NORMAL, "50 km/h deve ser normal");
    
    /* Velocidade de alerta (entre 90% e 100%) */
    status = determine_speed_status(55, limit, warning_threshold);
    zassert_equal(status, SPEED_STATUS_WARNING, "55 km/h deve ser alerta");
    
    /* Velocidade de infração (>= limite) */
    status = determine_speed_status(60, limit, warning_threshold);
    zassert_equal(status, SPEED_STATUS_VIOLATION, "60 km/h deve ser infração");
    
    status = determine_speed_status(80, limit, warning_threshold);
    zassert_equal(status, SPEED_STATUS_VIOLATION, "80 km/h deve ser infração");
}

/**
 * @brief Testa obtenção do limite correto
 */
ZTEST(calculations_tests, test_get_speed_limit)
{
    uint32_t light_limit = 60;
    uint32_t heavy_limit = 40;
    
    /* Veículo leve deve retornar limite de leve */
    uint32_t limit = get_speed_limit(VEHICLE_TYPE_LIGHT, light_limit, heavy_limit);
    zassert_equal(limit, 60, "Limite para leve deve ser 60");
    
    /* Veículo pesado deve retornar limite de pesado */
    limit = get_speed_limit(VEHICLE_TYPE_HEAVY, light_limit, heavy_limit);
    zassert_equal(limit, 40, "Limite para pesado deve ser 40");
}

ZTEST_SUITE(calculations_tests, NULL, NULL, NULL, NULL, NULL);
