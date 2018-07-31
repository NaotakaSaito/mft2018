#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <time.h>

#define SERIAL_PORT "/dev/ttyUSB0"

typedef struct  {
	int pos;
	char buf[512];
} STRBUF;

pthread_mutex_t mutex; //2つのスレッド間で変数の保護を行う

bool shutter = false;

void* serial(void * pParam) {
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
		return NULL;
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
					pthread_mutex_lock(&mutex);
					shutter = true;
					pthread_mutex_unlock(&mutex);
				}
			}
		}

		// エコーバック
		//write(fd, buf, len);
	}

	close(fd);                              // デバイスのクローズ
	return NULL;
}

void* camera(void *pParam) {

	static int mode = 0;
	bool isShow = true;

	struct tm *localtime(const time_t *timer);

	cv::Mat ready = cv::imread("ready.png", cv::IMREAD_COLOR);
	cv::Mat ka = cv::imread("ka.png", cv::IMREAD_COLOR);
	cv::Mat me = cv::imread("me.png", cv::IMREAD_COLOR);
	cv::Mat ha = cv::imread("ha.png", cv::IMREAD_COLOR);
	cv::Mat fail = cv::imread("fail.png", cv::IMREAD_COLOR);

	// test time
	time_t timer;
	struct tm *date;
	char filename[256];

	//

	cv::VideoCapture cap(0);//デバイスのオープン
	//cap.open(0);//こっちでも良い．

	if(!cap.isOpened())//カメラデバイスが正常にオープンしたか確認．
	{
		//読み込みに失敗したときの処理
		return NULL;
	}

	while(1)//無限ループ
	{
		cv::Mat frame;
		cap >> frame; // get a new frame from camera

		//
		//取得したフレーム画像に対して，クレースケール変換や2値化などの処理を書き込む．
		//
		int key = cv::waitKey(1);
		switch(key) {
			case 0x30:
				mode = 0;
				isShow = true;
				break;
			case 0x31:
				mode = 1;
				isShow = true;
				break;
			case 0x32:
				mode = 2;
				isShow = true;
				break;
			case 0x33:
				mode = 3;
				isShow = true;
				break;
			case 0x34:
				mode = 4;
				isShow = true;
				break;
			case 0x35:
				mode = 5;
				isShow = true;
				break;
			default:
				break;
		}

		switch(mode) {
			case 0:
				if(isShow) {
					cv::imshow("window", ready);//画像を表示．
					isShow = false;
				}
				break;
			case 1:
				if(isShow) {
					cv::imshow("window", ka);//画像を表示．
					isShow = false;
				}
				break;
			case 2:
				if(isShow) {
					cv::imshow("window", me);//画像を表示．
					isShow = false;
				}
				break;
			case 3:
				if(isShow) {
					cv::imshow("window", ha);//画像を表示．
					isShow = false;
				}
				break;
			case 4:
				if(isShow) {
					timer = time(NULL);          // 日付時刻を取得
					date = localtime(&timer);    // tm構造体に変換
					strftime(filename, sizeof(filename), "%Y_%m%d_%H%M.png", date);	// 書式変換してファイル名を生成
					cv::imshow("window", frame);//画像を表示．
					cv::imwrite(filename, frame);
					isShow = false;
				}
				break;
			case 5:
				if(isShow) {
					cv::imshow("window", fail);//画像を表示．
					system("mpg321 onara_018.mp3");
					isShow = false;
				}
			default:
				break;
		}

		pthread_mutex_lock(&mutex);
		if(shutter) {
			//フレーム画像を保存する．
			cv::imwrite("img.png", frame);
			shutter = false;
		}
		pthread_mutex_unlock(&mutex);
	}
	cv::destroyAllWindows();
	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t thread1, thread2;
	int ret1,ret2;

	pthread_mutex_init(&mutex, NULL);

	pthread_create(&thread1,NULL,serial,NULL);
	pthread_create(&thread2,NULL,camera,NULL);

	pthread_join(thread1,NULL);
	pthread_join(thread2,NULL);

	pthread_mutex_destroy(&mutex); 

	printf("done\n");

	return 0;
}

