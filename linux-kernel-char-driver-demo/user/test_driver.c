#include <stdio.h>
#include <fcntl.h> // file/device control: open , O_RDWR
#include <unistd.h>// read write close
#include <sys/ioctl.h>
#include <string.h>

#define MY_MAGIC 'a'
#define SET_CONFIG _IOW(MY_MAGIC, 1, struct device_config)//write to device. user->kernel
#define GET_CONFIG _IOR(MY_MAGIC, 2, struct device_config)//read from device. kernel->user
#define SET_MSG _IOW(MY_MAGIC, 1, char *)
#define GET_MSG _IOR(MY_MAGIC, 2, char *)

struct device_config {
    int mode;
    int speed;
    char name[32];
};

int main() {
    int fd = open("/dev/my_device", O_RDWR);
    if(fd < 0) { perror("open"); return -1; }

    // write
    char msg[] = "Hello Kernel!";
    if(write(fd, msg, strlen(msg)) < 0) perror("write");

    // read
    char buffer[256];
    if(read(fd, buffer, sizeof(buffer)) > 0)
        printf("Read from device: %s\n", buffer);

    // ioctl set
    struct device_config cfg = {1, 500, "MyDevice"};
    if(ioctl(fd, SET_CONFIG, &cfg) < 0) perror("SET_CONFIG");

    // ioctl get
    struct device_config cfg2;
    if(ioctl(fd, GET_CONFIG, &cfg2) == 0)
        printf("Config: mode=%d, speed=%d, name=%s\n",
               cfg2.mode, cfg2.speed, cfg2.name);

    close(fd);
    return 0;
}
