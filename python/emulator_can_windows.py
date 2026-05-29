import serial
import struct
import time
import math
import sys

# CONFIGURACIÓN DE PUERTOS COM VIRTUALES
# Python inyecta por el COM12, cruzado con el COM14 que escucha la aplicación en Qt
PORT_CAN_OUT = "COM12"  
CAN_ID_MAPPING = {
    "inclinacion": 0x1CFDD600,
    "distancia_brazo": 0x1CFDD601,
    "temp_motor": 0x1CFDD602,
    "presion_hidraulica": 0x1CFDD603
}

print("🚀 Emulador de Máquina CAN Bus Dinámica (SLCAN) para Windows.")

try:
    # Abrir el puerto COM crudo configurado con el estándar Lawicel
    ser = serial.Serial(PORT_CAN_OUT, 115200, timeout=0.05)
    
    # Inicializar el canal SLCAN enviando los comandos de apertura estándar
    ser.write(b'C\r') # Cierra canal previo por seguridad
    ser.write(b'S5\r') # Configura velocidad a 250k
    ser.write(b'O\r') # Abre el canal serial CAN
    ser.flush()
    
    print(f"🛰️ Bus SLCAN abierto con éxito en {PORT_CAN_OUT}.")
    print("Transmitiendo telemetría continua a 50Hz (Hacia el COM14 de Qt)...")
    print("Presiona Ctrl+C para detener el emulador.\n")
except Exception as e:
    print(f"❌ Error crítico abriendo el puerto serial virtual: {e}")
    print("Asegúrate de que el par COM2 <-> COM4 esté creado en com0com y que Qt no esté ocupando el COM2.")
    sys.exit(1)

start_time = time.time()
machine_running = True

# Función auxiliar para formatear tramas CAN al formato ASCII Lawicel (SLCAN)
def format_slcan_extended_frame(arbitration_id, payload_bytes):
    # En Lawicel, las tramas con ID extendido de 29 bits inician con la letra 'T' mayúscula
    # Seguido de 8 caracteres hexadecimales para el ID
    # Seguido de 1 caracter para la longitud de datos (DLC = 8)
    # Seguido de los bytes de datos convertidos a texto hexadecimal en mayúsculas
    # Termina obligatoriamente con un retorno de carro '\r'
    id_hex = f"{arbitration_id:08X}"
    dlc_hex = f"{len(payload_bytes)}"
    data_hex = "".join(f"{b:02X}" for b in payload_bytes)
    
    slcan_string = f"T{id_hex}{dlc_hex}{data_hex}\r"
    return slcan_string.encode('ascii')

try:
    while machine_running:
        elapsed = time.time() - start_time

        # CÁLCULO DE TELEMETRÍA CINEMÁTICA Y FÍSICA
        inclinacion = 28 * math.sin(elapsed * 0.3)
        distancia_brazo = 1.5 + 1.1 * math.sin(elapsed * 0.5)
        temp_motor = 89.0 + 9.0 * math.sin(elapsed * 0.1)
        presion_hidraulica = 190.0 + 25.0 * math.cos(elapsed * 0.2)

        sensor_values = {
            "inclinacion": inclinacion,
            "distancia_brazo": distancia_brazo,
            "temp_motor": temp_motor,
            "presion_hidraulica": presion_hidraulica,
        }

        # CONSTRUCCIÓN Y DESPACHO DE LAS TRAMAS SERIALES SLCAN
        for sensor_name, value in sensor_values.items():
            can_id = CAN_ID_MAPPING[sensor_name]

            # Empaquetar el número flotante en formato binario de 8 bytes (Double)
            payload = struct.pack('<d', value)

            # Generar el comando Lawicel legítimo
            slcan_frame = format_slcan_extended_frame(can_id, payload)

            # Escribir directo en la tubería de Windows
            ser.write(slcan_frame)
        
        ser.flush()

        # ESCUCHA DE RETORNO ADAPTATIVA
        # Leemos si la HMI de Qt inyectó bytes de comando en el bus
        if ser.in_waiting > 0:
            raw_input = ser.read(ser.in_waiting)
            # Buscamos los IDs de los comandos en formato hexadecimal dentro de los bytes
            if b"1CFF0100" in raw_input.upper():
                print("\n🛑 [ALERTA CAN] TRAMA DE PARO DETECTADA: PARO TOTAL DEL MOTOR.")
                machine_running = False
            elif b"1CFF0200" in raw_input.upper():
                print("\n🚨 [ALERTA CAN] TRAMA DE BLOQUEO DETECTADA: PALA CONGELADA.")
                # Nota: Para congelar localmente, podríamos modificar el flujo, 
                # pero al ser un paro total forzamos la salida limpia de consola
                machine_running = False

        # Monitoreo local en la terminal de comandos sin saltos de línea
        print(f"[ENVÍO CAN -> {PORT_CAN_OUT}] Ángulo: {inclinacion:5.1f}° | Dist: {distancia_brazo:4.2f}m | Temp: {temp_motor:4.1f}°C", end="\r")

        # Frecuencia cíclica de envío (50 Hz = Cada 20ms)
        time.sleep(0.02)

    print("\n💀 El emulador ha procesado la orden de detención remota enviada por la HMI de Qt.")

except KeyboardInterrupt:
    print("\n🛑 Emulación CAN finalizada manualmente por el usuario.")
finally:
    # Cerrar el canal y el puerto COM de forma segura al salir
    try:
        ser.write(b'C\r')
        ser.close()
        print("🔌 Puerto COM12 liberado correctamente.")
    except:
        pass
