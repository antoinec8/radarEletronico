# Guia de Início Rápido - Radar Eletrônico

## ⚡ Quick Start

### 1. Compilar

```powershell
cd c:\zephyrproject\radar_eletronico
west build -b mps2/an385 -p auto
```

### 2. Executar

```powershell
west build -t run
```

### 3. Testar

```powershell
cd tests
west build -b mps2/an385 -p auto
west build -t run
```

## 🎯 Comandos Úteis

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

## 🧪 Simulação Rápida

### Usando Python Script

```powershell
# Veículo leve a 50 km/h
python simulate_vehicle.py --type light --speed 50

# Veículo pesado a 70 km/h
python simulate_vehicle.py --type heavy --speed 70
```

### Manual (Monitor QEMU)

1. Execute: `west build -t run`
2. Pressione `Ctrl+A` depois `C`
3. Digite os comandos mostrados pelo script

## 📊 Cenários de Teste

### Cenário 1: Velocidade Normal (Verde)

```python
# Leve a 50 km/h (limite: 60)
python simulate_vehicle.py --type light --speed 50
```

**Resultado Esperado**: Display verde, sem infração

### Cenário 2: Velocidade de Alerta (Amarelo)

```python
# Leve a 56 km/h (90% de 60 = 54)
python simulate_vehicle.py --type light --speed 56
```

**Resultado Esperado**: Display amarelo, sem infração

### Cenário 3: Infração (Vermelho)

```python
# Leve a 70 km/h (limite: 60)
python simulate_vehicle.py --type light --speed 70
```

**Resultado Esperado**: Display vermelho, câmera acionada, placa capturada

### Cenário 4: Veículo Pesado

```python
# Pesado a 50 km/h (limite: 40)
python simulate_vehicle.py --type heavy --speed 50
```

**Resultado Esperado**: Display vermelho (infração para pesado)

## 🔧 Troubleshooting

### Erro de Compilação

**Problema**: `GPIO device not ready`

**Solução**: Verifique se o overlay está sendo carregado:
```powershell
west build -b mps2/an385 -p auto -- -DDTC_OVERLAY_FILE=mps2_an385.overlay
```

### Sem Output

**Problema**: Nenhuma mensagem aparece

**Solução**: Habilite logs imediatos no `prj.conf`:
```
CONFIG_LOG_MODE_IMMEDIATE=y
```

### Testes Falham

**Problema**: Testes não passam

**Solução**: 
1. Verifique se está no diretório `tests/`
2. Recompile do zero:
```powershell
Remove-Item -Recurse -Force build
west build -b mps2/an385
west build -t run
```

**Dúvidas?** Consulte o README.md completo!
