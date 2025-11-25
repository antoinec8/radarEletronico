/**
 * @file plate_validator.h
 * @brief Validação de placas Mercosul
 * 
 * Valida formatos de placas de todos os países do Mercosul:
 * - Brasil: ABC1D23 (3L-1N-1L-2N)
 * - Argentina: AB123CD (2L-3N-2L)
 * - Paraguai: ABCD123 (4L-3N)
 * - Uruguai: ABC1234 (3L-4N)
 */

#ifndef RADAR_PLATE_VALIDATOR_H
#define RADAR_PLATE_VALIDATOR_H

#include <stdbool.h>
#include <ctype.h>
#include <string.h>

/**
 * @brief Enum para identificar país de origem da placa
 */
typedef enum {
    COUNTRY_UNKNOWN = 0,
    COUNTRY_BRAZIL,     /**< Brasil: ABC1D23 */
    COUNTRY_ARGENTINA,  /**< Argentina: AB123CD */
    COUNTRY_PARAGUAY,   /**< Paraguai: ABCD123 */
    COUNTRY_URUGUAY     /**< Uruguai: ABC1234 */
} mercosul_country_t;

/**
 * @brief Valida se uma placa segue o formato Mercosul BRASILEIRO
 * 
 * Formato Brasil: ABC1D23
 * - 3 letras (A-Z)
 * - 1 dígito (0-9)
 * - 1 letra (A-Z)
 * - 2 dígitos (0-9)
 * 
 * @param plate String com a placa (deve ter 7 caracteres + \0)
 * @return true se válida, false caso contrário
 */
static inline bool validate_brazil_plate(const char *plate)
{
    if (plate == NULL || strlen(plate) != 7) {
        return false;
    }
    
    /* Posições 0, 1, 2: letras */
    if (!isalpha((unsigned char)plate[0]) || 
        !isalpha((unsigned char)plate[1]) || 
        !isalpha((unsigned char)plate[2])) {
        return false;
    }
    
    /* Posição 3: dígito */
    if (!isdigit((unsigned char)plate[3])) {
        return false;
    }
    
    /* Posição 4: letra */
    if (!isalpha((unsigned char)plate[4])) {
        return false;
    }
    
    /* Posições 5, 6: dígitos */
    if (!isdigit((unsigned char)plate[5]) || 
        !isdigit((unsigned char)plate[6])) {
        return false;
    }
    
    return true;
}

/**
 * @brief Valida se uma placa segue o formato Mercosul ARGENTINO
 * 
 * Formato Argentina: AB123CD
 * - 2 letras (A-Z)
 * - 3 dígitos (0-9)
 * - 2 letras (A-Z)
 * 
 * @param plate String com a placa (deve ter 7 caracteres + \0)
 * @return true se válida, false caso contrário
 */
static inline bool validate_argentina_plate(const char *plate)
{
    if (plate == NULL || strlen(plate) != 7) {
        return false;
    }
    
    /* Posições 0, 1: letras */
    if (!isalpha((unsigned char)plate[0]) || 
        !isalpha((unsigned char)plate[1])) {
        return false;
    }
    
    /* Posições 2, 3, 4: dígitos */
    if (!isdigit((unsigned char)plate[2]) || 
        !isdigit((unsigned char)plate[3]) ||
        !isdigit((unsigned char)plate[4])) {
        return false;
    }
    
    /* Posições 5, 6: letras */
    if (!isalpha((unsigned char)plate[5]) || 
        !isalpha((unsigned char)plate[6])) {
        return false;
    }
    
    return true;
}

/**
 * @brief Valida se uma placa segue o formato Mercosul PARAGUAIO
 * 
 * Formato Paraguai: ABCD123
 * - 4 letras (A-Z)
 * - 3 dígitos (0-9)
 * 
 * @param plate String com a placa (deve ter 7 caracteres + \0)
 * @return true se válida, false caso contrário
 */
static inline bool validate_paraguay_plate(const char *plate)
{
    if (plate == NULL || strlen(plate) != 7) {
        return false;
    }
    
    /* Posições 0, 1, 2, 3: letras */
    if (!isalpha((unsigned char)plate[0]) || 
        !isalpha((unsigned char)plate[1]) ||
        !isalpha((unsigned char)plate[2]) || 
        !isalpha((unsigned char)plate[3])) {
        return false;
    }
    
    /* Posições 4, 5, 6: dígitos */
    if (!isdigit((unsigned char)plate[4]) || 
        !isdigit((unsigned char)plate[5]) ||
        !isdigit((unsigned char)plate[6])) {
        return false;
    }
    
    return true;
}

/**
 * @brief Valida se uma placa segue o formato Mercosul URUGUAIO
 * 
 * Formato Uruguai: ABC1234
 * - 3 letras (A-Z)
 * - 4 dígitos (0-9)
 * 
 * @param plate String com a placa (deve ter 7 caracteres + \0)
 * @return true se válida, false caso contrário
 */
static inline bool validate_uruguay_plate(const char *plate)
{
    if (plate == NULL || strlen(plate) != 7) {
        return false;
    }
    
    /* Posições 0, 1, 2: letras */
    if (!isalpha((unsigned char)plate[0]) || 
        !isalpha((unsigned char)plate[1]) ||
        !isalpha((unsigned char)plate[2])) {
        return false;
    }
    
    /* Posições 3, 4, 5, 6: dígitos */
    if (!isdigit((unsigned char)plate[3]) || 
        !isdigit((unsigned char)plate[4]) ||
        !isdigit((unsigned char)plate[5]) ||
        !isdigit((unsigned char)plate[6])) {
        return false;
    }
    
    return true;
}

/**
 * @brief Identifica o país de origem da placa e valida
 * 
 * @param plate String com a placa
 * @param country Ponteiro para receber o país identificado (pode ser NULL)
 * @return true se a placa é válida em algum formato Mercosul, false caso contrário
 */
static inline bool validate_mercosul_plate(const char *plate, mercosul_country_t *country)
{
    if (plate == NULL || strlen(plate) != 7) {
        if (country != NULL) {
            *country = COUNTRY_UNKNOWN;
        }
        return false;
    }
    
    /* Tenta validar em cada formato */
    if (validate_brazil_plate(plate)) {
        if (country != NULL) {
            *country = COUNTRY_BRAZIL;
        }
        return true;
    }
    
    if (validate_argentina_plate(plate)) {
        if (country != NULL) {
            *country = COUNTRY_ARGENTINA;
        }
        return true;
    }
    
    if (validate_paraguay_plate(plate)) {
        if (country != NULL) {
            *country = COUNTRY_PARAGUAY;
        }
        return true;
    }
    
    if (validate_uruguay_plate(plate)) {
        if (country != NULL) {
            *country = COUNTRY_URUGUAY;
        }
        return true;
    }
    
    if (country != NULL) {
        *country = COUNTRY_UNKNOWN;
    }
    return false;
}

/**
 * @brief Retorna o nome do país em string
 * 
 * @param country Enum do país
 * @return String com o nome do país
 */
static inline const char* get_country_name(mercosul_country_t country)
{
    switch (country) {
        case COUNTRY_BRAZIL:
            return "Brasil";
        case COUNTRY_ARGENTINA:
            return "Argentina";
        case COUNTRY_PARAGUAY:
            return "Paraguai";
        case COUNTRY_URUGUAY:
            return "Uruguai";
        default:
            return "Desconhecido";
    }
}

#endif /* RADAR_PLATE_VALIDATOR_H */
