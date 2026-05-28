import can
import struct
import time
import math

# CONFIGURACIÓN DE RED CAN INDUSTRIAL (Mapeada en config.json de Qt)
# Cada sensor tiene su propio ID único de transmisión de 29 bits (Estándar J1939)
CAN_MAPPING = {
    "inclinacion": 0x1CFDD600,
    "distancia_brazo": 0x1CFDD601,
    "temp_motor": 0x1CFDD602,
    "presion_hidraulica": 0x1CFDD603
}

print("🚀 Emulador de Máquina CAN Bus Dinámica (SLCAN) Iniciado.")

try:
    # Abrir el puerto COM virtual del lado de Python
    # Se conecta al COM2, cruzado internamente con el COM4 que escucha Qt
    bus = can.interface.Bus(channel='COM2', interface='slcan', bitrate=250000)
    print("🛰️ Transmitiendo ráfagas CAN en 'COM2' (Tubería cruzada hacia el 'COM4' de Qt)...")
    print("Presiona Ctrl+C para detener el emulador.\n")
except Exception as e:
    print(f"❌ Error crítico abriendo el puerto serial virtual: {e}")
    print("Asegúrate de que la pareja COM2 <-> COM4 esté activa en com0com.")
    exit(1)

start_time = time.time()

try:
    while True:
        elapsed = time.time() - start_time

        # -------------------------------------------------------------
        # 1. CÁLCULO DE TELEMETRÍA CINEMÁTICA Y FÍSICA
        # -------------------------------------------------------------
        inclinacion = 28 * math.sin(elapsed * 0.3)
        distancia_brazo = 1.5 + 1.1 * math.sin(elapsed * 0.5)
        temp_motor = 89.0 + 9.0 * math.sin(elapsed * 0.1)
        presion_hidraulica = 190.0 + 25.0 * math.cos(elapsed * 0.2)

        # Emparejar los cálculos con sus respectivos IDs del bus
        sensor_values = {
            "inclinacion": inclinacion,
            "distancia_brazo": distancia_brazo,
            "temp_motor": temp_motor,
            "presion_hidraulica": presion_hidraulica
        }

        # -------------------------------------------------------------
        # 2. CONSTRUCCIÓN Y DESPACHO DE TRAMAS CAN INDEPENDIENTES
        # -------------------------------------------------------------
        for sensor_name, value in sensor_values.items():
            can_id = CAN_MAPPING[sensor_name]

            # Empaquetar el número flotante en formato binario de 8 bytes (Double)
            # '<d' = Little-endian, consistente con el QDataStream de C++
            payload = struct.pack('<d', value)

            # Crear el objeto de la trama CAN
            frame = can.Message(
                arbitration_id=can_id,
                data=payload,
                is_extended_id=True # Esencial para IDs de 29 bits (J1939)
            )

            # Inyectar la trama al puerto serial SLCAN
            bus.send(frame)

        # Monitoreo local en la terminal de comandos
        print(f"[CAN] Ráfaga de {len(sensor_values)} tramas J1939 inyectada con éxito.", end="\r")

        # Frecuencia cíclica de envío (50 Hz = Cada 20ms)
        time.sleep(0.02)

except KeyboardInterrupt:
    print("\n🛑 Emulación CAN finalizada por el usuario.")
finally:
    bus.shutdown()