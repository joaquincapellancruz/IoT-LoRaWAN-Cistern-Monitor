#include "LoRaWan_APP.h"
#include <Arduino.h>
#include <NewPing.h>
#include <algorithm>

/* === LoRaWAN ABP obligatorios === */
uint8_t devEui[8]            = {0};
uint8_t appEui[8]            = {0};
uint8_t appKey[16]           = {0};
uint16_t userChannelsMask[6] = {0x00FF,0,0,0,0,0};

/* === Parámetros ABP (NWKSKEY, APPSKEY, DEVADDR) === */
uint8_t nwkSKey[16] = {
  0x15,0xB1,0xD0,0xEF,0xA4,0x63,0xDF,0xBE,
  0x3D,0x11,0x18,0x1E,0x1E,0xC7,0xDA,0x85
};
uint8_t appSKey[16] = {
  0xD7,0x2C,0x78,0x75,0x8C,0xDC,0xCA,0xBF,
  0x55,0xEE,0x4A,0x77,0x8D,0x16,0xEF,0x67
};
uint32_t devAddr    = 0x26011BAA;

/* === Configuración LoRaWAN === */
LoRaMacRegion_t loraWanRegion   = LORAMAC_REGION_EU868;
DeviceClass_t   loraWanClass    = CLASS_A;
bool overTheAirActivation       = false;   // ABP
bool loraWanAdr                 = false;
bool isTxConfirmed              = false;
uint32_t appTxDutyCycle         = 15000;   // ms entre envíos
uint8_t  appPort                = 1;
uint8_t  confirmedNbTrials      = 1;

/* === Pines y constantes del sensor ultrasónico === */
#define TRIG_PIN         48
#define ECHO_PIN         47
#define TOTAL_MEDICIONES 10

NewPing sonar(TRIG_PIN, ECHO_PIN, 800);

/* === Parámetros manuales de la cisterna === */
// Área de la base: 48.5 cm × 70 cm
static const float AREA_CM2          = 48.5f * 70.0f;
// Zona muerta (altura que no cuenta): 40 cm
static const float ALTURA_MUERTA_CM  = 40.0f;
// Altura interna máxima: 81 cm
static const float ALTURA_MAXIMA_CM  = 81.0f;

/* === Buffer global para el payload LoRa === */
static char paquete[32];

/* === Toma TOTAL_MEDICIONES lecturas y devuelve la mediana === */
float sensarMediana() {
  float lecturas[TOTAL_MEDICIONES];
  int cnt = 0;
  for (int i = 0; i < TOTAL_MEDICIONES; i++) {
    float d = sonar.ping_cm();
    if (d > 0.0f) {
      lecturas[cnt++] = d;
      Serial.printf("Lectura %d: %.2f cm\n", cnt, d);
    }
    delay(50);
  }
  if (cnt == 0) return -1.0f;
  std::sort(lecturas, lecturas + cnt);
  if (cnt % 2 == 0) {
    return (lecturas[cnt/2 - 1] + lecturas[cnt/2]) / 2.0f;
  } else {
    return lecturas[cnt/2];
  }
}

/* === Prepara el frame a enviar con el cálculo de volumen === */
static void prepareTxFrame(uint8_t port) {
  // Medimos distancia sensor→superficie de agua
  float dist_cm = sensarMediana();
  if (dist_cm < 0) {
    Serial.println("⚠️ No se leyó el sensor, asumiendo tanque vacío");
    dist_cm = ALTURA_MAXIMA_CM;
  }

  // Altura de agua efectiva = altura_total - distancia_leída - zona muerta
  float altura_agua = ALTURA_MAXIMA_CM - dist_cm - 10.3f;
  // Volumen en cm³
  float vol_cm3 = altura_agua * AREA_CM2;
  // Convertir a galones (1 gal ≈ 3785.41 cm³)
  float galones = vol_cm3 / 3785.41f;

  if (altura_agua < 0) altura_agua = 0;
  
  // Formatear con 2 decimales
  int len = snprintf(paquete, sizeof(paquete), "%.2f", galones);
  appDataSize = len;
  memcpy(appData, paquete, len);

  Serial.printf("⇧ Uplink: altura_agua=%.2f cm → %.2f gal\n", altura_agua, galones);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== Iniciando medidor de nivel ===");
  Serial.printf("Área=%.2f cm², Zona muerta=%.2f cm, Altura máxima=%.2f cm\n",
                AREA_CM2, ALTURA_MUERTA_CM, ALTURA_MAXIMA_CM);

  // Inicializa LoRaWAN stack
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  LoRaWAN.init(loraWanClass, loraWanRegion);
  LoRaWAN.setDefaultDR(3);  // SF9BW125

  deviceState = DEVICE_STATE_INIT;
}

void loop() {
  switch (deviceState) {
    case DEVICE_STATE_INIT:
      deviceState = DEVICE_STATE_JOIN;
      break;

    case DEVICE_STATE_JOIN:
      LoRaWAN.join();
      break;

    case DEVICE_STATE_SEND:
      prepareTxFrame(appPort);
      LoRaWAN.send();
      deviceState = DEVICE_STATE_CYCLE;
      break;

    case DEVICE_STATE_CYCLE:
      txDutyCycleTime = appTxDutyCycle;
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;

    case DEVICE_STATE_SLEEP:
      LoRaWAN.sleep(loraWanClass);
      break;

    default:
      deviceState = DEVICE_STATE_INIT;
      break;
  }
}








