/*
 *
 * CSEE 4840 Lab 2 for 2019
 *
 * Name/UNI: Please Changeto Yourname (pcy2301)
 */
#include "fbputchar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "usbkeyboard.h"
#include <pthread.h>

/* Update SERVER_HOST to be the IP address of
 * the chat server you are connecting to
 */
/* micro36.ee.columbia.edu */
#define SERVER_HOST "128.59.148.182"
#define SERVER_PORT 42000

#define BUFFER_SIZE 128
#define MAX_PER_ROW 64
#define INIT_ROW 22
#define INIT_COL 0
#define MAXROW 23
#define HIG_BOUND_FIR 0
#define LOW_BOUND_FIR 9
#define HIG_BOUND_SEC 11
#define LOW_BOUND_SEC 20
#define HIG_BOUND_THI 22
#define LOW_BOUND_THI 23

/*
 * References:
 *
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */

int sockfd; /* Socket file descriptor */

struct libusb_device_handle *keyboard;
uint8_t endpoint_address;

pthread_t network_thread;
void *network_thread_f(void *);
void Custum_Initial();
void InitiateRow(int, int);
char *MoveString(char *, int);
void ActScroll(int, int, int, char *);
int JudgeClass(char);


int main()
{
  int err, col, currentCol, currentRow;
  char dispCharacter;
  char writeString[500];
  char *writeStringHead;
  char *writeHead;
  struct sockaddr_in serv_addr;
  int count=0;
  int i, flag;
  struct usb_keyboard_packet packet;
  int transferred;
  char keystate[12];
  int row2=HIG_BOUND_SEC;
  char writeStringBuffer1[MAX_PER_ROW];
  char writeStringBuffer2[MAX_PER_ROW];

  if ((err = fbopen()) != 0) {
    fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
    exit(1);
  }
  /* initial the screen */
  Custum_Initial();
  /* Draw rows of asterisks across the top and bottom of the screen */
  for (col = 0 ; col < 64 ; col++) {
    fbputchar('*', 10, col);
    fbputchar('*',21,col);
  }
  
  /* Open the keyboard */
  if ( (keyboard = openkeyboard(&endpoint_address)) == NULL ) {
    fprintf(stderr, "Did not find a keyboard\n");
    exit(1);
  }
    
  /* Create a TCP communications socket */
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    fprintf(stderr, "Error: Could not create socket\n");
    exit(1);
  }

  /* Get the server address */
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);
  if ( inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0) {
    fprintf(stderr, "Error: Could not convert host IP \"%s\"\n", SERVER_HOST);
    exit(1);
  }

  /* Connect the socket to the server */
  if ( connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "Error: connect() failed.  Is the server running?\n");
    exit(1);
  }

  /* Start the network thread */
  pthread_create(&network_thread, NULL, network_thread_f, NULL);

  /* Look for and handle keypresses */
  currentRow=HIG_BOUND_THI;
  currentCol=INIT_COL;
  writeStringHead = writeString;
  writeHead = writeString;
  for (;;) {
    libusb_interrupt_transfer(keyboard, endpoint_address,
			      (unsigned char *) &packet, sizeof(packet),
			      &transferred, 0);
    if (transferred == sizeof(packet)) {
      sprintf(keystate, "%02x %02x %02x", packet.modifiers, packet.keycode[0],
	      packet.keycode[1]);
      printf("%s\n", keystate);
      fbputs(keystate, 6, 0);
      /* Here we find a key pressed*/
      if (packet.keycode[0]!=0){
	       printf("count = %d, col= %d, row= %d\n", count, currentCol, currentRow); /* Delete this line later */
         if (packet.keycode[1] == 00){/* we only read in when only one character key is pressed to prevent shack */
          dispCharacter = keyValue(packet.modifiers, packet.keycode[0]);
         }
         else{
           continue;
         }
         
         /* Assume we have a function JudgeClass to judge whether it's a control or a letter, return flag=0 if it is a letter, flag!=0 if it is a function  */
         flag = JudgeClass(packet.keycode[0]);
         if (flag==1){ /* if Enter is pressed */
           
           
           /* Assume the string to write is less than BUFFER_SIZE */
           write(sockfd, writeString, strlen(writeString)); /* send to server*/
           
           /* whenever put in long string */
           /* check the length first */
            if (strlen(writeString)<=MAX_PER_ROW) {/* if length is shorter than a single line width */
                fbputs(writeString,row2,0);
                row2++;
                writeString[0] = '\0';
            }
            else{/* if length is larger than one line, split them into two lines */
                strncpy(writeStringBuffer1,writeString,MAX_PER_ROW);
                writeStringBuffer1[MAX_PER_ROW]='\0';
                strncpy(writeStringBuffer2,writeString+MAX_PER_ROW,strlen(writeString)-MAX_PER_ROW);
                writeStringBuffer2[strlen(writeString)-MAX_PER_ROW]='\0';
                fbputs(writeStringBuffer1,row2,0);
                row2++;
                fbputs(writeStringBuffer2,row2,0);
                row2++;
                writeString[0] = '\0';
                writeStringBuffer1[0]='\0';
                writeStringBuffer2[0]='\0';
            }
            InitiateRow(HIG_BOUND_THI,LOW_BOUND_THI);
            count = 0;
            currentCol = 0;
            currentRow = HIG_BOUND_THI;
         }
         else if (flag == 0){/* if a character is pressed */
           /* we only allow maximum 128 character*/
           if (count<128) {
              fbputchar(dispCharacter, currentRow, currentCol);
	            writeString[count] = dispCharacter;
              writeString[count+1]='\0';
	            currentCol++;
	            count++;
	            printf("count2 = %d, col2= %d, row2= %d\n", count, currentCol, currentRow);
              if (currentCol==MAX_PER_ROW && currentRow==HIG_BOUND_THI) {
                currentCol = 0;
                currentRow = LOW_BOUND_THI;
              }
           }
           else
           {
             continue;
           }
                      
	       }
         else if (flag == 2) {/* if delete is pressed */
           /*Do other function here*/
         }
         else if (flag == 3){/* if direction key is pressed */

         }
         
      }

      if (packet.keycode[0] == 0x29) { /* ESC pressed? */
	    break;
      }
    }
  }

  /* Terminate the network thread */
  pthread_cancel(network_thread);

  /* Wait for the network thread to finish */
  pthread_join(network_thread, NULL);

  return 0;
}

void Custum_Initial()
{
  int col, row;
  for (row = 0; row < 24; row++){
    for (col = 0 ; col < 64 ; col++) {
      fbputchar(' ', row, col);
    }
  }	
}

void InitiateRow(int min, int max){
  int col, row;
  for (row = min; row < max + 1; row++){
    for (col = 0 ; col < 64 ; col++) {
      fbputchar(' ', row, col);
    }
  }	
}



/* Scroll the screen if condition matches*/
void ActScroll(int minRow, int maxRow, int colPerRow, char *myString){
  /* myString should already been modified as exactly like we want to show in the screen*/
   int lineNum;
   int count=0;
   int row, col;
   InitiateRow(minRow, maxRow);
   lineNum = maxRow - minRow;
   for (row=minRow;row<maxRow;row++){
      for (col=0;col<colPerRow;col++){
        fbputchar(myString[count],row,col);
        count++;
      }
   }   
}

/* Move all element in a string forward n, others become '\0' */
char *MoveString(char toBeMoveString[], int moveLength){
  char *movedString;
  movedString=&toBeMoveString[moveLength];
  return movedString;
}

int JudgeClass(char dispCharacter){
  switch (dispCharacter){
    case 0x28:
      return 1;
      break;
    case 0x58:
      return 1;
      break;
      default:
      return 0;
  }
}


void *network_thread_f(void *ignored)
{
  char recvBuf[BUFFER_SIZE];
  char tempBuf1[MAX_PER_ROW+1];
  char tempBuf2[MAX_PER_ROW+1];
  char myBuff[700];
  int n;
  int countLength=0;
  int i;
  int tempRow=1;
  /* Read: Receive data */
  /* Assume the length is smaller than BUFFER_SIZE*/
  while ( (n = read(sockfd, &recvBuf, BUFFER_SIZE - 1)) > 0 ) {
    recvBuf[n] = '\0';
    printf("%s", recvBuf);
    for (i=0;i<strlen(recvBuf);i++){
      myBuff[i+countLength]=recvBuf[i];
    }
    if (n < MAX_PER_ROW || n==MAX_PER_ROW){  
      fbputs(recvBuf, tempRow, 0);
      tempRow++;
    }
    else{
      for (i=0;i<MAX_PER_ROW;i++){
        tempBuf1[i]=recvBuf[i];
      }
	tempBuf1[i]='\0';
      for (i=0;i < n-MAX_PER_ROW;i++){
        tempBuf2[i]=recvBuf[i];
      }
	tempBuf2[i]='\0';
      fbputs(tempBuf1, tempRow, 0);
      tempRow++;
      fbputs(tempBuf2, tempRow, 0);
      tempRow++;
    }	  
    /* Scroll up when it is full */
    if (tempRow>LOW_BOUND_FIR){
      strcpy(myBuff, MoveString(myBuff,MAX_PER_ROW));
      ActScroll(HIG_BOUND_FIR,LOW_BOUND_FIR,MAX_PER_ROW, myBuff);
    }
  }
  
  return NULL;
}

