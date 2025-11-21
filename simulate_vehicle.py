#!/usr/bin/env python3
"""
Script de Simulação de Veículos para Radar Eletrônico

Este script facilita a simulação de passagem de veículos
enviando comandos para o monitor QEMU.

Uso:
    python simulate_vehicle.py --type light --speed 50
    python simulate_vehicle.py --type heavy --speed 70
"""

import argparse
import time
import socket
import sys

def calculate_timing(speed_kmh, distance_mm=1000):
    """
    Calcula o tempo necessário entre sensores para uma velocidade específica.
    
    Args:
        speed_kmh: Velocidade desejada em km/h
        distance_mm: Distância entre sensores em mm
    
    Returns:
        Tempo em segundos
    """
    # Converte para m/s: km/h * (1000m/1km) * (1h/3600s)
    speed_ms = speed_kmh / 3.6
    # Tempo = distância / velocidade
    distance_m = distance_mm / 1000
    time_s = distance_m / speed_ms
    return time_s

def simulate_vehicle(vehicle_type='light', speed_kmh=50, qemu_monitor_host='localhost', 
                     qemu_monitor_port=55555):
    """
    Simula a passagem de um veículo.
    
    Args:
        vehicle_type: 'light' (2 eixos) ou 'heavy' (3 eixos)
        speed_kmh: Velocidade do veículo em km/h
        qemu_monitor_host: Host do monitor QEMU
        qemu_monitor_port: Porta do monitor QEMU
    """
    # Determina número de eixos
    axles = 2 if vehicle_type == 'light' else 3
    
    print(f"🚗 Simulando veículo {vehicle_type.upper()}")
    print(f"   Eixos: {axles}")
    print(f"   Velocidade: {speed_kmh} km/h")
    
    # Calcula timing
    sensor_delay = calculate_timing(speed_kmh)
    axle_interval = 0.1  # 100ms entre eixos
    
    print(f"   Tempo entre sensores: {sensor_delay*1000:.1f} ms\n")
    
    try:
        # Conecta ao monitor QEMU (se disponível)
        # Nota: QEMU precisa ser iniciado com -monitor tcp:localhost:55555,server,nowait
        print("Tentando conectar ao monitor QEMU...")
        print("(Certifique-se de que QEMU foi iniciado com -monitor tcp:localhost:55555,server,nowait)\n")
        
        # Por enquanto, apenas mostra os comandos
        print("═══ Comandos QEMU ═══")
        print("Execute estes comandos no monitor QEMU (Ctrl+A, C):\n")
        
        for i in range(axles):
            print(f"# Eixo {i+1}")
            print("qom-set /machine/gpio gpio5 1")
            print("qom-set /machine/gpio gpio5 0")
            if i < axles - 1:
                print(f"# Aguardar ~{axle_interval*1000:.0f}ms\n")
        
        print(f"\n# Aguardar ~{sensor_delay*1000:.0f}ms\n")
        print("# Sensor 2")
        print("qom-set /machine/gpio gpio6 1")
        print("qom-set /machine/gpio gpio6 0")
        print("\n═══════════════════════\n")
        
    except Exception as e:
        print(f"Erro: {e}")
        return False
    
    return True

def main():
    parser = argparse.ArgumentParser(
        description='Simula passagem de veículos no radar eletrônico'
    )
    parser.add_argument(
        '--type', 
        choices=['light', 'heavy'], 
        default='light',
        help='Tipo de veículo: light (leve, 2 eixos) ou heavy (pesado, 3 eixos)'
    )
    parser.add_argument(
        '--speed', 
        type=int, 
        default=50,
        help='Velocidade do veículo em km/h'
    )
    parser.add_argument(
        '--distance',
        type=int,
        default=1000,
        help='Distância entre sensores em mm (padrão: 1000)'
    )
    
    args = parser.parse_args()
    
    print("\n" + "="*50)
    print("  SIMULADOR DE RADAR ELETRÔNICO")
    print("="*50 + "\n")
    
    simulate_vehicle(args.type, args.speed)
    
    print("\n💡 Dica: Para automatizar, inicie o QEMU com:")
    print("   west build -t run -- -monitor tcp:localhost:55555,server,nowait")
    print("\nEm seguida, este script poderia enviar comandos automaticamente.")

if __name__ == '__main__':
    main()
