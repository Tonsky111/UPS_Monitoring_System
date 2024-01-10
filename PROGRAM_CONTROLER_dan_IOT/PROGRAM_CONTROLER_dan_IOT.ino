#include <SoftwareSerial.h>
#include <PZEM004Tv30.h>
#include <ESP8266WiFi.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

//KONEKSI CLOUD INFLUXDB
#define INFLUXDB_URL "http://167.71.213.138:8086"
#define INFLUXDB_TOKEN "DTrKLxLSh_QIZqRYd_oD7LkjfWGl0UexRz9RmubQM8Gwuz6OFm5xr3D0jYaUME9rPsoGcvoKRskxOi4s6k_0IA=="
#define INFLUXDB_ORG "VisiBukaka"
#define INFLUXDB_BUCKET "win_monitoring_dev"

#define DEVICE  "ESP8266"
#define TZ_INFO "UTC7"                                                                                         //Time zone info
#define INFLUXDB_SEND_TIME (30000)                                                                              //SEND DATA TO INFLUX EVERY 1 Second
#define PZEM_REFRESH_TIME (10000)                                                                               //Delay Pembacaan Sensor


//Variabel data influxDBZ
static uint8_t pzemV1 = 0;                                                                                        
static uint8_t pzemC1 = 0;
static uint8_t pzemV2 = 0;
static uint8_t pzemC2 = 0;
static bool relayStat = 0;


#define relay  D5                                                                                             //relaypin
PZEM004Tv30 pzem1 (D1, D2), pzem2 (D3, D4);                                                                   // RX, TX for SoftwareSerial


// Declare InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Declare Data point
Point sensor("Sensor_Data");

const char *ssid = "Haikal SuperSpeed";
const char *password = "haikalganteng";

//declare global function
static void SystemInit(void);
static void wifiSetup(void);
static void InfluxInit(void);
static void pzemInit(void);
static void Sensor_PZEM(void);
static void RelayLogic(void);
static void InfluxTsk(void);

static uint32_t pzemRefresh = 0;                                                                              //refresh sensor delay
static uint32_t influxdb_send_timestamp = 0;                                                                  //refresh influxDB delay

static unsigned long interval = 20000;
static unsigned long startTime = 0;



void setup() {
    
    SystemInit();
    wifiSetup();
    InfluxInit();
    pzemInit();

}

void loop() {

    Sensor_PZEM();
    RelayLogic();
    InfluxTsk();
}



static void pzemInit (void){
  
  pzemRefresh = millis();

}


static void SystemInit (void){
    
    Serial.begin (9600);
    pinMode (relay, OUTPUT);

}
static void wifiSetup(void){

    Serial.println("Initializing...");

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi...");
    }
    
    Serial.println("Connected to WiFi!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

}

static void InfluxInit(){

        sensor.addTag ("device", DEVICE);
        timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
        
        if (client.validateConnection()) {                                                                                               //CHECK SERVER CONNECTION
          Serial.print("Connected to InfluxDB: ");
          Serial.println(client.getServerUrl());
        } else {
          Serial.print("InfluxDB connection failed: ");
          Serial.println(client.getLastErrorMessage());
        }

}

static void Sensor_PZEM (void){

    float voltage1, current1, voltage2, current2;
    
    
    uint32_t now = millis();
    
//        if (now - pzemRefresh  >= PZEM_REFRESH_TIME){
//    
//            pzemRefresh = now;
    
            voltage1 = pzem1.voltage();
            current1 = pzem1.current();  
            voltage2 = pzem2.voltage();    
            current2 = pzem2.current();
    
            if (voltage1 != NAN) {
                Serial.print ( "Tegangan PLN : " );
                Serial.print ( voltage1 );
                Serial.println ( "V" );
                pzemV1 = (uint8_t)voltage1;
            } else {
                Serial.println("Error reading voltage 1");
            }
    
            if (current1 != NAN) {
                Serial.print("Arus PLN : ");
                Serial.print(current1);
                Serial.println("A");
                pzemC1 = (uint8_t)current1;
            } else {
                Serial.println("Error reading current 1");
            }
    
            if (voltage2 != NAN) {
                Serial.print ( "Tegangan UPS : " );
                Serial.print ( voltage2 );
                Serial.println ( "V" );
                pzemV2 = (uint8_t)voltage2;
            } else {
                Serial.println("Error reading voltage 2");
            }
    
            if (current2 != NAN) {
                Serial.print("Arus UPS : ");
                Serial.print(current2);
                Serial.println("A");
                pzemC2 = (uint8_t)current2;
            } else {
                Serial.println("Error reading current 2");
            }

    delay (5000);
//        }
    
      
}

static void RelayLogic (void){
    
    bool relayState = HIGH;
    unsigned long now = millis();

    if(pzemV1 > 100 && pzemV2 > 100){                                                                               // PLN ON, UPS ON
        
        Serial.println ("SUMBER LISTRIK : PLN");
        relayState = HIGH;
        digitalWrite (relay, relayState);
        Serial.println ("PDU: ON");
        Serial.print ("Relay: ");
        Serial.println (relayState);
        Serial.println ();
        relayStat = (bool)relayState;
        startTime = 0;
    
    }  

    else if (pzemV1 < 100 && pzemV2 > 100) {                                                                        // PLN OFF, UPS ON
      
      if(startTime == 0){

                          startTime = millis();
                        
                        }
      
        Serial.println("SUMBER LISTRIK : UPS");
        Serial.print ("Relay: ");
        Serial.println (relayState);
        Serial.println ();
        digitalWrite (relay, relayState);
        relayStat = (bool)relayState;


        if (relayState == HIGH){
        unsigned long currentTime = millis();
        
            if ( currentTime - startTime < interval ){
            
                    digitalWrite ( relay, relayState );
                    Serial.println ("PDU ON.");
                    Serial.print ("Relay...: ");
                    Serial.println (relayState);
                    Serial.println ();
                    relayStat = (bool)relayState;
            
            } else {
                    
                   relayState = LOW;  
                   digitalWrite (relay, relayState);
                   relayStat = (bool)relayState;
                   }
        } else {

                relayState = LOW;
                digitalWrite ( relay, relayState );
                Serial.println ("PDU: OFF...");
                Serial.print ("Relay..: ");
                Serial.println (relayState);
                Serial.println ();
                relayStat = (bool)relayState;
                startTime = 0;
            
            }
            
    }  else if (pzemV1 > 100 && pzemV2 < 100) {                                                                                                // UPS MATI PLN MATI
                                                                                                       
                relayState = LOW;                                                                                           
                Serial.println ("PLN ON & UPS OFF");
                Serial.println ("Indikasi UPS ERROR");
                digitalWrite ( relay, relayState );
                relayStat = (bool)relayState;
                Serial.print ("Relay: ");
                Serial.println (relayState);
                Serial.println ();
                startTime = 0;
        
            }

            else {
            relayState = LOW;                                                                                           
            Serial.println ("PLN OFF & UPS OFF");
            digitalWrite ( relay, relayState );
            relayStat = (bool)relayState;
            Serial.print ("Relay: ");
            Serial.println (relayState);
            Serial.println ();
            startTime = 0;
              }

}

static void InfluxTsk (void){

    uint32_t now = millis();
    if (now - influxdb_send_timestamp >= INFLUXDB_SEND_TIME ){

        influxdb_send_timestamp = now;
        sensor.clearFields();
        sensor.addField ("RSSI",WiFi.RSSI());
        sensor.addField ("Tegangan PLN", pzemV1);
        sensor.addField ("Arus PLN", pzemC1);
        sensor.addField ("Tegangan UPS", pzemV2);
        sensor.addField ("Arus UPS", pzemC2);
        sensor.addField ("Relay Status", relayStat);

        Serial.print ("Writing");
        Serial.println (client.pointToLineProtocol(sensor));
        Serial.println ();
       
        if(!client.writePoint(sensor)){

            Serial.print ("InfluxDB write failed");
            Serial.println (client.getLastErrorMessage());
        }




    }


}
