#ifndef __SI7021_H__ 
#define __SI7021_H__

#define SI_RH_MM 0xE5 // Measure relative humidity, hold master mode
#define SI_RH_NOMM 0xF5 // Measure relative humidity, don't hold master mode

#define SI_T_MM 0xE3 // Measure temperature, hold master mode
#define SI_T_NOMM 0xF3 // Measure temperature, don't hold master mode

#define SI_TEMP 0xE0 // Get the previously measured temperature

#define SI_RESET 0xFE // Reset

#define SI_REG1_WR 0xE6
#define SI_REG1_RD 0xE7

#define SI_HEATER_WR 0x51
#define SI_HEATER_RD 0x11


#endif
