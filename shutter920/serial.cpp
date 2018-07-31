#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string>       // ヘッダファイルインクルード
#include <iostream>

#define SERIAL_PORT "/dev/ttyUSB0"
typedef struct  {
	int pos;
	char buf[512];
} STRBUF;

int main(int argc, char *argv[])
{
	unsigned char msg[] = "serial port open...\n";
	char buf[255];             // バッファ
	int fd;                             // ファイルディスクリプタ
	struct termios tio;                 // シリアル通信設定
	int baudRate = B115200;
	int i;
	int len;
	int ret;
	int size;
	STRBUF tmp = {0,""};
	std::string str;

	fd = open(SERIAL_PORT, O_RDWR);     // デバイスをオープンする
	if (fd < 0) {
		printf("open error\n");
		return -1;
	}

	tio.c_cflag += CREAD;               // 受信有効
	tio.c_cflag += CLOCAL;              // ローカルライン（モデム制御なし）
	tio.c_cflag += CS8;                 // データビット:8bit
	tio.c_cflag += 0;                   // ストップビット:1bit
	tio.c_cflag += 0;                   // パリティ:None

	cfsetispeed( &tio, baudRate );
	cfsetospeed( &tio, baudRate );

	cfmakeraw(&tio);                    // RAWモード

	tcsetattr( fd, TCSANOW, &tio );     // デバイスに設定を行う

	ioctl(fd, TCSETS, &tio);            // ポートの設定を有効にする

	// 送受信処理ループ
	memset(&tmp,0,sizeof(STRBUF));
	while(1) {
		len = read(fd, buf, sizeof(buf));
		if (0 < len) {
			for(i = 0; i < len; i++) {
				//	printf("%02x", buf[i]);
				tmp.buf[tmp.pos++] = buf[i];
				//printf("%c %02x %d\n",tmp.buf[tmp.pos],tmp.buf[tmp.pos],tmp.pos);
				if(buf[i] == 0x0a) {
					tmp.buf[tmp.pos++] = '\0';
					tmp.pos = 0;
					str = tmp.buf;
					std::cout << str;
				}
			}
		}

		// エコーバック
		//write(fd, buf, len);
	}

	close(fd);                              // デバイスのクローズ
	return 0;
}
