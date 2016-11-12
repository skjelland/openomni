#include "stdafx.h"
#include <stdint.h>
#include <inttypes.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

uint16_t calculate_CRC16(char* pData, uint16_t wLength, uint16_t poly, uint16_t init, uint16_t final)
{

	_TUCHAR i;
	uint16_t wData;
	uint16_t wCrc = init;

	if (wLength == 0)
		return (~wCrc);

	while (wLength--) {
		wCrc ^= *(unsigned char *)pData++ << 8;
		for (i = 0; i < 8; i++)
			wCrc = wCrc & 0x8000 ? (wCrc << 1) ^ poly : wCrc << 1;
	}
	return wCrc & final;
}

static uint8_t
nibbleFromChar(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return 255;
}

uint8_t
*hexStringToBytes(char *inhex)
{
	uint8_t *retval;
	uint8_t *p;
	int len, i;

	len = strlen(inhex) / 2;
	retval = (uint8_t*)malloc(len + 1);
	for (i = 0, p = (uint8_t *)inhex; i<len; i++) {
		retval[i] = (nibbleFromChar(*p) << 4) | nibbleFromChar(*(p + 1));
		p += 2;
	}
	retval[len] = 0;
	return retval;
}


char * hex2data(char *hexstring)
{	
	char *pos = hexstring;
	size_t count = 0;
	int len = strlen(hexstring);
	char *data = (char*) malloc(len);

	if ((hexstring[0] == '\0') || (strlen(hexstring) % 2)) {
		//hexstring contains no data
		//or hexstring has an odd length
		return data;
	}

	for (count = 0; count < len; count++) {
		char buf[5] = { '0', 'x', pos[0], pos[1], 0 };
		long tmp = strtol(buf, NULL, 16);
		data[count] = tmp;
		pos += 2;
	}

	return data;
}


int test(char *body, int numbytes, char *crc, uint16_t poly, uint16_t init, uint16_t final) {
	uint16_t chk = calculate_CRC16(body, numbytes, poly, init, final);
	uint16_t b = strtol(crc, NULL, 16);
	//if(chk==b)
	//	printf("crc=%s chk=%x b=%x poly=%x\n", crc, chk, b, poly);
	return chk == b;
}

void doloop(char *body, int numbytes, char *crc, uint16_t init, uint16_t final) {
	for (int po = 0; po < 0xffff; po++) {
		int r1 = test(body, numbytes, crc, po, init, final);
		if (r1 == 1) {
			printf("MATCH crc=%s po=0x%04x init=0x%04x final=0x%04x\n", crc, po, init, final);
		}
	//	if (po % 100000 == 0)
	//		printf("TRACE po=%x init=%x final=%x\n", po, init, final);
	}
	//printf("TRACE doloop done crc=%s init=%x final=%x\n", crc, init, final);
}

int main()
{

	// just to verify algorithm

	uint16_t poly = 0x1021;
	char TestStr[] = { "123456789" };

	printf("msg=%s29b1 m=%s c=0x29b1\n", TestStr, TestStr);

	unsigned short CrcTest = calculate_CRC16(TestStr, sizeof(TestStr) - 1, poly, 0xffff, 0xffff);
	int r0 = test(TestStr, 9, "29b1", 0x1021, 0xffff, 0xffff);
	doloop(TestStr, 9, "29b1", 0xffff, 0xffff);

	// pick a random pod message

	//char *msg = "d80b782901007d01384000020002160e40000015051be5";
	//char *msg = "12345678929b1";
	//char *msg = "001d7000000053ff8060";
	char *msg = "a13e0b5d6280f6";

	int l = strlen(msg);
	char m[100];
	char c[6];

	memcpy(m, msg, l - 4);
	memcpy(c, &msg[l - 4], 4);

	m[l - 4] = '\0';
	c[4] = '\0';

	printf("msg=%s m=%s c=%s\n", msg, m, c);

	char *mbytes = hex2data(m);
	uint8_t *mbytes2 = hexStringToBytes(m);

	int init = 0x0000;
	int final = 0x0000;

	int numbytes = (l - 4) / 2;

	// test some common versions

	doloop(mbytes, numbytes, c, 0xffff, 0xffff);
	doloop(mbytes, numbytes, c, 0x0000, 0xffff);
	doloop(mbytes, numbytes, c, 0xffff, 0xffff);
	doloop(mbytes, numbytes, c, 0x0000, 0x0000);

	// brute!

	long t = 0;
	time_t start, now;

	time(&start);

	for (int init = 0; init < 0xffff; init++) {
		for (int final = 0; final < 0xffff; final++) {

			doloop(mbytes, numbytes, c, init, final);
			t += 0xffff;
			if (final % 100 == 0) {
				time(&now);
				long el = now - start;
				double persec = (double)t / (double)el;
				printf("TRACE init=0x%04x final=0x%04x %f\n", init, final, persec);
			}
		}
	}
}