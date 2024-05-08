#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <assert.h>
#include <easylogger/elog.h>

#include "cellular_serial.h"

#define FALSE 1
#define TRUE 0

#define DEBUG_ENABLE	0	

int g_SerialPortFd = 0;

int speed_arr[] = {
	B921600,
	B460800,
	B230400,
	B115200,
	B57600,
	B38400,
	B19200,
	B9600,
	B4800,
	B2400,
	B1200,
	B300,
};

int name_arr[] = {
	921600,
	460800,
	230400,
	115200,
	57600,
	38400,
	19200,
	9600,
	4800,
	2400,
	1200,
	300,
};

void set_speed(int fd, int speed)
{
	unsigned int i;
	int status;
	struct termios Opt;
	tcgetattr(fd, &Opt);

	for (i = 0; i < sizeof(speed_arr) / sizeof(int); i++)
	{
		if (speed == name_arr[i])
		{
			tcflush(fd, TCIOFLUSH);
			cfsetispeed(&Opt, speed_arr[i]);
			cfsetospeed(&Opt, speed_arr[i]);
			status = tcsetattr(fd, TCSANOW, &Opt);
			if (status != 0)
				perror("tcsetattr fd1");
			return;
		}
		tcflush(fd, TCIOFLUSH);
	}

	if (i == 12)
	{
		printf("\tSorry, please set the correct baud rate!\n\n");
		//print_usage(stderr, 1);
	}
}
/*
	*@brief   设置串口数据位，停止位和效验位
	*@param  fd     类型  int  打开的串口文件句柄*
	*@param  databits 类型  int 数据位   取值 为 7 或者8*
	*@param  stopbits 类型  int 停止位   取值为 1 或者2*
	*@param  parity  类型  int  效验类型 取值为N,E,O,,S
*/
int set_Parity(int fd, int databits, int stopbits, int parity)
{
	struct termios options;
	if (tcgetattr(fd, &options) != 0)
	{
		perror("SetupSerial 1");
		return (FALSE);
	}
	options.c_cflag &= ~CSIZE;
	options.c_iflag = 0;
	options.c_oflag = 0;
	switch (databits) /*设置数据位数*/
	{
	case 7:
		options.c_cflag |= CS7;
		break;
	case 8:
		options.c_cflag |= CS8;
		break;
	default:
		fprintf(stderr, "Unsupported data size\n");
		return (FALSE);
	}

	switch (parity)
	{
	case 'n':
	case 'N':
		options.c_cflag &= ~PARENB; /* Clear parity enable */
		options.c_iflag &= ~INPCK;	/* Enable parity checking */
		break;
	case 'o':
	case 'O':
		options.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/
		options.c_iflag |= INPCK;			  /* Disnable parity checking */
		break;
	case 'e':
	case 'E':
		options.c_cflag |= PARENB;	/* Enable parity */
		options.c_cflag &= ~PARODD; /* 转换为偶效验*/
		options.c_iflag |= INPCK;	/* Disnable parity checking */
		break;
	case 'S':
	case 's': /*as no parity*/
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		break;
	default:
		fprintf(stderr, "Unsupported parity\n");
		return (FALSE);
	}
	/* 设置停止位*/
	switch (stopbits)
	{
	case 1:
		options.c_cflag &= ~CSTOPB;
		break;
	case 2:
		options.c_cflag |= CSTOPB;
		break;
	default:
		fprintf(stderr, "Unsupported stop bits\n");
		return (FALSE);
	}
	/* Set input parity option */
	// if (parity != 'n' || parity != 'N')
	// options.c_iflag |= INPCK;
	options.c_cc[VTIME] = 10; // 15 seconds
	options.c_cc[VMIN] = 0;

	// options.c_lflag &= ~(ECHO | ICANON);

	//options.c_cflag   |= IXON|IXOFF|IXANY;   //  软件数据流控制

	options.c_cflag |= CLOCAL | CREAD;

	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	options.c_oflag &= ~OPOST;

	options.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

	tcflush(fd, TCIFLUSH); /* Update the options and do it NOW */
	if (tcsetattr(fd, TCSANOW, &options) != 0)
	{
		perror("SetupSerial 3");
		return (FALSE);
	}
	return (TRUE);
}

/**
	*@breif 打开串口
*/
int OpenDev(char *Dev)
{
	int fd = open(Dev, O_RDWR); //| O_NOCTTY | O_NDELAY
	if (-1 == fd)
	{ /*设置数据位数*/
		perror("Can't Open Serial Port");
		return -1;
	}
	else
		return fd;
}

int Module_SerialPort_Init(char *dev)
{
	g_SerialPortFd = OpenDev(dev);

	if (g_SerialPortFd > 0)
	{
		set_speed(g_SerialPortFd, 115200);
	}
	else
	{
		close(g_SerialPortFd);
		return 1;
	}

	if (set_Parity(g_SerialPortFd, 8, 1, 'N') == FALSE)
	{
		close(g_SerialPortFd);
		return 2;
	}

	return 0;
}

void Module_SerialPort_Close(void)
{
	if (g_SerialPortFd == 0)
		return;
	close(g_SerialPortFd);
}

void Module_SerialPort_SendStr(char *pStr)
{
	if (g_SerialPortFd == 0)
		return;
	if (pStr == NULL)
		return;
	write(g_SerialPortFd, pStr, strlen(pStr));

#if DEBUG_ENABLE
	printf("%s",pStr);
#endif
}


int Module_SerialPort_Recive(char *pBuf, int len)
{
	int nread = 0;	/* Read the counts of data */
	char buff[2000]; /* Recvice data buffer */

	if (g_SerialPortFd == 0)
		return -1;

	nread = read(g_SerialPortFd, buff, sizeof(buff));
	if (nread > 0)
	{
		buff[nread] = '\0';
		memset(pBuf,0,strlen(pBuf));
		memcpy(pBuf, buff, nread);
#if DEBUG_ENABLE		
		//printf("RECV[%d]: %s\n", nread, buff);
		log_i("%s\r\n",buff);
#endif

	}
	return nread;
}
