#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

// Led pin configurations
static const struct gpio_dt_spec red   = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

// Button config
#define BUTTON_0 DT_ALIAS(sw0)
static const struct gpio_dt_spec button_0 = GPIO_DT_SPEC_GET_OR(BUTTON_0, gpios, {0});
static struct gpio_callback button_0_data;

// Tilat
int led_state = 0;         // 0=red, 1=yellow, 2=green, 4=pause
int prev_led_state = 0;    // talletettu tila ennen pausea

#define STACKSIZE 500
#define PRIORITY 5

void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);

K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);

// ************* Button interrupt handler *************
void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    printk("Button pressed\n");
    if (led_state == 4) {
        // palautetaan tila
        led_state = prev_led_state;
        printk("Pause OFF, return to state %d\n", led_state);
    } else {
        // otetaan talteen ja siirrytään pauselle
        prev_led_state = led_state;
        led_state = 4;
        printk("Pause ON\n");
    }
}

// ************* Init functions *************
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

int init_button(void) {
    int ret;

    if (!gpio_is_ready_dt(&button_0)) {
        printk("Error: button not ready\n");
        return -1;
    }

    ret = gpio_pin_configure_dt(&button_0, GPIO_INPUT);
    if (ret != 0) return -1;

    ret = gpio_pin_interrupt_configure_dt(&button_0, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) return -1;

    gpio_init_callback(&button_0_data, button_0_handler, BIT(button_0.pin));
    gpio_add_callback(button_0.port, &button_0_data);

    printk("Button OK\n");
    return 0;
}

// ************* Main *************
int main(void)
{
    init_led();
    init_button();
    return 0;
}

// ************* Tasks *************
void red_led_task(void *, void *, void*) {
    while (true) {
        if (led_state == 0) {
            gpio_pin_set_dt(&red, 1);
            printk("RED ON\n");
            k_sleep(K_SECONDS(1));
            gpio_pin_set_dt(&red, 0);
            printk("RED OFF\n");
            if (led_state != 4) {
                led_state = 1;   // seuraava tila keltainen
            }
        }
        k_msleep(100);
    }
}

void yellow_led_task(void *, void *, void*) {
    while (true) {
        if (led_state == 1) {
            gpio_pin_set_dt(&red, 1);
            gpio_pin_set_dt(&green, 1);
            printk("YELLOW ON\n");
            k_sleep(K_SECONDS(1));
            gpio_pin_set_dt(&red, 0);
            gpio_pin_set_dt(&green, 0);
            printk("YELLOW OFF\n");
            // Tarkista pause-tila ennen siirtymistä seuraavaan tilaan
            if (led_state != 4) {
                led_state = 2;   // seuraava tila vihreä
            }
        }
        k_msleep(100);
    }
}

void green_led_task(void *, void *, void*) {
    while (true) {
        if (led_state == 2) {
            gpio_pin_set_dt(&green, 1);
            printk("GREEN ON\n");
            k_sleep(K_SECONDS(1));
            gpio_pin_set_dt(&green, 0);
            printk("GREEN OFF\n");
            // Tarkista pause-tila ennen siirtymistä seuraavaan tilaan
            if (led_state != 4) {
                led_state = 0;   // seuraava tila punainen
            }
        }
        k_msleep(100);
    }
}