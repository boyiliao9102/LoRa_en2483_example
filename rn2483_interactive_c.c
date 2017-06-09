#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define DEBUG 0

char init_msg[]="rn2483_interface_c example code version 0.1 \nTerminal the program when input \"exit\"\n ";

int set_interface_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                //error_message ("error %d from tcgetattr", errno);
                error("error %d from tcgetattr", errno);
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                //error_message ("error %d from tcsetattr", errno);
                error("error %d from tcsetattr", errno);
                return -1;
        }
        return 0;
}

void
set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                //error_message ("error %d from tggetattr", errno);
                error("error %d from tggetattr", errno);
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                //error_message ("error %d setting term attributes", errno);
                error("error %d setting term attributes", errno);
}



char *portname = "/dev/ttyUSB0";
char *send_msg = "sys get ver\r\n";
char send_msg_in[128];
int i;
int send_msg_size;
char buf [128];
char rt_buf [128];

int main(void)
{

    int n;
    char line[4];
    int exit_count=0; 
    int in_counter=0;
    int fd;


    fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
    //open devices
    if (fd < 0)
    {
        //error_message ("error %d opening %s: %s", errno, portname, strerror (errno));
        error("error %d opening %s: %s", errno, portname, strerror (errno));
        return -1;
    }

#if 0

    // fixed cmd message buffer dump example 
    printf("====== start =====\n");
    for(i=0;i<(strlen(send_msg)+1);i++)
        printf("0x%x|",send_msg[i]);

    printf("\n");
    printf("====== end =====\n");

#endif



    set_interface_attribs (fd, B57600, 0);  // set speed to 57600 bps, 8n1 (no parity)
    set_blocking (fd, 0);                // set no blocking
    //set_blocking (fd, 1);                // set blocking

    //init message
    printf("%s\n",init_msg);

START:


    // process user command
    memset(send_msg_in,0,strlen(send_msg_in));
    printf("please input command >> ");
    fgets(send_msg_in,sizeof(send_msg_in),stdin);
    printf("user input command = %s\n",send_msg_in);
    send_msg_size = strlen(send_msg_in);

#if DEBUG
    printf("user input string size=%d\n",send_msg_size);
#endif

    if(send_msg_size == 5){  
         if(strncmp(send_msg_in,"exit",4) == 0)
             return 0;
    }

    // append 0x0d 0x0a 0x00 to cmd string.
    send_msg_in[send_msg_size-1]= 0x0d;
    send_msg_in[send_msg_size]= 0x0a;
    send_msg_in[send_msg_size+1]= 0x0;

#if DEBUG

    // debug user input command
    printf("====== start input =====\n");
    for(i=0;i<(strlen(send_msg_in)+1);i++)
        printf("0x%x|",send_msg_in[i]);

    printf("\n");
    printf("====== end input =====\n");

#endif


    //write (fd, "hello!\n", 7);           // send 7 character greeting
    write (fd, send_msg_in, strlen(send_msg_in));           // send character greeting

    //usleep (100*1000);                    
    usleep ((7 + 25) * 100);             // sleep enough to transmit the 7 plus
                                           // receive 25:  approx 100 uS per char transmit

     
    /* int n = read (fd, buf, sizeof(buf)); */  // read up to 100 characters if ready to read

     while(1) {
         n = read (fd, buf, sizeof(buf));
         if ( n > 0){
#if DEBUG
             printf("\nread %i bytes\n",n);
             printf("\n:%s\n",buf);
#endif
             if(in_counter == 0){
                 strcpy(rt_buf,buf);
                 in_counter=in_counter+n;
             }
             else{
                 strncpy(rt_buf+in_counter,buf,n);
                 in_counter=in_counter+n;
             }
                 
         }
             //printf("\n>> n= %d\n",n);
             if(n==0){
                 exit_count++;
                 if(exit_count == 2)
                     break;
             }
         
     }   

#if DEBUG
    printf("number = %d\n",n);   
#endif

    printf("rt msg = ");
    for(i=0;i<strlen(rt_buf);i++){ 
        //printf("0x%x",buf[i]);
        printf("%c",rt_buf[i]);
    }
    printf("\n");

    //reset buffer, counter
    memset(buf,0,strlen(buf));
    memset(rt_buf,0,strlen(rt_buf));
    exit_count=0;
    in_counter=0;

goto START;
 
   close(fd);
    
    return 0;
}
