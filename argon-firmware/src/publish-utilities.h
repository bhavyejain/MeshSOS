#pragma once

#ifndef PUBLISH_UTILITIES_H
#define PUBLISH_UTILITIES_H

char* createEventPayload(const char* emergency, const char* lat, const char* lon, const char* acc);
const char* getJsonValue(const char* key, const char* obj);
bool publishToCloud(const char* filter, const char* message);
bool publishToMesh(const char* filter, const char* message);

#endif