#include <ESP8266WiFi.h>
#include <EasyDDNS.h>
#include <esp8266httpclient.h>
#include <DHT.h>
#include <time.h>
#include <GoogleLog.h>
#include <Arduino.h>
#include <HTTPSRedirect.h>


#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Replace with your network details
const char* ssid     = "MyWiFi";
const char* password = "ThisIsMyPassword";
// Replace with your ddns details
const char* ddnshostname = "putyour.ddns.net";
const char* ddnslogin = "mylogin";
const char* ddnspassword = "mypassword";

// GoogleLog
// Replace with your own script id to make server side changes
//https://script.google.com/macros/s/PUTHEREYOURGSCRIPTIDASSEENONGSCRIPTDOCUMENTATION/exec?command=appendRow&value=uno,due,tre
const char *GScriptId = "PUTHEREYOURGSCRIPTIDASSEENONGSCRIPTDOCUMENTATION";
// echo | openssl s_client -connect script.google.com:443 |& openssl x509 -fingerprint -noout
const char* fingerprint = "D8 4E 4F 08 10 7A FC A7 07 EA BF 2C 0B 4D 8C B9 F4 C5 7F EF";

// Web Server on port 80
WiFiServer server(80);

// DHT Sensor
const int DHTPin = 0;
DHT dht(DHTPin, DHTTYPE);

// Temporary variables
static char celsiusTemp[7];
static char fahrenheitTemp[7];
static char humidityTemp[7];
char IP[20];
time_t now;
char mydata[150]="";
float h=0;
float t=0;

unsigned long previousMillis=1;

void ICACHE_FLASH_ATTR printFloat(float val, char *buff) {
   char smallBuff[30];
   int val1 = (int) val;
   unsigned int val2;
   if (val < 0) {
      val2 = (int) (-100.0 * val) % 100;
   } else {
      val2 = (int) (100.0 * val) % 100;
   }
   os_sprintf(smallBuff, "%i,%02u", val1, val2);
   strcpy(buff, smallBuff);
}

// only runs once on boot
void setup() {
  // Note: setup() must finish within approx. 1s, or the the watchdog timer
  // will reset the chip. Hence don't put too many requests in setup()
  // ref: https://github.com/esp8266/Arduino/issues/34

  // Initializing serial port for debugging purposes
  Serial.begin(115200);
  yield();
  //delay(10);

  dht.begin();
  
  // Connecting to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    //delay(500);
    yield();
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  
  // Starting the web server
  server.begin();
  Serial.print("Web server running. Waiting for the ESP IP... ");
  yield();
  //delay(10000);
  
  // Printing the ESP IP address
  Serial.println(WiFi.localIP());
  sprintf(IP,WiFi.localIP().toString().c_str());

  // time
  configTime(2 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("\nWaiting for time... ");
  while (!time(nullptr)) {
    Serial.print(".");
    //delay(1000);
    yield();
  }

  //Init DDNS
  EasyDDNS.service("noip");
  EasyDDNS.client(ddnshostname,ddnslogin,ddnspassword);
  EasyDDNS.update(1);

  //Init GoogleLog
  GoogleLog.service(GScriptId,"Foglio1",fingerprint);
  get_temp();
  prepare_data("Reboot");
  GoogleLog.update(1,mydata);
}

void prepare_data(char* comment){
  time(&now);//= time(nullptr);
  Serial.println(ctime(&now));
  //18/09/2018 8.43.22
  struct tm * timeinfo;
  timeinfo=localtime(&now);
  char t_s[20];
  char h_s[20];
  char myip[17];
  EasyDDNS.old_ip.toCharArray(myip,17);
  printFloat(t,t_s);
  printFloat(h,h_s);
  sprintf(mydata,"%s;%s;%s;%s;%02d/%02d/%04d %02d.%02d.%02d;%s",ddnshostname,myip,t_s,h_s,timeinfo->tm_mday,timeinfo->tm_mon+1,timeinfo->tm_year+1900,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,comment);
  Serial.println(mydata);
}

void do_work_intervalled(unsigned long update_interval){
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= update_interval) {
    previousMillis = currentMillis;

    EasyDDNS.update(1); // Check for New Ip if are elapsed 5 minutes.
    get_temp();
    prepare_data("-");
    // Send memory data to Google Sheets
    if (!(isnan(h)))
       GoogleLog.update(1,mydata);
  }
}

void get_temp()
{

  for (int i=0; i<5; i++){
    delay(2000);
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    h = dht.readHumidity();
    if (!(isnan(h))) break;
  }
  
  for (int i=0; i<5; i++){
    delay(2000);
    // Read temperature as Celsius (the default)
    t = dht.readTemperature();
    // Check if any reads failed and exit early (to try again).
    if (!(isnan(t))) break;
  }
    
  if (isnan(t)) {
    strcpy(celsiusTemp,"Failed");
  }
  else {
    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(t, h, false);
    dtostrf(hic, 6, 2, celsiusTemp);
  }
  if (isnan(h)) {
    strcpy(humidityTemp, "Failed");
  }
  else {
    dtostrf(h, 6, 2, humidityTemp);
  }

    Serial.print(IP);
    Serial.print("Temperature: ");
    Serial.print(celsiusTemp);
    Serial.print("*C\tHumidity: ");
    Serial.print(humidityTemp);
    Serial.print("\%");
    Serial.println(ctime(&now));
}

// runs over and over again
void loop() {
  // Listenning for new clients
  WiFiClient client = server.available();

  do_work_intervalled(30*60*1000); //ogni 30'
  //do_work_intervalled(30*1000); //ogni 30"

  if (client) {
    Serial.print("New client: my public IP: ");
    Serial.println(EasyDDNS.old_ip);
    // bolean to locate when the http request ends
    boolean blank_line = true;
    while (client.connected()) {
      if (client.available()) {

        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");
        client.println();
        // your actual web page that displays temperature and humidity
        client.println("<!DOCTYPE HTML>");
        client.println("<html>");
        client.print("<head><meta http-equiv=\"refresh\" content=\"5;URL='http://");
        client.print(IP);
        client.println("/'\" /></head><body><h1>ESP8266 - Temperature and Humidity</h1><h3>Temperature in Celsius: ");
        client.println(celsiusTemp);
        client.println("*C</h3><h3>Humidity: ");
        client.println(humidityTemp);
        client.println("%</h3><h3>Last sensor update: ");
        client.println(ctime(&now));
        client.println("</h3><h4>");
        client.println("</h4></body></html>");     
        break;
      }
    }
    // closing the client connection
    delay(1000);
    //yield();
    client.stop();
    Serial.println("Client disconnected.");
  }
  yield();
}
