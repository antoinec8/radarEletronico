/**
 * @file plate_validator.h
 * @brief Validação de placas Mercosul
 * 
 * Valida o formato de placas brasileiras no padrão Mercosul (ABC1D23)
 */

#ifndef RADAR_PLATE_VALIDATOR_H
#define RADAR_PLATE_VALIDATOR_H

#include <stdbool.h>
#include <ctype.h>
#include <string.h>

/**
 * @brief Valida se uma placa segue o formato Mercosul
 * 
 * Formato Mercosul: ABC1D23
 * - 3 letras (A-Z)
 * - 1 dígito (0-9)
 * - 1 letra (A-Z)
 * - 2 dígitos (0-9)
 * 
 * @param plate String com a placa (deve ter 7 caracteres + \0)
 * @return true se válida, false caso contrário
 */
static inline bool validate_mercosul_plate(const char *plate)
{
    if (plate == NULL) {
        return false;
    }
    
    /* Verifica o comprimento */
    if (strlen(plate) != 7) {
        return false;
    }
    
    /* Posições 0, 1, 2: devem ser letras (A-Z) */
    if (!isalpha((unsigned char)plate[0]) || 
        !isalpha((unsigned char)plate[1]) || 
        !isalpha((unsigned char)plate[2])) {
        return false;
    }
    
    /* Posição 3: deve ser dígito (0-9) */
    if (!isdigit((unsigned char)plate[3])) {
        return false;
    }
    
    /* Posição 4: deve ser letra (A-Z) */
    if (!isalpha((unsigned char)plate[4])) {
        return false;
    }
    
    /* Posições 5, 6: devem ser dígitos (0-9) */
    if (!isdigit((unsigned char)plate[5]) || 
        !isdigit((unsigned char)plate[6])) {
        return false;
    }
    
    return true;
}

#endif /* RADAR_PLATE_VALIDATOR_H */
