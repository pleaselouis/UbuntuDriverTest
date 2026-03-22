#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define DEVICE_NAME "my_device"
#define MY_MAGIC 'a'

#define SET_CONFIG _IOW(MY_MAGIC, 1, struct device_config)//write to device. user->kernel
#define GET_CONFIG _IOR(MY_MAGIC, 2, struct device_config)//read from device. kernel->user

struct device_config {
    int mode;
    int speed;
    char name[32];
};

static char msg[256];
static struct device_config config;

static int major;// major number

static DEFINE_MUTEX(device_mutex);//mutex

// open
static int dev_open(struct inode *inode, struct file *file) {
    if(!mutex_trylock(&device_mutex))//if already locked than...
        return -EBUSY;  // device busy
    printk(KERN_INFO "Device opened\n");//print to dmesg
    return 0;
}

// release
static int dev_release(struct inode *inode, struct file *file) {
    mutex_unlock(&device_mutex);//unlock mutex
    printk(KERN_INFO "Device closed\n");
    return 0;
}

// read buffer form user space to kernel space
static ssize_t dev_read(struct file *file, char __user *buffer, size_t len, loff_t *offset) {
    size_t msg_len = strlen(msg);
    if (*offset >= msg_len)
        return 0; // EOF

    if (len > msg_len - *offset)
        len = msg_len - *offset;

    if(copy_to_user(buffer, msg + *offset, len))//copy buffer data to user space. !=0 is fail
        return -EFAULT;

    *offset += len;
    return len;
}

// write buffer form kernel space to user space
static ssize_t dev_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset) {
    if(len >= sizeof(msg))
        return -EINVAL;

    if(copy_from_user(msg, buffer, len))//copy buffer data to kernel space. !=0 is fail
        return -EFAULT;

    msg[len] = '\0';//make sure string end
    printk(KERN_INFO "Received: %s\n", msg);
    return len;
}

// ioctl
static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch(cmd) {
        case SET_CONFIG:
            if(copy_from_user(&config, (struct device_config __user *)arg, sizeof(config)))
                return -EFAULT;
            printk(KERN_INFO "Config set: mode=%d, speed=%d, name=%s\n",
                   config.mode, config.speed, config.name);
            return 0;

        case GET_CONFIG:
            if(copy_to_user((struct device_config __user *)arg, &config, sizeof(config)))
                return -EFAULT;
            return 0;

        default:
            return -ENOTTY;
    }
}

// bindind to file operations
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
    .write = dev_write,
    .unlocked_ioctl = dev_ioctl,
};

// init
static int __init my_init(void) {
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if(major < 0) {
        printk(KERN_ERR "Failed to register device\n");
        return major;
    }
    mutex_init(&device_mutex);
    printk(KERN_INFO "Registered device with major %d\n", major);
    return 0;
}

// exit
static void __exit my_exit(void) {
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "Device unregistered\n");
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
