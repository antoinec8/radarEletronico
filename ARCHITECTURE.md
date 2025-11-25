# Arquitetura Detalhada do Sistema

## Diagrama de Fluxo de Dados

```
┌─────────────────────────────────────────────────────────────────┐
│                      SISTEMA RADAR ELETRONICO                    │
└─────────────────────────────────────────────────────────────────┘

    SENSORES (GPIO)              THREADS (6)                SAIDAS
    ===============              ===========                ======

┌──────────────┐
│   GPIO 5     │──► Interrupcao
│  (Sensor 1)  │    (Conta eixos)
└──────────────┘         │
                         ▼
                  ┌─────────────┐
                  │   SENSOR    │
                  │   THREAD    │──────► sensor_msgq
                  └─────────────┘         (k_msgq)
┌──────────────┐         │                    │
│   GPIO 6     │──► Interrupcao               │
│  (Sensor 2)  │    (Mede tempo)              │
└──────────────┘                              ▼
                                       ┌─────────────┐
                                       │    MAIN     │
                                       │   THREAD    │
                                       └─────────────┘
                                              │
                                    ┌─────────┼─────────┐
                                    ▼         ▼         ▼
                            display_msgq  ZBUS(trigger) Calculos
                                 │           │
                                 ▼           ▼
                          ┌─────────┐  ┌──────────────┐
                          │DISPLAY  │  │CAMERA_INTEGR.│
                          │ THREAD  │  │   THREAD     │
                          └─────────┘  └──────────────┘
                                 │           │
                                 ▼           ▼
                           Console      ┌──────────────┐
                          (c/ cores)    │CAMERA_EVT_   │
                                        │PROCESSOR     │
                                        └──────────────┘
                                              │
                                              ▼
                                        ┌──────────────┐
                                        │CAMERA_THREAD │◄─── Modulo Externo
                                        │(camera_      │     github.com/
                                        │ service)     │     ecom042/
                                        └──────────────┘     camera_service
                                              │
                                              ▼
                                        ZBUS(result)
                                        MSG_SUBSCRIBER
                                              │
                                              ▼
                                        ┌─────────┐
                                        │  MAIN   │
                                        │ (valida)│
                                        └─────────┘
                                             │
                                             ▼
                                      Log/Console
                                      (ERR-16 em vermelho)
```

## Fluxo de Detecção Completo

### Passo 1: Detecção de Eixos
```
GPIO 5 (Sensor 1) → IRQ → sensor1_callback()
│
├─ Estado IDLE → COUNTING_AXLES (axle_count = 1)
├─ Estado COUNTING_AXLES → axle_count++ (se < timeout)
└─ Registra sensor1_last_trigger
```

### Passo 2: Detecção de Passagem
```
GPIO 6 (Sensor 2) → IRQ → sensor2_callback()
│
├─ Estado COUNTING_AXLES → MEASURING_SPEED
└─ Estado MEASURING_SPEED → Calcula time_delta
                          → Envia sensor_data_msg_t
                          → Estado IDLE
```

### Passo 3: Processamento
```
Main Thread recebe sensor_data_msg_t
│
├─ Calcula velocidade: calculate_speed_kmh()
├─ Classifica veículo: classify_vehicle()
├─ Determina limite: get_speed_limit()
├─ Determina status: determine_speed_status()
│
└─ Envia display_data_msg_t → Display Thread
```

### Passo 4: Exibição
```
Display Thread recebe display_data_msg_t
│
├─ Formata string com cores ANSI
├─ Escolhe cor: Verde/Amarelo/Vermelho
└─ Exibe no console (printk)
```

### Passo 5: Infracao (se aplicavel)
```
Se status == VIOLATION:
│
├─ Main cria camera_trigger_event_t
├─ Publica no camera_trigger_chan (ZBUS)
│
├─ Camera Integration Thread processa
│   └─ Aciona camera_service (modulo externo)
│
├─ Camera Thread (modulo) processa
│   ├─ Simula captura (delay 0-64ms)
│   ├─ Seleciona placa do banco (630 placas)
│   │   ├─ 82% validas (139 Mercosul: BR/AR/PY/UY)
│   │   ├─ 9% invalidas (formato incorreto)
│   │   └─ 9% erro (ERR-16)
│   └─ Publica camera_result_event_t
│
├─ Camera Event Processor recebe (MSG_SUBSCRIBER)
│   └─ Processa resultado
│
└─ Main recebe resultado
    ├─ Valida placa: validate_mercosul_plate()
    │   ├─ Brasil: ABC1D23 (3L-1N-1L-2N)
    │   ├─ Argentina: AB123CD (2L-3N-2L)
    │   ├─ Paraguai: ABCD123 (4L-3N)
    │   └─ Uruguai: ABC1234 (3L-4N)
    ├─ Atualiza display:
    │   ├─ Placa valida: mostra placa
    │   ├─ Codigo erro: mostra ERR-16 (vermelho)
    │   └─ Formato invalido: log apenas
    └─ Log: Sucesso ou Falha
```

## Prioridades de Threads (6 threads)

```
Prioridade  Thread                  Justificativa
==========  ======                  =============
    5       Sensor                  Alta: Resposta rapida a IRQs
    6       Camera Integration      Media-Alta: Trigger de camera
    6       Camera Event Processor  Media-Alta: Processa resultados (MSG_SUBSCRIBER)
    7       Camera Thread           Media-Alta: Modulo externo (camera_service)
    7       Display                 Media-Baixa: Apresentacao visual
   default  Main                    Padrao: Orquestracao geral
```

## Modulo Externo: camera_service

```
Origem: github.com/ecom042/camera_service
Integracao: ZBUS MSG_SUBSCRIBER (bloqueante)

Banco de Dados:
  - Total: 630 placas
  - Mercosul validas: 139 placas
    ├─ Brasil: 100 placas (71.9%)
    ├─ Uruguai: 19 placas (13.7%)
    ├─ Argentina: 10 placas (7.2%)
    └─ Paraguai: 10 placas (7.2%)

Distribuicao de Resultados:
  - 82% placas validas (formato correto)
  - 9% placas invalidas (formato incorreto)
  - 9% codigos de erro (ERR-16)

Performance:
  - Delay de captura: 0-64ms (aleatorio)
  - Tempo de resposta: <100ms tipico
```

## Estruturas de Dados

### sensor_data_msg_t
```c
┌────────────────────┐
│ time_delta_ms      │ → uint32_t (tempo entre sensores)
│ vehicle_type       │ → enum (LIGHT/HEAVY)
│ axle_count         │ → uint8_t (número de eixos)
└────────────────────┘
```

### display_data_msg_t
```c
┌────────────────────┐
│ speed_kmh          │ → uint32_t (velocidade calculada)
│ vehicle_type       │ → enum (LIGHT/HEAVY)
│ status             │ → enum (NORMAL/WARNING/VIOLATION)
│ speed_limit        │ → uint32_t (limite aplicável)
└────────────────────┘
```

## Configuracoes Kconfig → Comportamento

```
CONFIG_RADAR_SENSOR_DISTANCE_MM = 2700
    ↓
    Usado em: calculate_speed_kmh()
    Efeito: Maior distancia = mesma velocidade com mais tempo
    Calculo: velocidade = (distancia * 3600) / (tempo_ms * 1000)


CONFIG_RADAR_SPEED_LIMIT_LIGHT_KMH = 60
    ↓
    Usado em: get_speed_limit() quando vehicle_type == LIGHT
    Efeito: Threshold para VIOLATION


CONFIG_RADAR_WARNING_THRESHOLD_PERCENT = 90
    ↓
    Usado em: determine_speed_status()
    Efeito: 90% do limite = AMARELO, 100%+ = VERMELHO


CONFIG_RADAR_AXLE_TIMEOUT_MS = 2000
    ↓
    Usado em: sensor_thread.c validacao de eixos
    Efeito: Timeout dinamico ~662ms = (2700mm × 3600) / (60 km/h × 1000) + 500ms
    Nota: Valor real calculado em runtime, diferente do config


Simulacao (sensor_thread.c):
    ↓
    Intervalo: K_SECONDS(3) - gera veiculos a cada 3 segundos
    Ciclo: 4 veiculos por rodada
      ├─ 50 km/h LIGHT (NORMAL)
      ├─ 56 km/h LIGHT (WARNING)
      ├─ 70 km/h LIGHT (VIOLATION)
      └─ 50 km/h HEAVY (VIOLATION - limite 40)
```

## Cores ANSI e Estados

```
Verde   (#32m) → SPEED_STATUS_NORMAL
    │
    └─ Condicao: speed < (limit * threshold / 100)
    └─ Exemplo: 50 km/h com limite 60 e threshold 90 (54)

Amarelo (#33m) → SPEED_STATUS_WARNING
    │
    └─ Condicao: (limit * threshold / 100) ≤ speed < limit
    └─ Exemplo: 55 km/h com limite 60 e threshold 90

Vermelho(#31m) → SPEED_STATUS_VIOLATION
    │
    └─ Condicao: speed ≥ limit
    └─ Exemplo: 65 km/h com limite 60
    └─ Acao: Aciona camera via ZBUS

Vermelho(#31m) → Codigo de Erro (ERR-16)
    │
    └─ Usado quando camera retorna erro
    └─ Exibido no campo de placa
```

## Formato de Display

```
┌────────────────────────────────────────┐
│ Tipo: LEVE      Vel: 65 km/h           │
│ Limite: 60 km/h Status: VIOLACAO       │
│ Placa: ABC1D23                         │
└────────────────────────────────────────┘

Caracteristicas:
  - Alinhamento perfeito: 40 caracteres por linha
  - Pre-formatacao de strings ANTES dos codigos ANSI
  - Campo "Limite" inclui "km/h"
  - Placas de erro mostradas como "ERR-16" em vermelho
  - Sem acentos ou caracteres especiais
```

## Validacao de Placas Mercosul (4 Paises)

### Brasil (validate_brazil_plate)
```
Formato: ABC1D23 (3L-1N-1L-2N)
         ││││││└─ [6] Digito (0-9)
         │││││└── [5] Digito (0-9)
         ││││└─── [4] Letra (A-Z)
         │││└──── [3] Digito (0-9)
         ││└───── [2] Letra (A-Z)
         │└────── [1] Letra (A-Z)
         └─────── [0] Letra (A-Z)

Validacao:
    ├─ Comprimento == 7
    ├─ isalpha(plate[0,1,2,4])
    └─ isdigit(plate[3,5,6])

Exemplos: ABC1D23, XYZ9W88, LMN5T99
```

### Argentina (validate_argentina_plate)
```
Formato: AB123CD (2L-3N-2L)
         ││││││└─ [6] Letra (A-Z)
         │││││└── [5] Letra (A-Z)
         ││││└─── [4] Digito (0-9)
         │││└──── [3] Digito (0-9)
         ││└───── [2] Digito (0-9)
         │└────── [1] Letra (A-Z)
         └─────── [0] Letra (A-Z)

Validacao:
    ├─ Comprimento == 7
    ├─ isalpha(plate[0,1,5,6])
    └─ isdigit(plate[2,3,4])

Exemplos: AB123CD, XY456ZW, LM789NO
```

### Paraguai (validate_paraguay_plate)
```
Formato: ABCD123 (4L-3N)
         ││││││└─ [6] Digito (0-9)
         │││││└── [5] Digito (0-9)
         ││││└─── [4] Digito (0-9)
         │││└──── [3] Letra (A-Z)
         ││└───── [2] Letra (A-Z)
         │└────── [1] Letra (A-Z)
         └─────── [0] Letra (A-Z)

Validacao:
    ├─ Comprimento == 7
    ├─ isalpha(plate[0,1,2,3])
    └─ isdigit(plate[4,5,6])

Exemplos: ABCD123, WXYZ456, LMNO789
```

### Uruguai (validate_uruguay_plate)
```
Formato: ABC1234 (3L-4N)
         ││││││└─ [6] Digito (0-9)
         │││││└── [5] Digito (0-9)
         ││││└─── [4] Digito (0-9)
         │││└──── [3] Digito (0-9)
         ││└───── [2] Letra (A-Z)
         │└────── [1] Letra (A-Z)
         └─────── [0] Letra (A-Z)

Validacao:
    ├─ Comprimento == 7
    ├─ isalpha(plate[0,1,2])
    └─ isdigit(plate[3,4,5,6])

Exemplos: ABC1234, XYZ5678, LMN9012
```

### Auto-deteccao (validate_mercosul_plate)
```
Algoritmo:
    ├─ Se plate == NULL → false
    ├─ Se strlen(plate) != 7 → false
    ├─ Tenta validate_brazil_plate() → return se true
    ├─ Tenta validate_argentina_plate() → return se true
    ├─ Tenta validate_paraguay_plate() → return se true
    ├─ Tenta validate_uruguay_plate() → return se true
    └─ return false (nenhum formato valido)
```

## Pontos de Teste

### Testes Unitarios (ztest) - 12 testes
```
test_calculations.c:
✓ test_calculate_speed_basic      → Casos comuns
✓ test_calculate_speed_edge_cases → Div por zero, overflow
✓ test_classify_vehicle            → 1-10 eixos
✓ test_determine_speed_status      → 3 estados
✓ test_get_speed_limit             → LIGHT vs HEAVY

test_plate_validator.c:
✓ test_valid_brazil_plates         → ABC1D23, XYZ9W88
✓ test_valid_argentina_plates      → AB123CD, XY456ZW
✓ test_valid_paraguay_plates       → ABCD123, WXYZ456
✓ test_valid_uruguay_plates        → ABC1234, XYZ5678
✓ test_invalid_plates              → Tamanho, formato, NULL
✓ test_edge_cases                  → Minusculas, especiais
✓ test_mercosul_auto_detection     → Detecta pais automaticamente

Comandos:
  west build -b mps2/an385 -p auto tests
  west build -t run
  west build -b mps2/an385 -p always  # Retornar ao projeto

Resultado: 12/12 testes passando (100%)
```

### Testes de Integracao (Manual)
```
✓ Veiculo leve abaixo do limite     → Verde
✓ Veiculo leve proximo ao limite    → Amarelo
✓ Veiculo leve acima do limite      → Vermelho + Camera
✓ Veiculo pesado abaixo do limite   → Verde
✓ Veiculo pesado acima do limite    → Vermelho + Camera
✓ Timeout entre eixos               → Reset de estado
✓ Placas brasileiras                → ABC1D23 validado
✓ Placas argentinas                 → AB123CD validado
✓ Placas paraguaias                 → ABCD123 validado
✓ Placas uruguaias                  → ABC1234 validado
✓ Erro da camera                    → ERR-16 em vermelho
✓ Formato invalido                  → Log apenas, sem display
✓ Sucesso da camera                 → Placa exibida
```

## Metricas de Qualidade

- **Cobertura de Testes**: 100% (12/12 testes passando)
- **Complexidade Ciclomatica**: Baixa (funcoes pequenas)
- **Acoplamento**: Baixo (comunicacao via mensagens/ZBUS)
- **Coesao**: Alta (responsabilidades bem definidas)
- **Documentacao**: Comentarios Doxygen em todas as funcoes
- **Uso de Memoria**: FLASH 97780 B (2.33%), RAM 19424 B (0.46%)
- **Performance**: Camera 0-64ms, timeout dinamico ~662ms

## Estatisticas do Sistema

```
Threads: 6 (main, sensor, display, camera_integration,
            camera_evt_processor, camera_thread)

Canais ZBUS: 2 (camera_trigger_chan, camera_result_chan)

Filas de Mensagens: 2 (sensor_msgq, display_msgq)

Paises Mercosul: 4 (Brasil, Argentina, Paraguai, Uruguai)

Banco de Placas: 630 total, 139 Mercosul-validas
  - Brasil: 100 placas (71.9%)
  - Uruguai: 19 placas (13.7%)
  - Argentina: 10 placas (7.2%)
  - Paraguai: 10 placas (7.2%)

Taxa de Sucesso Camera: 82% validas, 9% invalidas, 9% erro

Intervalo de Simulacao: 3 segundos por ciclo

Distancia entre Sensores: 2700mm
```

---
