/* 
 * ������ͥåȥ���������ʶ���ô��ʬ��  ����ץ륽���������� (5)
 *
 * UDP���������饤����ȡ������ॢ���Ƚ����б���
 *
 *	����ѥ��롧gcc -Wall -o UDPEchoClient-Timeout UDPEchoClient-Timeout.c
 *		�Ȥ�����UDPEchoClient-Timeout <Server IP> <Echo Word> [<Echo Port>]
 *
 *		�����Ѥ���ݡ����ֹ�� 10000 + �����ֹ�β�4��
 *			����ϳ����ֹ椬 0312013180 �ξ�硧 10000 + 3180 = 13180
 */

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#define ECHOMAX			(255)	/* ������ʸ����κ���Ĺ */
#define TIMEOUT_SECS	(2)		/* �����ॢ����������(�ÿ�) */
#define MAXTRIES		(5)		/* ����������� */

int tries = 0;							/* ������������� */
void AlarmSignalHandler(int signo);		/* SIGALRM ȯ�����Υ����ʥ�ϥ�ɥ� */

int main(int argc, char *argv[])
{
  char *servIP;					/* �����������Ф�IP���ɥ쥹 */
  char *echoString;				/* �����������Ф���������ʸ���� */
  int echoStringLen;			/* �����������Ф���������ʸ�����Ĺ�� */
  unsigned short servPort;		/* �����������ФΥݡ����ֹ� */

  int sock;						/* �����åȥǥ�������ץ� */
  struct sigaction sigAction;	/* �����ʥ�ϥ�ɥ������ѹ�¤�� */
  struct sockaddr_in servAddr;	/* �������������ѥ��ɥ쥹��¤�� */

  int sendMsgLen;				/* ������å�������Ĺ�� */
  struct sockaddr_in fromAddr;	/* ��å������������ѥ��ɥ쥹��¤�� */
  unsigned int fromAddrLen;		/* ��å������������ѥ��ɥ쥹��¤�Τ�Ĺ�� */
  char msgBuffer[ECHOMAX+1];	/* ��å������������Хåե� */
  int recvMsgLen;				/* ������å�������Ĺ�� */

  /* �����ο����ǧ���롥*/
  if ((argc < 3) || (argc > 4)) {
    fprintf(stderr,"Usage: %s <Server IP> <Echo Word> [<Echo Port>]\n", argv[0]);
    exit(1);
  }
  /* ��1�������饨���������Ф�IP���ɥ쥹��������롥*/
  servIP = argv[1];

  /* ��2�������饨���������Ф���������ʸ�����������롥*/
  echoString = argv[2];

  /* �����������Ф���������ʸ�����Ĺ����������롥*/
  echoStringLen = strlen(echoString);
  if (echoStringLen > ECHOMAX) {
    fprintf(stderr, "Echo word too long\n");
    exit(1);
  }
  /* ��3�������饨���������ФΥݡ����ֹ��������롥*/
  if (argc == 4) {
    servPort = atoi(argv[3]);
  }
  else {
    servPort = 7;
  }

  /* ��å��������������˻Ȥ������åȤ�������롥*/
  sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0) {
    fprintf(stderr, "socket() failed\n");
    exit(1);
  }

  /* �����ʥ�ϥ�ɥ�����ꤹ�롥*/
  sigAction.sa_handler = AlarmSignalHandler;

  /* �ϥ�ɥ���ǥ֥�å����륷���ʥ�����ꤹ��(���ƤΥ����ʥ��֥�å�����)��*/
  if (sigfillset(&sigAction.sa_mask) < 0) {
    fprintf(stderr, "sigfillset() failed\n");
    exit(1);
  }

  /* �����ʥ�ϥ�ɥ�˴ؤ��륪�ץ��������̵����*/
  sigAction.sa_flags = 0;

  /* �����ʥ�ϥ�ɥ������ѹ�¤�Τ�Ȥäơ������ʥ�ϥ�ɥ����Ͽ���롥*/
  if (sigaction(SIGALRM, &sigAction, 0) < 0) {
    fprintf(stderr, "sigaction() failed\n");
    exit(1);
  }

  /* �������������ѥ��ɥ쥹��¤�Τ�ɬ�פ��ͤ��Ǽ���롥*/
  memset(&servAddr, 0, sizeof(servAddr));			/* ��¤�Τ򥼥�ǽ���� */
  servAddr.sin_family		= AF_INET;				/* ���󥿡��ͥåȥ��ɥ쥹�ե��ߥ� */
  servAddr.sin_addr.s_addr	= inet_addr(servIP);	/* �����Ф�IP���ɥ쥹 */
  servAddr.sin_port			= htons(servPort);		/* �����ФΥݡ����ֹ� */

  /* �����������Фإ�å��������������롥*/
  sendMsgLen = sendto(sock, echoString, echoStringLen, 0,
    (struct sockaddr*)&servAddr, sizeof(servAddr));

  /* ���������٤���å�������Ĺ�����������줿��å�������Ĺ�������������Ȥ��ǧ���롥*/
  if (echoStringLen != sendMsgLen) {
    fprintf(stderr, "sendto() sent a different number of bytes than expected\n");
    exit(1);
  }

  /* �����ॢ���Ȼ��֤����ꤹ�롥*/
  alarm(TIMEOUT_SECS);

  /* ��������å�������������롥*/
  fromAddrLen = sizeof(fromAddr);
  recvMsgLen = recvfrom(sock, msgBuffer, ECHOMAX, 0,
    (struct sockaddr *) &fromAddr, &fromAddrLen);

  /* �����ॢ���Ȼ��������������롼�ס�*/
  while (recvMsgLen < 0) {

    /* SIGALRM ��ȯ���������ɤ������ǧ���롥*/
    if (errno != EINTR) {
      fprintf(stderr, "recvfrom() failed\n");
      exit(1);
    }

    /* �����ॢ���Ȳ����ɽ�����롥*/
    printf("timed out : %d\n", tries);

    /* ������������󥿤Ⱥ��������������Ӥ��롥*/
    if (tries >= MAXTRIES) {
      fprintf(stderr, "No Response\n");
      exit(1);
    }

    /* �����������Фإ�å���������������롥*/
    sendMsgLen = sendto(sock, echoString, echoStringLen, 0,
      (struct sockaddr*)&servAddr, sizeof(servAddr));
    if (echoStringLen != sendMsgLen) {
      fprintf(stderr, "sendto() sent a different number of bytes than expected\n");
      exit(1);
    }

    /* �����ॢ���Ȼ��֤����ꤹ�롥*/
    alarm(TIMEOUT_SECS);

    /* ��������å�������������롥*/
    fromAddrLen = sizeof(fromAddr);
    recvMsgLen = recvfrom(sock, msgBuffer, ECHOMAX, 0,
      (struct sockaddr *) &fromAddr, &fromAddrLen);
  }

  /* �����ॢ���Ȼ��֤�����򥭥�󥻥뤹�롥*/
  alarm(0);

  /* ������å�������Ĺ�������������Ȥ��ǧ���롥*/
  if (recvMsgLen != echoStringLen) {
    fprintf(stderr, "recvfrom() failed\n");
    exit(1);
  }

  /* ��������å��������������������������ФǤ��뤳�Ȥ��ǧ���롥*/
  if (fromAddr.sin_addr.s_addr != servAddr.sin_addr.s_addr) {
    fprintf(stderr,"Error: received a packet from unknown source\n");
    exit(1);
  }

  /* ����������������å�������NULLʸ���ǽ�ü����ɽ�����롥*/
  msgBuffer[recvMsgLen] = '\0';
  printf("Received: %s\n", msgBuffer);

  /* �����åȤ��Ĥ��ƥץ�����λ���롥*/
  close(sock);
  exit(0);
}

/* SIGALRM ȯ�����Υ����ʥ�ϥ�ɥ� */
void AlarmSignalHandler(int signo)
{
  tries += 1;
}
