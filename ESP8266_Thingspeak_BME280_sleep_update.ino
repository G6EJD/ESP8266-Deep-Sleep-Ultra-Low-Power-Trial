#include <Wire.h>
#include <ESP8266WiFi.h>
#include <Adafruit_BME280.h>

extern "C" { 
 #include "c_types.h" 
 #include "ets_sys.h" 
 #include "os_type.h" 
 #include "osapi.h" 
 #include "mem.h" 
 #include "user_interface.h" 
}

// put your setup code here, to run once:
// ***********************************************************************************
// **** NOTE: The ESP8266 only supports deep sleep mode by connecting GPIO 16 to Reset
// Wiring:
// ESP8266 4 = sda -> BME280 sda
// ESP8266 5 = scl -> BME280 scl
// ESP8266 5v      -> BME280 Vin
// ESP8266 Gnd     -> BME280 Gnd
// ESP8266 16      -> ESP8266 RST (or D0 on an ESP8266 D1 Mini, remember it's GPIO16 that needs to be linked to RST
// ***********************************************************************************
const char *ssid     = "yourSSID";     // Wi-Fi credentials
const char *password = "yourPASSWORD";

// ThingSpeak Settings
char thingSpeakAddress[] = "api.thingspeak.com"; 
String       writeAPIKey = "your_TS_API_write_key_here";   // Get the key for your channel to approve writing
const int UpdateThingSpeakInterval = 10 * 60 * 1000000;      // e.g. 15 * 60 * 1000000; for a 15-Min update interval (15-mins x 60-secs * 1000000uS)
#define pressure_offset 3.3 // Adjusts BME280 pressure value for my location at 40M asl
#define sda 4
#define scl 5

WiFiServer server(80);
Adafruit_BME280 bme;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print(".");}
  setOutputPower(0); // 0dBm, but has no discernable effect on power consumption, it should do!
  server.begin();
  Wire.begin(sda,scl);
  if (!bme.begin()) 
  { 
    Serial.println("Could not find a sensor, check wiring!");
  }
  else 
  {
    Serial.println("Found a sensor continuing");
    while (isnan(bme.readPressure())) { Serial.println(bme.readPressure()); } // The serial print of sensor value is not needed, but helps in debugging
  }

  float temperature = 20.1;   //bme.readTemperature();
  float humidity    = 65;     //bme.readHumidity();
  float pressure    = 1010.5; //bme.readPressure()/100.0F + pressure_offset;
  float voltage = analogRead(A0)*3.3/1024/(820/(820+270)); // scaled 0-1023 for 0 to 1 volt input
  UpdateThingSpeak("field1="+String(temperature)+"&field2="+String(humidity)+"&field3="+String(pressure)+"&field4="+String(voltage)); // Send the data as text
  Serial.println("Going to sleep");
  ESP.deepSleep(UpdateThingSpeakInterval, WAKE_RF_DEFAULT); // Sleep for the time set by 'UpdateThingSpeakInterval'
}

void loop(){
  // Do nothing as it will never get here!
}

void UpdateThingSpeak(String DataForUpload) // takes on average 2.56Secs to complete an update, from power-up to entering sleep
{
  //Serial.println(DataForUpload); // Diagnostic print so you can see what data is being sent to the server
  WiFiClient client = server.available();
  if (client.connect(thingSpeakAddress, 80))
  {         
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: "+writeAPIKey+"\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(DataForUpload.length());
    client.print("\n\n");
    client.print(DataForUpload);
    Serial.println("Upload of data complete");
    if (client.connected()) { Serial.println("Connecting to ThingSpeak..."); }
    else                    { Serial.println("Connection to ThingSpeak failed"); }
  }
  else
  {
    Serial.println("Connection to ThingSpeak Failed");   
  }
  client.flush(); // Flush the buffers
  client.stop();  // close the connection
}

void setOutputPower(float dBm) {  
  if(dBm > 20.5) {  
     dBm = 20.5;  
  } else if(dBm < 0) {  
     dBm = 0;  
  }  
  uint8_t val = (dBm*4.0f);  
  system_phy_set_max_tpw(val);  
}  


