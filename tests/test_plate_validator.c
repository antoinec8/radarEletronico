/**
 * @file test_plate_validator.c
 * @brief Testes unitários para validação de placas Mercosul
 * 
 * Testa todos os formatos Mercosul:
 * - Brasil: ABC1D23
 * - Argentina: AB123CD
 * - Paraguai: ABCD123
 * - Uruguai: ABC1234
 */

#include <zephyr/ztest.h>
#include "../src/utils/plate_validator.h"

/**
 * @brief Testa placas BRASILEIRAS válidas (ABC1D23)
 */
ZTEST(plate_validator_tests, test_valid_brazil_plates)
{
    mercosul_country_t country;
    
    zassert_true(validate_mercosul_plate("ABC1D23", &country), 
                 "ABC1D23 deve ser válida");
    zassert_equal(country, COUNTRY_BRAZIL, "Deve identificar como Brasil");
    
    zassert_true(validate_mercosul_plate("XYZ9A99", &country), 
                 "XYZ9A99 deve ser válida");
    zassert_equal(country, COUNTRY_BRAZIL, "Deve identificar como Brasil");
    
    zassert_true(validate_mercosul_plate("AAA0A00", NULL), 
                 "AAA0A00 deve ser válida (sem identificar país)");
}

/**
 * @brief Testa placas ARGENTINAS válidas (AB123CD)
 */
ZTEST(plate_validator_tests, test_valid_argentina_plates)
{
    mercosul_country_t country;
    
    zassert_true(validate_mercosul_plate("AB123CD", &country), 
                 "AB123CD deve ser válida");
    zassert_equal(country, COUNTRY_ARGENTINA, "Deve identificar como Argentina");
    
    zassert_true(validate_mercosul_plate("XY999ZW", &country), 
                 "XY999ZW deve ser válida");
    zassert_equal(country, COUNTRY_ARGENTINA, "Deve identificar como Argentina");
}

/**
 * @brief Testa placas PARAGUAIAS válidas (ABCD123)
 */
ZTEST(plate_validator_tests, test_valid_paraguay_plates)
{
    mercosul_country_t country;
    
    zassert_true(validate_mercosul_plate("ABCD123", &country), 
                 "ABCD123 deve ser válida");
    zassert_equal(country, COUNTRY_PARAGUAY, "Deve identificar como Paraguai");
    
    zassert_true(validate_mercosul_plate("WXYZ999", &country), 
                 "WXYZ999 deve ser válida");
    zassert_equal(country, COUNTRY_PARAGUAY, "Deve identificar como Paraguai");
}

/**
 * @brief Testa placas URUGUAIAS válidas (ABC1234)
 */
ZTEST(plate_validator_tests, test_valid_uruguay_plates)
{
    mercosul_country_t country;
    
    zassert_true(validate_mercosul_plate("ABC1234", &country), 
                 "ABC1234 deve ser válida");
    zassert_equal(country, COUNTRY_URUGUAY, "Deve identificar como Uruguai");
    
    zassert_true(validate_mercosul_plate("XYZ9999", &country), 
                 "XYZ9999 deve ser válida");
    zassert_equal(country, COUNTRY_URUGUAY, "Deve identificar como Uruguai");
}

/**
 * @brief Testa placas inválidas (todos os formatos)
 */
ZTEST(plate_validator_tests, test_invalid_plates)
{
    mercosul_country_t country;
    
    /* Tamanho incorreto */
    zassert_false(validate_mercosul_plate("ABC123", &country), 
                  "ABC123 deve ser inválida (muito curta)");
    zassert_equal(country, COUNTRY_UNKNOWN, "País deve ser UNKNOWN");
    
    zassert_false(validate_mercosul_plate("ABC1D234", &country), 
                  "ABC1D234 deve ser inválida (muito longa)");
    
    zassert_false(validate_mercosul_plate("", &country), 
                  "String vazia deve ser inválida");
    
    /* Formato que não corresponde a nenhum país */
    zassert_false(validate_mercosul_plate("1234567", &country), 
                  "Apenas números é inválido");
    zassert_equal(country, COUNTRY_UNKNOWN, "País deve ser UNKNOWN");
    
    zassert_false(validate_mercosul_plate("ABCDEFG", &country), 
                  "Apenas letras é inválido");
    
    /* NULL pointer */
    zassert_false(validate_mercosul_plate(NULL, &country), 
                  "NULL deve ser inválido");
}

/**
 * @brief Testa casos limítrofes e ambiguidades
 */
ZTEST(plate_validator_tests, test_edge_cases)
{
    mercosul_country_t country;
    
    /* Letras minúsculas (ctype.h aceita) */
    zassert_true(validate_mercosul_plate("abc1d23", &country), 
                 "Minúsculas são aceitas por isalpha");
    zassert_equal(country, COUNTRY_BRAZIL, "Deve identificar como Brasil");
    
    /* Caracteres especiais */
    zassert_false(validate_mercosul_plate("AB@1D23", &country), 
                  "@ não é letra");
    zassert_false(validate_mercosul_plate("ABC1D2#", NULL), 
                  "# não é dígito");
    zassert_false(validate_mercosul_plate("ABC-D23", NULL), 
                  "- não é dígito");
}

/**
 * @brief Testa função auxiliar get_country_name
 */
ZTEST(plate_validator_tests, test_country_names)
{
    zassert_equal(strcmp(get_country_name(COUNTRY_BRAZIL), "Brasil"), 0,
                  "Nome do Brasil incorreto");
    zassert_equal(strcmp(get_country_name(COUNTRY_ARGENTINA), "Argentina"), 0,
                  "Nome da Argentina incorreto");
    zassert_equal(strcmp(get_country_name(COUNTRY_PARAGUAY), "Paraguai"), 0,
                  "Nome do Paraguai incorreto");
    zassert_equal(strcmp(get_country_name(COUNTRY_URUGUAY), "Uruguai"), 0,
                  "Nome do Uruguai incorreto");
    zassert_equal(strcmp(get_country_name(COUNTRY_UNKNOWN), "Desconhecido"), 0,
                  "Nome para desconhecido incorreto");
}

ZTEST_SUITE(plate_validator_tests, NULL, NULL, NULL, NULL, NULL);
