# Changelog - Radar Eletronico

## [1.0.0] - 2024-11-XX

### Implementado
- Sistema completo de radar eletronico com 4 threads
- Simulacao de sensores (GPIO indisponiveis em QEMU)
- Calculo de velocidade e classificacao de veiculos
- Deteccao de infracoes (NORMAL/ALERTA/INFRACAO)
- Camera/LPR com comunicacao ZBUS
- Geracao de placas Mercosul validas (ABC1D23)
- Validacao de placas
- Display com cores ANSI e formatacao ASCII
- Taxa de falha configuravel para camera (20%)
- 5 parametros Kconfig configuravelveis
- 8 testes unitarios (calculations + plate validator)
- Documentacao completa (4 arquivos MD)

### Corrigido
- Board name: mps2_an385 → mps2/an385
- GPIO fallback para modo simulacao
- Display buffer: 256B → 700B
- ZBUS camera: LISTENER → SUBSCRIBER (fix timeout)
- Caracteres Unicode → ASCII (compatibilidade QEMU)

### Detalhes Tecnicos
- **Plataforma**: mps2/an385 (ARM Cortex-M3, QEMU)
- **Zephyr**: v4.2.0-4939-g727c15a03876
- **Memoria**: FLASH 40KB (0.95%), RAM 12KB (0.29%)
- **Threads**: 4 (main, sensor, display, camera)
- **Comunicacao**: k_msgq + ZBUS
- **Prioridades**: main=5, sensor=4, display=7, camera=6

### Placas Capturadas (exemplos)
- JSA1H12, NBX8Z80, JFE8E09, UMM6I18
- FOS7Z95, FGB5Z99, NSW8E34, BDC9R90
- YLI7B38, YFY6G56, ASX4Z55

### Falhas Conhecidas
- Camera duplica log "Camera acionada!" (sem impacto funcional)
- GPIO nao disponivel em QEMU (esperado)
