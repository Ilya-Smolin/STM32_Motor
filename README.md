# STM32_Motor
Controlling motor via STM32 microprocessor, communicating with PC with UART
---

## Forward  
```text
GPIO_SetBits (GPIOA, GPIO_Pin_5);
GPIO_ResetBits (GPIOA, GPIO_Pin_6);
```
## Reverse
```text
GPIO_ResetBits (GPIOA, GPIO_Pin_5 | GPIO_Pin_6);
```
## Stop
```text
GPIO_ResetBits (GPIOA, GPIO_Pin_5 | GPIO_Pin_6);
```
## Message sent with UART
```text
send_UART (0x65);			
send_UART (state);
send_UART (dac);
send_UART (0x66);
```
## Switching states or speed
```text
0xFE - switch state
  0 - stop
  1 -forward
  2 - reverse
  
0xFD - switch speed
  0..252, where 252 is 100% of DAC
  
0x68 - reset
```
## Buffer
Using ring buffer to receive messages
