#include <Arduino.h>
#include <cstring>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <Update.h>
#include <BleKeyboard.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include "esp_sleep.h"

// -------------------- HARDWARE --------------------
#define PIN_CLK 2
#define PIN_DT  3
#define PIN_SW  4
#define PIN_IR  5
#define PIN_LED 8   // ESP32-C3 SuperMini: LED integrato Active LOW

// -------------------- TIMING --------------------
static constexpr uint32_t TIMEOUT_INACTIVITY_MS = 120000UL; // 2 minuti
static constexpr uint32_t LONG_PRESS_MS         = 2000UL;
static constexpr uint32_t BUTTON_DEBOUNCE_MS    = 30UL;

// -------------------- ACCESS POINT --------------------
static const char *AP_SSID     = "Volumefy-Setup";
static const char *AP_PASSWORD = "12345678";

// -------------------- MODALITA' --------------------
enum OperatingMode : uint8_t {
  MODE_BLE = 0,
  MODE_IR  = 1
};

// Il catalogo contiene sia profili esatti verificati sia famiglie di compatibilita'.
// Il pannello mostra sempre il livello del profilo: non mascheriamo un alias di
// famiglia come se fosse una cattura esatta del singolo telecomando.
enum IrProtocol : uint8_t {
  IR_NEC = 0,
  IR_SAMSUNG32,
  IR_RC5,
  IR_RC6,
  IR_SONY12,
  IR_RCA
};

enum ProfileStatus : uint8_t {
  PROFILE_VERIFIED = 0,
  PROFILE_FAMILY,
  PROFILE_TV_REMOTE,
  PROFILE_COMMUNITY,
  PROFILE_COMPAT
};

struct RemoteProfile {
  const char *category;
  const char *brand;
  const char *model;
  IrProtocol protocol;
  uint16_t address;
  uint8_t volumeUp;
  uint8_t volumeDown;
  uint8_t mute;
  ProfileStatus status;
  const char *note;
};

static const RemoteProfile REMOTES[] = {
  {"TV", "Samsung", "H6300 / Samsung32", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_VERIFIED, "Codici H6300 verificati nel database IR."},
  {"TV", "LG", "MKJ39170828", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_VERIFIED, "Codici MKJ39170828 verificati nel database IR."},
  {"Audio", "Yamaha", "RX-V375", IR_NEC, 0x007A, 0x1A, 0x1B, 0x1C, PROFILE_VERIFIED, "Profilo audio mantenuto dalla versione precedente."},
  {"Audio", "Logitech", "Z906", IR_NEC, 0xA002, 0xAA, 0x6A, 0xEA, PROFILE_VERIFIED, "Profilo NECext compatibile mantenuto dalla versione precedente."},
  {"Soundbar", "Dutch Originals", "Sound Bar", IR_NEC, 0x0000, 0x5E, 0x85, 0x09, PROFILE_VERIFIED, "Profilo mantenuto dalla versione precedente."},
  {"Audio", "Elac", "EA101EQ-G", IR_NEC, 0x0000, 0x46, 0x16, 0x55, PROFILE_VERIFIED, "Profilo mantenuto dalla versione precedente."},
  {"Audio", "Philips", "FW750C", IR_RC5, 0x0010, 0x10, 0x11, 0x0D, PROFILE_VERIFIED, "Profilo RC5 audio mantenuto dalla versione precedente."},
  {"TV", "Samsung", "AA59-00443A", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "AA59-00484A", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "AA59-00580A", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "AA59-00602A", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "AA59-00714A", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "AA59-00741A", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "AA59-00786A", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "BN59-00511A", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "BN59-01081A", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "BN59-01175N", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "BN59-01178W", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "BN59-01179A", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "BN59-01180A", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "BN59-01198R", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "BN59-01247A", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "BN59-01301A", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "BN59-01303A", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "BN59-01315B", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "BN59-01315J", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "BN59-01330C", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "BN59-01358C", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "BN59-01385C", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "BN59-01388", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "BN59-01391A", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "UE32F4000", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "UE48JU6490U", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "UE75TU7125K", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "UN32EH5000F", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "UN40C5000QFXZA", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "UN60JU6500", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "QN43Q60AAF", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "QM55RA", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "P2770HD", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "Samsung", "LN46C650L1F", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume Samsung TV. Verificare con i tasti Test."},
  {"TV", "LG", "24LJ4840", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "27GR95QE", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "32LF650V", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "32LN5406", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "32LW4500", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "37LN5403", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "43NANO779PA", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "48LV340H", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "55LB870V-ZA", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "55UN7300AUD", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "75UJ6470", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "AKB33871403", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "AKB69680401", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "AKB72913118", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "AKB72914048", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "AKB72915206", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "AKB73275675", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "AKB74915305", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "AKB75095307", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "AKB75375608", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "AKB75675311", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "AKB75855501", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "AKB76043102", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "OLED C1", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "OLED C3 / OLED48C37LA", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "LG", "OLED65C8PUA", IR_NEC, 0x0004, 0x02, 0x03, 0x09, PROFILE_FAMILY, "Modello presente in IRDB; usa la famiglia volume LG TV. Verificare con i tasti Test."},
  {"TV", "Sony", "Bravia", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Sony", "Bravia KD-55XF80xx", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Sony", "Bravia 49XF80xx", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Sony", "Bravia 43XF80xx", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Sony", "Bravia KDL-46W905A", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Sony", "FW-75BZ40H", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Sony", "KD-55X80CK", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Sony", "KDL-55HX850", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Sony", "RM-GD014", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Sony", "RM-YD017", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Sony", "RM-YD018", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Sony", "RM-YD028", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Sony", "RM-YD092", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Sony", "RMEA002", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Sony", "RMF-TX500U", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Sony", "RMT-TX100D", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Sony", "RMT-TX200U", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Sony", "RM-ED016", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Sony", "RM-ED045", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Sony", "XBR RMT-TX200U", IR_SONY12, 0x0001, 0x12, 0x13, 0x14, PROFILE_FAMILY, "Famiglia Sony SIRC TV: VOL+/VOL-/Mute. I comandi SIRC includono le ripetizioni minime previste dal protocollo."},
  {"TV", "Philips", "14GX8510", IR_RC5, 0x0000, 0x10, 0x11, 0x0D, PROFILE_FAMILY, "Famiglia Philips TV RC5. Alcuni modelli recenti possono usare un protocollo diverso: provare prima i tasti Test."},
  {"TV", "Philips", "14PT136B00", IR_RC5, 0x0000, 0x10, 0x11, 0x0D, PROFILE_FAMILY, "Famiglia Philips TV RC5. Alcuni modelli recenti possono usare un protocollo diverso: provare prima i tasti Test."},
  {"TV", "Philips", "22IT TV Monitor", IR_RC5, 0x0000, 0x10, 0x11, 0x0D, PROFILE_FAMILY, "Famiglia Philips TV RC5. Alcuni modelli recenti possono usare un protocollo diverso: provare prima i tasti Test."},
  {"TV", "Philips", "32PFL4208T", IR_RC5, 0x0000, 0x10, 0x11, 0x0D, PROFILE_FAMILY, "Famiglia Philips TV RC5. Alcuni modelli recenti possono usare un protocollo diverso: provare prima i tasti Test."},
  {"TV", "Philips", "32PFL7403S", IR_RC5, 0x0000, 0x10, 0x11, 0x0D, PROFILE_FAMILY, "Famiglia Philips TV RC5. Alcuni modelli recenti possono usare un protocollo diverso: provare prima i tasti Test."},
  {"TV", "Philips", "32PFL7962D12", IR_RC5, 0x0000, 0x10, 0x11, 0x0D, PROFILE_FAMILY, "Famiglia Philips TV RC5. Alcuni modelli recenti possono usare un protocollo diverso: provare prima i tasti Test."},
  {"TV", "Philips", "40HFL3010T12", IR_RC5, 0x0000, 0x10, 0x11, 0x0D, PROFILE_FAMILY, "Famiglia Philips TV RC5. Alcuni modelli recenti possono usare un protocollo diverso: provare prima i tasti Test."},
  {"TV", "Philips", "40PFL6533-F7D", IR_RC5, 0x0000, 0x10, 0x11, 0x0D, PROFILE_FAMILY, "Famiglia Philips TV RC5. Alcuni modelli recenti possono usare un protocollo diverso: provare prima i tasti Test."},
  {"TV", "Philips", "436M6VBPAB", IR_RC5, 0x0000, 0x10, 0x11, 0x0D, PROFILE_FAMILY, "Famiglia Philips TV RC5. Alcuni modelli recenti possono usare un protocollo diverso: provare prima i tasti Test."},
  {"TV", "Philips", "50PUT6103-79", IR_RC5, 0x0000, 0x10, 0x11, 0x0D, PROFILE_FAMILY, "Famiglia Philips TV RC5. Alcuni modelli recenti possono usare un protocollo diverso: provare prima i tasti Test."},
  {"TV", "Philips", "5766 Series", IR_RC5, 0x0000, 0x10, 0x11, 0x0D, PROFILE_FAMILY, "Famiglia Philips TV RC5. Alcuni modelli recenti possono usare un protocollo diverso: provare prima i tasti Test."},
  {"TV", "Philips", "48OLED806", IR_RC5, 0x0000, 0x10, 0x11, 0x0D, PROFILE_FAMILY, "Famiglia Philips TV RC5. Alcuni modelli recenti possono usare un protocollo diverso: provare prima i tasti Test."},
  {"TV", "Philips", "7000LED / 42PFL7695H", IR_RC5, 0x0000, 0x10, 0x11, 0x0D, PROFILE_FAMILY, "Famiglia Philips TV RC5. Alcuni modelli recenti possono usare un protocollo diverso: provare prima i tasti Test."},
  {"TV", "Philips", "7956 Series", IR_RC5, 0x0000, 0x10, 0x11, 0x0D, PROFILE_FAMILY, "Famiglia Philips TV RC5. Alcuni modelli recenti possono usare un protocollo diverso: provare prima i tasti Test."},
  {"TV", "Philips", "XXPFL9955H", IR_RC5, 0x0000, 0x10, 0x11, 0x0D, PROFILE_FAMILY, "Famiglia Philips TV RC5. Alcuni modelli recenti possono usare un protocollo diverso: provare prima i tasti Test."},
  {"TV", "Philips", "TV Universal", IR_RC5, 0x0000, 0x10, 0x11, 0x0D, PROFILE_FAMILY, "Famiglia Philips TV RC5. Alcuni modelli recenti possono usare un protocollo diverso: provare prima i tasti Test."},
  {"TV", "TCL", "40S615", IR_RCA, 0x000F, 0xF4, 0x74, 0xFC, PROFILE_VERIFIED, "Profilo RCA della voce IRDB del modello."},
  {"TV", "TCL", "65C635K", IR_RCA, 0x000F, 0xF4, 0x74, 0xFC, PROFILE_VERIFIED, "Profilo RCA della voce IRDB del modello."},
  {"TV", "Xiaomi", "Mi TV 4A 43", IR_NEC, 0x0080, 0x16, 0x15, 0x5A, PROFILE_COMMUNITY, "Profilo NEC da cattura community: testare VOL+/VOL-/Mute prima di salvarlo tra i preferiti."},
  {"Soundbar", "Samsung", "HW-T400", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_TV_REMOTE, "Usa i codici TV Samsung. Attivare sulla soundbar la modalita Samsung-TV REMOTE/TV remote se prevista dal modello."},
  {"Soundbar", "Samsung", "HW-Q600A", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_TV_REMOTE, "Usa i codici TV Samsung. Attivare sulla soundbar la modalita Samsung-TV REMOTE/TV remote se prevista dal modello."},
  {"Soundbar", "Samsung", "HW-F550 / HW-F551", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Profilo compatibilita Samsung Soundbar: usa la famiglia IR Samsung TV; richiede che la soundbar accetti il controllo da telecomando TV."},
  {"Soundbar", "Samsung", "HW-FM45", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Profilo compatibilita Samsung Soundbar: usa la famiglia IR Samsung TV; richiede che la soundbar accetti il controllo da telecomando TV."},
  {"Soundbar", "Samsung", "AH59-02547B family", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Profilo compatibilita Samsung Soundbar: usa la famiglia IR Samsung TV; richiede che la soundbar accetti il controllo da telecomando TV."},
  {"Soundbar", "Samsung", "AH59-02692P family", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Profilo compatibilita Samsung Soundbar: usa la famiglia IR Samsung TV; richiede che la soundbar accetti il controllo da telecomando TV."},
  {"Soundbar", "Samsung", "AH59-02758A family", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Profilo compatibilita Samsung Soundbar: usa la famiglia IR Samsung TV; richiede che la soundbar accetti il controllo da telecomando TV."},
  {"Soundbar", "Samsung", "AH59-02767C family", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Profilo compatibilita Samsung Soundbar: usa la famiglia IR Samsung TV; richiede che la soundbar accetti il controllo da telecomando TV."},
  {"Soundbar", "Samsung", "AH59-002692H family", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_FAMILY, "Profilo compatibilita Samsung Soundbar: usa la famiglia IR Samsung TV; richiede che la soundbar accetti il controllo da telecomando TV."},
  {"Soundbar", "Samsung", "Q600A", IR_SAMSUNG32, 0x0007, 0x07, 0x0B, 0x0F, PROFILE_TV_REMOTE, "Usa i codici TV Samsung. Attivare sulla soundbar la modalita Samsung-TV REMOTE/TV remote se prevista dal modello."},
  {"Soundbar", "Philips", "HTL2060 / 94", IR_RC5, 0x0005, 0x10, 0x11, 0x0D, PROFILE_COMPAT, "Profilo compatibilita Philips/RC5 AV. Non tutti i modelli Philips usano RC5: usare Test prima di impostarlo come preferito."},
  {"Soundbar", "Philips", "HTL1510B", IR_RC5, 0x0005, 0x10, 0x11, 0x0D, PROFILE_COMPAT, "Profilo compatibilita Philips/RC5 AV. Non tutti i modelli Philips usano RC5: usare Test prima di impostarlo come preferito."},
  {"Soundbar", "Philips", "HTL1520B", IR_RC5, 0x0005, 0x10, 0x11, 0x0D, PROFILE_COMPAT, "Profilo compatibilita Philips/RC5 AV. Non tutti i modelli Philips usano RC5: usare Test prima di impostarlo come preferito."},
  {"Soundbar", "Philips", "TAB5706", IR_RC5, 0x0005, 0x10, 0x11, 0x0D, PROFILE_COMPAT, "Profilo compatibilita Philips/RC5 AV. Non tutti i modelli Philips usano RC5: usare Test prima di impostarlo come preferito."},
  {"Soundbar", "Philips", "5000 Series", IR_RC5, 0x0005, 0x10, 0x11, 0x0D, PROFILE_COMPAT, "Profilo compatibilita Philips/RC5 AV. Non tutti i modelli Philips usano RC5: usare Test prima di impostarlo come preferito."},
  {"Soundbar", "Philips", "HTL2160", IR_RC5, 0x0005, 0x10, 0x11, 0x0D, PROFILE_COMPAT, "Profilo compatibilita Philips/RC5 AV. Non tutti i modelli Philips usano RC5: usare Test prima di impostarlo come preferito."},
  {"Soundbar", "Philips", "HTL2150", IR_RC5, 0x0005, 0x10, 0x11, 0x0D, PROFILE_COMPAT, "Profilo compatibilita Philips/RC5 AV. Non tutti i modelli Philips usano RC5: usare Test prima di impostarlo come preferito."},
  {"Soundbar", "Philips", "HTL3140B", IR_RC6, 0x0010, 0x10, 0x11, 0x0D, PROFILE_VERIFIED, "RC6 address 0x10: VOL+ 0x10, VOL- 0x11, Mute 0x0D. Profilo corretto dalla cattura HTL2163B, telecomando compatibile con HTL3140B."},
  {"Soundbar", "Philips", "HTL2163B/12", IR_RC6, 0x0010, 0x10, 0x11, 0x0D, PROFILE_VERIFIED, "Cattura IR fornita: Philips HTL2163B/12, protocollo RC6 address 0x10; VOL+ 0x10, VOL- 0x11, Mute 0x0D."},
};

static constexpr size_t REMOTE_COUNT = sizeof(REMOTES) / sizeof(REMOTES[0]);
static constexpr size_t FAVORITE_BYTES = (REMOTE_COUNT + 7U) / 8U;

BleKeyboard bleKeyboard("Volumefy", "Custom", 100);
IRsend irsend(PIN_IR);
Preferences preferences;
WebServer server(80);
DNSServer dnsServer;

// -------------------- WEB OTA --------------------
// L'OTA e' esposto solo nella modalita IR/AP. In modalita BLE il Wi-Fi
// non viene inizializzato, quindi non vi e' consumo aggiuntivo.
bool otaInProgress = false;
bool otaUploadSuccess = false;
bool otaRebootPending = false;
uint32_t otaRebootAt = 0;

OperatingMode currentMode = MODE_BLE;
uint8_t activeRemote = 0;
uint8_t favoriteBits[FAVORITE_BYTES] = {0};
uint8_t irVolumeRepeat = 2; // 1..6 trasmissioni equivalenti per scatto encoder

int lastCLKState = HIGH;
uint32_t lastActivityTime = 0;
uint32_t lastLedBlinkTime = 0;

// Stato pulsante con debounce e discriminazione short/long press.
bool rawButtonState = HIGH;
bool stableButtonState = HIGH;
bool buttonArmed = false;
bool longPressHandled = false;
uint32_t rawButtonChangedAt = 0;
uint32_t buttonDownAt = 0;

// RC5 usa un toggle bit che deve cambiare fra pressioni successive.
bool rc5Toggle = false;
bool rc6Toggle = false;

// -------------------- UTILITY --------------------
void markActivity() {
  lastActivityTime = millis();
}

String remoteName(size_t index) {
  if (index >= REMOTE_COUNT) return "Profilo non valido";
  String s = REMOTES[index].brand;
  s += " ";
  s += REMOTES[index].model;
  return s;
}

void saveMode(OperatingMode mode) {
  preferences.putUChar("mode", static_cast<uint8_t>(mode));
}

void saveActiveRemote(uint8_t index) {
  preferences.putUChar("remote", index);
}

bool isFavorite(size_t index) {
  if (index >= REMOTE_COUNT) return false;
  return (favoriteBits[index / 8U] & (1U << (index % 8U))) != 0;
}

void toggleFavorite(size_t index) {
  if (index >= REMOTE_COUNT) return;
  favoriteBits[index / 8U] ^= (1U << (index % 8U));
}

void saveFavorites() {
  preferences.putBytes("favBits", favoriteBits, FAVORITE_BYTES);
}

void saveIrVolumeRepeat() {
  preferences.putUChar("irRepeat", irVolumeRepeat);
}

void loadSettings() {
  preferences.begin("volumefy", false);

  uint8_t storedMode = preferences.getUChar("mode", static_cast<uint8_t>(MODE_BLE));
  currentMode = (storedMode == static_cast<uint8_t>(MODE_IR)) ? MODE_IR : MODE_BLE;

  activeRemote = preferences.getUChar("remote", 0);
  if (activeRemote >= REMOTE_COUNT) {
    activeRemote = 0;
    saveActiveRemote(activeRemote);
  }

  // Preferiti a bitset dinamico: non siamo più limitati a 64 profili.
  memset(favoriteBits, 0, sizeof(favoriteBits));
  const size_t storedFavBytes = preferences.getBytesLength("favBits");
  if (storedFavBytes > 0) {
    const size_t toRead = (storedFavBytes < FAVORITE_BYTES) ? storedFavBytes : FAVORITE_BYTES;
    preferences.getBytes("favBits", favoriteBits, toRead);
  } else {
    // Migrazione automatica dal formato precedente uint64_t, se presente.
    const uint64_t legacyMask = preferences.getULong64("favs", 0ULL);
    const size_t legacyBytes = (FAVORITE_BYTES < sizeof(legacyMask)) ? FAVORITE_BYTES : sizeof(legacyMask);
    memcpy(favoriteBits, &legacyMask, legacyBytes);
    if (legacyMask != 0ULL) saveFavorites();
  }

  irVolumeRepeat = preferences.getUChar("irRepeat", 2);
  if (irVolumeRepeat < 1 || irVolumeRepeat > 6) {
    irVolumeRepeat = 2;
    saveIrVolumeRepeat();
  }
}

// -------------------- IR --------------------
// RCA standard: 24 bit, MSB first: D:4 F:8 ~D:4 ~F:8, carrier ~58 kHz.
void sendRca(uint8_t address, uint8_t command, uint16_t repeats) {
  const uint32_t data =
      ((static_cast<uint32_t>(address) & 0x0F) << 20) |
      (static_cast<uint32_t>(command) << 12) |
      ((static_cast<uint32_t>(~address) & 0x0F) << 8) |
      static_cast<uint8_t>(~command);

  irsend.sendGeneric(3680, 3680, 460, 1840, 460, 920, 460, 7360,
                     data, 24, 58, true, repeats, 33);
}

void sendIrCommand(uint8_t command, bool volumeCommand = false) {
  if (currentMode != MODE_IR || activeRemote >= REMOTE_COUNT) return;

  const RemoteProfile &r = REMOTES[activeRemote];
  const uint16_t userRepeats = volumeCommand ? static_cast<uint16_t>(irVolumeRepeat - 1) : 0;

  switch (r.protocol) {
    case IR_NEC: {
      const uint64_t data = irsend.encodeNEC(r.address, command);
      irsend.sendNEC(data, kNECBits, userRepeats);
      break;
    }

    case IR_SAMSUNG32: {
      const uint64_t data = irsend.encodeSAMSUNG(static_cast<uint8_t>(r.address), command);
      irsend.sendSAMSUNG(data, kSamsungBits, userRepeats);
      break;
    }

    case IR_RC5: {
      const uint64_t data = irsend.encodeRC5(static_cast<uint8_t>(r.address), command, rc5Toggle);
      irsend.sendRC5(data, kRC5Bits, userRepeats);
      rc5Toggle = !rc5Toggle;
      break;
    }

    case IR_RC6: {
      // Philips HTL2163B/HTL3140B: RC6 Mode 0, carrier 36 kHz.
      // I repeat della stessa pressione mantengono lo stesso toggle; il toggle
      // cambia solo tra due pressioni logiche successive.
      uint64_t data = irsend.encodeRC6(r.address, command, kRC6Mode0Bits);
      if (rc6Toggle) data = irsend.toggleRC6(data, kRC6Mode0Bits);
      irsend.sendRC6(data, kRC6Mode0Bits, userRepeats);
      rc6Toggle = !rc6Toggle;
      break;
    }

    case IR_SONY12: {
      // Sony SIRC richiede normalmente almeno 3 frame per una pressione logica.
      // 1x mantiene quindi i repeat minimi del protocollo; la velocita web aggiunge repeat.
      const uint16_t sonyRepeats = static_cast<uint16_t>(2 + (volumeCommand ? (irVolumeRepeat - 1) : 0));
      const uint64_t data = irsend.encodeSony(12, command, r.address);
      irsend.sendSony(data, 12, sonyRepeats);
      break;
    }

    case IR_RCA:
      sendRca(static_cast<uint8_t>(r.address), command, userRepeats);
      break;
  }
}

void volumeUp() {
  if (currentMode == MODE_BLE) {
    if (bleKeyboard.isConnected()) bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
  } else if (activeRemote < REMOTE_COUNT) {
    sendIrCommand(REMOTES[activeRemote].volumeUp, true);
  }
  markActivity();
}

void volumeDown() {
  if (currentMode == MODE_BLE) {
    if (bleKeyboard.isConnected()) bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
  } else if (activeRemote < REMOTE_COUNT) {
    sendIrCommand(REMOTES[activeRemote].volumeDown, true);
  }
  markActivity();
}

void muteUnmute() {
  if (currentMode == MODE_BLE) {
    if (bleKeyboard.isConnected()) bleKeyboard.write(KEY_MEDIA_MUTE);
  } else if (activeRemote < REMOTE_COUNT) {
    sendIrCommand(REMOTES[activeRemote].mute);
  }
  markActivity();
}

// -------------------- CAMBIO MODALITA' --------------------
void switchOperatingMode() {
  OperatingMode nextMode = (currentMode == MODE_BLE) ? MODE_IR : MODE_BLE;
  saveMode(nextMode);

  // Feedback visivo minimale prima del reboot.
  digitalWrite(PIN_LED, LOW);
  delay(80);
  digitalWrite(PIN_LED, HIGH);

  preferences.end();
  delay(30);
  ESP.restart();
}

// -------------------- ENCODER --------------------
void checkEncoder() {
  const int currentCLK = digitalRead(PIN_CLK);

  if (currentCLK != lastCLKState && currentCLK == LOW) {
    // DIREZIONE INVERTITA rispetto al firmware originale:
    // prima DT != CLK => volume UP; ora => volume DOWN.
    if (digitalRead(PIN_DT) != currentCLK) {
      volumeDown();
    } else {
      volumeUp();
    }
  }

  lastCLKState = currentCLK;
}

// -------------------- PULSANTE --------------------
void checkButton() {
  const uint32_t now = millis();
  const bool raw = digitalRead(PIN_SW);

  if (raw != rawButtonState) {
    rawButtonState = raw;
    rawButtonChangedAt = now;
  }

  if ((now - rawButtonChangedAt) >= BUTTON_DEBOUNCE_MS && raw != stableButtonState) {
    stableButtonState = raw;

    // Se ci siamo svegliati con SW ancora premuto, non trasformiamo il gesto
    // di wake-up in Mute o in cambio modalita'. Prima richiediamo un rilascio.
    if (!buttonArmed) {
      if (stableButtonState == HIGH) buttonArmed = true;
      return;
    }

    if (stableButtonState == LOW) {
      buttonDownAt = now;
      longPressHandled = false;
      markActivity();
    } else {
      // Il Mute viene inviato solo al rilascio, se NON e' stata riconosciuta
      // una pressione lunga.
      if (!longPressHandled && buttonDownAt != 0 && (now - buttonDownAt) < LONG_PRESS_MS) {
        muteUnmute();
      }
      buttonDownAt = 0;
      longPressHandled = false;
    }
  }

  if (buttonArmed && stableButtonState == LOW && !longPressHandled && buttonDownAt != 0 &&
      (now - buttonDownAt) >= LONG_PRESS_MS) {
    longPressHandled = true;
    switchOperatingMode();
  }
}

// -------------------- WEB UI --------------------
const char *profileStatusLabel(ProfileStatus status) {
  switch (status) {
    case PROFILE_VERIFIED:  return "Verificato";
    case PROFILE_FAMILY:    return "Famiglia compatibile";
    case PROFILE_TV_REMOTE: return "TV Remote mode";
    case PROFILE_COMMUNITY: return "Community";
    case PROFILE_COMPAT:    return "Da provare";
  }
  return "Profilo";
}

String jsEscape(const char *text) {
  String out;
  if (!text) return out;
  out.reserve(strlen(text) + 8);
  for (const char *p = text; *p; ++p) {
    const char c = *p;
    if (c == '\\' || c == '"') { out += '\\'; out += c; }
    else if (c == '\n') out += F("\\n");
    else if (c == '\r') { }
    else out += c;
  }
  return out;
}

void redirectHome() {
  server.sendHeader("Location", "/", true);
  server.send(303, "text/plain", "");
}

void handleRoot() {
  markActivity();

  String html;
  html.reserve(36000);

  html += F("<!doctype html><html lang='it'><head><meta charset='utf-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Volumefy IR</title><style>");
  html += F(":root{color-scheme:dark}*{box-sizing:border-box}body{font-family:system-ui,-apple-system,sans-serif;margin:0;background:#0b1220;color:#f3f4f6}");
  html += F("main{max-width:880px;margin:auto;padding:18px}.top{display:flex;justify-content:space-between;align-items:center;gap:12px;flex-wrap:wrap}");
  html += F(".card{background:#172033;border:1px solid #263247;border-radius:16px;padding:18px;margin:14px 0;box-shadow:0 8px 30px #0002}");
  html += F("h1{font-size:1.7rem;margin:.2rem 0}h2{font-size:1.04rem;margin:0 0 12px;color:#dbe4f0}.count{color:#9ca3af}");
  html += F("label{display:block;color:#aeb9ca;font-size:.85rem;margin:10px 0 5px}select,input[type=search],button,.btn{border:0;border-radius:10px;padding:12px;font-size:1rem}");
  html += F("select,input[type=search],input[type=file]{width:100%;background:#f8fafc;color:#111827;border:1px solid #cbd5e1}input[type=range]{width:100%}");
  html += F("input[type=file]{padding:10px}.progress{height:12px;background:#0f172a;border-radius:999px;overflow:hidden;margin-top:12px}.progress>div{height:100%;width:0;background:#2563eb;transition:width .15s}.otaMsg{min-height:1.4em;margin-top:8px;color:#cbd5e1;font-size:.9rem}");
  html += F("button,.btn{display:inline-block;background:#2563eb;color:#fff;text-decoration:none;cursor:pointer;margin:4px 4px 4px 0}.secondary{background:#334155}.danger{background:#7f1d1d}");
  html += F(".remote{font-size:1.25rem;font-weight:700}.sub{color:#aeb9ca;font-size:.9rem}.badge{display:inline-block;padding:5px 9px;border-radius:999px;background:#334155;color:#e5e7eb;font-size:.78rem;margin-top:8px}");
  html += F(".grid2{display:grid;grid-template-columns:1fr 1fr;gap:10px}.grid3{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}.favgrid{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:8px}");
  html += F(".fav{background:#202c40;border-radius:12px;padding:11px}.fav a{text-decoration:none;color:white}.fav small{display:block;color:#94a3b8;margin-top:3px}");
  html += F(".note{background:#0f172a;border-left:3px solid #64748b;padding:10px 12px;border-radius:8px;color:#cbd5e1;font-size:.9rem;margin-top:10px}");
  html += F(".status{font-size:.85rem;color:#93c5fd;margin-top:8px}.empty{color:#94a3b8}.row{display:flex;gap:8px;align-items:center;flex-wrap:wrap}");
  html += F("@media(max-width:620px){.grid2,.grid3{grid-template-columns:1fr}main{padding:12px}.card{padding:15px}}");
  html += F("</style></head><body><main>");

  html += F("<div class='top'><div><h1>Volumefy IR</h1><div class='count'>");
  html += String(REMOTE_COUNT);
  html += F(" profili nel catalogo</div></div><span class='badge'>AP · 192.168.4.1</span></div>");

  html += F("<div class='card'><h2>Telecomando attivo</h2><div class='remote'>");
  html += REMOTES[activeRemote].brand;
  html += F(" · ");
  html += REMOTES[activeRemote].model;
  html += F("</div><div class='sub'>");
  html += REMOTES[activeRemote].category;
  html += F("</div><span class='badge'>");
  html += profileStatusLabel(REMOTES[activeRemote].status);
  html += F("</span><div class='note'>");
  html += REMOTES[activeRemote].note;
  html += F("</div><div class='row' style='margin-top:10px'>");
  html += F("<a class='btn secondary' href='/favorite?id=");
  html += String(activeRemote);
  html += F("'>");
  html += (isFavorite(activeRemote) ? "★ Rimuovi preferito" : "☆ Aggiungi ai preferiti");
  html += F("</a></div></div>");

  html += F("<div class='card'><h2>Prova il telecomando attivo</h2><div class='grid3'>");
  html += F("<a class='btn secondary' href='/test?action=down'>Volume −</a>");
  html += F("<a class='btn secondary' href='/test?action=mute'>Mute</a>");
  html += F("<a class='btn secondary' href='/test?action=up'>Volume +</a>");
  html += F("</div></div>");

  html += F("<div class='card'><h2>Scegli telecomando</h2>");
  html += F("<div class='grid2'><div><label for='type'>Tipo</label><select id='type'></select></div><div><label for='brand'>Marca</label><select id='brand'></select></div></div>");
  html += F("<label for='search'>Cerca modello</label><input id='search' type='search' placeholder='es. Q600A, OLED, BN59, Bravia…' autocomplete='off'>");
  html += F("<form action='/activate' method='get'><label for='model'>Modello / telecomando</label><select id='model' name='id'></select>");
  html += F("<div id='modelInfo' class='status'></div><button type='submit' style='margin-top:12px'>Imposta come attivo</button></form></div>");

  html += F("<div class='card'><h2>Preferiti</h2><div class='favgrid'>");
  bool anyFavorite = false;
  for (size_t i = 0; i < REMOTE_COUNT; ++i) {
    if (!isFavorite(i)) continue;
    anyFavorite = true;
    html += F("<div class='fav'><a href='/activate?id="); html += String(i); html += F("'><strong>★ ");
    html += REMOTES[i].brand; html += F(" "); html += REMOTES[i].model;
    html += F("</strong><small>"); html += REMOTES[i].category; html += F(" · "); html += profileStatusLabel(REMOTES[i].status);
    html += F("</small></a></div>");
  }
  if (!anyFavorite) html += F("<span class='empty'>Nessun preferito salvato.</span>");
  html += F("</div></div>");

  html += F("<div class='card'><h2>Velocità volume IR</h2><p class='sub'>Regola i repeat di VOL+/VOL−. La frequenza portante del protocollo non viene alterata.</p>");
  html += F("<form action='/irspeed' method='get'><input id='speed' type='range' name='value' min='1' max='6' step='1' value='");
  html += String(irVolumeRepeat);
  html += F("' oninput='document.getElementById(&quot;speedValue&quot;).textContent=this.value+&quot;×&quot;'>");
  html += F("<div class='row' style='justify-content:space-between'><strong id='speedValue'>"); html += String(irVolumeRepeat); html += F("×</strong><button type='submit'>Salva velocità</button></div></form>");
  html += F("<p class='sub'>Mute resta una sola pressione logica. Sony SIRC mantiene comunque i repeat minimi richiesti dal protocollo.</p></div>");

  html += F("<div class='card'><h2>Aggiornamento firmware OTA</h2>");
  html += F("<p class='sub'>Carica direttamente il <strong>firmware.bin</strong> generato da PlatformIO. Durante l'aggiornamento il deep sleep viene sospeso; al termine il dispositivo si riavvia automaticamente.</p>");
  html += F("<form id='otaForm' enctype='multipart/form-data'><label for='otaFile'>Firmware .bin</label><input id='otaFile' name='update' type='file' accept='.bin,application/octet-stream' required>");
  html += F("<button id='otaButton' type='submit' style='margin-top:12px'>Aggiorna firmware</button></form>");
  html += F("<div class='progress'><div id='otaBar'></div></div><div id='otaMsg' class='otaMsg'></div>");
  html += F("<div class='note'>Usa il file <strong>.pio/build/esp32-c3-devkitm-1/firmware.bin</strong>. Non scollegare l'alimentazione durante la scrittura.</div></div>");

  html += F("<div class='card'><h2>Comandi fisici</h2><p>Encoder: volume. Pressione breve: mute/unmute. Pressione ~2 s: passa alla modalità Bluetooth.</p>");
  html += F("<p class='sub'>Modalità, telecomando attivo, velocità e preferiti restano salvati anche dopo deep sleep o spegnimento.</p></div>");

  // Catalogo compatto per i filtri client-side.
  html += F("<script>const active="); html += String(activeRemote); html += F(";const C=[");
  for (size_t i = 0; i < REMOTE_COUNT; ++i) {
    if (i) html += ',';
    html += F("{id:"); html += String(i);
    html += F(",t:\""); html += jsEscape(REMOTES[i].category);
    html += F("\",b:\""); html += jsEscape(REMOTES[i].brand);
    html += F("\",m:\""); html += jsEscape(REMOTES[i].model);
    html += F("\",s:\""); html += jsEscape(profileStatusLabel(REMOTES[i].status));
    html += F("\"}");
  }
  html += F("];const T=document.getElementById('type'),B=document.getElementById('brand'),M=document.getElementById('model'),Q=document.getElementById('search'),I=document.getElementById('modelInfo');");
  html += F("const uniq=a=>[...new Set(a)].sort((x,y)=>x.localeCompare(y,'it',{numeric:true}));");
  html += F("function fill(sel,a){sel.innerHTML=a.map(x=>`<option value=\"${x}\">${x}</option>`).join('')}");
  html += F("function models(keep){let q=Q.value.trim().toLowerCase();let a=C.filter(x=>x.t===T.value&&x.b===B.value&&(!q||x.m.toLowerCase().includes(q)));M.innerHTML=a.map(x=>`<option value=\"${x.id}\">${x.m} — ${x.s}</option>`).join('');if(keep&&a.some(x=>x.id===active))M.value=active;info();}");
  html += F("function brands(keep){let a=uniq(C.filter(x=>x.t===T.value).map(x=>x.b));fill(B,a);let ac=C.find(x=>x.id===active);if(keep&&ac&&a.includes(ac.b))B.value=ac.b;models(keep);}");
  html += F("function info(){let x=C.find(x=>String(x.id)===M.value);I.textContent=x?`${x.t} · ${x.b} · ${x.s}`:'Nessun modello con questo filtro';}");
  html += F("let ac=C.find(x=>x.id===active)||C[0];fill(T,uniq(C.map(x=>x.t)));T.value=ac.t;brands(true);M.value=active;info();T.onchange=()=>{Q.value='';brands(false)};B.onchange=()=>{Q.value='';models(false)};Q.oninput=()=>models(false);M.onchange=info;");
  html += F("const OF=document.getElementById('otaForm'),OB=document.getElementById('otaButton'),OM=document.getElementById('otaMsg'),OP=document.getElementById('otaBar');OF.addEventListener('submit',e=>{e.preventDefault();const f=document.getElementById('otaFile').files[0];if(!f)return;OB.disabled=true;OM.textContent='Caricamento firmware…';OP.style.width='0%';const x=new XMLHttpRequest();x.open('POST','/update',true);x.upload.onprogress=p=>{if(p.lengthComputable){const n=Math.round(p.loaded*100/p.total);OP.style.width=n+'%';OM.textContent='Caricamento '+n+'%';}};x.onload=()=>{if(x.status===200){OP.style.width='100%';OM.textContent='Aggiornamento completato. Riavvio in corso…';}else{OB.disabled=false;OM.textContent='Errore OTA: '+x.responseText;}};x.onerror=()=>{OB.disabled=false;OM.textContent='Connessione interrotta durante l’aggiornamento.';};const d=new FormData();d.append('update',f,f.name);x.send(d);});</script>");
  html += F("</main></body></html>");
  server.send(200, "text/html; charset=utf-8", html);
}

void handleActivate() {
  if (!server.hasArg("id")) return redirectHome();
  const int id = server.arg("id").toInt();
  if (id >= 0 && id < static_cast<int>(REMOTE_COUNT)) {
    activeRemote = static_cast<uint8_t>(id);
    saveActiveRemote(activeRemote);
    markActivity();
  }
  redirectHome();
}

void handleFavorite() {
  if (!server.hasArg("id")) return redirectHome();
  const int id = server.arg("id").toInt();
  if (id >= 0 && id < static_cast<int>(REMOTE_COUNT)) {
    toggleFavorite(static_cast<size_t>(id));
    saveFavorites();
    markActivity();
  }
  redirectHome();
}

void handleIrSpeed() {
  if (server.hasArg("value")) {
    int value = server.arg("value").toInt();
    if (value < 1) value = 1;
    if (value > 6) value = 6;
    irVolumeRepeat = static_cast<uint8_t>(value);
    saveIrVolumeRepeat();
    markActivity();
  }
  redirectHome();
}

void handleTest() {
  if (server.hasArg("action")) {
    const String action = server.arg("action");
    if (action == "up") volumeUp();
    else if (action == "down") volumeDown();
    else if (action == "mute") muteUnmute();
  }
  redirectHome();
}

void handleOtaFinished() {
  markActivity();
  server.sendHeader("Connection", "close");

  if (Update.hasError() || !otaUploadSuccess) {
    otaInProgress = false;
    server.send(500, "text/plain; charset=utf-8", "Aggiornamento non riuscito. Il firmware precedente resta attivo.");
    return;
  }

  server.send(200, "text/plain; charset=utf-8", "OK - riavvio");
  otaInProgress = false;
  otaRebootPending = true;
  otaRebootAt = millis() + 1200UL;
}

void handleOtaUpload() {
  HTTPUpload &upload = server.upload();

  switch (upload.status) {
    case UPLOAD_FILE_START:
      otaInProgress = true;
      otaUploadSuccess = false;
      markActivity();
      // UPDATE_SIZE_UNKNOWN usa lo spazio massimo disponibile nella partizione OTA.
      if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
        otaInProgress = false;
      }
      break;

    case UPLOAD_FILE_WRITE:
      markActivity();
      if (!Update.hasError()) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          // Update mantiene internamente lo stato di errore.
        }
      }
      break;

    case UPLOAD_FILE_END:
      markActivity();
      if (!Update.hasError() && Update.end(true)) {
        otaUploadSuccess = true;
      } else {
        otaUploadSuccess = false;
        otaInProgress = false;
      }
      break;

    case UPLOAD_FILE_ABORTED:
      Update.abort();
      otaUploadSuccess = false;
      otaInProgress = false;
      markActivity();
      break;
  }
}

void startIrMode() {
  // BLE non viene mai inizializzato in questa modalita'.
  irsend.begin();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  dnsServer.start(53, "*", WiFi.softAPIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/activate", HTTP_GET, handleActivate);
  server.on("/favorite", HTTP_GET, handleFavorite);
  server.on("/irspeed", HTTP_GET, handleIrSpeed);
  server.on("/test", HTTP_GET, handleTest);
  server.on("/update", HTTP_POST, handleOtaFinished, handleOtaUpload);

  // Endpoint comuni dei captive portal.
  server.on("/generate_204", HTTP_GET, handleRoot);
  server.on("/hotspot-detect.html", HTTP_GET, handleRoot);
  server.on("/fwlink", HTTP_GET, handleRoot);
  server.onNotFound(handleRoot);

  server.begin();
}

void startBleMode() {
  // Wi-Fi e trasmettitore IR restano non inizializzati/spenti.
  WiFi.mode(WIFI_OFF);
  digitalWrite(PIN_IR, LOW);
  bleKeyboard.begin();
}

// -------------------- LED --------------------
void handleLedAndState() {
  const uint32_t now = millis();

  if (currentMode == MODE_BLE) {
    if (bleKeyboard.isConnected()) {
      // Come prima: impulso breve ogni 15 secondi.
      if (now - lastLedBlinkTime >= 15000UL) {
        digitalWrite(PIN_LED, LOW);
        delay(30);
        digitalWrite(PIN_LED, HIGH);
        lastLedBlinkTime = now;
      }
    } else {
      // Come prima: lampeggio ogni secondo durante advertising/discoverable.
      if (now - lastLedBlinkTime >= 1000UL) {
        digitalWrite(PIN_LED, !digitalRead(PIN_LED));
        lastLedBlinkTime = now;
      }
    }
  } else {
    // IR/AP: breve impulso ogni 5 secondi, per distinguerla dalla modalita' BLE.
    if (now - lastLedBlinkTime >= 5000UL) {
      digitalWrite(PIN_LED, LOW);
      delay(30);
      digitalWrite(PIN_LED, HIGH);
      lastLedBlinkTime = now;
    }
  }
}

// -------------------- DEEP SLEEP --------------------
void goToSleep() {
  digitalWrite(PIN_LED, HIGH);

  if (currentMode == MODE_IR) {
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
  }

  preferences.end();

  // Mantiene la stessa logica del progetto originale: wake premendo SW (GPIO4 LOW).
  esp_deep_sleep_enable_gpio_wakeup(1ULL << PIN_SW, ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_deep_sleep_start();
}

// -------------------- SETUP / LOOP --------------------
void setup() {
  pinMode(PIN_CLK, INPUT_PULLUP);
  pinMode(PIN_DT, INPUT_PULLUP);
  pinMode(PIN_SW, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_IR, OUTPUT);

  digitalWrite(PIN_LED, HIGH);
  digitalWrite(PIN_IR, LOW);

  lastCLKState = digitalRead(PIN_CLK);
  rawButtonState = digitalRead(PIN_SW);
  stableButtonState = rawButtonState;
  rawButtonChangedAt = millis();

  // Richiediamo sempre che il tasto sia rilasciato prima di interpretare pressioni.
  // Questo evita che il pulsante usato per risvegliare dal deep sleep generi un comando.
  buttonArmed = (stableButtonState == HIGH);

  loadSettings();
  markActivity();

  if (currentMode == MODE_BLE) {
    startBleMode();
  } else {
    startIrMode();
  }
}

void loop() {
  checkEncoder();
  checkButton();
  handleLedAndState();

  if (currentMode == MODE_IR) {
    dnsServer.processNextRequest();
    server.handleClient();

    if (otaRebootPending && static_cast<int32_t>(millis() - otaRebootAt) >= 0) {
      ESP.restart();
    }
  }

  // Mai entrare in deep sleep mentre la flash OTA e' in scrittura.
  if (!otaInProgress && !otaRebootPending &&
      millis() - lastActivityTime >= TIMEOUT_INACTIVITY_MS) {
    goToSleep();
  }

  delay(1);
}
