/* 
 * ■情報ネットワーク実践論（橋本担当分）  サンプルソースコード (5)
 *
 * UDPエコークライアント：タイムアウト処理対応版
 *
 *	コンパイル：gcc -Wall -o UDPEchoClient-Timeout UDPEchoClient-Timeout.c
 *		使い方：UDPEchoClient-Timeout <Server IP> <Echo Word> [<Echo Port>]
 *
 *		※利用するポート番号は 10000 + 学籍番号の下4桁
 *			［例］学籍番号が 0312013180 の場合： 10000 + 3180 = 13180
 */

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#define ECHOMAX			(255)	/* エコー文字列の最大長 */
#define TIMEOUT_SECS	(2)		/* タイムアウト設定値(秒数) */
#define MAXTRIES		(5)		/* 最大送信回数 */

int tries = 0;							/* 送信回数カウンタ */
void AlarmSignalHandler(int signo);		/* SIGALRM 発生時のシグナルハンドラ */

int main(int argc, char *argv[])
{
  char *servIP;					/* エコーサーバのIPアドレス */
  char *echoString;				/* エコーサーバへ送信する文字列 */
  int echoStringLen;			/* エコーサーバへ送信する文字列の長さ */
  unsigned short servPort;		/* エコーサーバのポート番号 */

  int sock;						/* ソケットディスクリプタ */
  struct sigaction sigAction;	/* シグナルハンドラ設定用構造体 */
  struct sockaddr_in servAddr;	/* エコーサーバ用アドレス構造体 */

  int sendMsgLen;				/* 送信メッセージの長さ */
  struct sockaddr_in fromAddr;	/* メッセージ送信元用アドレス構造体 */
  unsigned int fromAddrLen;		/* メッセージ送信元用アドレス構造体の長さ */
  char msgBuffer[ECHOMAX+1];	/* メッセージ送受信バッファ */
  int recvMsgLen;				/* 受信メッセージの長さ */

  /* 引数の数を確認する．*/
  if ((argc < 3) || (argc > 4)) {
    fprintf(stderr,"Usage: %s <Server IP> <Echo Word> [<Echo Port>]\n", argv[0]);
    exit(1);
  }
  /* 第1引数からエコーサーバのIPアドレスを取得する．*/
  servIP = argv[1];

  /* 第2引数からエコーサーバへ送信する文字列を取得する．*/
  echoString = argv[2];

  /* エコーサーバへ送信する文字列の長さを取得する．*/
  echoStringLen = strlen(echoString);
  if (echoStringLen > ECHOMAX) {
    fprintf(stderr, "Echo word too long\n");
    exit(1);
  }
  /* 第3引数からエコーサーバのポート番号を取得する．*/
  if (argc == 4) {
    servPort = atoi(argv[3]);
  }
  else {
    servPort = 7;
  }

  /* メッセージの送受信に使うソケットを作成する．*/
  sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0) {
    fprintf(stderr, "socket() failed\n");
    exit(1);
  }

  /* シグナルハンドラを設定する．*/
  sigAction.sa_handler = AlarmSignalHandler;

  /* ハンドラ内でブロックするシグナルを設定する(全てのシグナルをブロックする)．*/
  if (sigfillset(&sigAction.sa_mask) < 0) {
    fprintf(stderr, "sigfillset() failed\n");
    exit(1);
  }

  /* シグナルハンドラに関するオプション指定は無し．*/
  sigAction.sa_flags = 0;

  /* シグナルハンドラ設定用構造体を使って，シグナルハンドラを登録する．*/
  if (sigaction(SIGALRM, &sigAction, 0) < 0) {
    fprintf(stderr, "sigaction() failed\n");
    exit(1);
  }

  /* エコーサーバ用アドレス構造体へ必要な値を格納する．*/
  memset(&servAddr, 0, sizeof(servAddr));			/* 構造体をゼロで初期化 */
  servAddr.sin_family		= AF_INET;				/* インターネットアドレスファミリ */
  servAddr.sin_addr.s_addr	= inet_addr(servIP);	/* サーバのIPアドレス */
  servAddr.sin_port			= htons(servPort);		/* サーバのポート番号 */

  /* エコーサーバへメッセージを送信する．*/
  sendMsgLen = sendto(sock, echoString, echoStringLen, 0,
    (struct sockaddr*)&servAddr, sizeof(servAddr));

  /* 送信されるべきメッセージの長さと送信されたメッセージの長さが等しいことを確認する．*/
  if (echoStringLen != sendMsgLen) {
    fprintf(stderr, "sendto() sent a different number of bytes than expected\n");
    exit(1);
  }

  /* タイムアウト時間を設定する．*/
  alarm(TIMEOUT_SECS);

  /* エコーメッセージを受信する．*/
  fromAddrLen = sizeof(fromAddr);
  recvMsgLen = recvfrom(sock, msgBuffer, ECHOMAX, 0,
    (struct sockaddr *) &fromAddr, &fromAddrLen);

  /* タイムアウト時の送受信処理ループ．*/
  while (recvMsgLen < 0) {

    /* SIGALRM が発生したかどうかを確認する．*/
    if (errno != EINTR) {
      fprintf(stderr, "recvfrom() failed\n");
      exit(1);
    }

    /* タイムアウト回数を表示する．*/
    printf("timed out : %d\n", tries);

    /* 送信回数カウンタと最大送信回数を比較する．*/
    if (tries >= MAXTRIES) {
      fprintf(stderr, "No Response\n");
      exit(1);
    }

    /* エコーサーバへメッセージを再送信する．*/
    sendMsgLen = sendto(sock, echoString, echoStringLen, 0,
      (struct sockaddr*)&servAddr, sizeof(servAddr));
    if (echoStringLen != sendMsgLen) {
      fprintf(stderr, "sendto() sent a different number of bytes than expected\n");
      exit(1);
    }

    /* タイムアウト時間を設定する．*/
    alarm(TIMEOUT_SECS);

    /* エコーメッセージを受信する．*/
    fromAddrLen = sizeof(fromAddr);
    recvMsgLen = recvfrom(sock, msgBuffer, ECHOMAX, 0,
      (struct sockaddr *) &fromAddr, &fromAddrLen);
  }

  /* タイムアウト時間の設定をキャンセルする．*/
  alarm(0);

  /* 受信メッセージの長さが正しいことを確認する．*/
  if (recvMsgLen != echoStringLen) {
    fprintf(stderr, "recvfrom() failed\n");
    exit(1);
  }

  /* エコーメッセージの送信元がエコーサーバであることを確認する．*/
  if (fromAddr.sin_addr.s_addr != servAddr.sin_addr.s_addr) {
    fprintf(stderr,"Error: received a packet from unknown source\n");
    exit(1);
  }

  /* 受信したエコーメッセージをNULL文字で終端し，表示する．*/
  msgBuffer[recvMsgLen] = '\0';
  printf("Received: %s\n", msgBuffer);

  /* ソケットを閉じてプログラムを終了する．*/
  close(sock);
  exit(0);
}

/* SIGALRM 発生時のシグナルハンドラ */
void AlarmSignalHandler(int signo)
{
  tries += 1;
}
