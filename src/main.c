#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/__assert.h>
#include <stdlib.h>
#include <string.h>
#include "seq_parser.h"
#include <ctype.h>
#include "TimeParser.h"

/*1p suoritus: Aikamerkkijono robotista

Edellisessä viikkotehtävässä toteutettiin liikennevaloihin ajastinkeskeytys käyttäen aikamerkkijonosta parseroitua aikaa. Tehdään nyt sama Robotista käsin, eli pusketaan aikamerkkijonoja UARTtiin ja luetaan UARTin kautta robottille palautettu sekuntiluku tai virhekoodit, eli ne mitä edellisen viikkotehtävän parserifunktio palauttaa. 

Robotti sitten tarkistaa, että olihan palautusarvo testikeississä oletettu. Jos siis annetaan virheellinen merkkijono, niin odotetaan vastauksena sitä vastaavaa virhekoodia. Tässä on vapaus muokata virhekoodeja haluamakseen, eli esimerkiksi negatiivisten arvojen sijasta voi käyttää kirjainkoodeja. Mutta katso ohjevideo. 

Testikeissit vaaditaan sekä oikeelliselle että virheellisille merkkijonolle. 

Esimerkki: 
Oikeanlainen merkkijono: 000120 -> saadaan 1 minuutti ja 20 sekuntia -> yhteensä 80. Tämä luku 80 palautetaan UARTin kautta robotille. Robotti tarkistaa skriptissä, että onhan luku oikea. 
Virheellinen merkkijono 001067 -> palautetaan virhekoodi (vaikka -1) robotille (koska sekunteja 67 ei voi olla ). Robotti tarkistaa skriptissä että onhan virhekoodi odotettu -1. 

+1p suoritus: Lisätään aikamerkkijonon testikeissejä

Tehdään Robotin skriptiin lisää testikeissejä sen mukaan mitä yksikkötestaustekniikoita edellisessä viikkotehtävässä 5 käytitte. Hox, tässä voi samalla täydentää edellisen tehtävän vastausta!

Jokaisessa testikeisseissä testaa aina sekä 1) oikeellinen suoritus oikeanlaisella aikamerkkijonolla että 2) virheellinen suoritus väärällä aikamerkkijonolla. Samoin kuin aiemmassa viikkotehtävässä. 

Esimerkkejä:
- tarkista että merkkijono on aina tasan 6 merkkiä pitkä.
- tarkista että merkkijonon palautusarvo ei ole 0 sekuntia (koska tällöin ajastinkeskeytys olisi turha..)
- tarkista että merkkijonossa on vain numeroita, katso tätä varten c-kielen dokumentaatiosta funktio isdigit().
- jne jne
- (aiemmassa tehtävässä tarkistettiin myös onko aikamerkkijono null, mutta tyhjää merkkijonoa ei taida Robotista pystyä lähettämään..)

+1p suoritus: Lisätään sekvenssitestausta

Tehdään Robottiin liikennevalosekvenssien ("RYGRYGRYG"..) testaus, niin että liikennevalo-ohjema tarkistaa, että onhan sekvenssi ok (eli siinä ei ole vääriä merkkejä) ja suorittaa sekvenssin jos kaikki merkit ovat oikein. Jos yksikin merkki on väärin, laite hylkää sekvenssin ja palauttaa robotille virhekoodin. 

Esimerkki: Syötetään UARTTIIN sekvenssi "RYG" -> Robotti testaa että onhan kaikki merkit sallittuja ja suorittaa sekvenssin.

Esimerkki: Syötetään UARTTIIN sekvenssi "RjG" -> Robotti testaa merkkijonon ja huomaa että siinä on väärä merkki j, jolloin palauttaa robotille virhekoodin.  */




// Debug flag - korvaa DEBUG-vakion
static bool debug_enabled = false;

// Debug print makro
#define DEBUG_PRINT(...) do { \
    if (debug_enabled) { \
        printk(__VA_ARGS__); \
    } \
} while (0)

// Led pin configurations
static const struct gpio_dt_spec red   = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

// Button configs
#define BUTTON_0 DT_ALIAS(sw0)
#define BUTTON_1 DT_ALIAS(sw1)
#define BUTTON_2 DT_ALIAS(sw2)
#define BUTTON_3 DT_ALIAS(sw3)
#define BUTTON_4 DT_ALIAS(sw4)

static const struct gpio_dt_spec button_0 = GPIO_DT_SPEC_GET_OR(BUTTON_0, gpios, {0});
static const struct gpio_dt_spec button_1 = GPIO_DT_SPEC_GET_OR(BUTTON_1, gpios, {0});
static const struct gpio_dt_spec button_2 = GPIO_DT_SPEC_GET_OR(BUTTON_2, gpios, {0});
static const struct gpio_dt_spec button_3 = GPIO_DT_SPEC_GET_OR(BUTTON_3, gpios, {0});
static const struct gpio_dt_spec button_4 = GPIO_DT_SPEC_GET_OR(BUTTON_4, gpios, {0});

static struct gpio_callback button_0_data;
static struct gpio_callback button_1_data;
static struct gpio_callback button_2_data;
static struct gpio_callback button_3_data;
static struct gpio_callback button_4_data;

// FIFO puskuri sekvenssille
K_FIFO_DEFINE(seq_fifo);

// Dispatcher–valo synkronointiin
K_MUTEX_DEFINE(disp_mutex);
K_CONDVAR_DEFINE(red_cv);
K_CONDVAR_DEFINE(yellow_cv);
K_CONDVAR_DEFINE(green_cv);

// Release-signaali dispatcherille
K_CONDVAR_DEFINE(release_cv);

//struct seq_item {
    //void *fifo_reserved;  // Zephyr FIFO header
  //  char color;           // 'R', 'Y', 'G'
  //  int duration_ms;      // aika ms
//};

int led_state = 0;         // 0=red, 1=yellow, 2=green, 4=pause, 5=yellow_blink
int prev_led_state = 0;
bool manual_red = false;
bool manual_yellow = false;
bool manual_green = false;
bool yellow_blink_mode = false;

struct seq_item *current_item = NULL;

#define MAX_SEQ_LEN 20
static struct seq_item *seq_buffer[MAX_SEQ_LEN];
static int seq_len = 0;
static uint32_t button_press_count = 0;

#define STACKSIZE 500
#define PRIORITY 5
#define UART_BUF_SIZE 64

void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
void yellow_blink_task(void *, void *, void*);
void uart_rx_task(void *, void *, void*);
void dispatcher_task(void *, void *, void*);

K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(yellow_blink_thread,STACKSIZE,yellow_blink_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(uart_rx_thread, STACKSIZE, uart_rx_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(dispatcher_thread, STACKSIZE, dispatcher_task, NULL, NULL, NULL, PRIORITY, 0, 0);

#define TIME_LEN_ERROR   -1
#define TIME_ZERO_ERROR  -2
#define TIME_CHAR_ERROR  -3
#define TIME_HOUR_ERROR  -4
#define TIME_MIN_ERROR   -5
#define TIME_SEC_ERROR   -6
#define SEQ_INVALID_CHAR_ERROR  -7
#define SEQ_EMPTY_ERROR         -8
#define SEQ_TOO_LONG_ERROR      -9


// ************* Button interrupt handlers *************

// Button 1 - Pause 
void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    __ASSERT(dev != NULL, "Device pointer is NULL in button handler");
    __ASSERT(cb != NULL, "Callback pointer is NULL in button handler");
    __ASSERT(pins != 0, "No pins specified in button interrupt");
    
    // Assertti led_state arvojen tarkistukseen
    __ASSERT(led_state >= 0 && led_state <= 5, "Invalid led_state: %d", led_state);
    
    button_press_count++;
    DEBUG_PRINT("Button pressed! Total count = %u\n", button_press_count);   
    DEBUG_PRINT("Button 1 pressed - Pause\n");
    
    if (led_state == 4) {
        // Tarkista että prev_led_state on validi
        __ASSERT(prev_led_state >= 0 && prev_led_state <= 5 && prev_led_state != 4, 
                "Invalid prev_led_state: %d", prev_led_state);
        led_state = prev_led_state;
        DEBUG_PRINT("Pause OFF, return to state %d\n", led_state);
    } else {
        prev_led_state = led_state;
        led_state = 4;
        DEBUG_PRINT("Pause ON\n");
    }
}

// Button 2 - pusketaan 'R' FIFO:iin
void button_1_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    __ASSERT(dev != NULL, "Device pointer is NULL in button handler");
    __ASSERT(cb != NULL, "Callback pointer is NULL in button handler");
    
    button_press_count++;
    DEBUG_PRINT("Button pressed! Total count = %u\n", button_press_count);
    
    struct seq_item *it = k_malloc(sizeof(*it));
    __ASSERT(it != NULL, "Failed to allocate memory for seq_item");
    
    if (it) {
        it->color = 'R';
        // Assertti että duration on järkevä (asetetaan default-arvo)
        it->duration_ms = 1000; // Default duration button presses
        __ASSERT(it->duration_ms > 0 && it->duration_ms < 60000, 
                "Invalid duration: %d ms", it->duration_ms);
        
        k_fifo_put(&seq_fifo, it);
        DEBUG_PRINT("Button 2 pressed - queued R\n");
    } else {
        DEBUG_PRINT("Button 2: malloc failed\n");
    }
}

// Button 3 - pusketaan 'Y'
void button_2_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    __ASSERT(dev != NULL, "Device pointer is NULL in button handler");
    __ASSERT(cb != NULL, "Callback pointer is NULL in button handler");
    
    button_press_count++;
    DEBUG_PRINT("Button pressed! Total count = %u\n", button_press_count);
    
    struct seq_item *it = k_malloc(sizeof(*it));
    __ASSERT(it != NULL, "Failed to allocate memory for seq_item");
    
    if (it) {
        it->color = 'Y';
        it->duration_ms = 1000;
        __ASSERT(it->duration_ms > 0 && it->duration_ms < 60000, 
                "Invalid duration: %d ms", it->duration_ms);
        
        k_fifo_put(&seq_fifo, it);
        DEBUG_PRINT("Button 3 pressed - queued Y\n");
    } else {
        DEBUG_PRINT("Button 3: malloc failed\n");
    }
}

// Button 4 - pusketaan 'G'
void button_3_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    __ASSERT(dev != NULL, "Device pointer is NULL in button handler");
    __ASSERT(cb != NULL, "Callback pointer is NULL in button handler");
    
    button_press_count++;
    DEBUG_PRINT("Button pressed! Total count = %u\n", button_press_count);
    
    struct seq_item *it = k_malloc(sizeof(*it));
    __ASSERT(it != NULL, "Failed to allocate memory for seq_item");
    
    if (it) {
        it->color = 'G';
        it->duration_ms = 1000;
        __ASSERT(it->duration_ms > 0 && it->duration_ms < 60000, 
                "Invalid duration: %d ms", it->duration_ms);
        
        k_fifo_put(&seq_fifo, it);
        DEBUG_PRINT("Button 4 pressed - queued G\n");
    } else {
        DEBUG_PRINT("Button 4: malloc failed\n");
    }
}

// Button 5 - yellow blink mode
void button_4_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    __ASSERT(dev != NULL, "Device pointer is NULL in button handler");
    __ASSERT(cb != NULL, "Callback pointer is NULL in button handler");
    __ASSERT(led_state >= 0 && led_state <= 5, "Invalid led_state: %d", led_state);
    
    button_press_count++;
    DEBUG_PRINT("Button pressed! Total count = %u\n", button_press_count);
    
    if (!yellow_blink_mode) {
        prev_led_state = led_state;
        led_state = 5;
        yellow_blink_mode = true;
        DEBUG_PRINT("Button 5 pressed - Yellow blink mode ON\n");
    } else {
        yellow_blink_mode = false;
        __ASSERT(prev_led_state >= 0 && prev_led_state <= 5, 
                "Invalid prev_led_state: %d", prev_led_state);
        led_state = prev_led_state;
        
        // Assertit GPIO-operaatioille
        int ret = gpio_pin_set_dt(&red, 0);
        __ASSERT(ret == 0, "Failed to turn off red LED: %d", ret);
        ret = gpio_pin_set_dt(&green, 0);
        __ASSERT(ret == 0, "Failed to turn off green LED: %d", ret);
        
        DEBUG_PRINT("Button 5 pressed - Yellow blink mode OFF\n");
    }
}

// ************* Init functions *************
int init_led(void) {
    int ret;
    
    // Assertti LED GPIO-spekseille
    __ASSERT(red.port != NULL, "Red LED port is NULL");
    __ASSERT(green.port != NULL, "Green LED port is NULL");
    
    ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
    __ASSERT(ret == 0, "Red LED init failed: %d", ret);
    if (ret < 0) return ret;
    
    ret = gpio_pin_set_dt(&red, 0);
    __ASSERT(ret == 0, "Failed to set red LED initial state: %d", ret);

    ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
    __ASSERT(ret == 0, "Green LED init failed: %d", ret);
    if (ret < 0) return ret;
    
    ret = gpio_pin_set_dt(&green, 0);
    __ASSERT(ret == 0, "Failed to set green LED initial state: %d", ret);

    printk("Ledit alustettu ok\n");
    return 0;
}

int init_button(const struct gpio_dt_spec *button, struct gpio_callback *callback,
                gpio_callback_handler_t handler, const char *name) {
    int ret;
    
    // Assertit parametreille
    __ASSERT(button != NULL, "Button spec is NULL for %s", name ? name : "unknown");
    __ASSERT(callback != NULL, "Callback is NULL for %s", name ? name : "unknown");
    __ASSERT(handler != NULL, "Handler is NULL for %s", name ? name : "unknown");
    __ASSERT(name != NULL, "Button name is NULL");
    __ASSERT(button->port != NULL, "Button port is NULL for %s", name);
    
    __ASSERT(gpio_is_ready_dt(button), "GPIO device not ready for %s", name);

    if (!gpio_is_ready_dt(button)) {
        printk("Error: %s not ready\n", name);
        return -1;
    }

    ret = gpio_pin_configure_dt(button, GPIO_INPUT);
    __ASSERT(ret == 0, "Failed to configure %s: %d", name, ret);
    if (ret != 0) return -1;

    ret = gpio_pin_interrupt_configure_dt(button, GPIO_INT_EDGE_TO_ACTIVE);
    __ASSERT(ret == 0, "Failed to configure interrupt for %s: %d", name, ret);
    if (ret != 0) return -1;

    gpio_init_callback(callback, handler, BIT(button->pin));
    
    ret = gpio_add_callback(button->port, callback);
    __ASSERT(ret == 0, "Failed to add callback for %s: %d", name, ret);

    printk("%s OK\n", name);
    return 0;
}

int init_all_buttons(void) {
    if (init_button(&button_0, &button_0_data, button_0_handler, "Button 1") != 0) return -1;
    if (init_button(&button_1, &button_1_data, button_1_handler, "Button 2") != 0) return -1;
    if (init_button(&button_2, &button_2_data, button_2_handler, "Button 3") != 0) return -1;
    if (init_button(&button_3, &button_3_data, button_3_handler, "Button 4") != 0) return -1;
    if (init_button(&button_4, &button_4_data, button_4_handler, "Button 5") != 0) return -1;
    return 0;
}

// ************* Main *************
int main(void)
{
    int rc;
    
    // Assertti alkutilalle
    __ASSERT(seq_len == 0, "Initial seq_len should be 0, got %d", seq_len);
    __ASSERT(button_press_count == 0, "Initial button_press_count should be 0");
    __ASSERT(current_item == NULL, "Initial current_item should be NULL");
    
    rc = init_led();
    __ASSERT(rc == 0, "LED initialization failed with code %d", rc);
    if (rc) {
        printk("LED init failed %d\n", rc);
    }
    
    rc = init_all_buttons();
    __ASSERT(rc == 0, "Button initialization failed with code %d", rc);
    if (rc) {
        printk("Button init failed %d\n", rc);
    }

    printk("App started\n");
    return 0;
}

// ************* Tasks *************
void red_led_task(void *p1, void *p2, void *p3) {
    while (1) {
        int ret = k_mutex_lock(&disp_mutex, K_FOREVER);
        __ASSERT(ret == 0, "Failed to lock mutex in red_led_task: %d", ret);
        
        k_condvar_wait(&red_cv, &disp_mutex, K_FOREVER);

        // Assertti että current_item on asetettu
        __ASSERT(current_item != NULL, "current_item is NULL in red_led_task");
        __ASSERT(current_item->color == 'R', "Wrong color in red_led_task: %c", current_item->color);
        __ASSERT(current_item->duration_ms > 0 && current_item->duration_ms < 60000, 
                "Invalid duration in red_led_task: %d ms", current_item->duration_ms);
        
        struct seq_item *item = current_item;
        k_mutex_unlock(&disp_mutex);

        ret = gpio_pin_set_dt(&red, 1);
        __ASSERT(ret == 0, "Failed to turn on red LED: %d", ret);
        
        DEBUG_PRINT("RED ON for %d ms\n", item->duration_ms);
        k_sleep(K_MSEC(item->duration_ms));
        
        ret = gpio_pin_set_dt(&red, 0);
        __ASSERT(ret == 0, "Failed to turn off red LED: %d", ret);
        DEBUG_PRINT("RED OFF\n");

        k_condvar_signal(&release_cv);
    }
}

void yellow_led_task(void *p1, void *p2, void *p3) {
    while (1) {
        int ret = k_mutex_lock(&disp_mutex, K_FOREVER);
        __ASSERT(ret == 0, "Failed to lock mutex in yellow_led_task: %d", ret);
        
        k_condvar_wait(&yellow_cv, &disp_mutex, K_FOREVER);

        __ASSERT(current_item != NULL, "current_item is NULL in yellow_led_task");
        __ASSERT(current_item->color == 'Y', "Wrong color in yellow_led_task: %c", current_item->color);
        __ASSERT(current_item->duration_ms > 0 && current_item->duration_ms < 60000, 
                "Invalid duration in yellow_led_task: %d ms", current_item->duration_ms);
        
        struct seq_item *item = current_item;
        k_mutex_unlock(&disp_mutex);

        // Sytytetään punainen + vihreä yhtä aikaa = keltainen
        ret = gpio_pin_set_dt(&red, 1);
        __ASSERT(ret == 0, "Failed to turn on red LED for yellow: %d", ret);
        ret = gpio_pin_set_dt(&green, 1);
        __ASSERT(ret == 0, "Failed to turn on green LED for yellow: %d", ret);
        
        DEBUG_PRINT("YELLOW ON for %d ms\n", item->duration_ms);
        k_sleep(K_MSEC(item->duration_ms));

        // Sammutetaan molemmat
        ret = gpio_pin_set_dt(&red, 0);
        __ASSERT(ret == 0, "Failed to turn off red LED for yellow: %d", ret);
        ret = gpio_pin_set_dt(&green, 0);
        __ASSERT(ret == 0, "Failed to turn off green LED for yellow: %d", ret);
        DEBUG_PRINT("YELLOW OFF\n");

        k_condvar_signal(&release_cv);
    }
}

void green_led_task(void *p1, void *p2, void *p3) {
    while (1) {
        int ret = k_mutex_lock(&disp_mutex, K_FOREVER);
        __ASSERT(ret == 0, "Failed to lock mutex in green_led_task: %d", ret);
        
        k_condvar_wait(&green_cv, &disp_mutex, K_FOREVER);

        __ASSERT(current_item != NULL, "current_item is NULL in green_led_task");
        __ASSERT(current_item->color == 'G', "Wrong color in green_led_task: %c", current_item->color);
        __ASSERT(current_item->duration_ms > 0 && current_item->duration_ms < 60000, 
                "Invalid duration in green_led_task: %d ms", current_item->duration_ms);
        
        struct seq_item *item = current_item;
        k_mutex_unlock(&disp_mutex);

        ret = gpio_pin_set_dt(&green, 1);
        __ASSERT(ret == 0, "Failed to turn on green LED: %d", ret);
        
        DEBUG_PRINT("GREEN ON for %d ms\n", item->duration_ms);
        k_sleep(K_MSEC(item->duration_ms));
        
        ret = gpio_pin_set_dt(&green, 0);
        __ASSERT(ret == 0, "Failed to turn off green LED: %d", ret);
        DEBUG_PRINT("GREEN OFF\n");

        k_condvar_signal(&release_cv);
    }
}

void yellow_blink_task(void *p1, void *p2, void *p3) {
    while (true) {
        // Assertti led_state konsistenssin tarkistukseen
        if (led_state == 5) {
            __ASSERT(yellow_blink_mode == true, 
                    "led_state is 5 but yellow_blink_mode is false");
        }
        
        if (led_state == 5 && yellow_blink_mode) {
            int ret = gpio_pin_set_dt(&red, 1);
            __ASSERT(ret == 0, "Failed to turn on red LED in blink: %d", ret);
            ret = gpio_pin_set_dt(&green, 1);
            __ASSERT(ret == 0, "Failed to turn on green LED in blink: %d", ret);
            
            DEBUG_PRINT("YELLOW BLINK ON\n");
            k_sleep(K_MSEC(500));
            
            ret = gpio_pin_set_dt(&red, 0);
            __ASSERT(ret == 0, "Failed to turn off red LED in blink: %d", ret);
            ret = gpio_pin_set_dt(&green, 0);
            __ASSERT(ret == 0, "Failed to turn off green LED in blink: %d", ret);
            
            DEBUG_PRINT("YELLOW BLINK OFF\n");
            k_sleep(K_MSEC(500));
        }
        k_msleep(100);
    }
}

void uart_rx_task(void *p1, void *p2, void *p3) {
    const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
    __ASSERT(uart_dev != NULL, "UART device is NULL");
    
    char buf[UART_BUF_SIZE];
    int idx = 0;

    if (!device_is_ready(uart_dev)) {
        __ASSERT(false, "UART device not ready");
        printk("UART device not ready\n");
        return;
    }

    while (true) {
        unsigned char c;
        if (uart_poll_in(uart_dev, &c) == 0) {
            if (c == '\r' || c == '\n') {
                __ASSERT(idx < UART_BUF_SIZE, "Buffer index out of bounds: %d", idx);
                buf[idx] = '\0';
                idx = 0;

                // --- Aikaseuranta alkaa ---
                uint32_t start = k_uptime_get_32();

                // Debug-komento käsittely
                if (buf[0] == 'D') {
                    if (strlen(buf) >= 3 && buf[1] == ',') {
                        int val = atoi(&buf[2]);
                        if (val == 0) {
                            debug_enabled = false;
                            printk("Debug OFF\n");
                        } else if (val == 1) {
                            debug_enabled = true;
                            printk("Debug ON\n");
                        } else {
                            printk("Invalid debug value: %d (use 0 or 1)\n", val);
                        }
                    } else {
                        printk("Invalid debug command format. Use D,0 or D,1\n");
                    }
                    continue;
                }

char cmd;
int val;

// --- SEQUENCE TEST MODE ---
// Jos ensimmäinen merkki on R, Y tai G, käsitellään sekvenssinä
if (buf[0] == 'R' || buf[0] == 'Y' || buf[0] == 'G') {
    struct seq_item items[16];
    int count = 0;
    int ret = parse_sequence(buf, items, 16, &count);
    printk("%dX", ret);
    continue;
}

// --- TIME PARSER TEST MODE ---
int len = strlen(buf);

// --- Tyhjä syöte ---
if (len == 0) {
    printk("%dX", SEQ_EMPTY_ERROR);  // -8
    continue;
}

// --- Sekvenssit (alkaa R/Y/G) ---
if (buf[0] == 'R' || buf[0] == 'Y' || buf[0] == 'G') {

    // Liian pitkä sekvenssi
    if (len > 20) {
        printk("%dX", SEQ_TOO_LONG_ERROR);  // -9
        continue;
    }

    struct seq_item items[16];
    int count = 0;
    int ret = parse_sequence(buf, items, 16, &count);
    printk("%dX", ret);
    continue;
}

// --- Aikaparseri (vain jos alkaa numerolla) ---
if (isdigit((unsigned char)buf[0])) {

    // Jos sisältää kirjaimia tai muuta → virhe heti
    bool all_digits = true;
    for (int i = 0; i < len; i++) {
        if (!isdigit((unsigned char)buf[i])) {
            all_digits = false;
            break;
        }
    }
    if (!all_digits) {
        printk("%dX", TIME_CHAR_ERROR);  // -3
        continue;
    }

    // Liian lyhyt tai pitkä → virhe ennen nollatarkistusta
    if (len != 6) {
        printk("%dX", TIME_LEN_ERROR);  // -1
        continue;
    }

    // Nolla-aika
    if (strcmp(buf, "000000") == 0) {
        printk("%dX", TIME_ZERO_ERROR);  // -2
        continue;
    }

    // Jos kaikki OK → parsitaan
    int result = time_parse(buf);
    printk("%dX", result);
    continue;
}

// --- Jos mikään yllä ei täsmää ---
if (len > 20) {
    printk("%dX", SEQ_TOO_LONG_ERROR);  // -9
    continue;
}

printk("%dX", SEQ_INVALID_CHAR_ERROR);  // -7
continue;


if (sscanf(buf, "%c,%d", &cmd, &val) >= 1) {

    // S = sekvenssin parserointi ---
    if (cmd == 'S') {
    struct seq_item items[16];
    int count = 0;
    int ret = parse_sequence(buf + 2, items, 16, &count);

    if (ret == 0) {
        for (int i = 0; i < count; i++) {
            struct seq_item *copy = k_malloc(sizeof(*copy));
            if (copy) {
                *copy = items[i];
                k_fifo_put(&seq_fifo, copy);
            }
        }
        printk("0X");  //   Sekvenssi: Robotti odottaa '0X'
    } else {
        printk("%dX", ret);  //  Tulostetaan virhekoodi: esim. -7X, -8X, -9X
    }

    continue;  // ei jatketa muihin käsittelyihin
}
    // --- TAVALLINEN KOMENTO ---
    if (sscanf(buf, "%c,%d", &cmd, &val) == 2) {

        // Tarkistetaan että komento on sallittu
        __ASSERT(cmd == 'R' || cmd == 'Y' || cmd == 'G' || cmd == 'T',
                 "Invalid command: %c", cmd);
        __ASSERT(val > 0 && val < 60000, "Invalid value: %d", val);

        if (cmd == 'T') {
            DEBUG_PRINT("RX: Repeat sequence %d times\n", val);
            __ASSERT(seq_len <= MAX_SEQ_LEN, "seq_len overflow: %d", seq_len);

            for (int r = 0; r < val; r++) {
                __ASSERT(r < 100, "Too many repetitions: %d", r);

                for (int i = 0; i < seq_len; i++) {
                    __ASSERT(i < MAX_SEQ_LEN, "Sequence index out of bounds: %d", i);
                    __ASSERT(seq_buffer[i] != NULL, "seq_buffer[%d] is NULL", i);

                    struct seq_item *orig = seq_buffer[i];
                    struct seq_item *copy = k_malloc(sizeof(*copy));
                    __ASSERT(copy != NULL, "Failed to allocate memory for copy");

                    *copy = *orig;
                    k_fifo_put(&seq_fifo, copy);
                }
            }
        } else {
            struct seq_item *item = k_malloc(sizeof(*item));
            __ASSERT(item != NULL, "Failed to allocate memory for new item");
            item->color = cmd;
            item->duration_ms = val;
            k_fifo_put(&seq_fifo, item);
            DEBUG_PRINT("RX: %c for %d ms queued\n", cmd, val);
        }
    }
}


                // --- Aikaseuranta loppuu ---
                uint32_t end = k_uptime_get_32();
                __ASSERT(end >= start, "Time measurement error: end < start");
                DEBUG_PRINT("UART command handled in %u ms\n", end - start);

            } else {
                if (idx < UART_BUF_SIZE - 1) {
                    buf[idx++] = c;
                } else {
                    __ASSERT(false, "UART buffer overflow, idx: %d", idx);
                }
            }
        }
        k_msleep(10);
    }
}

void dispatcher_task(void *p1, void *p2, void *p3) {
    while (1) {
        struct seq_item *item = k_fifo_get(&seq_fifo, K_FOREVER);
        __ASSERT(item != NULL, "Received NULL item from FIFO");
        
        if (!item) continue;

        // Assertit item-sisällölle
        __ASSERT(item->color == 'R' || item->color == 'Y' || item->color == 'G',
                "Invalid color in dispatcher: %c", item->color);
        __ASSERT(item->duration_ms > 0 && item->duration_ms < 60000,
                "Invalid duration in dispatcher: %d ms", item->duration_ms);

        
        int ret = k_mutex_lock(&disp_mutex, K_FOREVER);
        __ASSERT(ret == 0, "Failed to lock mutex in dispatcher: %d", ret);
        
        __ASSERT(current_item == NULL, "current_item not NULL when setting new item");
        current_item = item;

        switch (item->color) {
            case 'R': k_condvar_signal(&red_cv);    break;
            case 'Y': k_condvar_signal(&yellow_cv); break;
            case 'G': k_condvar_signal(&green_cv);  break;
            default:
                __ASSERT(false, "Invalid color in switch: %c", item->color);
        }

        k_condvar_wait(&release_cv, &disp_mutex, K_FOREVER);
        
        __ASSERT(current_item == item, "current_item changed unexpectedly");
        current_item = NULL;
        k_mutex_unlock(&disp_mutex);

        k_free(item);
    }
}