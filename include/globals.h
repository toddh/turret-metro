#ifndef _GLOBALSH_
#define _GLOBALSH_

#include <RTTStream.h>

#ifdef USE_RTT_STREAM
extern RTTStream _rttstream;
#define console _rttstream
#else
#define console Serial
#endif

// Logging control macros
// Set to 1 to enable, 0 to disable
#define LOG_HWSTEP 0
#define LOG_I2C 0
#define LOG_IDLE_STATE 0
#define LOG_HOLD 1
#define LOG_HOMING 1

// Conditional logging macros
#if LOG_HWSTEP
  #define LOG_HWSTEP_PRINT(...) console.print(__VA_ARGS__)
  #define LOG_HWSTEP_PRINTLN(...) console.println(__VA_ARGS__)
  #define LOG_HWSTEP_PRINTF(...) console.printf(__VA_ARGS__)
#else
  #define LOG_HWSTEP_PRINT(...)
  #define LOG_HWSTEP_PRINTLN(...)
  #define LOG_HWSTEP_PRINTF(...)
#endif

#if LOG_I2C
  #define LOG_I2C_PRINT(...) console.print(__VA_ARGS__)
  #define LOG_I2C_PRINTLN(...) console.println(__VA_ARGS__)
  #define LOG_I2C_PRINTF(...) console.printf(__VA_ARGS__)
#else
  #define LOG_I2C_PRINT(...)
  #define LOG_I2C_PRINTLN(...)
  #define LOG_I2C_PRINTF(...)
#endif

#if LOG_HOLD
  #define LOG_HOLD_PRINTF(...) console.printf(__VA_ARGS__)
#else
  #define LOG_HOLD_PRINTF(...)
#endif

#if LOG_HOMING
  #define LOG_HOMING_PRINTF(...) console.printf(__VA_ARGS__)
#else
  #define LOG_HOMING_PRINTF(...)
#endif

#if LOG_IDLE_STATE
  #define LOG_IDLE_PRINT(...) console.print(__VA_ARGS__)
  #define LOG_IDLE_PRINTLN(...) console.println(__VA_ARGS__)
  #define LOG_IDLE_PRINTF(...) console.printf(__VA_ARGS__)
#else
  #define LOG_IDLE_PRINT(...)
  #define LOG_IDLE_PRINTLN(...)
  #define LOG_IDLE_PRINTF(...)
#endif


#endif

