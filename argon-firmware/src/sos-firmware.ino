/*
 * Project: MeshSOS
 * Description: A mesh networking based SOS support system for senior citizens
 * Author: Bhavye Jain
 * Version: 3.0.2
 * Date: 5 April 2020
 */

#include "google-maps-device-locator.h"
#include "publish-utilities.h"
#include "JsonParserGeneratorRK.h"

SYSTEM_THREAD(ENABLED);         // Enable application loop and system processes to execute independently in separate threads (Now execution is never blocked by a system interrupt)
SYSTEM_MODE(SEMI_AUTOMATIC);    // Start executing setup() and loop() even if not connected to cloud

// variables for I/O
const int button_med = A1;
const int button_pol = A2;

// variables to store input values from buttons
int val_med = 0;
int val_pol = 0;

// cloud variables
int btn_medical;
int btn_police;

// strings to build the main SOS message
const String DEVICE = System.deviceID();
String MESSAGE_MEDICAL = "medical-" + DEVICE;
String MESSAGE_POLICE = "police-" + DEVICE;

// device geolocation
GoogleMapsDeviceLocator locator;
String latitude = "-1";       // setting default to -1 to ease error detection later
String longitude = "-1";
String accuracy = "-1";

// variables for WiFi toggle (testing)
const int wifi_btn = A5;
int val_wifi = 0;
bool wifi_flag = true;

Timer location_timer(1200000, onLocationTimeout, false);      // a timer for 20 minutes : update location of the device every 20 minutes
bool getlocation = false;

int sos_attempts = 0;     // number of attempts made to send the message
Timer ack_timeout(5000, onAckTimeout, true);      // single-shot timer of 5 seconds

/** 
 * The type of sos for which acknowledgement is awaited.
 * -1 : No ack is awaited
 *  0 : Medical emergency
 *  1 : Police Emergency
*/
int sos_sent = -1;  

// flags to check what kind of message to re-send
bool resend_med = false;
bool resend_pol = false;

String publish_filters[] = {"emergency/medical", "emergency/police"};
String publish_messages[] = {MESSAGE_MEDICAL, MESSAGE_POLICE};

void setup() {
  delay(3000);      // this delay allows to open serial monitor in time :-)

  Serial.println("**** SETUP STARTED ****");

  WiFi.on();        // manually turn on WiFi before connectting
  Serial.println("**** SETUP : CONNECTING TO WIFI ****");
  WiFi.connect();   // connect WiFi manually (SEMI_AUTOMATIC mode)
  delay(1000);
  
  pinMode(button_med, INPUT_PULLDOWN);  // take input from medical emergency button
  pinMode(button_pol, INPUT_PULLDOWN);  // take input from police emergency button

  pinMode(wifi_btn, INPUT_PULLDOWN);    // take input from wifi toggle button

  Particle.subscribe("ACK", hookResponseHandler, MY_DEVICES);   // subscribe to acknowledgements from server
  Mesh.subscribe("m_emergency", meshEmergencyHandler);      // subscribe to emergency messages being broadcasted in the mesh
  Mesh.subscribe("m_ack", meshAckHandler);    // subscribe to acknowledgements being broadcasted in mesh

  Serial.println("**** SETUP : CONNECTING TO MESH ****");
  Mesh.connect();       // connect after subscribes to separate threads, otherwise, threads would be tied

  Serial.println("**** SETUP : CONNECTING TO CLOUD ****");
  Particle.connect();   // connect after subscribes to separate threads, otherwise, threads would be tied

  locator.withSubscribe(locationCallBack);
  if(WiFi.ready()){
    locator.publishLocation();      // get initial location 
  }
  location_timer.start();     // keep updating location regularly

  Serial.begin(9600);   // initialize serial output
  Serial.print("DEVICE: "); Serial.println(DEVICE); // print device ID
  Serial.print("LOCATION::  "); Serial.println("lat: " + String(latitude) + " lon: " + String(longitude) + "  acc: " + String(accuracy));
}

void loop() {

  if(!WiFi.ready() && !WiFi.connecting()){
    Serial.println("**** LOOP : WIFI CONNECTING ****");
    WiFi.connect();
  }

  // read inputs from respective buttons
  val_med = digitalRead(button_med);  
  val_pol = digitalRead(button_pol);  
  
  // update values of cloud-visible variables
  btn_medical = val_med;
  btn_police = val_pol;

  if(val_med == 1 && sos_sent == -1){   // no pending acknowledgement
    Serial.println("** btn_med PRESSED **");

    sos_sent = 0;   // expect an ACK for medical emergency
    ack_timeout.start();    // start timer for receiving an acknowledgement
    sos_attempts = 1;

    sendSosMessage(0);
    delay(500);   // do not publish multiple times for a long press
  }

  if(val_pol == 1 && sos_sent == -1){
    Serial.println("** btn_pol PRESSED **");

    sos_sent = 1;   // expect an ACK for police emergency
    ack_timeout.start();    // start timer for receiving an acknowledgement
    sos_attempts = 1;

    sendSosMessage(1);
    delay(500);   // do not publish multiple times for a long press
  }

  if(sos_sent != -1){     // check if resending some message is required
    if(resend_med){
      resendSosMessage(0);
      resend_med = false;
    }
    if(resend_pol){
      resendSosMessage(1);
      resend_pol = false;
    }
  }

  // toggle wifi for testing
  val_wifi = digitalRead(wifi_btn);
  if(val_wifi == 1){
    toggleWifi();

    delay(500);
  }

  if(getlocation && WiFi.ready()){      // update location
    locator.publishLocation();
    getlocation = false;
  }
}

// send SOS distress message on basis of internet connectivity
void sendSosMessage(int index){
  if(WiFi.ready()){     // if the device is connected to the cloud, directly publish message to the cloud
    locator.publishLocation();    // update device location
    String payload = createEventPayload(publish_messages[index], latitude, longitude, accuracy);     // create JSON object with all required data to be sent
    publishToCloud(publish_filters[index], payload);
  }
  else{   // publish to mesh
    String filter = "m_";
    filter.concat(publish_filters[index]);    // convert emergency to m_emergency
    filter.concat("/0");    // flag : don't update location
    String payload = createEventPayload(publish_messages[index], latitude, longitude, accuracy);     // create JSON object with all required data to be sent
    publishToMesh(filter, payload);
  }
}

// handles resending of SOS messages and adding of update location data flag
void resendSosMessage(int index){     // avoid uneccessary request for location and handle case where we are resending with locations values as -1
  if(WiFi.ready()){   // use direct WiFi connectivity
    if(latitude.compareTo("-1") == 0 || longitude.compareTo("-1") == 0 || accuracy.compareTo("-1") == 0){    // ask for location only if existing values are -1 (no location)
      locator.publishLocation();
    }
    String payload = createEventPayload(publish_messages[index], latitude, longitude, accuracy);    // create JSON object with all required data to be sent
    ack_timeout.start();    // start timer for receiving an acknowledgement
    publishToCloud(publish_filters[index], payload);
  }
  else{   // use the mesh system
    String filter = "m_";
    filter.concat(publish_filters[index]);
    if(latitude.compareTo("-1") == 0 || longitude.compareTo("-1") == 0 || accuracy.compareTo("-1") == 0){
      /**
       * If this flag is set to 1, the device which will finally publish the message to the
       * particle cloud will swap the location data from the incoming data with its own location
       * data. This flag is set to 1 if the sending device is unable to update its location
       * and the values are still -1 (which means it cannot provide its location).
       * */
      filter.concat("/1");  // flag : update location
    }
    else{
      filter.concat("/0"); // flag : don't update location
    }
    String payload = createEventPayload(publish_messages[index], latitude, longitude, accuracy);    // create JSON object with all required data to be sent
    ack_timeout.start();    // start timer for receiving an acknowledgement
    publishToMesh(filter, payload);
  }
}

// handle emergency calls received through mesh
void meshEmergencyHandler(const char *event, const char *data){     
  Serial.print("MESH_RECEIVED:: event: ");
  Serial.print(event);
  Serial.print(", data: ");
  if(data){
    Serial.println(data);
  }
  else{
    Serial.println("NULL");
  }

  String filter = event;
  String message = data;
  String payload;

  int i = filter.indexOf('/');
  int j = filter.indexOf('/', i+1);
  String e_type = filter.substring((i+1), j).trim();    // get the emergency type
  String flag = filter.substring(j+1).trim();

  if(flag.compareTo("1") == 0){   // create a payload with updated location information if flag is 1
    String emergency = getJsonValue("emergency", message);
    payload = createEventPayload(emergency, latitude, longitude, accuracy);
  }
  else{
    payload = message;
  }

  if(e_type.equals("medical")){

    if(Particle.connected()){     // if the device is connected to the cloud, publish message to the cloud
      publishToCloud("emergency/medical", payload);
    }
  }
  if(e_type.equals("police")){

    if(Particle.connected()){     // if the device is connected to the cloud, publish message to the cloud
      publishToCloud("emergency/police", payload);
    }
  }
}

// handle responses from the server (via webhook)
void hookResponseHandler(const char *event, const char *data){     
  Serial.println("HOOK RESPONSE HANDLER : " + DEVICE);
  
  String ack = String(data);
  int i = ack.indexOf('/');
  String message = ack.substring(1, i).trim();        // extract the SOS message
  String ack_code = ack.substring(i+1, i+2).trim();   // extract ack_code (0 or 1)
  String coreid = getDeviceID(message);               // extract the device id from SOS message

  if(coreid.compareTo(DEVICE) == 0){         // if the sending device receives the ACK don't propagate
    if(sos_sent == -1){     // if no ACK is expected
      Serial.println("# ACK DUMPED #");
    }
    else if(ack_code.equals("1")){   // if SOS call successfully registered
      Serial.println("# ACK RECEIVED #");

      sos_sent = -1;
      ack_timeout.stop();
      sos_attempts = 0;
    }
    else{     // if error
      onAckTimeout();
    }
  }
  else{       // propagate in mesh
    Serial.println("** PUBLISH ACK IN MESH **");
    Mesh.publish("m_ack", ack);
  } 
}

// handle acknowlegdements published in mesh
void meshAckHandler(const char *event, const char *data){
  Serial.println("MESH ACK HANDLER : " + DEVICE);
  
  String ack = String(data);
  int i = ack.indexOf('/');
  String message = ack.substring(1, i).trim();
  String ack_code = ack.substring(i+1, i+2).trim();
  String coreid = getDeviceID(message);
  Serial.println("coreid: " + coreid);

  if(coreid.compareTo(DEVICE) == 0 && sos_sent != -1){    // if device id and core id match, accept the ACK and process
    if(ack_code.equals("1")){     // if SOS call successfully registered
      Serial.println("# ACK RECEIVED. #");

      sos_sent = -1;
      ack_timeout.stop();
      sos_attempts = 0;
    }
    else{   // if error
      onAckTimeout();
    }
  }
  else{     // if ACK is not expected or device ids do not match
    Serial.println("# ACK DUMPED #");
  }
}

// handles timeout of ack_timeout timer and error acknowledgement
void onAckTimeout(){
  Serial.println("** ack_timeout TIMED OUT **");
  Serial.print("ATTEMPTS: "); Serial.println(sos_attempts);
  
  if(sos_attempts < 3 && sos_sent != -1){
    Serial.println("** ATTEMPTING RESEND **");
    if(sos_sent == 0){    // if medical ACK was expected
      sos_sent = 0;   // expect an ACK for medical emergency
      sos_attempts++;
      resend_med = true;
    }
    else if(sos_sent == 1){     // if police ACK was expected
      sos_sent = 1;   // expect an ACK for police emergency
      sos_attempts++;
      resend_pol = true;
    }
  }
  else{   // if number of attempts reaches 3, reset the process, SOS request has failed
    sos_sent = -1;
    sos_attempts = 0;
    Serial.println("** SENDING SOS FAILED! **");
  }
}

// function to extract the device ID from received augmented SOS message
String getDeviceID(String data){    

  int i = data.indexOf('-');
  String devID = data.substring(i+1).trim();
  return devID;
}

// handle the response by Google Maps API
void locationCallBack(float lat, float lon, float acc){     
  latitude = String(lat);
  longitude = String(lon);
  accuracy = String(acc);
}

// request to update location after every 20 minutes
void onLocationTimeout(){
  getlocation = true;
}

// utility function to toggle WiFi on the chip
void toggleWifi(){
  if(wifi_flag){
    WiFi.disconnect();
    WiFi.off();
    wifi_flag = false;
  }
  else{
    WiFi.on();
    WiFi.connect();
    wifi_flag = true;
  }
}

// get value by key from JSON string
String getJsonValue(const char* key, const char* obj){
  JsonParserStatic<512, 40> jp;

  jp.clear();
  jp.addString(obj);
  
  if(!jp.parse()){
    Serial.println("Parsing JSON failed!");
    return "";
  }
  
  String value;
  if(!jp.getOuterValueByKey(key, value)){
    Serial.println("Fetching JSON value failed!");
    return "";
  }
  
  jp.nullTerminate();
  
  return value;
}