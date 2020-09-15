//automatically polls the listed circuits and sends them temperature compensation

#include <Ezo_uart.h>
#include <Adafruit_Sensor.h>
#include <SoftwareSerial.h> 
#include <Wire.h> //I2C用ライブラリ 
#include <bluefruit.h> //Adafruit用ライブラリ
//we have to include the SoftwareSerial library, or else we can't use it
#define rx A1                                          //define what pin rx is going to be
#define tx A2                                         //define what pin tx is going to be
SoftwareSerial myserial(rx, tx);                      //define how the soft serial port is going to work

int s1 = A5;                                           //Arduino pin 6 to control pin S1
int s2 = A4;                                           //Arduino pin 5 to control pin S2
int s3 = A3;                                           //Arduino pin 4 to control pin S3
int port = 1;                                         //what port to open

const uint8_t bufferlen = 32;                         //total buffer size for the response_data array
char response_data[bufferlen];                        //character array to hold the response data from modules
String inputstring = "";                              //a string to hold incoming data from the PC

// create objects to represent the Modules you're connecting to
// they can accept hardware or software serial ports, and a name of your choice
Ezo_uart Module3_RTD(myserial, "RTD");
Ezo_uart Module2_EC(myserial, "EC");
Ezo_uart Module1_PH(myserial, "PH");

// the modules are ordered in an array according to their position in the serial port expander
// so Modules[0] holds the module in port1, Modules[1] holds the module in port 2, etc
const uint8_t module_count = 3;                       //total size fo the Modules array
Ezo_uart Modules[module_count] = {                    //create an array to hold all the modules
  Module3_RTD, Module2_EC, Module1_PH
};

BLEService        infrared_service = BLEService(0x6A293BAFEA784D06B17B6BC46C78582B);
BLECharacteristic infrared_char = BLECharacteristic(0xFB5C531BE08445A99FA3935C869E2A58);

BLEDis bledis;    // DIS (Device Information Service) helper class instance
BLEBas blebas; 

uint8_t  bps = 0;

void setup() {
  Wire.begin(); //I2C通信開始
  Serial.begin(9600);                                 //Set the hardware serial port to 9600
  myserial.begin(9600);                               //set baud rate for the software serial port to 9600
  inputstring.reserve(20);                            //set aside some bytes for receiving data from the PC
  pinMode(s1, OUTPUT);                                //Set the digital pin as output
  pinMode(s2, OUTPUT);                                //Set the digital pin as output
  pinMode(s3, OUTPUT);                                //Set the digital pin as output

  // in order to use multiple modules more effectively we need to turn off continuous mode and the *ok response
  for (uint8_t i = 0; i < module_count; i++) {        // loop through the modules
    open_port(i + 1);                                 // open the port
    Modules[i].send_cmd_no_resp("c,0");               //send the command to turn off continuous mode
    //in this case we arent concerned about waiting for the reply
    delay(100);
    Modules[i].send_cmd_no_resp("*ok,0");             //send the command to turn off the *ok response
    //in this case we wont get a reply since its been turned off
    delay(100);
    Modules[i].flush_rx_buffer();                     //clear all the characters that we received from the responses of the above commands
  }
  
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



}



void print_reading(Ezo_uart &Module) {                //takes a reference to a Module
  //send_read() sends the read command to the module then converts the
  //answer to a float which can be retrieved with get_reading()
  //it returns a bool indicating if the reading was obtained successfully
  if (Module.send_read()) {
    Serial.print(Module.get_name());                  //prints the module's name
    Serial.print(": ");
    Serial.print(Module.get_reading());               //prints the reading we obtained
  }
}

void print_reading(Ezo_uart &Module, float tempcomp) {                //takes a reference to a Module
  //send_read_wth_temp_comp() sends the "RT" command with a temperature to the module
  //then converts the answer to a float which can be retrieved with get_reading()
  //it returns a bool indicating if the reading was obtained successfully
  if (Module.send_read_with_temp_comp(tempcomp)) {
    Serial.print(Module.get_name());                  //prints the module's name
    Serial.print(": ");
    Serial.print(Module.get_reading());               //prints the reading we obtained
  }
}

void open_port(uint8_t _port) {                                  //this function controls what port is opened on the serial port expander

  if (_port < 1 || _port > 8)_port = 1;                //if the value of the port is within range (1-8) then open that port. If it’s not in range set it to port 1
  uint8_t port_bits = _port - 1;

  digitalWrite(s1, bitRead(port_bits, 0));               //Here we have two commands combined into one.
  digitalWrite(s2, bitRead(port_bits, 1));               //The digitalWrite command sets a pin to 1/0 (high or low)
  digitalWrite(s3, bitRead(port_bits, 2));               //The bitRead command tells us what the bit value is for a specific bit location of a number
  delay(2);  //this is needed to make sure the channel switching event has completed
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
void loop() {

if ( Bluefruit.connected() ) {
  uint8_t infrared_data[12];
    

  float last_temp = 25; 
  float last_ph = 25;
  float last_do = 25;
  //variable that holds our temperature, defaults to 25C

  open_port(3);                                     //open the 4th port, which in this case is RTD
  print_reading(Module3_RTD);                       //acquire the reading from the RTD circuit first
  if (Module3_RTD.get_reading() < -1020) {          //if theres no probe attached
    last_temp = 25;                                 //default it to 25C
  } else {
    last_temp = Module3_RTD.get_reading();          //otherwise its the reading from the RTD module
  }
//  if (Module4_RTD.get_reading() < -1020) {          //if theres no probe attached
//    last_temp = 25;                                 //default it to 25C
//  } else {
//    last_temp = Module4_RTD.get_reading();          //otherwise its the reading from the RTD module
//  }

  //open the ports of the modules and send the temperature and get their reading one by one
  Serial.print(" ");
  open_port(1);
  //print_reading(Module1_PH, last_temp);
  if (Module1_PH.send_read()) {
//    Serial.print(Module1_PH.get_name());                  //prints the module's name
//    Serial.print(": ");
//    Serial.print(Module1_PH.get_reading());
    last_ph = Module1_PH.get_reading();
    //prints the reading we obtained
  }else{
    last_ph = 7;
  }
  open_port(2);
  //print_reading(Module2_EC, last_temp);
  //Serial.print(" ");
    if (Module2_EC.send_read()) {
//    Serial.print(Module2_EC.get_name());                  //prints the module's name
//    Serial.print(": ");
//    Serial.print(Module2_EC.get_reading());  
    last_do =Module2_EC.get_reading();
    //prints the reading we obtained
  }else{
    last_do = 0;
  }


  memcpy(infrared_data,&last_temp,4);
  memcpy(infrared_data+4,&last_do,4);
  memcpy(infrared_data+8,&last_ph,4);
  if ( infrared_char.notify(infrared_data, sizeof(infrared_data)) ) {
      bps++;
      Serial.print("\nHeart Rate Measurement updated to: "); Serial.println(bps);
    } else {
      digitalToggle(LED_RED);
      //Serial.println("\nERROR: Notify not set in the CCCD or not connected!");
    startAdv();
    }
}
  // Only send update once per second
  delay(1000);
  Serial.println("fin");
}
