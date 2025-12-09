#include "delay.h"
#include "stm32f10x.h"

void Delay_ms(uint32_t ms)
{
    uint32_t i;
    while(ms--)
    {
        for(i = 7200; i > 0; i--);
    }
}
