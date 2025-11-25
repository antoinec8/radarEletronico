# Radar Eletrônico - Zephyr

Sistema de radar eletronico simplificado implementado com Zephyr RTOS na plataforma emulada `mps2/an385`.

## Descrição do Projeto

Este projeto simula um radar eletrônico capaz de:
- ✅ Detectar passagem de veículos usando sensores magnéticos simulados (GPIOs)
- ✅ Calcular velocidade baseado no tempo entre sensores
- ✅ Classificar veículos em Leve (2 eixos) ou Pesado (3+ eixos)
- ✅ Detectar infrações com limites de velocidade diferenciados
- ✅ Exibir dados no console com cores ANSI (verde/amarelo/vermelho)
- ✅ Capturar placas Mercosul simuladas em caso de infração
- ✅ Validar placas e registrar infrações

## Arquitetura

### Threads

O sistema utiliza **4 threads** principais:

1. **Thread Principal (main)**: Orquestra o sistema, recebe dados dos sensores, calcula velocidade, detecta infrações e coordena câmera
2. **Thread de Sensores**: Máquina de estados para contar eixos e medir tempo entre sensores via interrupções GPIO
3. **Thread de Display**: Formata e exibe dados no console com cores ANSI
4. **Thread de Câmera/LPR**: Simula captura de placas via ZBUS

### Comunicação Inter-Threads

- **Filas de Mensagens (k_msgq)**:
  - `sensor_msgq`: Sensores → Principal
  - `display_msgq`: Principal → Display
  
- **ZBUS**:
  - `camera_trigger_chan`: Principal → Câmera (trigger)
  - `camera_result_chan`: Câmera → Principal (resultado)

### Máquina de Estados (Sensores)

```
IDLE → COUNTING_AXLES → MEASURING_SPEED → COMPLETE → IDLE
  ↑                                                      |
  └──────────────────────────────────────────────────────┘
```

Estados:
- **IDLE**: Aguardando primeiro eixo
- **COUNTING_AXLES**: Contando eixos no sensor 1 (classificação)
- **MEASURING_SPEED**: Medindo tempo entre sensor 1 e sensor 2
- **COMPLETE**: Dados enviados, volta ao IDLE

## Configurações (Kconfig)

Todas configuráveis via `menuconfig`:

| Configuração | Padrão | Descrição |
|-------------|--------|-----------|
| `CONFIG_RADAR_SENSOR_DISTANCE_MM` | 1000 | Distância entre sensores (mm) |
| `CONFIG_RADAR_SPEED_LIMIT_LIGHT_KMH` | 60 | Limite para veículos leves (km/h) |
| `CONFIG_RADAR_SPEED_LIMIT_HEAVY_KMH` | 40 | Limite para veículos pesados (km/h) |
| `CONFIG_RADAR_WARNING_THRESHOLD_PERCENT` | 90 | % do limite para alerta amarelo |
| `CONFIG_RADAR_CAMERA_FAILURE_RATE_PERCENT` | 20 | Taxa de falha da câmera (0-100%) |

## Compilação e Execução

### Pré-requisitos

- Zephyr SDK instalado e configurado
- West tool configurado
- QEMU para emulação ARM

## Testes

### Executar Testes Unitários

```bash
# Compilar os testes
west build -b mps2/an385 -p auto tests

# Executar os testes
west build -t run
```

**IMPORTANTE**: Após rodar os testes, para voltar ao projeto principal:

```bash
# Opção 1: Recompilar do zero
west build -b mps2/an385 -p always

# Opção 2: Limpar build e recompilar
Remove-Item -Recurse -Force build
west build -b mps2/an385
```

**Total: 12 testes - todos passando** 

### Testes Implementados

- ✅ **test_calculations.c**: Testa funções de cálculo (5 testes)
  - Cálculo de velocidade (casos normais e edge cases)
  - Classificação de veículos
  - Determinação de status (normal/alerta/infração)
  - Seleção de limites

- ✅ **test_plate_validator.c**: Testa validação de placas Mercosul (7 testes)
  - Placas válidas Brasil (ABC1D23), Argentina (AB123CD)
  - Placas válidas Paraguai (ABCD123), Uruguai (ABC1234)
  - Placas inválidas (formato errado, tamanho)
  - Edge cases (NULL, caracteres especiais)

## Executar o Projeto

### Compilar o Projeto

```bash
west build -b mps2/an385
```

### Executar no QEMU

```bash
west build -t run
```

**Nota**: O sistema possui **simulação automática** que gera veículos a cada 3 segundos:
- Veículo leve a 50 km/h (NORMAL)
- Veículo leve a 56 km/h (ALERTA)
- Veículo leve a 70 km/h (INFRAÇÃO - aciona câmera)
- Veículo pesado a 50 km/h (INFRAÇÃO)

Não é necessário inserir comandos manualmente!

### Configurar via Menuconfig

```bash
west build -t menuconfig
```

Navegue até "Configuração do Radar Eletrônico" para alterar os parâmetros.

## Simulação

### Simulação Automática (Padrão)

O sistema possui **simulação automática embutida** que gera veículos a cada 3 segundos:

1. **Veículo leve a 50 km/h** → Status NORMAL (verde)
2. **Veículo leve a 56 km/h** → Status ALERTA (amarelo)
3. **Veículo leve a 70 km/h** → Status INFRAÇÃO (vermelho) + câmera
4. **Veículo pesado a 50 km/h** → Status INFRAÇÃO (vermelho) + câmera

Basta executar `west build -t run` e observar!

### Script Python (Hardware Real)

O script `simulate_vehicle.py` é útil apenas para **hardware físico** com sensores GPIO reais:

```bash
python simulate_vehicle.py --type light --speed 70
```

Ele calcula os timings corretos e gera comandos GPIO que você executaria manualmente.
**Nota**: Como estamos usando QEMU com simulação automática, este script é apenas informativo.

## Fórmulas e Lógica

### Cálculo de Velocidade

```
velocidade (km/h) = (distância_mm * 3600) / (tempo_ms * 1000)
```

Exemplo:
- Distância: 1000 mm = 1 m
- Tempo: 60 ms = 0.06 s
- Velocidade = (1 / 0.06) * 3.6 = 60 km/h

### Classificação de Veículos

- **Leve**: 2 eixos ou menos
- **Pesado**: 3 ou mais eixos

### Status de Velocidade

- **Verde (Normal)**: `velocidade < (limite * threshold%)`
- **Amarelo (Alerta)**: `limite * threshold% ≤ velocidade < limite`
- **Vermelho (Infração)**: `velocidade ≥ limite`

### Validação de Placa Mercosul

O sistema valida placas dos 4 países do Mercosul com formatos diferentes:

#### Brasil (BR): ABC1D23
- **Formato**: 3 Letras + 1 Dígito + 1 Letra + 2 Dígitos
- **Exemplo**: TEP9J01, VDX2C03
- **Posições**:
  - 0-2: Letras (A-Z)
  - 3: Dígito (0-9)
  - 4: Letra (A-Z)
  - 5-6: Dígitos (0-9)

#### Argentina (AR): AB123CD
- **Formato**: 2 Letras + 3 Dígitos + 2 Letras
- **Exemplo**: AC456FH, BD789KL
- **Posições**:
  - 0-1: Letras (A-Z)
  - 2-4: Dígitos (0-9)
  - 5-6: Letras (A-Z)

#### Paraguai (PY): ABCD123
- **Formato**: 4 Letras + 3 Dígitos
- **Exemplo**: WXYZ456, KLMN789
- **Posições**:
  - 0-3: Letras (A-Z)
  - 4-6: Dígitos (0-9)

#### Uruguai (UY): ABC1234
- **Formato**: 3 Letras + 4 Dígitos
- **Exemplo**: FQN1875, ABC5678
- **Posições**:
  - 0-2: Letras (A-Z)
  - 3-6: Dígitos (0-9)

**Detecção Automática**: O sistema detecta automaticamente o país baseado no comprimento e padrão da placa.

## Estrutura de Arquivos

```
radar_eletronico/
├── CMakeLists.txt
├── Kconfig
├── prj.conf
├── README.md
├── src/
│   ├── main.c                          # Thread principal
│   ├── types.h                         # Definições de tipos
│   ├── threads/
│   │   ├── sensor_thread.c             # Thread de sensores
│   │   ├── display_thread.c            # Thread de display
│   │   └── camera_thread.c             # Thread de câmera
│   └── utils/
│       ├── calculations.h              # Funções de cálculo
│       └── plate_validator.h           # Validação de placas
└── tests/
    ├── CMakeLists.txt
    ├── prj.conf
    ├── testcase.yaml
    ├── test_calculations.c             # Testes de cálculos
    └── test_plate_validator.c          # Testes de validação
```

## Feedback Visual

O sistema usa códigos ANSI para cores no console:

- 🟢 **Verde**: Velocidade normal (dentro do limite)
- 🟡 **Amarelo**: Velocidade de alerta (próxima ao limite)
- 🔴 **Vermelho**: Infração (acima do limite)

Exemplo de saída:

```
+========================================+
|        RADAR ELETRONICO                |
+========================================+
| Tipo:       LEVE                       |
| Velocidade: 70 km/h                    |
| Limite:     60 km/h                    |
| Status:     INFRACAO                   |
| Placa:      ABC1D23                    |
+========================================+

>>> INFRACAO REGISTRADA - Placa: ABC1D23 <<<
```

## Debugging

### Habilitar Logs Detalhados

No `prj.conf`, ajuste:
```
CONFIG_LOG_DEFAULT_LEVEL=4  # Debug level
```

### Visualizar Estado dos Sensores

Os logs mostram as detecções:
```
[00:00:15.234,000] <wrn> main: *** INFRACAO DETECTADA! Acionando camera... ***
[00:00:15.234,000] <inf> camera_thread: === CAPTURA INICIADA ===
[00:00:15.297,000] <inf> camera_thread: Placa capturada: ABC1D23 (Brasil)
[00:00:15.297,000] <wrn> main: >>> INFRACAO REGISTRADA - Placa: ABC1D23 <<<
```

## Autores

Projeto desenvolvido junto a disciplina de Sistemas Embarcados por Antonio Carlos Freitas Lopes e Miriã da Silva Moreira.

---