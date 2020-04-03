#include "publish-utilities.h"
#include "JsonParserGeneratorRK.h"

// create JSON data
char* createEventPayload(const char* emergency, const char* lat, const char* lon, const char* acc){      

  JsonWriterStatic<512> jw;
  {
    JsonWriterAutoObject obj(&jw);

    jw.insertKeyValue("emergency", emergency);
    jw.insertKeyValue("latitude", lat);
    jw.insertKeyValue("longitude", lon);
    jw.insertKeyValue("accuracy", acc);
  }

  return jw.getBuffer();
}

// get value by key from JSON string
char* getJsonValue(const char* key, const char* obj){
  // JsonParserStatic<512, 20> jp;
  // jp.clear();
  // jp.addString(obj);
  
  // if(!jp.parse()){
  //   Serial.println("Parsing JSON failed!");
  //   return "";
  // }
  
  // char* value;
  // if(!jp.getOuterValueByKey(key, value)){
  //   Serial.println("Fetching JSON value failed!");
  //   return "";
  // }

  // jp.nullTerminate();
  // return value;
}

// publish the emergency message to Particle cloud
bool publishToCloud(const char* filter, const char* message){       

  // run the loop if publish() returns false => failed publish
  while(!Particle.publish(filter, message)){    // keep trying to publish until successful
    delay(1000);   // wait for 1 second (allowed publish rate is per second)
  }

  // print to serial output
  Serial.print("PUBLISH_TO_CLOUD :: ");
  Serial.println(message);
  
  return true;
}

// broadcast emergency message in the mesh
bool publishToMesh(const char* filter, const char* message){      

  int attempts = 0;
  while(!Mesh.ready()){     // if device not connected to the mesh, try to connect 3 times
    Mesh.connect();
    ++attempts;
    if(attempts >= 3)
      break;
  }

  if(Mesh.ready()){
    int pub = Mesh.publish(filter, message);
    // run the loop if publish() returns non 0 value => failed publish
    while(pub != 0){    // keep trying to publish until successful
      pub = Mesh.publish(filter, message);
      delay(1000);    // wait for 1 second (allowed publish rate is per second)
    }

    Serial.print("PUBLISH_TO_MESH :: ");
    Serial.println(message);

    return true;
  }
  else{       // unable to publish (device not connected to mesh)
    Serial.print("FAILED_PUBLISH_TO_MESH :: ");
    Serial.println(message);

    return false;
  }
}
