#include <U8g2lib.h> // Bibliothèque pour l'écran
#include <Wire.h> // Bibliothèque pour le support de l'I2C
#include <ESP8266WiFi.h> // Bibliothèque pour le WiFi
#include <ESP8266HTTPClient.h> // Bibliothèque pour envoyer des requêtes HTTP 
#include <ArduinoJson.h> // Bibliothèque pour le JSON

const char* boxSsid = "SSID"; // SSID du routeur
const char* boxPassword = "PASSWORD"; // Mot de passe WiFi du routeur

const String requestHost = "x.x.x.x"; // IP du serveur Minecraft
const int requestPort = 25565; // Port du serveur Minecraft
const String requestApi = "/api/2/call"; // API de JSONAPI
String requestData;

U8G2_SSD1306_128X64_VCOMH0_F_HW_I2C u8g2(/* rotation de l'écran */ U8G2_R0, /* reset */ U8X8_PIN_NONE); 
/*
 * https://github.com/olikraus/u8g2/wiki/u8g2setupcpp
 * Initialisation de l'écran
 */


void setup() {

  WiFi.begin(boxSsid, boxPassword); // Connexion au routeur

  requestData = makeJsonPlayersOnlineCount();
   
  u8g2.begin(); // Démarrage de l'écran
  u8g2.enableUTF8Print();
}

void loop() { 
  
  while(WiFi.status() != WL_CONNECTED) { // Tant que l'ESP8266 n'est pas connecté au routeur
    delay(500);
    oledPrint("Connexion en", "cours...", ""); 
  }
  
  String returnedJson = sendPostRequest(requestHost, requestPort, requestApi, requestData);

  if(returnedJson != "E") {

    DynamicJsonBuffer jsonBuffer;

    JsonArray& jsonArray = jsonBuffer.parseArray(returnedJson); // On parse la réponse du serveur contenue dans un tableau
    JsonObject& json = jsonArray[0]; // On parse ce que contient ce tableau

    if(!json.success()) { // Si il y a une erreur dans le parsage
      oledPrint("Erreur 2...", "", "");
      return;
    }

    /*
     * http://mcjsonapi.com/#json-response-structure
     */
    
    bool json_is_success = json["is_success"]; // On parse un booléen pour voir si la requête a bien été comprise par JSONAPI
    
    if(json_is_success) {
      int json_players = json["success"]; // On parse le nombre de joueurs connectés sur le serveur
      
      oledPrint("  Il y a " + String(json_players), "joueur(s) sur", " le serveur");
    }
    else { // Si is_success == false
      oledPrint("Erreur 3...", "", "");
    }
        
  } 
  else { // Si sendPostRequest() nous renvoie une erreur
    oledPrint("Erreur 1...", "", "");
  }
  

  delay(5000); // On attend 5s avant de renvoyer une requête
  
}


String makeJsonPlayersOnlineCount() {

  /*
   * http://mcjsonapi.com/#json-request-structure
   */


  DynamicJsonBuffer jsonBuffer; // Buffer dynamique

  JsonObject& json = jsonBuffer.createObject();
  json["name"] = "players.online.count"; // Nom de la méthode JSONAPI
  json["key"] = "3cc970df802657ce972f701edd28a0024835075505ec1efdb7af257d1058d02c"; // utilisateur + méthode JSONAPI utilisée + mdp -> en sha256 (http://mcjsonapi.com/#jsonapi-key-format)
  json["username"] = "admin"; // Nom d'utilisateur

  JsonArray& arguments = json.createNestedArray("arguments"); // Arguments (un tableau vide dans notre cas car la méthode players.online.count ne prend pas d'argument)

  String encodedJson;
  json.printTo(encodedJson);

  return encodedJson; // On retourne la chaîne encodée 
}


String sendPostRequest(String host, int port, String api, String data) {
  
  HTTPClient httpClient;

  httpClient.setReuse(true); // On essaie de rester connecté au serveur 
  httpClient.begin("http://" + host + ":" + port + api); 
  httpClient.addHeader("Host", host + ":" + port);
  httpClient.addHeader("Accept", "*/*");
  httpClient.addHeader("Content-Type", "application/json");
  httpClient.addHeader("Content-Length", String(data.length()));

  int returnCode = httpClient.POST(data); // On envoie la requête POST à JSONAPI et on récupère le code HTTP

  if(returnCode == 200) { // HTTP OK
    String returnedJson = httpClient.getString(); // On récupère ce que nous retourne le serveur
    httpClient.end();
    return returnedJson; 
  }
  else { // Si on obtient un autre code que 200 (HTTP OK)
    httpClient.end();
    return "E"; // On retourne "E" pour signaler une erreur
  }
}


void oledPrint(String line1, String line2, String line3){
  
  u8g2.clearBuffer(); // On efface le buffer
  u8g2.setFont(u8g2_font_10x20_tf); // On définit la police d'écriture 

  u8g2.setCursor(0, 15); // x = 0, y = 15
  u8g2.print(line1);

  u8g2.setCursor(0, 35); // y = 35
  u8g2.print(line2);

  u8g2.setCursor(0, 55); // y = 55
  u8g2.print(line3);
  
  u8g2.sendBuffer(); // On envoie tout ça à l'écran
}
