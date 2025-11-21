/**
 * @file sensor_thread.c
 * @brief Thread de monitoramento de sensores
 * 
 * Implementa máquina de estados para:
 * - Detectar passagem de veículos
 * - Contar eixos (classificação)
 * - Medir tempo entre sensores (velocidade)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "../types.h"

LOG_MODULE_REGISTER(sensor_thread, LOG_LEVEL_DBG);

/* Configuração dos GPIOs - QEMU mps2/an385 */
/* Nota: Como a placa emulada pode não ter GPIO configurado,
 * vamos usar uma abordagem de simulação para demonstração */
#if DT_NODE_EXISTS(DT_NODELABEL(gpio0))
#define GPIO_NODE DT_NODELABEL(gpio0)
#define GPIO_AVAILABLE 1
#else
#define GPIO_AVAILABLE 0
#warning "GPIO0 não disponível - usando modo simulação"
#endif

#define SENSOR1_PIN 5  /* Sensor magnético 1 (conta eixos) */
#define SENSOR2_PIN 6  /* Sensor magnético 2 (marca fim) */

/* Timeouts */
#define AXLE_TIMEOUT_MS 2000  /* Timeout entre eixos do mesmo veículo */

/* Variáveis da máquina de estados */
static sensor_state_t current_state = SENSOR_STATE_IDLE;
static uint8_t axle_count = 0;
static int64_t last_axle_time = 0;
static int64_t sensor1_last_trigger = 0;
static int64_t sensor2_trigger_time = 0;

/* Dispositivo GPIO */
static const struct device *gpio_dev;

/* Fila de mensagens para thread principal */
extern struct k_msgq sensor_msgq;

/**
 * @brief Callback de interrupção do Sensor 1 (conta eixos)
 */
static void sensor1_callback(const struct device *dev, struct gpio_callback *cb, 
                             uint32_t pins)
{
    int64_t now = k_uptime_get();
    
    switch (current_state) {
    case SENSOR_STATE_IDLE:
        /* Primeiro eixo detectado - inicia contagem */
        LOG_DBG("SENSOR1: Primeiro eixo detectado");
        current_state = SENSOR_STATE_COUNTING_AXLES;
        axle_count = 1;
        last_axle_time = now;
        sensor1_last_trigger = now;
        break;
        
    case SENSOR_STATE_COUNTING_AXLES:
        /* Verifica se não foi timeout */
        if ((now - last_axle_time) > AXLE_TIMEOUT_MS) {
            /* Timeout - recomeça contagem */
            LOG_WRN("SENSOR1: Timeout entre eixos, reiniciando");
            axle_count = 1;
            sensor1_last_trigger = now;
        } else {
            /* Mais um eixo do mesmo veículo */
            axle_count++;
            LOG_DBG("SENSOR1: Eixo %d detectado", axle_count);
            sensor1_last_trigger = now;
        }
        last_axle_time = now;
        break;
        
    case SENSOR_STATE_MEASURING_SPEED:
        /* Ignora pulsos do sensor 1 enquanto aguarda sensor 2 */
        LOG_DBG("SENSOR1: Ignorando pulso (aguardando sensor 2)");
        break;
        
    default:
        break;
    }
}

/**
 * @brief Callback de interrupção do Sensor 2 (marca fim)
 */
static void sensor2_callback(const struct device *dev, struct gpio_callback *cb, 
                             uint32_t pins)
{
    int64_t now = k_uptime_get();
    
    switch (current_state) {
    case SENSOR_STATE_IDLE:
        /* Sensor 2 disparou sem sensor 1 - ignora */
        LOG_WRN("SENSOR2: Disparou sem passar pelo sensor 1 (ignorado)");
        break;
        
    case SENSOR_STATE_COUNTING_AXLES:
        /* Veículo chegou ao sensor 2 - calcula velocidade */
        LOG_DBG("SENSOR2: Veículo detectado, iniciando medição");
        current_state = SENSOR_STATE_MEASURING_SPEED;
        sensor2_trigger_time = now;
        break;
        
    case SENSOR_STATE_MEASURING_SPEED:
        /* Segundo sensor disparou - finaliza medição */
        uint32_t time_delta = (uint32_t)(now - sensor1_last_trigger);
        
        LOG_INF("=== Detecção Completa ===");
        LOG_INF("Eixos: %d", axle_count);
        LOG_INF("Tempo: %u ms", time_delta);
        
        /* Prepara mensagem para thread principal */
        sensor_data_msg_t msg = {
            .time_delta_ms = time_delta,
            .vehicle_type = (axle_count <= 2) ? VEHICLE_TYPE_LIGHT : VEHICLE_TYPE_HEAVY,
            .axle_count = axle_count
        };
        
        /* Envia para fila (não-bloqueante) */
        if (k_msgq_put(&sensor_msgq, &msg, K_NO_WAIT) != 0) {
            LOG_ERR("Fila de sensores cheia!");
        }
        
        /* Volta ao estado inicial */
        current_state = SENSOR_STATE_IDLE;
        axle_count = 0;
        break;
        
    default:
        break;
    }
}

/* Estruturas de callback */
static struct gpio_callback sensor1_cb_data;
static struct gpio_callback sensor2_cb_data;

/**
 * @brief Inicializa os GPIOs e interrupções
 */
static int init_sensors(void)
{
#if GPIO_AVAILABLE
    int ret;
    
    gpio_dev = DEVICE_DT_GET(GPIO_NODE);
    if (!device_is_ready(gpio_dev)) {
        LOG_WRN("GPIO device not ready - running in simulation mode");
        return -ENODEV;
    }
    
    /* Configura Sensor 1 (GPIO 5) como entrada com pull-down */
    ret = gpio_pin_configure(gpio_dev, SENSOR1_PIN, GPIO_INPUT | GPIO_PULL_DOWN);
    if (ret != 0) {
        LOG_WRN("Failed to configure SENSOR1_PIN - running in simulation mode");
        return ret;
    }
    
    /* Configura Sensor 2 (GPIO 6) como entrada com pull-down */
    ret = gpio_pin_configure(gpio_dev, SENSOR2_PIN, GPIO_INPUT | GPIO_PULL_DOWN);
    if (ret != 0) {
        LOG_WRN("Failed to configure SENSOR2_PIN - running in simulation mode");
        return ret;
    }
    
    /* Configura interrupção no Sensor 1 (borda de subida) */
    ret = gpio_pin_interrupt_configure(gpio_dev, SENSOR1_PIN, GPIO_INT_EDGE_RISING);
    if (ret != 0) {
        LOG_WRN("Failed to configure interrupt for SENSOR1 - running in simulation mode");
        return ret;
    }
    
    /* Configura interrupção no Sensor 2 (borda de subida) */
    ret = gpio_pin_interrupt_configure(gpio_dev, SENSOR2_PIN, GPIO_INT_EDGE_RISING);
    if (ret != 0) {
        LOG_WRN("Failed to configure interrupt for SENSOR2 - running in simulation mode");
        return ret;
    }
    
    /* Inicializa e adiciona callback do Sensor 1 */
    gpio_init_callback(&sensor1_cb_data, sensor1_callback, BIT(SENSOR1_PIN));
    gpio_add_callback(gpio_dev, &sensor1_cb_data);
    
    /* Inicializa e adiciona callback do Sensor 2 */
    gpio_init_callback(&sensor2_cb_data, sensor2_callback, BIT(SENSOR2_PIN));
    gpio_add_callback(gpio_dev, &sensor2_cb_data);
    
    LOG_INF("Sensores inicializados (GPIO %d e %d)", SENSOR1_PIN, SENSOR2_PIN);
    return 0;
#else
    LOG_INF("Modo simulação ativado (GPIO não disponível)");
    return 0;
#endif
}

/**
 * @brief Simula detecção de veículo (para teste sem GPIO)
 */
static void simulate_vehicle_detection(vehicle_type_t type, uint32_t speed_kmh)
{
    uint32_t time_delta = (CONFIG_RADAR_SENSOR_DISTANCE_MM * 3600) / (speed_kmh * 1000);
    uint8_t axles = (type == VEHICLE_TYPE_LIGHT) ? 2 : 3;
    
    LOG_INF("=== SIMULACAO: Veiculo detectado ===");
    LOG_INF("Tipo: %s, Velocidade: %u km/h", 
            type == VEHICLE_TYPE_LIGHT ? "LEVE" : "PESADO", speed_kmh);
    
    sensor_data_msg_t msg = {
        .time_delta_ms = time_delta,
        .vehicle_type = type,
        .axle_count = axles
    };
    
    k_msgq_put(&sensor_msgq, &msg, K_NO_WAIT);
}

/**
 * @brief Thread de sensores (apenas monitora timeouts)
 */
void sensor_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    LOG_INF("Thread de sensores iniciada");
    
    int sensor_status = init_sensors();
    if (sensor_status != 0) {
        LOG_WRN("GPIOs nao disponiveis - modo simulacao ativado");
        LOG_INF("Gerando deteccoes de teste automaticamente...\n");
        
        /* Modo simulação: gera detecções automáticas para demonstração */
        int demo_count = 0;
        while (1) {
            k_sleep(K_SECONDS(5));
            
            switch (demo_count % 4) {
            case 0:
                LOG_INF("\n>>> Simulando: Veiculo LEVE a 50 km/h (Normal)");
                simulate_vehicle_detection(VEHICLE_TYPE_LIGHT, 50);
                break;
            case 1:
                LOG_INF("\n>>> Simulando: Veiculo LEVE a 56 km/h (Alerta)");
                simulate_vehicle_detection(VEHICLE_TYPE_LIGHT, 56);
                break;
            case 2:
                LOG_INF("\n>>> Simulando: Veiculo LEVE a 70 km/h (Infracao)");
                simulate_vehicle_detection(VEHICLE_TYPE_LIGHT, 70);
                break;
            case 3:
                LOG_INF("\n>>> Simulando: Veiculo PESADO a 50 km/h (Infracao)");
                simulate_vehicle_detection(VEHICLE_TYPE_HEAVY, 50);
                break;
            }
            demo_count++;
        }
    }
    
    /* Loop principal: verifica timeout de eixos (apenas se GPIO disponível) */
    while (1) {
        k_sleep(K_MSEC(500));
        
        if (current_state == SENSOR_STATE_COUNTING_AXLES) {
            int64_t now = k_uptime_get();
            if ((now - last_axle_time) > AXLE_TIMEOUT_MS) {
                LOG_WRN("Timeout na contagem de eixos, resetando estado");
                current_state = SENSOR_STATE_IDLE;
                axle_count = 0;
            }
        }
    }
}

/* Definição da thread */
#define SENSOR_THREAD_STACK_SIZE 1024
#define SENSOR_THREAD_PRIORITY 5

K_THREAD_DEFINE(sensor_thread, SENSOR_THREAD_STACK_SIZE,
                sensor_thread_entry, NULL, NULL, NULL,
                SENSOR_THREAD_PRIORITY, 0, 0);
