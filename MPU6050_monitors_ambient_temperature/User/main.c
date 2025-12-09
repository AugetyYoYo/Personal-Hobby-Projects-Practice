#include "stm32f10x.h" 
#include "stdio.h"
#include "string.h"

#define I2C_SCL_PIN     GPIO_Pin_6
#define I2C_SDA_PIN     GPIO_Pin_7
#define I2C_GPIO_PORT GPIOB

#define MPU_ADDR        0xD0


void Delay_ms(uint32_t ms) {
    uint32_t i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 7200; j++); 
}


void I2C_Delay(void) {
    volatile int i = 500; 
    while (i--);
}


void LED_Init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_SetBits(GPIOC, GPIO_Pin_13); 
}



void USART1_Init(void) {
   
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    
    GPIO_InitTypeDef GPIO_InitStructure;
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
   
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);

   
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &USART_InitStructure);

    
    USART_Cmd(USART1, ENABLE);
}


int fputc(int ch, FILE *f) {
    USART_SendData(USART1, (uint8_t)ch);

    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
    return ch;
}



void I2C_W_SCL(uint8_t BitValue) {
    GPIO_WriteBit(I2C_GPIO_PORT, I2C_SCL_PIN, (BitAction)BitValue);
    I2C_Delay();
}
void I2C_W_SDA(uint8_t BitValue) {
    GPIO_WriteBit(I2C_GPIO_PORT, I2C_SDA_PIN, (BitAction)BitValue);
    I2C_Delay();
}
uint8_t I2C_R_SDA(void) {
    return GPIO_ReadInputDataBit(I2C_GPIO_PORT, I2C_SDA_PIN);
}

void MyI2C_Init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = I2C_SCL_PIN | I2C_SDA_PIN;
    GPIO_Init(I2C_GPIO_PORT, &GPIO_InitStructure);
    I2C_W_SCL(1);
    I2C_W_SDA(1);
}

void I2C_Start(void) {
    I2C_W_SDA(1);
    I2C_W_SCL(1);
    I2C_W_SDA(0); 
    I2C_W_SCL(0);
}

void I2C_Stop(void) {
    I2C_W_SDA(0);
    I2C_W_SCL(1);
    I2C_W_SDA(1); 
}

void I2C_SendByte(uint8_t Byte) {
    uint8_t i;
    for (i = 0; i < 8; i++) {
        I2C_W_SDA(Byte & (0x80 >> i));
        I2C_W_SCL(1); 
        I2C_W_SCL(0); 
    }
    I2C_W_SCL(1);
    I2C_W_SCL(0);
}

uint8_t I2C_ReadByte(void) {
    uint8_t i, Byte = 0;
    I2C_W_SDA(1); 
    
    for (i = 0; i < 8; i++) {
        I2C_W_SCL(1); 
        if (I2C_R_SDA()) {
            Byte |= (0x80 >> i);
        }
        I2C_W_SCL(0); 
    }
    

    I2C_W_SDA(1); 
    I2C_W_SCL(1);
    I2C_W_SCL(0);
    return Byte;
}



uint8_t MPU_ReadReg(uint8_t RegAddress) {
    uint8_t Data;
    I2C_Start();
    I2C_SendByte(MPU_ADDR); 
    I2C_SendByte(RegAddress);
    
    I2C_Start();
    I2C_SendByte(MPU_ADDR | 0x01); 
    Data = I2C_ReadByte();
    I2C_Stop(); 
    return Data;
}

void MPU_WriteReg(uint8_t RegAddress, uint8_t Data) {
    I2C_Start();
    I2C_SendByte(MPU_ADDR);
    I2C_SendByte(RegAddress);
    I2C_SendByte(Data);
    I2C_Stop();
}

void MPU_Init(void) {
    MPU_WriteReg(0x6B, 0x00); // PWR_MGMT_1: 解除睡眠模式
    MPU_WriteReg(0x19, 0x00); // SMPLRT_DIV
    

    uint8_t id = MPU_ReadReg(0x75);
    if (id == 0x68) {
        printf("MPU6050 init OK. ID: 0x%X\r\n", id);
    } else {
        printf("MPU6050 init FAIL. ID: 0x%X\r\n", id);
    }
}

float MPU_GetTemperature(void) {
    uint8_t temp_h, temp_l;
    int16_t raw_temp;
    
    temp_h = MPU_ReadReg(0x41); 
    temp_l = MPU_ReadReg(0x42); 
    
    raw_temp = (temp_h << 8) | temp_l;
    
    return (float)raw_temp / 340.0f + 36.53f;
}


int main(void) {
    float temperature;
    char displayStr[64];

    LED_Init();     
    USART1_Init(); 
    MyI2C_Init();
    
    printf("--- STM32 Temp Monitor Started ---\r\n");
    MPU_Init(); 

    while (1) {
        temperature = MPU_GetTemperature();

        snprintf(displayStr, sizeof(displayStr), 
                 "Temperature: %.2f C\r\n", temperature);

        printf(displayStr);
        
        GPIO_ResetBits(GPIOC, GPIO_Pin_13); 
        Delay_ms(500);
        GPIO_SetBits(GPIOC, GPIO_Pin_13); 
        Delay_ms(500); 
    }
}
