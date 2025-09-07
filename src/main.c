#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

// Led pin configurations
static const struct gpio_dt_spec red   = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

// Global variable (tila)
int led_state = 0;   // 0=punainen, 1=keltaisen vuoro, 2=vihreä

// Thread määritykset
#define STACKSIZE 500
#define PRIORITY 5

void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);

K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);

// Ledien alustukset
int init_led(void) {
    int ret;

    ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) return ret;
    gpio_pin_set_dt(&red, 0);

    ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) return ret;
    gpio_pin_set_dt(&green, 0);

    printk("Ledit alustettu ok\n");
    return 0;
}

int main(void)
{
    init_led();
    return 0;
}

// Punainen taski
void red_led_task(void *, void *, void*) {
    while (true) {
        if (led_state == 0) {
            gpio_pin_set_dt(&red, 1);
            printk("RED ON\n");
            k_sleep(K_SECONDS(1));
            gpio_pin_set_dt(&red, 0);
            printk("RED OFF\n");
            led_state = 1;   // seuraava tila = keltainen
        }
        k_msleep(100);
    }
}

// Keltainen taski
void yellow_led_task(void *, void *, void*) {
    while (true) {
        if (led_state == 1) {
            // keltainen = punainen + vihreä
            gpio_pin_set_dt(&red, 1);
            gpio_pin_set_dt(&green, 1);
            printk("YELLOW ON\n");
            k_sleep(K_SECONDS(1));
            gpio_pin_set_dt(&red, 0);
            gpio_pin_set_dt(&green, 0);
            printk("YELLOW OFF\n");
            led_state = 2;   // seuraava tila = vihreä
        }
        k_msleep(100);
    }
}

// Vihreä taski
void green_led_task(void *, void *, void*) {
    while (true) {
        if (led_state == 2) {
            gpio_pin_set_dt(&green, 1);
            printk("GREEN ON\n");
            k_sleep(K_SECONDS(1));
            gpio_pin_set_dt(&green, 0);
            printk("GREEN OFF\n");
            led_state = 0;   // seuraava tila = punainen
        }
        k_msleep(100);
    }
}
