# Radar Eletronico - Zephyr RTOS

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![Platform](https://img.shields.io/badge/platform-mps2%2Fan385-blue.svg)]()
[![Zephyr](https://img.shields.io/badge/zephyr-v4.2.0-blue.svg)]()
[![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)]()


Sistema de radar eletronico simplificado implementado com Zephyr RTOS na plataforma emulada `mps2/an385`.

## 📋 Descricao do Projeto

Este projeto simula um radar eletrônico capaz de:
- ✅ Detectar passagem de veículos usando sensores magnéticos simulados (GPIOs)
- ✅ Calcular velocidade baseado no tempo entre sensores
- ✅ Classificar veículos em Leve (2 eixos) ou Pesado (3+ eixos)
- ✅ Detectar infrações com limites de velocidade diferenciados
- ✅ Exibir dados no console com cores ANSI (verde/amarelo/vermelho)
- ✅ Capturar placas Mercosul simuladas em caso de infração
- ✅ Validar placas e registrar infrações

## 🏗️ Arquitetura

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

## ⚙️ Configurações (Kconfig)

Todas configuráveis via `menuconfig`:

| Configuração | Padrão | Descrição |
|-------------|--------|-----------|
| `CONFIG_RADAR_SENSOR_DISTANCE_MM` | 1000 | Distância entre sensores (mm) |
| `CONFIG_RADAR_SPEED_LIMIT_LIGHT_KMH` | 60 | Limite para veículos leves (km/h) |
| `CONFIG_RADAR_SPEED_LIMIT_HEAVY_KMH` | 40 | Limite para veículos pesados (km/h) |
| `CONFIG_RADAR_WARNING_THRESHOLD_PERCENT` | 90 | % do limite para alerta amarelo |
| `CONFIG_RADAR_CAMERA_FAILURE_RATE_PERCENT` | 20 | Taxa de falha da câmera (0-100%) |

## 🚀 Compilação e Execução

### Pré-requisitos

- Zephyr SDK instalado e configurado
- West tool configurado
- QEMU para emulação ARM

### Compilar o Projeto

```bash
cd c:\zephyrproject\radar_eletronico
west build -b mps2/an385 -p auto
```

### Executar no QEMU

```bash
west build -t run
```

### Configurar via Menuconfig

```bash
west build -t menuconfig
```

Navegue até "Configuração do Radar Eletrônico" para alterar os parâmetros.

## 🧪 Testes

### Executar Testes Unitários

```bash
cd tests
west build -b mps2/an385 -p auto
west build -t run
```

### Testes Implementados

- ✅ **test_calculations.c**: Testa funções de cálculo
  - Cálculo de velocidade (casos normais e edge cases)
  - Classificação de veículos
  - Determinação de status (normal/alerta/infração)
  - Seleção de limites

- ✅ **test_plate_validator.c**: Testa validação de placas
  - Placas válidas no formato Mercosul (ABC1D23)
  - Placas inválidas (formato errado, tamanho)
  - Edge cases (NULL, caracteres especiais)

### Executar Todos os Testes com Twister

```bash
west twister -T tests -p mps2/an385
```

## 🎮 Simulação Manual

Para testar manualmente, você pode simular pulsos nos GPIOs usando o monitor QEMU:

1. Execute o projeto: `west build -t run`
2. No console QEMU, pressione `Ctrl+A` depois `C` para acessar o monitor
3. Simule pulsos:
   ```
   # Simular veículo leve (2 eixos) a 50 km/h
   # Sensor 1 - Eixo 1
   qom-set /machine/gpio gpio5 1
   qom-set /machine/gpio gpio5 0
   
   # Sensor 1 - Eixo 2
   qom-set /machine/gpio gpio5 1
   qom-set /machine/gpio gpio5 0
   
   # Sensor 2 (após ~72ms para 50 km/h)
   qom-set /machine/gpio gpio6 1
   qom-set /machine/gpio gpio6 0
   ```

**Nota**: Para facilitar a simulação, considere criar um script Python externo que se conecte ao QEMU via monitor interface.

## 📊 Fórmulas e Lógica

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

Formato: **ABC1D23**
- Posições 0-2: Letras (A-Z)
- Posição 3: Dígito (0-9)
- Posição 4: Letra (A-Z)
- Posições 5-6: Dígitos (0-9)

## 📁 Estrutura de Arquivos

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

## 🎨 Feedback Visual

O sistema usa códigos ANSI para cores no console:

- 🟢 **Verde**: Velocidade normal (dentro do limite)
- 🟡 **Amarelo**: Velocidade de alerta (próxima ao limite)
- 🔴 **Vermelho**: Infração (acima do limite)

Exemplo de saída:
```
╔════════════════════════════════════════╗
║      RADAR ELETRÔNICO - DETECÇÃO      ║
╠════════════════════════════════════════╣
║ Tipo: LEVE                            ║
║ Velocidade: 65 km/h                   ║  (em vermelho)
║ Limite: 60 km/h                       ║
║ Status: INFRAÇÃO                      ║  (em vermelho)
╚════════════════════════════════════════╝

✓ Placa registrada: ABC1D23
✓ Infração arquivada com sucesso!
```

## 🛠️ Boas Práticas Implementadas

1. **Separação de Responsabilidades**: Cada thread tem uma função específica
2. **Funções Puras**: Cálculos em funções testáveis sem efeitos colaterais
3. **Comunicação Assíncrona**: ZBUS para desacoplamento
4. **Tratamento de Erros**: Timeouts, validações e verificações de retorno
5. **Documentação**: Comentários Doxygen em todas as funções
6. **Testes Automatizados**: Cobertura de funções críticas com ztest
7. **Configurabilidade**: Kconfig para parâmetros ajustáveis
8. **Logs Estruturados**: Níveis de log apropriados (DBG, INF, WRN, ERR)

## 🔍 Debugging

### Habilitar Logs Detalhados

No `prj.conf`, ajuste:
```
CONFIG_LOG_DEFAULT_LEVEL=4  # Debug level
```

### Visualizar Estado dos Sensores

Os logs da thread de sensores mostram cada transição de estado:
```
[DBG] SENSOR1: Primeiro eixo detectado
[DBG] SENSOR1: Eixo 2 detectado
[DBG] SENSOR2: Veículo detectado, iniciando medição
[INF] === Detecção Completa ===
[INF] Eixos: 2
[INF] Tempo: 72 ms
```

## 🎯 Critérios de Avaliação Atendidos

- ✅ **Qualidade de Código**: Modular, comentado, seguindo convenções
- ✅ **Criatividade**: Máquina de estados, feedback visual colorido, simulação de falhas
- ✅ **Testes Automáticos**: Suite completa com ztest
- ✅ **Uso do Zephyr**: Multithreading, Kconfig, GPIO, Display, ZBUS
- ✅ **Git**: Commits organizados com mensagens descritivas

## 📝 Licença

Projeto acadêmico - Sistemas Embarcados com Zephyr RTOS

## 👨‍💻 Autores
Desenvolvido como projeto da disciplina de Sistemas Embarcados por Antonio Carlos Freitas Lopes e Miriã da Silva Moreira.


---

