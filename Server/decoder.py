#!/usr/bin/env python3
import os
import socket
import json
import base64
import sqlite3
import logging
from logging.handlers import RotatingFileHandler
from Crypto.Cipher import AES

# ——— Rutas y logging ———
BASE_DIR = os.path.abspath(os.path.dirname(__file__))
DB_PATH  = os.path.join(BASE_DIR, 'data', 'lora_data.db')
LOG_DIR  = os.path.join(BASE_DIR, 'logs')
os.makedirs(LOG_DIR, exist_ok=True)

logger = logging.getLogger('lora_decoder')
logger.setLevel(logging.INFO)
handler = RotatingFileHandler(
    filename=os.path.join(LOG_DIR, 'lora_packets.log'),
    maxBytes=5*1024*1024,
    backupCount=3
)
handler.setFormatter(logging.Formatter('%(asctime)s %(levelname)s: %(message)s'))
logger.addHandler(handler)

# ——— Asegurar que la tabla existe ———
with sqlite3.connect(DB_PATH) as conn:
    c = conn.cursor()
    c.execute("""
        CREATE TABLE IF NOT EXISTS mediciones (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            volumen   REAL    NOT NULL
        )
    """)
    conn.commit()

# ——— Parámetro ABP (solo AppSKey) ———
AppSKey = bytes.fromhex("D72C78758CDCCABF55EE4A778D16EF67")

# ——— Funciones de descifrado ———
def decrypt_frm(app_skey, dev_addr, fcnt, frm_payload):
    aes   = AES.new(app_skey, AES.MODE_ECB)
    block = bytearray(16)
    block[0] = 0x01
    block[5] = 0x00                # 0 = uplink
    block[6:10] = dev_addr         # DevAddr LSB
    block[10:14] = fcnt.to_bytes(4, 'little')
    plain = bytearray()
    for i, b in enumerate(frm_payload):
        block[15] = (i // 16) + 1
        s = aes.encrypt(bytes(block))
        plain.append(b ^ s[i % 16])
    return bytes(plain)

def decode_lora_packet(data_b64):
    raw      = base64.b64decode(data_b64)
    dev_addr = raw[1:5]
    fcnt     = int.from_bytes(raw[6:8], 'little')
    frm      = raw[9:-4]
    payload  = decrypt_frm(AppSKey, dev_addr, fcnt, frm)
    return payload.decode(errors="ignore")

# ——— Listener UDP ———
UDP_IP   = "0.0.0.0"
UDP_PORT = 1730

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
logger.info("UDP listener iniciado en %s:%d", UDP_IP, UDP_PORT)
print(f"[+] Escuchando en {UDP_IP}:{UDP_PORT}")

while True:
    data, addr = sock.recvfrom(4096)


    # Extraer JSON
    start = data.find(b'{')
    end   = data.rfind(b'}')
    if start < 0 or end < 0:
        continue
    try:
        msg = json.loads(data[start:end+1].decode('utf-8'))
    except json.JSONDecodeError:
        continue

    for rx in msg.get('rxpk', []):
        if 'data' not in rx:
            continue

        try:
            raw_str = decode_lora_packet(rx['data']).strip()
            vol = float(raw_str)  # ValueError si no es numérico


            # Insertar volumen en mediciones
            with sqlite3.connect(DB_PATH) as conn:
                c = conn.cursor()
                c.execute(
                    "INSERT INTO mediciones (volumen) VALUES (?)",
                    (vol,)
                )
                conn.commit()

            logger.info(
                "%s:%d → volumen=%.3f",
                addr[0], addr[1], vol
            )

        except ValueError:
            # No era un float: descartar
            logger.debug(
                "%s:%d payload no numérico → '%s'",
                addr[0], addr[1], raw_str
            )
        except Exception as e:
            logger.error("Error procesando paquete: %s", e, exc_info=True)