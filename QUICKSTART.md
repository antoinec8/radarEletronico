# Guia de Início Rápido - Radar Eletrônico

## Quick Start

### 1. Testar

```powershell
# Compilar os testes
west build -b mps2/an385 -p auto tests

# Executar os testes
west build -t run

# Voltar ao projeto principal
west build -b mps2/an385 -p always
```

### 2. Compilar o Projeto

```powershell
west build -b mps2/an385
```

### 3. Executar

```powershell
west build -t run
```

**Pronto!** O sistema possui simulação automática que gera veículos a cada 3 segundos.

## Comandos Úteis

### Limpar Build

```powershell
Remove-Item -Recurse -Force build
```

### Menuconfig

```powershell
west build -t menuconfig
```

### Ver Logs Detalhados

Edite `prj.conf` e altere:
```
CONFIG_LOG_DEFAULT_LEVEL=4
```

Recompile:
```powershell
west build
west build -t run
```

## Simulação

### Simulação Automática (Embutida)

O sistema **já possui simulação automática**! Ao executar `west build -t run`, você verá:

- 🟢 Veículo leve a 50 km/h (NORMAL)
- 🟡 Veículo leve a 56 km/h (ALERTA)
- 🔴 Veículo leve a 70 km/h (INFRAÇÃO) → Aciona câmera
- 🔴 Veículo pesado a 50 km/h (INFRAÇÃO) → Aciona câmera

Um novo veículo aparece **3 segundos**.

### Script Python (Apenas Informativo)

O script `simulate_vehicle.py` é útil apenas se você tiver **hardware físico**:

```powershell
# Calcula timings para hardware real
python simulate_vehicle.py --type light --speed 70
```

Como estamos usando QEMU com simulação automática, **não é necessário usar este script**!

## O Que Observar

### Veículos Simulados

A cada 3 segundos você verá uma dessas **4 detecções**:

1. **Leve 50 km/h** → 🟢 NORMAL (sem câmera)
2. **Leve 56 km/h** → 🟡 ALERTA (sem câmera)
3. **Leve 70 km/h** → 🔴 INFRAÇÃO (aciona câmera, captura placa)
4. **Pesado 50 km/h** → 🔴 INFRAÇÃO (aciona câmera, captura placa)

### Câmera e Placas

Quando há infração, a câmera é acionada e pode:
- ✅ **82%** - Capturar placa válida Mercosul:
  - Brasil (BR) (71.9%)
  - Uruguai (UY) (13.7%)
  - Argentina (AR) (7.2%)
  - Paraguai (PY) (7.2%)
- ❌ **9%** - Capturar placa formato inválido (rejeitada, não registra)
- 🔴 **9%** - Falhar (erro ERR-16 mostrado em vermelho)

## 🔧 Troubleshooting

### Erro de Compilação

**Problema**: `camera_service not found`

**Solução**: Verifique se o módulo externo está no lugar correto:
```powershell
# Estrutura esperada:
# c:\zephyrproject\camera_service\camera_service\
```

### Display Desalinhado

**Problema**: Bordas do quadrado tortas

**Solução**: Use terminal com suporte a ANSI (PowerShell, Windows Terminal)

### Testes Falham

**Problema**: Testes não passam

**Solução**: 
```powershell
west build -b mps2/an385 -p auto -T tests
west build -t run
```
---