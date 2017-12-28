#include <Arduino.h>

#include <ESP.h>
#include <ESP8266WiFi.h>

#include "DataDefinition.h"
#include "PrivateConfig.h"

extern "C" {
  #include "user_interface.h"
  #include <espnow.h>
}

GwData data;

uint8_t sendBuffer[sizeof(data)];

ADC_MODE(ADC_VCC);

void setup() {
  pinMode( D1, OUTPUT );
  digitalWrite( D1, HIGH );

  #ifdef DEBUG
  Serial.begin(74880);
  Serial.println();
  Serial.println();
  Serial.printf( "[%8d] -- ESP_Now Controller\n", millis());
  Serial.println();
  #endif

  WiFi.mode(WIFI_STA); // Station mode for esp-now sensor node
  WiFi.disconnect();

  #ifdef DEBUG
  Serial.printf("This mac: %s, ", WiFi.macAddress().c_str());
  Serial.printf("target mac: %02x%02x%02x%02x%02x%02x", remoteMac[0], remoteMac[1], remoteMac[2], remoteMac[3], remoteMac[4], remoteMac[5]);
  Serial.printf(", channel: %i\n", WIFI_CHNL);
  #endif

  if (esp_now_init() != 0) {
    #ifdef DEBUG
    Serial.println("*** ESP_Now init failed");
    #endif
    ESP.reset();
  }
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(remoteMac, ESP_NOW_ROLE_SLAVE, WIFI_CHNL, NULL, 0);

  esp_now_register_send_cb([](uint8_t* mac, uint8_t sendStatus) {
    #ifdef DEBUG
    Serial.printf("[%8d] -- send_cb, send done, status = %i\n", millis(), sendStatus);
    #endif
  });
}

void loop() {
  system_rtc_mem_read( 65, &data.counter, 1 );
  if ( data.counter < 0 )
  {
    data.counter = 0;
  }
  data.counter++;
  system_rtc_mem_write( 65, &data.counter, 1 );

  strcpy( data.name, OPENHAB_ITEM_NAME );
  strcpy( data.action, OPENHAB_ACTION );
  data.battery = ((double)ESP.getVcc()) / 1000.0;
  data.timestamp = millis();
  memcpy( sendBuffer, &data, sizeof(data));

  #ifdef DEBUG
  Serial.printf( "data.name = %s\n", data.name );

  Serial.print( "data.counter = " );
  Serial.println( data.counter );

  Serial.print( "data.timestamp = " );
  Serial.println( data.timestamp );

  Serial.print( "data.battery = " );
  Serial.println( data.battery );

  Serial.printf( "data.action = %s\n", data.action );
  Serial.printf( "data size = %d\n", sizeof(data) );
  #endif

  esp_now_send( NULL, sendBuffer, sizeof(data));

  #ifdef DEBUG
  Serial.printf( "[%8d] -- deep sleep\n", millis());
  #endif

  delay( 100 );
  digitalWrite( D1, LOW );
  ESP.deepSleep(0);
}
