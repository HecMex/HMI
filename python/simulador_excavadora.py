import socket
import json
import time
import math
import threading # <-- REQUERIDO: Para escuchar a Qt en segundo plano

# CONFIGURACIÓN DE RED (Coincide con la sección "udp" de tu config.json)
UDP_IP = "127.0.0.1"
SEND_PORT = 5005
RECV_PORT = 5006  # Puerto donde este script escuchará los paros de Qt

# Inicializar los sockets de red
sock_send = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_recv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_recv.bind((UDP_IP, RECV_PORT))
sock_recv.settimeout(0.1) # Evita bloquear por completo el script al cerrar

# CONTROLADORES DE ESTADO FÍSICO DE LA MÁQUINA
machine_running = True
shovel_moving = True

# =====================================================================
# HILO DE ESCUCHA: PROCESAMIENTO DE COMANDOS DE LA HMI QT
# =====================================================================
def listen_hmi_commands():
    global machine_running, shovel_moving
    while machine_running:
        try:
            data, addr = sock_recv.recvfrom(1024)
            command = data.decode('utf-8')
            
            if command == "STOP_ALL":
                print("\n🛑 [ALERTA] COMANDO RECIBIDO: PARO TOTAL DE EMERGENCIA.")
                machine_running = False
            elif command == "STOP_SHOVEL":
                print("\n🚨 [ALERTA] COMANDO RECIBIDO: BLOQUEO DE CILINDROS DE LA PALA.")
                shovel_moving = False
                
        except socket.timeout:
            continue

# Arrancar el hilo de red en segundo plano
thread = threading.Thread(target=listen_hmi_commands, daemon=True)
thread.start()

print(f"🚀 Simulador Dinámico de Sensores HMI Iniciado.")
print(f"Enviando JSONs a puerto {SEND_PORT} | Escuchando paros en puerto {RECV_PORT}...\n")

start_time = time.time()

try:
    while machine_running:
        elapsed = time.time() - start_time

        # -------------------------------------------------------------
        # 1. CÁLCULO DE LA FÍSICA Y SIMULACIÓN DE SENSORES
        # -------------------------------------------------------------
        # Sensor A: Inclinación del Chasis
        inclinacion = 28 * math.sin(elapsed * 0.3)
        
        # Sensor B: Distancia del brazo a la cabina
        # Si la HMI mandó bloquear la pala, detenemos su movimiento dinámico en el punto de riesgo
        if shovel_moving:
            distancia_brazo = 1.5 + 1.1 * math.sin(elapsed * 0.5)
        else:
            distancia_brazo = 0.5 # Bloqueada físicamente en zona crítica
        
        # Sensor C: Temperatura del Motor en °C
        temp_motor = 89.0 + 9.0 * math.sin(elapsed * 0.1)
        
        # Sensor D: Presión del Sistema Hidráulico en PSI
        presion_hidraulica = 190.0 + 25.0 * math.cos(elapsed * 0.2)

        # -------------------------------------------------------------
        # 2. EMPAQUETADO INDIVIDUAL EN FORMATO JSON INDUSTRIAL
        # -------------------------------------------------------------
        rafaga_sensores = [
            {"id": "inclinacion", "val": round(inclinacion, 2)},
            {"id": "distancia_brazo", "val": round(distancia_brazo, 2)},
            {"id": "temp_motor", "val": round(temp_motor, 2)},
            {"id": "presion_hidraulica", "val": round(presion_hidraulica, 2)}
        ]

        # -------------------------------------------------------------
        # 3. ENVÍO DE DATOS POR LA RED
        # -------------------------------------------------------------
        for sensor_data in rafaga_sensores:
            json_string = json.dumps(sensor_data)
            sock_send.sendto(json_string.encode('utf-8'), (UDP_IP, SEND_PORT))

        # Monitoreo local en la consola de Python
        print(f"[ENVÍO] Ángulo: {inclinacion:5.1f}° | Dist: {distancia_brazo:4.2f}m | Temp: {temp_motor:4.1f}°C", end="\r")

        # Frecuencia de actualización (50Hz)
        time.sleep(0.02)

    # Si machine_running pasa a False (Comando STOP_ALL desde Qt)
    print("\n💀 La excavadora ha sido apagada de emergencia de forma remota por orden de la HMI.")

except KeyboardInterrupt:
    print("\n🛑 Simulación dinámica detenida por el usuario.")
finally:
    sock_send.close()
    sock_recv.close()
