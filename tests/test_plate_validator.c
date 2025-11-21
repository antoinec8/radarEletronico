/**
 * @file test_plate_validator.c
 * @brief Testes unitários para validação de placas Mercosul
 */

#include <zephyr/ztest.h>
#include "../src/utils/plate_validator.h"

/**
 * @brief Testa placas válidas
 */
ZTEST(plate_validator_tests, test_valid_plates)
{
    /* Placas válidas no formato Mercosul: ABC1D23 */
    zassert_true(validate_mercosul_plate("ABC1D23"), 
                 "ABC1D23 deve ser válida");
    zassert_true(validate_mercosul_plate("XYZ9A99"), 
                 "XYZ9A99 deve ser válida");
    zassert_true(validate_mercosul_plate("AAA0A00"), 
                 "AAA0A00 deve ser válida");
    zassert_true(validate_mercosul_plate("ZZZ9Z99"), 
                 "ZZZ9Z99 deve ser válida");
}

/**
 * @brief Testa placas inválidas
 */
ZTEST(plate_validator_tests, test_invalid_plates)
{
    /* Tamanho incorreto */
    zassert_false(validate_mercosul_plate("ABC123"), 
                  "ABC123 deve ser inválida (muito curta)");
    zassert_false(validate_mercosul_plate("ABC1D234"), 
                  "ABC1D234 deve ser inválida (muito longa)");
    zassert_false(validate_mercosul_plate(""), 
                  "String vazia deve ser inválida");
    
    /* Formato incorreto */
    zassert_false(validate_mercosul_plate("1BC1D23"), 
                  "Primeiro caractere deve ser letra");
    zassert_false(validate_mercosul_plate("A2C1D23"), 
                  "Segundo caractere deve ser letra");
    zassert_false(validate_mercosul_plate("AB31D23"), 
                  "Terceiro caractere deve ser letra");
    zassert_false(validate_mercosul_plate("ABCAD23"), 
                  "Quarto caractere deve ser dígito");
    zassert_false(validate_mercosul_plate("ABC1123"), 
                  "Quinto caractere deve ser letra");
    zassert_false(validate_mercosul_plate("ABC1DA3"), 
                  "Sexto caractere deve ser dígito");
    zassert_false(validate_mercosul_plate("ABC1D2A"), 
                  "Sétimo caractere deve ser dígito");
    
    /* NULL pointer */
    zassert_false(validate_mercosul_plate(NULL), 
                  "NULL deve ser inválido");
}

/**
 * @brief Testa casos limítrofes
 */
ZTEST(plate_validator_tests, test_edge_cases)
{
    /* Letras minúsculas (ctype.h aceita, mas Mercosul é maiúscula) */
    zassert_true(validate_mercosul_plate("abc1d23"), 
                 "Minúsculas são aceitas por isalpha");
    
    /* Caracteres especiais */
    zassert_false(validate_mercosul_plate("AB@1D23"), 
                  "@ não é letra");
    zassert_false(validate_mercosul_plate("ABC1D2#"), 
                  "# não é dígito");
    zassert_false(validate_mercosul_plate("ABC-D23"), 
                  "- não é dígito");
}

ZTEST_SUITE(plate_validator_tests, NULL, NULL, NULL, NULL, NULL);
