# ESP8266-temperature-logger-on-Google-Drive-Sheets
Keep a log of hunimidy and temperature in a Sheet

Installation from 0 to run
install arduino ide (run correctly with 1.8.10)
add in file<settings>additional urls: https://arduino.esp8266.com/stable/package_esp8266com_index.json 
install from board manager: esp8266 by ESP8266 Community
install from library manager: DHT sensor library by Adafruit
dowload and manual install https://github.com/electronicsguy/ESP8266/tree/master/HTTPSRedirect
install library EasyDDNS (https://github.com/ayushsharma82/EasyDDNS)
install library GoogleLog (https://github.com/marcozaino/GoogleLog)
  modify old_ip variable of EasyDDNSClass as following in theese instructions
select esp8266 board before compile
restart and update all library if Android signal them







How to modify a library:
search the library on arduino library folder EasyDDNS.h
Modify definition of class changing old_ip from private to public as follow

class EasyDDNSClass{
public:
  void service(String ddns_service);
  void client(String ddns_domain, String ddns_username,String ddns_password = "");
  void update(unsigned long ddns_update_interval);
  String old_ip;

private:
  unsigned long interval;
  unsigned long previousMillis;

  String new_ip;
  //String old_ip;
  String update_url;
  String ddns_u;
  String ddns_p;
  String ddns_d;
  String ddns_choice;
};
