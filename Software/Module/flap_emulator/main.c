#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h> 
#include <sys/timerfd.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <termios.h>
#include "chain_comm.h"

struct pollfd pfds[3];
int id;
uint8_t is_col_end;
uint8_t letter[4];

void fork_flap(int _id, int rows, int cols, int rx_fd, int tx_fd);

void write_page(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        DEBUG_PRINT("write_page\n");
    }else{
        // command info
        cmd_info->rx_data_len = 66;
        cmd_info->cmd = cmd_write_page;
        cmd_info->cmd_callback = write_page;
    }
}

void goto_app(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        DEBUG_PRINT("goto_app\n");
    }else{
        // command info
        cmd_info->rx_data_len = 0;
        cmd_info->cmd = cmd_goto_app;
        cmd_info->cmd_callback = goto_app;
    }
}

void goto_btl(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        DEBUG_PRINT("goto_btl\n");
    }else{
        // command info
        cmd_info->rx_data_len = 0;
        cmd_info->cmd = cmd_goto_btl;
        cmd_info->cmd_callback = goto_btl;
    }
}

void get_config(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        DEBUG_PRINT("get_config\n");
        tx_data[0] = is_col_end;
    }else{
        // command info
        cmd_info->rx_data_len = 0;
        cmd_info->cmd = cmd_get_config;
        cmd_info->cmd_callback = get_config;
    }
}

void set_char(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        DEBUG_PRINT("set_char\n");
        memcpy(letter,rx_data,4);
    }else{
        // command info
        cmd_info->rx_data_len = 4;
        cmd_info->cmd = cmd_set_char;
        cmd_info->cmd_callback = set_char;
    }
}

void get_char(uint8_t* rx_data,uint8_t* tx_data,cmd_info_t* cmd_info)
{
    if(cmd_info == NULL){
        DEBUG_PRINT("get_char\n");
        memcpy(tx_data,letter,4);
    }else{
        // command info
        cmd_info->rx_data_len = 0;
        cmd_info->cmd = cmd_get_char;
        cmd_info->cmd_callback = get_char;
    }
}

int main(int argc, char *argv[]) {

    if(argc < 3){
        printf("number of flap rows and cols requred as agument.\n");
        exit(EXIT_FAILURE);
    }
    int rows = strtol(argv[2], NULL, 10);
    int cols = strtol(argv[1], NULL, 10);

    install_command(write_page);
    install_command(get_config);
    install_command(set_char);
    install_command(get_char);

    printf("emulating a split flap display of %dx%d (%d)\n",cols,rows,rows*cols);

    mkfifo("rx_fifo", 0666);
    int fifo_rx = open("rx_fifo", O_RDWR | O_NONBLOCK);
    mkfifo("tx_fifo", 0666);
    int fifo_tx = open("tx_fifo", O_RDWR | O_NONBLOCK);

    if(argc == 4){
        int serial_port = open(argv[3], O_RDWR | O_NOCTTY | O_NDELAY | O_TRUNC);
        if(serial_port < 0){
            printf("Could not open serial port %s\n",argv[3]);
            exit(EXIT_FAILURE);
        }
        struct termios options;
        tcgetattr(serial_port, &options);          
        cfsetispeed(&options, B115200);     
        cfmakeraw(&options) ;                              
        tcsetattr(serial_port, TCSANOW, &options);          
        fcntl(serial_port, F_SETFL, FNDELAY);  
        fifo_rx = serial_port;
        fifo_tx = serial_port;  
    }

    fork_flap(0,rows,cols,fifo_rx,fifo_tx); 
}


void fork_flap(int _id, int rows, int cols, int rx_fd, int tx_fd){
    int pipe_fd[2];
    if(!pipe(pipe_fd)){
        pid_t pid = fork();
        if(pid == 0){ // child
            if(_id+1 < rows * cols){ 
                fork_flap(_id+1,rows,cols,pipe_fd[0], tx_fd); 
                exit(0);
            }  
        }
        else if(pid > 0){ // parent
            id = _id;
            is_col_end = ((id+1)%rows == 0);
            printf("hello from flap process %d\n",id);
            pfds[0].fd = rx_fd;
            pfds[0].events = POLLIN;
            pfds[1].fd = (_id+1 < rows * cols)? pipe_fd[1] : tx_fd;
            pfds[1].events = POLLOUT;
            pfds[2].fd = timerfd_create(CLOCK_REALTIME, 0);
            pfds[2].events = POLLIN;

            struct itimerspec timerValue;
            bzero(&timerValue, sizeof(timerValue));
            timerValue.it_value.tv_sec = 0;
            timerValue.it_value.tv_nsec = 250000000;
            timerValue.it_interval.tv_sec = 0;
            timerValue.it_interval.tv_nsec = 0;
            timerfd_settime(pfds[2].fd, 0, &timerValue, NULL);

            while(poll(pfds, 3, -1) >= 0) {
                if (pfds[0].revents & POLLIN) {
                    timerfd_settime(pfds[2].fd, 0, &timerValue, NULL);  // activate timer
                    pfds[2].events = POLLIN;                            // activate timer
                    chain_comm(comm_rx_data);
                }
                if (pfds[1].revents & POLLOUT) {
                    pfds[1].events = 0;     // deactivate pollout 
                    timerfd_settime(pfds[2].fd, 0, &timerValue, NULL);  // activate timer
                    pfds[2].events = POLLIN;                            // activate timer
                    chain_comm(comm_tx_data);
                }
                if (pfds[2].revents & POLLIN) {
                    int timersElapsed = 0;
                    pfds[2].events = 0;
                    size_t s = read(pfds[2].fd, &timersElapsed, 4);
                    chain_comm(comm_timeout);
                }
                usleep(10000);
            }
            return;
        }
    }
}