#include <Adafruit_HDC1000.h>

#include <Adafruit_Sensor.h>

#include <Wire.h> //I2C用ライブラリ
//#include <SparkFun_GridEYE_Arduino_Library.h> //センサ用ライブラリ
#include <bluefruit.h> //Adafruit用ライブラリ

#include <SoftwareSerial.h>
//#include "Adafruit_HDC1000.h"

#include <Adafruit_TSL2561_U.h>
Adafruit_HDC1000 hdc = Adafruit_HDC1000();

BLEService        infrared_service = BLEService(0x6A293BAFEA784D06B17B6BC46C78582B);
BLECharacteristic infrared_char = BLECharacteristic(0xFB5C531BE08445A99FA3935C869E2A58);

BLEDis bledis;    // DIS (Device Information Service) helper class instance
BLEBas blebas;    // BAS (Battery Service) helper class instance

Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

uint8_t  bps = 0;

//GridEYE grideye;
float a;
float b;
float c;

 /* Create a sensors_event_t object in memory to hold our results */
 sensors_event_t event;
 
void setup()
{
  Wire.begin(); //I2C通信開始
  //grideye.begin(); //GridEYEの開始
  Serial.begin(9600); //シリアル通信開始
//  while ( !Serial ) delay(10);   // for nrf52840 with native usb

  Serial.println("Bluefruit52 may16a Test");
  Serial.println("-----------------------\n");

  // Initialise the Bluefruit module
  Serial.println("Initialise the Bluefruit nRF52 module");
  Bluefruit.begin();

  // Set the advertised device name (keep it short!)
  Serial.println("Setting Device Name to 'Bluefruit52 test'");
  Bluefruit.setName("Bluefruit52 test");

  // Set the connect/disconnect callback handlers
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // Configure and Start the Device Information Service
  Serial.println("Configuring the Device Information Service");
  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Bluefruit Feather52");
  bledis.begin();

  // Start the BLE Battery Service and set it to 100%
  Serial.println("Configuring the Battery Service");
  blebas.begin();
  blebas.write(100);

  // Setup the Heart Rate Monitor service using
  // BLEService and BLECharacteristic classes
  Serial.println("Configuring the Infrared Service");
  setup_Infrared();

  // Setup the advertising packet(s)
  Serial.println("\nSetting up the advertising payload(s)");
  startAdv();

  Serial.println("Ready Player One!!!");
  Serial.println("\nAdvertising");

    if (!hdc.begin()) {
    Serial.println("Couldn't find sensor!");
    while (1);
  }
  
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include HRM Service UUID
  Bluefruit.Advertising.addService(infrared_service);

  // Include Name
  Bluefruit.Advertising.addName();

  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(300);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

void setup_Infrared(void)
{
  infrared_service.begin();

  infrared_char.setProperties(CHR_PROPS_NOTIFY);
  infrared_char.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  infrared_char.setMaxLen(64);//size変更
  infrared_char.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
  infrared_char.begin();

  uint8_t infrared_data[12];
  for (unsigned char i = 0; i < 12; i++) { //センサから値を取得
    //infrared_data[i] = grideye.getPixelTemperature(i);
    infrared_data[i] = 0;
    Serial.print(infrared_data[i]); Serial.print(" ");
  }
  infrared_char.write(infrared_data, sizeof(infrared_data));

}

void connect_callback(uint16_t conn_handle)
{
  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);

  char central_name[32] = { 0 };
  connection->getPeerName(central_name, sizeof(central_name));

  Serial.print("Connected to ");
  Serial.println(central_name);
}

/**
   Callback invoked when a connection is dropped
   @param conn_handle connection where this event happens
   @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
*/
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;

  Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
  Serial.println("Advertising!");
}

void cccd_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint16_t cccd_value)
{
  // Display the raw request packet
  Serial.print("CCCD Updated: ");
  //Serial.printBuffer(request->data, request->len);
  Serial.print(cccd_value);
  Serial.println("");

  // Check the characteristic this CCCD update is associated with in case
  // this handler is used for multiple CCCD records.
  if (chr->uuid == infrared_char.uuid) {
    if (chr->notifyEnabled(conn_hdl)) {
      Serial.println("Heart Rate Measurement 'Notify' enabled");
    } else {
      Serial.println("Heart Rate Measurement 'Notify' disabled");
    }
  }
}

void loop()
{
      /* Get a new sensor event */ 
    sensors_event_t event;
    tsl.getEvent(&event);

  if ( Bluefruit.connected() ) {
    uint8_t infrared_data[12];
//    for (int i = 0; i < 2; i++) { //センサから値を取得
//      //infrared_data[i] = grideye.getPixelTemperature(i);
//      infrared_data[i] =1;
//      Serial.print(infrared_data[i]); Serial.print(" ");
//    }

     a = hdc.readTemperature();
     b = hdc.readHumidity();
     memcpy(infrared_data,&a,4);
     memcpy(infrared_data+4,&b,4);

     
       if (event.light){
    Serial.print(event.light); Serial.println(" lux");
    //h[0] = event.light;
     //= event.light;
    c = event.light;
    //Serial.print(sizeof(event.light));
        }
       else
       {
    /* If event.light = 0 lux the sensor is probably saturated
       and no reliable data could be generated! */
        Serial.println("Sensor overload");
        c = 0;
      }
  
    memcpy(infrared_data+8,&c,4);
    // Note: We use .notify instead of .write!
    // If it is connected but CCCD is not enabled
    // The characteristic's value is still updated although notification is not sent
    if ( infrared_char.notify(infrared_data, sizeof(infrared_data)) ) {
      bps++;
      Serial.print("\nHeart Rate Measurement updated to: "); Serial.println(bps);
    } else {
      digitalToggle(LED_RED);
      //Serial.println("\nERROR: Notify not set in the CCCD or not connected!");
    startAdv();
    }
  
  Serial.print("Temp: "); Serial.print(a);
  Serial.print("\t\tHum: "); Serial.println(b);
  delay(500);

  }

  // Only send update once per second
  delay(1000);
}
