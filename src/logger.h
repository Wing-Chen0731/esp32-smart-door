#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4

#ifndef CURRENT_LOG_LEVEL
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG
#endif

#define LOG_ERROR(tag, msg) \
  do { \
    if (CURRENT_LOG_LEVEL >= LOG_LEVEL_ERROR) { \
      Serial.print("[ERROR] "); \
      Serial.print(tag); \
      Serial.print(": "); \
      Serial.println(msg); \
    } \
  } while (0)

#define LOG_WARN(tag, msg) \
  do { \
    if (CURRENT_LOG_LEVEL >= LOG_LEVEL_WARN) { \
      Serial.print("[WARN] "); \
      Serial.print(tag); \
      Serial.print(": "); \
      Serial.println(msg); \
    } \
  } while (0)

#define LOG_INFO(tag, msg) \
  do { \
    if (CURRENT_LOG_LEVEL >= LOG_LEVEL_INFO) { \
      Serial.print("[INFO] "); \
      Serial.print(tag); \
      Serial.print(": "); \
      Serial.println(msg); \
    } \
  } while (0)

#define LOG_DEBUG(tag, msg) \
  do { \
    if (CURRENT_LOG_LEVEL >= LOG_LEVEL_DEBUG) { \
      Serial.print("[DEBUG] "); \
      Serial.print(tag); \
      Serial.print(": "); \
      Serial.println(msg); \
    } \
  } while (0)

#endif
