#include "delay.h"
/**
 ******************************************************************************
 * @file delay.c
 * @brief This file is used to add a delay to the program. 
 * @author  Nicolas Gutierrez
 * @version V1
 * @date   9-December-2023
 ******************************************************************************
 */

/// @brief Generates a delay in ms.
/// @param time_ms  time to delay in ms.
void delay_ms(uint32_t time_ms)
{
	uint32_t i;
	for (i = 0; i < ((F_CPU / 18 / 1000UL) * time_ms); i++)
	{
		__asm__("nop");
	}
}
