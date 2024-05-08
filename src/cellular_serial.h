#ifndef __CELLULAR_SERIAL_H
#define __CELLULAR_SERIAL_H

int Module_SerialPort_Init(char *dev);
void Module_SerialPort_Close(void);
void Module_SerialPort_SendStr(char *pStr);
int Module_SerialPort_Recive(char *pBuf, int len);

#endif