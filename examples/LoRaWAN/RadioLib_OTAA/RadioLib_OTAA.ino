/*
  RadioLib LoRaWAN ABP Example

  This example joins a LoRaWAN network and will send
  uplink packets. Before you start, you will have to
  register your device at https://www.thethingsnetwork.org/
  After your device is registered, you can run this example.
  The device will join the network and start uploading data.

  Running this examples REQUIRES you to check "Resets DevNonces"
  on your LoRaWAN dashboard. Refer to the network's
  documentation on how to do this.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/

  For LoRaWAN details, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/LoRaWAN

*/

#include <RadioLib.h>
#include "LoRaBoards.h"

// OLED Display includes
#include <Wire.h>
#include <U8g2lib.h>

#include <Preferences.h>
Preferences store;

// OLED Display object - already defined in LoRaBoards.h

#if  defined(USING_SX1276)
SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);
#elif defined(USING_SX1262)
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#elif defined(USING_SX1278)
SX1278 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);
#elif   defined(USING_LR1121)
LR1121 radio = new Module(RADIO_CS_PIN, RADIO_DIO9_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif

// joinEUI - previous versions of LoRaWAN called this AppEUI
// for development purposes you can use all zeros - see wiki for details
#define RADIOLIB_LORAWAN_JOIN_EUI  0x240708D6F45C3DBE


// the Device EUI & two keys can be generated on the TTN console
#ifndef RADIOLIB_LORAWAN_DEV_EUI   // Replace with your Device EUI
#define RADIOLIB_LORAWAN_DEV_EUI   0x58A0CB00001166AB
#endif
#ifndef RADIOLIB_LORAWAN_APP_KEY   // Replace with your App Key 
#define RADIOLIB_LORAWAN_APP_KEY   0x24, 0x07, 0x08, 0xD6, 0xF4, 0x5C, 0x3D, 0xBE, 0x6D, 0x17, 0x8C, 0x3A, 0x38, 0x7C, 0x82, 0xE6
#endif
#ifndef RADIOLIB_LORAWAN_NWK_KEY   // Put your Nwk Key here
#define RADIOLIB_LORAWAN_NWK_KEY   0x24, 0x07, 0x08, 0xD6, 0xF4, 0x5C, 0x3D, 0xBE, 0x6D, 0x17, 0x8C, 0x3A, 0x38, 0x7C, 0x82, 0xE6
#endif

// how often to send an uplink - consider legal & FUP constraints - see notes
const uint32_t uplinkIntervalSeconds = 5UL * 60UL;    // minutes x seconds


// for the curious, the #ifndef blocks allow for automated testing &/or you can
// put your EUI & keys in to your platformio.ini - see wiki for more tips

// regional choices: EU868, US915, AU915, AS923, IN865, KR920, CN780, CN500
const LoRaWANBand_t Region = EU868;
const uint8_t subBand = 0;  // For US915, change this to 2, otherwise leave on 0

// ============================================================================


// copy over the EUI's & keys in to the something that will not compile if incorrectly formatted
uint64_t joinEUI =   RADIOLIB_LORAWAN_JOIN_EUI;
uint64_t devEUI  =   RADIOLIB_LORAWAN_DEV_EUI;
uint8_t appKey[] = { RADIOLIB_LORAWAN_APP_KEY };
uint8_t nwkKey[] = { RADIOLIB_LORAWAN_NWK_KEY };

// create the LoRaWAN node
LoRaWANNode node(&radio, &Region, subBand);

RTC_DATA_ATTR uint8_t LWsession[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];



// result code to text - these are error codes that can be raised when using LoRaWAN
// however, RadioLib has many more - see https://jgromes.github.io/RadioLib/group__status__codes.html for a complete list
String stateDecode(const int16_t result)
{
    switch (result) {
    case RADIOLIB_ERR_NONE:
        return "ERR_NONE";
    case RADIOLIB_ERR_CHIP_NOT_FOUND:
        return "ERR_CHIP_NOT_FOUND";
    case RADIOLIB_ERR_PACKET_TOO_LONG:
        return "ERR_PACKET_TOO_LONG";
    case RADIOLIB_ERR_RX_TIMEOUT:
        return "ERR_RX_TIMEOUT";
    case RADIOLIB_ERR_CRC_MISMATCH:
        return "ERR_CRC_MISMATCH";
    case RADIOLIB_ERR_INVALID_BANDWIDTH:
        return "ERR_INVALID_BANDWIDTH";
    case RADIOLIB_ERR_INVALID_SPREADING_FACTOR:
        return "ERR_INVALID_SPREADING_FACTOR";
    case RADIOLIB_ERR_INVALID_CODING_RATE:
        return "ERR_INVALID_CODING_RATE";
    case RADIOLIB_ERR_INVALID_FREQUENCY:
        return "ERR_INVALID_FREQUENCY";
    case RADIOLIB_ERR_INVALID_OUTPUT_POWER:
        return "ERR_INVALID_OUTPUT_POWER";
    case RADIOLIB_ERR_NETWORK_NOT_JOINED:
        return "RADIOLIB_ERR_NETWORK_NOT_JOINED";
    case RADIOLIB_ERR_DOWNLINK_MALFORMED:
        return "RADIOLIB_ERR_DOWNLINK_MALFORMED";
    case RADIOLIB_ERR_INVALID_REVISION:
        return "RADIOLIB_ERR_INVALID_REVISION";
    case RADIOLIB_ERR_INVALID_PORT:
        return "RADIOLIB_ERR_INVALID_PORT";
    case RADIOLIB_ERR_NO_RX_WINDOW:
        return "RADIOLIB_ERR_NO_RX_WINDOW";
    case RADIOLIB_ERR_INVALID_CID:
        return "RADIOLIB_ERR_INVALID_CID";
    case RADIOLIB_ERR_UPLINK_UNAVAILABLE:
        return "RADIOLIB_ERR_UPLINK_UNAVAILABLE";
    case RADIOLIB_ERR_COMMAND_QUEUE_FULL:
        return "RADIOLIB_ERR_COMMAND_QUEUE_FULL";
    case RADIOLIB_ERR_COMMAND_QUEUE_ITEM_NOT_FOUND:
        return "RADIOLIB_ERR_COMMAND_QUEUE_ITEM_NOT_FOUND";
    case RADIOLIB_ERR_JOIN_NONCE_INVALID:
        return "RADIOLIB_ERR_JOIN_NONCE_INVALID";
    // case RADIOLIB_ERR_MIC_MISMATCH:
    //     return "RADIOLIB_ERR_MIC_MISMATCH";
    // case RADIOLIB_ERR_MULTICAST_FCNT_INVALID:
    //     return "RADIOLIB_ERR_MULTICAST_FCNT_INVALID";
    case RADIOLIB_ERR_DWELL_TIME_EXCEEDED:
        return "RADIOLIB_ERR_DWELL_TIME_EXCEEDED";
    case RADIOLIB_ERR_CHECKSUM_MISMATCH:
        return "RADIOLIB_ERR_CHECKSUM_MISMATCH";
    case RADIOLIB_ERR_NO_JOIN_ACCEPT:
        return "RADIOLIB_ERR_NO_JOIN_ACCEPT";
    case RADIOLIB_LORAWAN_SESSION_RESTORED:
        return "RADIOLIB_LORAWAN_SESSION_RESTORED";
    case RADIOLIB_LORAWAN_NEW_SESSION:
        return "RADIOLIB_LORAWAN_NEW_SESSION";
    case RADIOLIB_ERR_NONCES_DISCARDED:
        return "RADIOLIB_ERR_NONCES_DISCARDED";
    case RADIOLIB_ERR_SESSION_DISCARDED:
        return "RADIOLIB_ERR_SESSION_DISCARDED";
    }
    return "See https://jgromes.github.io/RadioLib/group__status__codes.html";
}

// helper function to display any issues
void debug(bool failed, const __FlashStringHelper *message, int state, bool halt)
{
    if (failed) {
        Serial.print(message);
        Serial.print(" - ");
        Serial.print(stateDecode(state));
        Serial.print(" (");
        Serial.print(state);
        Serial.println(")");
        while (halt) {
            delay(1);
        }
    }
}

// helper function to display a byte array
void arrayDump(uint8_t *buffer, uint16_t len)
{
    for (uint16_t c = 0; c < len; c++) {
        char b = buffer[c];
        if (b < 0x10) {
            Serial.print('0');
        }
        Serial.print(b, HEX);
    }
    Serial.println();
}


void setup()
{
    Serial.begin(115200);

    setupBoards();
    
    // Initialize OLED Display
    Wire.begin(I2C_SDA, I2C_SCL);
    u8g2->begin();
    u8g2->enableUTF8Print();
    u8g2->setFont(u8g2_font_6x10_tr);
    u8g2->setFontDirection(0);
    
    // Show initial status on OLED
    u8g2->clearBuffer();
    u8g2->setFont(u8g2_font_NokiaLargeBold_tf);
    char deveui_init[20];
    sprintf(deveui_init, "%016llX", RADIOLIB_LORAWAN_DEV_EUI);
    uint16_t str_w = u8g2->getStrWidth(deveui_init);
    u8g2->drawStr((u8g2->getWidth() - str_w) / 2, 16, deveui_init);
    u8g2->drawHLine(5, 21, u8g2->getWidth() - 5);
    u8g2->setFont(u8g2_font_6x10_tr);
    u8g2->drawStr(0, 35, "LoRaWAN OTAA");
    u8g2->drawStr(0, 50, "Initializing...");
    u8g2->sendBuffer();

#ifdef  RADIO_TCXO_ENABLE
    pinMode(RADIO_TCXO_ENABLE, OUTPUT);
    digitalWrite(RADIO_TCXO_ENABLE, HIGH);
#endif

    delay(5000);  // Give time to switch to the serial monitor

    Serial.println(F("\nSetup ... "));

    Serial.println(F("Initialise the radio"));

    int16_t state = 0;  // return value for calls to RadioLib
    Serial.println(F("Initialise the radio"));
    state = radio.begin();
    debug(state != RADIOLIB_ERR_NONE, F("Initialise radio failed"), state, true);

    // Setup LED for powerbank keep-alive
    pinMode(BOARD_LED, OUTPUT);
    digitalWrite(BOARD_LED, !LED_ON); // Start with LED off

#ifdef USING_DIO2_AS_RF_SWITCH
#ifdef USING_SX1262
    // Some SX126x modules use DIO2 as RF switch. To enable
    // this feature, the following method can be used.
    // NOTE: As long as DIO2 is configured to control RF switch,
    //       it can't be used as interrupt pin!
    if (radio.setDio2AsRfSwitch() != RADIOLIB_ERR_NONE) {
        Serial.println(F("Failed to set DIO2 as RF switch!"));
        while (true);
    }
#endif //USING_SX1262
#endif //USING_DIO2_AS_RF_SWITCH

#ifdef RADIO_SWITCH_PIN
    // T-MOTION
    const uint32_t pins[] = {
        RADIO_SWITCH_PIN, RADIO_SWITCH_PIN, RADIOLIB_NC,
    };
    static const Module::RfSwitchMode_t table[] = {
        {Module::MODE_IDLE,  {0,  0} },
        {Module::MODE_RX,    {1, 0} },
        {Module::MODE_TX,    {0, 1} },
        END_OF_MODE_TABLE,
    };
    radio.setRfSwitchTable(pins, table);
#endif

#if  defined(USING_LR1121)
    // LR1121
    // set RF switch configuration for Wio WM1110
    // Wio WM1110 uses DIO5 and DIO6 for RF switching
    static const uint32_t rfswitch_dio_pins[] = {
        RADIOLIB_LR11X0_DIO5, RADIOLIB_LR11X0_DIO6,
        RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC
    };

    static const Module::RfSwitchMode_t rfswitch_table[] = {
        // mode                  DIO5  DIO6
        { LR11x0::MODE_STBY,   { LOW,  LOW  } },
        { LR11x0::MODE_RX,     { HIGH, LOW  } },
        { LR11x0::MODE_TX,     { LOW,  HIGH } },
        { LR11x0::MODE_TX_HP,  { LOW,  HIGH } },
        { LR11x0::MODE_TX_HF,  { LOW,  LOW  } },
        { LR11x0::MODE_GNSS,   { LOW,  LOW  } },
        { LR11x0::MODE_WIFI,   { LOW,  LOW  } },
        END_OF_MODE_TABLE,
    };
    radio.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);

    // LR1121 TCXO Voltage 2.85~3.15V
    radio.setTCXO(3.0);
#endif

    // Override the default join rate
    uint8_t joinDR = 0;  // SF12 for maximum range

    // Setup the OTAA session information
    node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);

    // ##### setup the flash storage
    store.begin("radiolib");
    // ##### if we have previously saved nonces, restore them and try to restore session as well
    if (store.isKey("nonces")) {
        uint8_t buffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];                                       // create somewhere to store nonces
        store.getBytes("nonces", buffer, RADIOLIB_LORAWAN_NONCES_BUF_SIZE); // get them from the store
        state = node.setBufferNonces(buffer);                                                           // send them to LoRaWAN
        debug(state != RADIOLIB_ERR_NONE, F("Restoring nonces buffer failed"), state, false);

        // recall session from RTC deep-sleep preserved variable
        state = node.setBufferSession(LWsession); // send them to LoRaWAN stack

        // if we have booted more than once we should have a session to restore, so report any failure
        // otherwise no point saying there's been a failure when it was bound to fail with an empty LWsession var.
        debug((state != RADIOLIB_ERR_NONE), F("Restoring session buffer failed"), state, false);

        // if Nonces and Session restored successfully, activation is just a formality
        // moreover, Nonces didn't change so no need to re-save them
        if (state == RADIOLIB_ERR_NONE) {
            Serial.println(F("Successfully restored session - now activating"));
            state = node.activateOTAA();
            debug((state != RADIOLIB_LORAWAN_SESSION_RESTORED), F("Failed to activate restored session"), state, true);

            // ##### close the store before returning
            store.end();
        }
    } else {  // store has no key "nonces"
        Serial.println(F("No Nonces saved - starting fresh."));
    }

    // Add option to clear problematic nonces if needed
    // Uncomment the next line if you're getting DevNonce errors in ChirpStack
    // store.remove("nonces");  // This will force fresh nonce generation


    // if we got here, there was no session to restore, so start trying to join
    uint32_t sleepForSeconds = 10*1000;
    state = RADIOLIB_ERR_NETWORK_NOT_JOINED;

    while (state != RADIOLIB_LORAWAN_NEW_SESSION) {

        Serial.println(F("Join ('login') to the LoRaWAN Network"));
        
        // Update OLED with join attempt status
        u8g2->clearBuffer();
        u8g2->setFont(u8g2_font_NokiaLargeBold_tf);
        char deveui_join[20];
        sprintf(deveui_join, "%016llX", RADIOLIB_LORAWAN_DEV_EUI);
        uint16_t str_w2 = u8g2->getStrWidth(deveui_join);
        u8g2->drawStr((u8g2->getWidth() - str_w2) / 2, 16, deveui_join);
        u8g2->drawHLine(5, 21, u8g2->getWidth() - 5);
        u8g2->setFont(u8g2_font_6x10_tr);
        u8g2->drawStr(0, 35, "Joining LoRaWAN...");
        u8g2->drawStr(0, 50, "Attempting OTAA");
        u8g2->sendBuffer();

        // Clear old nonces to force fresh generation and prevent DevNonce reuse
        Serial.println(F("Clearing old nonces to prevent DevNonce reuse"));
        store.remove("nonces");
        
        state = node.activateOTAA();

        // ##### save the join counters (nonces) to permanent store
        Serial.println(F("Saving nonces to flash"));
        uint8_t buffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];           // create somewhere to store nonces
        uint8_t *persist = node.getBufferNonces();                  // get pointer to nonces
        memcpy(buffer, persist, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);  // copy in to buffer
        store.putBytes("nonces", buffer, RADIOLIB_LORAWAN_NONCES_BUF_SIZE); // send them to the store

        // we'll save the session after an uplink

        if (state != RADIOLIB_LORAWAN_NEW_SESSION) {
            Serial.print(F("Join failed: "));
            Serial.println(state);

            // how long to wait before join attempts. This is an interim solution pending
            // implementation of TS001 LoRaWAN Specification section #7 - this doc applies to v1.0.4 & v1.1
            // it sleeps for longer & longer durations to give time for any gateway issues to resolve
            // or whatever is interfering with the device <-> gateway airwaves.
            // uint32_t sleepForSeconds = min((bootCountSinceUnsuccessfulJoin++ + 1UL) * 60UL, 3UL * 60UL);
            Serial.print(F("Boots since unsuccessful join: "));
            // Serial.println(bootCountSinceUnsuccessfulJoin);
            Serial.print(F("Retrying join in "));
            Serial.print(sleepForSeconds / 1000);
            Serial.println(F(" seconds"));
            
            // Update OLED with join failure status
            u8g2->clearBuffer();
            u8g2->setFont(u8g2_font_NokiaLargeBold_tf);
            char deveui_fail[20];
            sprintf(deveui_fail, "%016llX", RADIOLIB_LORAWAN_DEV_EUI);
            uint16_t str_w3 = u8g2->getStrWidth(deveui_fail);
            u8g2->drawStr((u8g2->getWidth() - str_w3) / 2, 16, deveui_fail);
            u8g2->drawHLine(5, 21, u8g2->getWidth() - 5);
            u8g2->setFont(u8g2_font_6x10_tr);
            u8g2->drawStr(0, 35, "Join Failed!");
            u8g2->drawStr(0, 50, "Retry in 10s");
            u8g2->sendBuffer();

            delay(sleepForSeconds);

            // gotoSleep(sleepForSeconds);
        }
    } // if activateOTAA state

    // Print the DevAddr
    Serial.print("[LoRaWAN] DevAddr: ");
    Serial.println((unsigned long)node.getDevAddr(), HEX);
    
    // Update OLED with successful join status and device info
    u8g2->clearBuffer();
    u8g2->setFont(u8g2_font_NokiaLargeBold_tf);
    char deveui_joined[20];
    sprintf(deveui_joined, "%016llX", RADIOLIB_LORAWAN_DEV_EUI);
    uint16_t str_w4 = u8g2->getStrWidth(deveui_joined);
    u8g2->drawStr((u8g2->getWidth() - str_w4) / 2, 16, deveui_joined);
    u8g2->drawHLine(5, 21, u8g2->getWidth() - 5);
    u8g2->setFont(u8g2_font_6x10_tr);
    u8g2->drawStr(0, 35, "LoRaWAN Joined!");
    u8g2->drawStr(0, 50, "Ready to send");
    u8g2->sendBuffer();
    
    // Display device info briefly
    delay(2000);
    displayDeviceInfo();

    // Enable the ADR algorithm (on by default which is preferable)
    node.setADR(true);

    // Set a datarate to start off with
    node.setDatarate(0);  // SF12 for maximum range

    // Manages uplink intervals to the TTN Fair Use Policy
    node.setDutyCycle(true, 1250);

    // Enable the dwell time limits - 400ms is the limit for the US
    node.setDwellTime(true, 400);

    Serial.println(F("Ready!\n"));

    if (u8g2) {
        u8g2->clearBuffer();
        u8g2->setFont(u8g2_font_NokiaLargeBold_tf );
        char deveui_ready[20];
        sprintf(deveui_ready, "%016llX", RADIOLIB_LORAWAN_DEV_EUI);
        uint16_t str_w =  u8g2->getStrWidth(deveui_ready);
        u8g2->drawStr((u8g2->getWidth() - str_w) / 2, 16, deveui_ready);
        u8g2->drawHLine(5, 21, u8g2->getWidth() - 5);

        u8g2->setCursor(0, 38);
        u8g2->print("Join LoRaWAN Ready!");
        u8g2->sendBuffer();
    }
}


// Function to display device info and status on OLED
void displayDeviceInfo() {
    if (!u8g2) return;
    
    u8g2->clearBuffer();
    u8g2->setFont(u8g2_font_NokiaLargeBold_tf);
    
    // Display DevEUI at the top
    char deveui_str1[20];
    sprintf(deveui_str1, "%016llX", RADIOLIB_LORAWAN_DEV_EUI);
    uint16_t str_w = u8g2->getStrWidth(deveui_str1);
    u8g2->drawStr((u8g2->getWidth() - str_w) / 2, 16, deveui_str1);
    u8g2->drawHLine(5, 21, u8g2->getWidth() - 5);
    
    u8g2->setFont(u8g2_font_6x10_tr);
    
    // Display DevEUI
    u8g2->drawStr(0, 35, "DevEUI:");
    char deveui_str[20];
    sprintf(deveui_str, "%016llX", RADIOLIB_LORAWAN_DEV_EUI);
    u8g2->drawStr(50, 35, deveui_str);
    
    // Display DevAddr if available
    if (node.getDevAddr() != 0) {
        u8g2->drawStr(0, 50, "DevAddr:");
        char devaddr_str[12];
        sprintf(devaddr_str, "%08lX", (unsigned long)node.getDevAddr());
        u8g2->drawStr(50, 50, devaddr_str);
    } else {
        u8g2->drawStr(0, 50, "DevAddr: N/A");
    }
    
    u8g2->sendBuffer();
}

// Function to display message status on OLED
void displayMessageStatus(int16_t state, uint32_t fCntUp, bool hasDownlink, size_t downlinkSize) {
    if (!u8g2) return;
    
    u8g2->clearBuffer();
    u8g2->setFont(u8g2_font_NokiaLargeBold_tf);
    
    // Display DevEUI at the top
    char deveui_str2[20];
    sprintf(deveui_str2, "%016llX", RADIOLIB_LORAWAN_DEV_EUI);
    uint16_t str_w = u8g2->getStrWidth(deveui_str2);
    u8g2->drawStr((u8g2->getWidth() - str_w) / 2, 16, deveui_str2);
    u8g2->drawHLine(5, 21, u8g2->getWidth() - 5);
    
    u8g2->setFont(u8g2_font_6x10_tr);
    
    // Display frame counter
    char fcnt_str[20];
    sprintf(fcnt_str, "Frame: %lu", fCntUp);
    u8g2->drawStr(0, 35, fcnt_str);
    
    // Display message status
    if (state < RADIOLIB_ERR_NONE) {
        u8g2->drawStr(0, 50, "Error sending");
    } else if (hasDownlink) {
        if (downlinkSize > 0) {
            u8g2->drawStr(0, 50, "Downlink data");
        } else {
            u8g2->drawStr(0, 50, "MAC commands");
        }
    } else {
        u8g2->drawStr(0, 50, "Uplink sent");
    }
    
    u8g2->sendBuffer();
}

// Function to keep powerbank alive with small periodic load
void powerbankKeepAlive() {
    static unsigned long lastBlink = 0;
    static bool ledState = false;
    unsigned long currentTime = millis();
    
    // Blink LED every 10 seconds to keep powerbank active
    if (currentTime - lastBlink >= 10000) {
        ledState = !ledState;
        digitalWrite(BOARD_LED, ledState ? LED_ON : !LED_ON);
        lastBlink = currentTime;
    }
}

void loop()
{
    // Keep powerbank alive with periodic LED activity
    powerbankKeepAlive();
    
    int16_t state = RADIOLIB_ERR_NONE;
    
    // Display device info on OLED
    displayDeviceInfo();

    // set battery fill level - the LoRaWAN network server
    // may periodically request this information
    // 0 = external power source
    // 1 = lowest (empty battery)
    // 254 = highest (full battery)
    // 255 = unable to measure
    uint8_t battLevel = 146;
    node.setDeviceStatus(battLevel);

    // This is the place to gather the sensor inputs
    // Instead of reading any real sensor, we just generate some random numbers as example
    uint8_t value1 = radio.random(100);
    uint16_t value2 = radio.random(2000);

    // Build payload byte array
    uint8_t uplinkPayload[3];
    uplinkPayload[0] = value1;
    uplinkPayload[1] = highByte(value2);   // See notes for high/lowByte functions
    uplinkPayload[2] = lowByte(value2);

    uint8_t downlinkPayload[10];  // Make sure this fits your plans!
    size_t  downlinkSize;         // To hold the actual payload size received

    // you can also retrieve additional information about an uplink or
    // downlink by passing a reference to LoRaWANEvent_t structure
    LoRaWANEvent_t uplinkDetails;
    LoRaWANEvent_t downlinkDetails;

    uint8_t fPort = 10;

    // Retrieve the last uplink frame counter
    uint32_t fCntUp = node.getFCntUp();

    // Send a confirmed uplink on the second uplink
    // and also request the LinkCheck and DeviceTime MAC commands
    Serial.println(F("Sending uplink"));
    if (fCntUp == 1) {
        Serial.println(F("and requesting LinkCheck and DeviceTime"));
        node.sendMacCommandReq(RADIOLIB_LORAWAN_MAC_LINK_CHECK);
        node.sendMacCommandReq(RADIOLIB_LORAWAN_MAC_DEVICE_TIME);
        state = node.sendReceive(uplinkPayload, sizeof(uplinkPayload), fPort, downlinkPayload, &downlinkSize, true, &uplinkDetails, &downlinkDetails);
    } else {
        state = node.sendReceive(uplinkPayload, sizeof(uplinkPayload), fPort, downlinkPayload, &downlinkSize, false, &uplinkDetails, &downlinkDetails);
    }
    debug(state < RADIOLIB_ERR_NONE, F("Error in sendReceive"), state, false);

    // Check if a downlink was received
    // (state 0 = no downlink, state 1/2 = downlink in window Rx1/Rx2)
    if (state > 0) {
        Serial.println(F("Received a downlink"));
        // Did we get a downlink with data for us
        if (downlinkSize > 0) {
            Serial.println(F("Downlink data: "));
            arrayDump(downlinkPayload, downlinkSize);
        } else {
            Serial.println(F("<MAC commands only>"));
        }
        
        // Display message status on OLED
        displayMessageStatus(state, fCntUp, true, downlinkSize);

        // print RSSI (Received Signal Strength Indicator)
        Serial.print(F("[LoRaWAN] RSSI:\t\t"));
        Serial.print(radio.getRSSI());
        Serial.println(F(" dBm"));

        // print SNR (Signal-to-Noise Ratio)
        Serial.print(F("[LoRaWAN] SNR:\t\t"));
        Serial.print(radio.getSNR());
        Serial.println(F(" dB"));

        // print extra information about the event
        Serial.println(F("[LoRaWAN] Event information:"));
        Serial.print(F("[LoRaWAN] Confirmed:\t"));
        Serial.println(downlinkDetails.confirmed);
        Serial.print(F("[LoRaWAN] Confirming:\t"));
        Serial.println(downlinkDetails.confirming);
        Serial.print(F("[LoRaWAN] Datarate:\t"));
        Serial.println(downlinkDetails.datarate);
        Serial.print(F("[LoRaWAN] Frequency:\t"));
        Serial.print(downlinkDetails.freq, 3);
        Serial.println(F(" MHz"));
        Serial.print(F("[LoRaWAN] Frame count:\t"));
        Serial.println(downlinkDetails.fCnt);
        Serial.print(F("[LoRaWAN] Port:\t\t"));
        Serial.println(downlinkDetails.fPort);
        Serial.print(F("[LoRaWAN] Time-on-air: \t"));
        Serial.print(node.getLastToA());
        Serial.println(F(" ms"));
        Serial.print(F("[LoRaWAN] Rx window: \t"));
        Serial.println(state);

        uint8_t margin = 0;
        uint8_t gwCnt = 0;
        if (node.getMacLinkCheckAns(&margin, &gwCnt) == RADIOLIB_ERR_NONE) {
            Serial.print(F("[LoRaWAN] LinkCheck margin:\t"));
            Serial.println(margin);
            Serial.print(F("[LoRaWAN] LinkCheck count:\t"));
            Serial.println(gwCnt);
        }

        uint32_t networkTime = 0;
        uint8_t fracSecond = 0;
        if (node.getMacDeviceTimeAns(&networkTime, &fracSecond, true) == RADIOLIB_ERR_NONE) {
            Serial.print(F("[LoRaWAN] DeviceTime Unix:\t"));
            Serial.println(networkTime);
            Serial.print(F("[LoRaWAN] DeviceTime second:\t1/"));
            Serial.println(fracSecond);
        }

    } else {
        Serial.println(F("[LoRaWAN] No downlink received"));
        
        // Display message status on OLED
        displayMessageStatus(state, fCntUp, false, 0);
    }

    // wait before sending another packet
    uint32_t minimumDelay = uplinkIntervalSeconds * 1000UL;
    uint32_t interval = node.timeUntilUplink();     // calculate minimum duty cycle delay (per FUP & law!)
    uint32_t delayMs = max(interval, minimumDelay); // cannot send faster than duty cycle allows

    Serial.print(F("[LoRaWAN] Next uplink in "));
    Serial.print(delayMs / 1000);
    Serial.println(F(" seconds\n"));

    // Keep powerbank alive during the main delay with frequent small activities
    unsigned long startDelay = millis();
    while (millis() - startDelay < delayMs) {
        powerbankKeepAlive();
        delay(1000); // Check every second
    }
}
