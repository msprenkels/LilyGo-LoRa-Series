/*
  RadioLib LoRaWAN ABP Example

  ABP = Activation by Personalisation, an alternative
  to OTAA (Over the Air Activation). OTAA is preferable.

  This example will send uplink packets to a LoRaWAN network.
  Before you start, you will have to register your device at
  https://www.thethingsnetwork.org/
  After your device is registered, you can run this example.
  The device will join the network and start uploading data.

  LoRaWAN v1.0.4/v1.1 requires the use of persistent storage.
  As this example does not use persistent storage, running this
  examples REQUIRES you to check "Resets frame counters"
  on your LoRaWAN dashboard. Refer to the notes or the
  network's documentation on how to do this.
  To comply with LoRaWAN's persistent storage, refer to
  https://github.com/radiolib-org/radiolib-persistence

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/

  For LoRaWAN details, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/LoRaWAN

*/

#include "configABP.h"

// Additional includes for minimal setup
#include <Wire.h>
#include <U8g2lib.h>

// Function to display device info and status on OLED
void displayDeviceInfo() {
    if (!u8g2) return;
    
    u8g2->clearBuffer();
    u8g2->setFont(u8g2_font_NokiaLargeBold_tf);
    
    // Display DevAddr at the top (ABP always has this)
    char devaddr_str1[12];
    sprintf(devaddr_str1, "%08lX", (unsigned long)devAddr);
    uint16_t str_w = u8g2->getStrWidth(devaddr_str1);
    u8g2->drawStr((u8g2->getWidth() - str_w) / 2, 16, devaddr_str1);
    u8g2->drawHLine(5, 21, u8g2->getWidth() - 5);
    
    u8g2->setFont(u8g2_font_6x10_tr);
    
    // Display region
    u8g2->drawStr(0, 50, "Region: EU868");
    
    u8g2->sendBuffer();
}

// Function to display message status on OLED
void displayMessageStatus(int16_t state, uint32_t fCntUp, bool hasDownlink, size_t downlinkSize) {
    if (!u8g2) return;
    
    u8g2->clearBuffer();
    u8g2->setFont(u8g2_font_NokiaLargeBold_tf);
    
    // Display DevAddr at the top (ABP always has this)
    char devaddr_str2[12];
    sprintf(devaddr_str2, "%08lX", (unsigned long)devAddr);
    uint16_t str_w = u8g2->getStrWidth(devaddr_str2);
    u8g2->drawStr((u8g2->getWidth() - str_w) / 2, 16, devaddr_str2);
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

void setup()
{
    Serial.begin(115200);

    // Minimal setup - avoid setupBoards() which can hang on USB-C power only
    Serial.println(F("\nSetup ... "));
    
    // Initialize I2C for OLED only (minimal)
    Wire.begin(I2C_SDA, I2C_SCL);
    
    // Initialize OLED display
    u8g2 = new DISPLAY_MODEL(U8G2_R0, U8X8_PIN_NONE);
    if (u8g2) {
        u8g2->begin();
        u8g2->clearBuffer();
        u8g2->setFont(u8g2_font_NokiaLargeBold_tf);
        u8g2->drawStr(0, 16, "Starting...");
        u8g2->sendBuffer();
    }

    Serial.println(F("OLED initialized"));

#ifdef  RADIO_TCXO_ENABLE
    pinMode(RADIO_TCXO_ENABLE, OUTPUT);
    digitalWrite(RADIO_TCXO_ENABLE, HIGH);
    Serial.println(F("TCXO enabled"));
#endif

    Serial.println(F("Initialise the radio"));
    
    // Try to initialize radio with timeout
    unsigned long startTime = millis();
    int state = RADIOLIB_ERR_NONE;
    
    // First try without RF switch configuration
    Serial.println(F("Trying radio.begin() without RF switch..."));
    state = radio.begin();
    
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("Radio init failed: "));
        Serial.println(state);
        if (u8g2) {
            u8g2->clearBuffer();
            u8g2->setFont(u8g2_font_6x10_tr);
            u8g2->drawStr(0, 16, "Radio init failed");
            u8g2->drawStr(0, 30, "Error: ");
            char errorStr[10];
            sprintf(errorStr, "%d", state);
            u8g2->drawStr(0, 45, errorStr);
            u8g2->sendBuffer();
        }
        return; // Stop here if radio fails
    }
    
    Serial.println(F("Radio initialized successfully"));

    // Setup LED for powerbank keep-alive
    pinMode(BOARD_LED, OUTPUT);
    digitalWrite(BOARD_LED, !LED_ON); // Start with LED off

#if  defined(USING_LR1121)
    Serial.println(F("Configuring LR1121 RF switch..."));
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
    Serial.println(F("RF switch configured"));

    // LR1121 TCXO Voltage 2.85~3.15V
    state = radio.setTCXO(3.0);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print(F("TCXO config failed: "));
        Serial.println(state);
    } else {
        Serial.println(F("TCXO configured"));
    }
#endif

    Serial.println(F("Initialise LoRaWAN Network credentials"));
    node.beginABP(devAddr, fNwkSIntKey, sNwkSIntKey, nwkSEncKey, appSKey);

    // Set datarate to SF12 for maximum range
    node.setDatarate(0);

    // Enable duty cycle protection for EU868 compliance
    node.setDutyCycle(true, 1250); // 1.25% duty cycle limit

    Serial.println(F("Activating ABP..."));
    int16_t abpState = node.activateABP();
    if (abpState != RADIOLIB_ERR_NONE) {
        Serial.print(F("Activate ABP failed: "));
        Serial.println(abpState);
        if (u8g2) {
            u8g2->clearBuffer();
            u8g2->setFont(u8g2_font_6x10_tr);
            u8g2->drawStr(0, 16, "ABP activation failed");
            u8g2->drawStr(0, 30, "Error: ");
            char errorStr[10];
            sprintf(errorStr, "%d", abpState);
            u8g2->drawStr(0, 45, errorStr);
            u8g2->sendBuffer();
        }
        return;
    }

    Serial.println(F("Ready!\n"));
    
    // Display device info on OLED
    displayDeviceInfo();
}

void loop()
{
    // Keep powerbank alive with periodic LED activity
    powerbankKeepAlive();
    
    // Display device info on OLED for 3 seconds
    displayDeviceInfo();
    delay(3000);
    powerbankKeepAlive(); // Keep powerbank alive during display
    
    Serial.println(F("Sending uplink"));

    // This is the place to gather the sensor inputs
    // Instead of reading any real sensor, we just generate some random numbers as example
    uint8_t value1 = radio.random(100);
    uint16_t value2 = radio.random(2000);

    // Build payload byte array
    uint8_t uplinkPayload[3];
    uplinkPayload[0] = value1;
    uplinkPayload[1] = highByte(value2);   // See notes for high/lowByte functions
    uplinkPayload[2] = lowByte(value2);

    // Perform an uplink
    int state = node.sendReceive(uplinkPayload, sizeof(uplinkPayload));
    debug(state < RADIOLIB_ERR_NONE, F("Error in sendReceive"), state, false);

    // Get the current frame counter for display
    uint32_t fCntUp = node.getFCntUp();

    // Check if a downlink was received
    // (state 0 = no downlink, state 1/2 = downlink in window Rx1/Rx2)
    if (state > 0) {
        Serial.println(F("Received a downlink"));
        
        // Display message status on OLED
        displayMessageStatus(state, fCntUp, true, 0);
        delay(2000);  // Show message status for 2 seconds
        powerbankKeepAlive(); // Keep powerbank alive during display
    } else {
        Serial.println(F("No downlink received"));
        
        // Display message status on OLED
        displayMessageStatus(state, fCntUp, false, 0);
        delay(2000);  // Show message status for 2 seconds
        powerbankKeepAlive(); // Keep powerbank alive during display
    }

    Serial.print(F("Next uplink in "));
    Serial.print(uplinkIntervalSeconds);
    Serial.println(F(" seconds\n"));

    // Wait until next uplink - observing legal duty cycle constraints
    uint32_t interval = node.timeUntilUplink();     // Calculate minimum duty cycle delay
    uint32_t delayMs = max((uint32_t)(uplinkIntervalSeconds * 1000UL), interval); // Respect both interval AND duty cycle
    
    Serial.print(F("[LoRaWAN] Duty cycle delay: "));
    Serial.print(interval / 1000);
    Serial.println(F(" seconds"));
    
    // Keep powerbank alive during the main delay with frequent small activities
    unsigned long startDelay = millis();
    while (millis() - startDelay < delayMs) {
        powerbankKeepAlive();
        delay(1000); // Check every second
    }
}
