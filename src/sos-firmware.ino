/*
 * Project: MeshSOS
 * Description: A mesh networking based SOS support system for senior citizens
 * Authors: Bhavye Jain
 * Date: 28 January 2020
 */

#include "google-maps-device-locator.h"
#include "publish-utilities.h"

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
String MESSAGE_MEDICAL = "medical-";
String MESSAGE_POLICE = "police-";

// device geolocation
GoogleMapsDeviceLocator locator;
String latitude = "-1";       // setting default to -1 to ease error detection later
String longitude = "-1";
String accuracy = "-1";

#if Wiring_WiFi
  // variables for WiFi toggle (testing)
  const int wifi_btn = A5;
  int val_wifi = 0;
  bool wifi_flag = true;
#endif

#if Wiring_Cellular
  // variables for Cellular toggle (testing)
  const int cellular_btn = A5;
  int val_cellular = 0;
  bool cellular_flag = true;
#endif

Timer location_timer(1200000, onLocationTimeout, false);      // a timer for 20 minutes : update location of the device every 20 minutes
bool getlocation = false;

int sos_attempts = 0;     // number of attempts made to send the message
Timer ack_timeout(3000, onAckTimeout, true);      // single-shot timer of 3 seconds

/** 
 * The type of sos for which acknowledgement is awaited.
 * -1 : No ack is awaited
 *  0 : Medical emergency
 *  1 : Police Emergency
*/
int sos_sent = -1;  

String publish_filters[] = {"emergency/medical", "emergency/police"};
String publish_messages[] = {MESSAGE_MEDICAL, MESSAGE_POLICE};

void setup() {
  
  pinMode(button_med, INPUT_PULLDOWN);  // take input from medical emergency button
  pinMode(button_pol, INPUT_PULLDOWN);  // take input from police emergency button

  #if Wiring_WiFi
    pinMode(wifi_btn, INPUT_PULLDOWN);    // take input from wifi toggle button
  #endif

  #if Wiring_Cellular
    pinMode(cellular_btn, INPUT_PULLDOWN);    // take input from cellular toggle button
  #endif

  Particle.variable("medical", btn_medical);  // make btn_medical visible to cloud
  Particle.variable("police", btn_police);  // make btn_police visible to cloud

  Particle.subscribe("hook-response/emergency", hookResponseHandler, ALL_DEVICES);
  Mesh.subscribe("m_emergency", meshEmergencyHandler);
  Mesh.subscribe("m_ack", meshAckHandler);

  // augment emergency messages with device ID
  for(int i = 0; i < 2; i++){
    publish_messages[i] = publish_messages[i].concat(DEVICE);
  }

  locator.withSubscribe(locationCallBack);
  locator.publishLocation();      // get initial location 
  location_timer.start();     // keep updating location regularly

  Serial.begin(9600);   // initialize serial output ($ particle serial monitor)
  Serial.print("Device: "); Serial.println(DEVICE); // print device ID
  Serial.print("Location::  "); Serial.println("lat: " + String(latitude) + " lon: " + String(longitude) + "  acc: " + String(accuracy));
}


void loop() {

  // read inputs from respective buttons
  val_med = digitalRead(button_med);  
  val_pol = digitalRead(button_pol);  
  
  // update values of cloud-visible variables
  btn_medical = val_med;
  btn_police = val_pol;

  if(val_med == 1 && sos_sent == -1){   // no pending acknowledgement
    locator.publishLocation();    // update device location
    String payload = createEventPayload(MESSAGE_MEDICAL, latitude, longitude, accuracy);    // create JSON object with all required data to be sent

    sos_sent = 0;   // expect an ACK for medical emergency
    ack_timeout.start();    // start timer for receiving an acknowledgement
    sos_attempts = 1;

    if(Particle.connected()){     // if the device is connected to the cloud, directly publish message to the cloud
      publishToCloud("emergency/medical", payload);
    }
    else{   // publish to mesh
      publishToMesh("m_emergency/medical", payload);
    }

    delay(500);   // do not publish multiple times for a long press
  }

  if(val_pol == 1 && sos_sent == -1){

    sos_sent = 1;   // expect an ACK for police emergency
    ack_timeout.start();    // start timer for receiving an acknowledgement
    sos_attempts = 1;

    sendSosMessage(1);
    delay(500);   // do not publish multiple times for a long press
  }

  #if Wiring_WiFi
    // toggle wifi for testing
    val_wifi = digitalRead(wifi_btn);
    if(val_wifi == 1){
      toggleWifi();

      delay(500);
    }
  #endif

  #if Wiring_Cellular
    // toggle cellular for testing
    val_cellular = digitalRead(cellular_btn);
    if(val_cellular == 1){
      toggleCellular();

      delay(500);
    }
  #endif

  if(getlocation){      // update location
    locator.publishLocation();
    getlocation = false;
  }
}

void sendSosMessage(int index){
  locator.publishLocation();    // update device location
  String payload = createEventPayload(publish_messages[index], latitude, longitude, accuracy);     // create JSON object with all required data to be sent

  if(Particle.connected()){     // if the device is connected to the cloud, directly publish message to the cloud
    publishToCloud(publish_filters[index], payload);
  }
  else{   // publish to mesh
    String filter = "m_";
    filter.concat(publish_filters[index]);
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

  int i = filter.indexOf('/');
  String e_type = filter.substring((i+1));    // get the emergency type

  if(e_type.equals("medical")){

    if(Particle.connected()){     // if the device is connected to the cloud, publish message to the cloud
      publishToCloud("emergency/medical", message);
    }
  }
  if(e_type.equals("police")){

    if(Particle.connected()){     // if the device is connected to the cloud, publish message to the cloud
      publishToCloud("emergency/police", message);
    }
  }
}

// handle responses from the server (via webhook)
void hookResponseHandler(const char *event, const char *data){     
  Serial.println("hookResponseHandler : " + DEVICE);
  
  String ack = String(data);
  int i = ack.indexOf('/');
  String message = ack.substring(0, i-1);
  String ack_code = ack.substring(i+1);
  String coreid = getDeviceID(message);

  if(coreid == DEVICE){         // if the sending device receives the ACK don't propagate
    if(ack_code.equals("1")){   // if SOS call successfully registered
      sos_sent = -1;
      ack_timeout.stop();
      sos_attempts = 0;
    }
    else{     // if error
      onAckTimeout();
    }
  }
  else{       // propagate in mesh
    Mesh.publish("m_ack", ack);
  } 
}

// handle acknowlegdements published in mesh
void meshAckHandler(const char *event, const char *data){
  Serial.println("meshAckHandler : " + DEVICE);
  
  String ack = String(data);
  int i = ack.indexOf('/');
  String message = ack.substring(0, i-1);
  String ack_code = ack.substring(i+1);
  String coreid = getDeviceID(message);

  if(coreid == DEVICE){
    if(ack_code.equals("1")){
      sos_sent = -1;
      ack_timeout.stop();
      sos_attempts = 0;
    }
    else{
      onAckTimeout();
    }
  }
}

// function to extract the device ID from received augmented message
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

#if Wiring_WiFi
void toggleWifi(){
  if(wifi_flag){
    WiFi.disconnect();
    wifi_flag = false;
  }
  else{
    WiFi.connect();
    wifi_flag = true;
  }
}
#endif

#if Wiring_Cellular
void toggleCellular(){
  if(cellular_flag){
    Cellular.disconnect();
    cellular_flag = false;
  }
  else{
    Cellular.connect();
    cellular_flag = true;
  }
}
#endif

void onAckTimeout(){
  if(sos_attempts < 3 && sos_sent != -1){
    if(sos_sent == 0){    // if medical ACK was expected
      locator.publishLocation();    // update device location
      String payload = createEventPayload(MESSAGE_MEDICAL, latitude, longitude, accuracy);    // create JSON object with all required data to be sent

      sos_sent = 0;   // expect an ACK for medical emergency
      ack_timeout.start();    // start timer for receiving an acknowledgement
      sos_attempts++;

      if(Particle.connected()){     // if the device is connected to the cloud, directly publish message to the cloud
        publishToCloud("emergency/medical", payload);
      }
      else{   // publish to mesh
        publishToMesh("m_emergency/medical", payload);
      }
    }
    else if(sos_sent == 1){     // if police ACK was expected
      locator.publishLocation();    // update device location
      String payload = createEventPayload(MESSAGE_POLICE, latitude, longitude, accuracy);     // create JSON object with all required data to be sent

      sos_sent = 1;   // expect an ACK for police emergency
      ack_timeout.start();    // start timer for receiving an acknowledgement
      sos_attempts++;

      if(Particle.connected()){     // if the device is connected to the cloud, directly publish message to the cloud
        publishToCloud("emergency/police", payload);
      }
      else{   // publish to mesh
        publishToMesh("m_emergency/police", payload);
      }
    }
  }
  else{   // if number of attempts reaches 3, reset the process, SOS request has failed
    sos_sent = -1;
    sos_attempts = 0;
    Serial.println("Sending SOS request failed!");
  }
}
