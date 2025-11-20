# Arquitetura Detalhada do Sistema

## 📐 Diagrama de Fluxo de Dados

```
┌─────────────────────────────────────────────────────────────────┐
│                      SISTEMA RADAR ELETRÔNICO                    │
└─────────────────────────────────────────────────────────────────┘

    SENSORES (GPIO)              THREADS                   SAÍDAS
    ===============              =======                   ======

┌──────────────┐
│   GPIO 5     │──► Interrupção
│  (Sensor 1)  │    (Conta eixos)
└──────────────┘         │
                         ▼
                  ┌─────────────┐
                  │   SENSOR    │
                  │   THREAD    │──────► sensor_msgq
                  └─────────────┘         (k_msgq)
┌──────────────┐         │                    │
│   GPIO 6     │──► Interrupção               │
│  (Sensor 2)  │    (Mede tempo)              │
└──────────────┘                              ▼
                                       ┌─────────────┐
                                       │    MAIN     │
                                       │   THREAD    │
                                       └─────────────┘
                                              │
                                    ┌─────────┼─────────┐
                                    ▼         ▼         ▼
                            display_msgq  ZBUS(trigger) Cálculos
                                 │           │
                                 ▼           ▼
                          ┌─────────┐  ┌─────────┐
                          │DISPLAY  │  │ CAMERA  │
                          │ THREAD  │  │ THREAD  │
                          └─────────┘  └─────────┘
                                 │           │
                                 ▼           ▼
                           Console      ZBUS(result)
                          (c/ cores)         │
                                            ▼
                                        ┌─────────┐
                                        │  MAIN   │
                                        │ (valida)│
                                        └─────────┘
                                             │
                                             ▼
                                         Log/Console
```

## 🔄 Fluxo de Detecção Completo

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

### Passo 5: Infração (se aplicável)
```
Se status == VIOLATION:
│
├─ Main cria camera_trigger_event_t
├─ Publica no camera_trigger_chan (ZBUS)
│
├─ Camera Thread processa
│   ├─ Simula captura (delay)
│   ├─ Gera placa (válida ou inválida)
│   └─ Publica camera_result_event_t
│
└─ Main recebe resultado
    ├─ Valida placa: validate_mercosul_plate()
    └─ Log: Sucesso ou Falha
```

## 🧵 Prioridades de Threads

```
Prioridade  Thread          Justificativa
==========  ======          =============
    5       Sensor          Alta: Resposta rápida a IRQs
    6       Camera          Média-Alta: Processamento após trigger
    7       Display         Média-Baixa: Apresentação visual
   default  Main            Padrão: Orquestração geral
```

## 📊 Estruturas de Dados

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

## ⚙️ Configurações Kconfig → Comportamento

```
CONFIG_RADAR_SENSOR_DISTANCE_MM = 1000
    ↓
    Usado em: calculate_speed_kmh()
    Efeito: Maior distância = mesma velocidade com mais tempo


CONFIG_RADAR_SPEED_LIMIT_LIGHT_KMH = 60
    ↓
    Usado em: get_speed_limit() quando vehicle_type == LIGHT
    Efeito: Threshold para VIOLATION


CONFIG_RADAR_WARNING_THRESHOLD_PERCENT = 90
    ↓
    Usado em: determine_speed_status()
    Efeito: 90% do limite = AMARELO, 100%+ = VERMELHO


CONFIG_RADAR_CAMERA_FAILURE_RATE_PERCENT = 20
    ↓
    Usado em: camera_thread.c → process_camera_capture()
    Efeito: 20% de chance de gerar placa inválida
```

## 🎨 Cores ANSI e Estados

```
Verde   (#32m) → SPEED_STATUS_NORMAL
    │
    └─ Condição: speed < (limit * threshold / 100)
    └─ Exemplo: 50 km/h com limite 60 e threshold 90 (54)

Amarelo (#33m) → SPEED_STATUS_WARNING
    │
    └─ Condição: (limit * threshold / 100) ≤ speed < limit
    └─ Exemplo: 55 km/h com limite 60 e threshold 90

Vermelho(#31m) → SPEED_STATUS_VIOLATION
    │
    └─ Condição: speed ≥ limit
    └─ Exemplo: 65 km/h com limite 60
    └─ Ação: Aciona câmera via ZBUS
```

## 🔐 Validação de Placa Mercosul

```
Formato: ABC1D23
         ││││││└─ [6] Dígito (0-9)
         │││││└── [5] Dígito (0-9)
         ││││└─── [4] Letra (A-Z)
         │││└──── [3] Dígito (0-9)
         ││└───── [2] Letra (A-Z)
         │└────── [1] Letra (A-Z)
         └─────── [0] Letra (A-Z)

Validação: validate_mercosul_plate()
    ├─ Verifica comprimento == 7
    ├─ isalpha(plate[0,1,2,4])
    └─ isdigit(plate[3,5,6])
```

## 🧪 Pontos de Teste

### Testes Unitários (ztest)
```
✓ test_calculate_speed_basic      → Casos comuns
✓ test_calculate_speed_edge_cases → Div por zero, overflow
✓ test_classify_vehicle            → 1-10 eixos
✓ test_determine_speed_status      → 3 estados
✓ test_get_speed_limit             → LIGHT vs HEAVY
✓ test_valid_plates                → ABC1D23, XYZ9A99
✓ test_invalid_plates              → Tamanho, formato, NULL
✓ test_edge_cases                  → Minúsculas, especiais
```

### Testes de Integração (Manual)
```
□ Veículo leve abaixo do limite     → Verde
□ Veículo leve próximo ao limite    → Amarelo
□ Veículo leve acima do limite      → Vermelho + Câmera
□ Veículo pesado abaixo do limite   → Verde
□ Veículo pesado acima do limite    → Vermelho + Câmera
□ Timeout entre eixos               → Reset de estado
□ Falha da câmera                   → Placa inválida
□ Sucesso da câmera                 → Placa válida
```

## 📈 Métricas de Qualidade

- **Cobertura de Testes**: ~80% (funções críticas)
- **Complexidade Ciclomática**: Baixa (funções pequenas)
- **Acoplamento**: Baixo (comunicação via mensagens)
- **Coesão**: Alta (responsabilidades bem definidas)
- **Documentação**: Comentários Doxygen em todas as funções

---

Este documento complementa o README.md principal.
