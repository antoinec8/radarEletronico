/**
 * @file display_thread.c
 * @brief Thread de atualização do display
 * 
 * Recebe dados da thread principal e exibe no Display Dummy
 * com formatação de cores ANSI (verde/amarelo/vermelho)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include "../types.h"

LOG_MODULE_REGISTER(display_thread, LOG_LEVEL_INF);

/* Códigos de cores ANSI para o console */
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_BOLD          "\x1b[1m"

/* Fila de mensagens do display */
extern struct k_msgq display_msgq;

/**
 * @brief Retorna o código de cor baseado no status
 */
static const char* get_color_code(speed_status_t status)
{
    switch (status) {
    case SPEED_STATUS_NORMAL:
        return ANSI_COLOR_GREEN;
    case SPEED_STATUS_WARNING:
        return ANSI_COLOR_YELLOW;
    case SPEED_STATUS_VIOLATION:
        return ANSI_COLOR_RED;
    default:
        return ANSI_COLOR_RESET;
    }
}

/**
 * @brief Retorna o texto do status
 */
static const char* get_status_text(speed_status_t status)
{
    switch (status) {
    case SPEED_STATUS_NORMAL:
        return "NORMAL";
    case SPEED_STATUS_WARNING:
        return "ALERTA";
    case SPEED_STATUS_VIOLATION:
        return "INFRACAO";
    default:
        return "DESCONHECIDO";
    }
}

/**
 * @brief Retorna o tipo do veículo como texto
 */
static const char* get_vehicle_type_text(vehicle_type_t type)
{
    return (type == VEHICLE_TYPE_LIGHT) ? "LEVE" : "PESADO";
}

/**
 * @brief Formata e exibe os dados no display
 */
static void display_data(const display_data_msg_t *data)
{
    const char *color = get_color_code(data->status);
    const char *status_text = get_status_text(data->status);
    const char *vehicle_text = get_vehicle_type_text(data->vehicle_type);
    
    /* Monta a mensagem formatada */
    char display_buffer[700];
    
    snprintf(display_buffer, sizeof(display_buffer),
             "\n"
             "+========================================+\n"
             "|      RADAR ELETRONICO - DETECCAO      |\n"
             "+========================================+\n"
             "| Tipo: %-8s                      |\n"
             "| Velocidade: %s%s%3u km/h%s                  |\n"
             "| Limite: %3u km/h                      |\n"
             "| Status: %s%s%-10s%s                |\n"
             "+========================================+\n",
             vehicle_text,
             ANSI_BOLD, color, data->speed_kmh, ANSI_COLOR_RESET,
             data->speed_limit,
             ANSI_BOLD, color, status_text, ANSI_COLOR_RESET);
    
    /* Exibe no console (Display Dummy mostra via LOG) */
    printk("%s", display_buffer);
    
    /* Log adicional para clareza */
    LOG_INF("%sVeiculo %s: %u km/h (Limite: %u) - Status: %s%s",
            color, vehicle_text, data->speed_kmh, 
            data->speed_limit, status_text, ANSI_COLOR_RESET);
}

/**
 * @brief Thread de display
 */
void display_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    display_data_msg_t msg;
    
    LOG_INF("Thread de display iniciada");
    
    /* Mensagem de boas-vindas */
    printk("\n");
    printk("+========================================+\n");
    printk("|    RADAR ELETRONICO INICIALIZADO      |\n");
    printk("|         Aguardando veiculos...        |\n");
    printk("+========================================+\n");
    printk("\n");
    
    while (1) {
        /* Aguarda mensagem da fila */
        if (k_msgq_get(&display_msgq, &msg, K_FOREVER) == 0) {
            display_data(&msg);
        }
    }
}

/* Definição da thread */
#define DISPLAY_THREAD_STACK_SIZE 2048
#define DISPLAY_THREAD_PRIORITY 7

K_THREAD_DEFINE(display_thread, DISPLAY_THREAD_STACK_SIZE,
                display_thread_entry, NULL, NULL, NULL,
                DISPLAY_THREAD_PRIORITY, 0, 0);
