//マルチスレッドプログラムと mutex の使い方

#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

void* thread1(void* pParam); //スレッド１
void* thread2(void* pParam); //スレッド２
int count1=0;
int count2=0;
pthread_mutex_t mutex; //2つのスレッド間で変数の保護を行う

int main(int argc, char *argv[]){
  pthread_t tid1, tid2; // スレッド識別変数

  pthread_mutex_init(&mutex, NULL);
  // スレッドの作成
  pthread_create(&tid1, NULL, thread1, NULL);
  pthread_create(&tid2, NULL, thread2, NULL);
  
  // スレッド終了待ち
  pthread_join(tid1,NULL);
  pthread_join(tid2,NULL);
  
  pthread_mutex_destroy(&mutex); 
  return 0;
}

//スレッド１
void* thread1(void* pParam)
{
  int i;
  while(1){
    //mutex 間は他のスレッドから変数を変更できない
    pthread_mutex_lock(&mutex);
    printf("count1:");
    for(i=0;i<10;i++){
      printf("%d:",count1);
      count1++;
    }
    printf("\n");
    sleep(1);
    pthread_mutex_unlock(&mutex);
    
    //mutex で変数を保護しないと他のスレッドから変数を変更できる
    printf("count2:");
    for(i=0;i<10;i++){
      printf("%d:",count2);
      count2++;
    }
    printf("\n");
    sleep(1);
  }
}

//スレッド２
void* thread2(void* pParam)
{
  while(1){
    //mutex で  count1 を保護
    pthread_mutex_lock(&mutex);
    count1=0;
    pthread_mutex_unlock(&mutex);

    count2=0;
  }
}
