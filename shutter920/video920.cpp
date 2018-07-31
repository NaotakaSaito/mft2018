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

// shared memory
char str[256];					// シリアルからの文字列データ
bool isStr = false;				// シリアルからの文字列有無
bool isStop = false;			// 制御の継続可否

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
	bool isThreadEnd = false;
	STRBUF tmp = {0,""};

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
	memset(str,0,sizeof(str));
	memset(&tmp,0,sizeof(STRBUF));
	while(1) {
		len = read(fd, buf, sizeof(buf));
		isThreadEnd = isStop;
		if (0 < len) {
			for(i = 0; i < len; i++) {
				//	printf("%02x", buf[i]);
				tmp.buf[tmp.pos++] = buf[i];
				//printf("%c %02x %d\n",tmp.buf[tmp.pos],tmp.buf[tmp.pos],tmp.pos);
				if(buf[i] == 0x0a) {
					tmp.buf[tmp.pos++] = '\0';
					tmp.pos = 0;
					pthread_mutex_lock(&mutex);
					strncat(str,tmp.buf,sizeof(tmp.buf));
					printf("[%ld],%s\n",strlen(str),str);
					isStr = true;
					pthread_mutex_unlock(&mutex);
				}
			}
			if(isThreadEnd) {
				break;
			}
		}
		// エコーバック
		//write(fd, buf, len);
	}
	printf("serial close");
	close(fd);                              // デバイスのクローズ
	return NULL;
}

void* camera(void *pParam) {
	pthread_t serialThread = (pthread_t) pParam;
	printf("%08lx\n",(long) serialThread);
	static int mode = 0;
	bool isChange = true;
	bool isRec = false;
	bool isPlay = false;
	bool isSerial = false;
	char serialData[256];
	bool isThreadEnd;
	int delay = 1;

#define REC_FRAME_RATE 60
#define PLAY_FRAME_RATE 6

	cv::Mat ready = cv::imread("ready.png", cv::IMREAD_COLOR);
	cv::Mat ka = cv::imread("ka.png", cv::IMREAD_COLOR);
	cv::Mat me = cv::imread("me.png", cv::IMREAD_COLOR);
	cv::Mat ha = cv::imread("ha.png", cv::IMREAD_COLOR);
	cv::Mat fail = cv::imread("fail.png", cv::IMREAD_COLOR);


	// test time
	time_t timer;
	struct tm *date;
	char filename[256] = {'\0'};
	char cmd[256];

	cv::VideoWriter rec;		// video書き込み用のクラス宣言
	cv::VideoCapture play;//デバイスのオープン

	cv::VideoCapture cap(0);//デバイスのオープン
	cap.set(cv::CAP_PROP_FPS, REC_FRAME_RATE);			// フレームレートを設定
	int frame_width=   cap.get(CV_CAP_PROP_FRAME_WIDTH);			//
	int frame_height=   cap.get(CV_CAP_PROP_FRAME_HEIGHT);		//
	//cap.open(0);//こっちでも良い．

	if(!cap.isOpened())//カメラデバイスが正常にオープンしたか確認．
	{
		return NULL; //読み込みに失敗したときの処理
	}

	memset(serialData,0,sizeof(serialData));
	while(1)//無限ループ
	{
		cv::Mat frame;
		cv::Mat playback_frame;
		cap >> frame; // get a new frame from camera
		//
		//取得したフレーム画像に対して，クレースケール変換や2値化などの処理を書き込む．
		//
		int key = cv::waitKey(delay);
		if(isSerial) {
			int l = 0;
			char *p;
			p = strtok(serialData,"\n");
			while(p) {
				printf("[%d] %s\n",l++,p);
				if(strncmp(p,"0",16) == 0) {
					printf("mode=0\n");
					key = 0x30;
					break;
				} else if(strncmp(p,"1",16) == 0) {
					printf("mode=1\n");
					key = 0x31;
				} else if(strncmp(p,"2",16) == 0) {
					printf("mode=2\n");
					key = 0x32;
				} else if(strncmp(p,"3",16) == 0) {
					printf("mode=3\n");
					key = 0x33;
				} else if(strncmp(p,"4",16) == 0) {
					printf("mode=4\n");
					key = 0x34;
				} else if(strncmp(p,"5",16) == 0) {
					printf("mode=5\n");
					key = 0x35;
				} else if(strncmp(p,"6",16) == 0) {
					printf("mode=6\n");
					key = 0x36;
					break;
				} 
				p=strtok(NULL,"\n");
			}
			memset(serialData,0,sizeof(serialData));
			isSerial = false;
		}
		switch(key) {
			case 27:			// ESC (EXIT);
				pthread_mutex_lock(&mutex);
				isStop = true;
				pthread_mutex_unlock(&mutex);
				break;
			case 0x30:			// ready
			case 0x31:			// KA
			case 0x32:			// ME
			case 0x33:			// HA
			case 0x35:			// fail
				if(mode != (key-0x30)) {;
					isChange = true;
					mode = key-0x30;
					if(isRec) {
						rec.release();
						isRec = false;
					}
					if(isPlay) {
						play.release();
						isPlay = false;
						delay = 1;
					}
				}
				break;
			case 0x34:			// HA
				if(mode != (key-0x30)) {;
					isChange = true;
					mode = key-0x30;
					if(isPlay) {
						play.release();
						isPlay = false;
						delay = 1;
					}
				}
				break;
			case 0x36:			// playback
				if(mode != (key-0x30)) {;
					isPlay = true;
					isChange = true;
					mode = key-0x30;
					if(isRec) {
						rec.release();
						isRec = false;
					}
				}
				break;
			default:
				if(key>0) {
					printf("%d\n",key);
				}
				break;
		}
		if(isThreadEnd) {
			cap.release();
			if(isRec) rec.release();
			if(isPlay) play.release();
			break;
		}

		switch(mode) {
			case 0:
				if(isChange) {
					cv::imshow("window", ready);//画像を表示．
					isChange = false;
				}
				break;
			case 1:
				if(isChange) {
					cv::imshow("window", ka);//画像を表示．
					isChange = false;
				}
				break;
			case 2:
				if(isChange) {
					cv::imshow("window", me);//画像を表示．
					isChange = false;
				}
				break;
			case 3:
				if(isChange) {
					cv::imshow("window", ha);//画像を表示．
					isChange = false;
				}
				break;
			case 4:
				if(isChange) {
					struct tm *localtime(const time_t *timer);			// 時刻取得用
					timer = time(NULL);          // 日付時刻を取得
					date = localtime(&timer);    // tm構造体に変換
					strftime(filename, sizeof(filename), "%Y_%m%d_%H%M.avi", date);	// 書式変換してファイル名を生成
					rec.open(filename,CV_FOURCC('M','J','P','G'),PLAY_FRAME_RATE, cv::Size(frame_width,frame_height),true);
					isChange = false;
					isRec = true;
					cv::imshow("window", ha);//画像を表示．
				}
				//cv::imshow("window", frame);//画像を表示．
				rec.write(frame);
				break;
			case 5:
				if(isChange) {
					cv::imshow("window", fail);//画像を表示．
					system("mpg321 onara_018.mp3");
					isChange = false;
				}
				break;
			case 6:
				if(isChange) {
					isChange = false;
					if(filename[0]){
						isPlay = true;
						play.open(filename);
						play.set(cv::CAP_PROP_FPS, PLAY_FRAME_RATE);			// フレームレートを設定
					}
				}
				if(isPlay) {
					play >> playback_frame; // get a new frame from camera
					if(!playback_frame.empty()) {
						cv::imshow("window", playback_frame);//画像を表示．
						delay = 1000/PLAY_FRAME_RATE;
					} else {
						isPlay=false;
						delay = 1;
						play.release();
					}
				}
				break;
			default:
				break;
		}
		pthread_mutex_lock(&mutex);
		if(isStr) {
			strncat(serialData,str,sizeof(serialData));
			isSerial = true;
			memset(str,0,sizeof(str));
			isStr = false;
		}
		isThreadEnd = isStop;
		pthread_mutex_unlock(&mutex);
	}
	printf("camera close\n");
	cv::destroyAllWindows();				// destroy openCV window
	pthread_cancel(serialThread);
	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t thread1, thread2;
	int ret1,ret2;

	pthread_mutex_init(&mutex, NULL);

	pthread_create(&thread1,NULL,serial,NULL);
	printf("%08lx\n",(long) thread1);
	pthread_create(&thread2,NULL,camera,(void*) thread1);

	pthread_join(thread1,NULL);
	pthread_join(thread2,NULL);

	pthread_mutex_destroy(&mutex); 

	printf("done\n");

	return 0;
}

