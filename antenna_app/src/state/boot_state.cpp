#include <boot_state.h>
#include <state_system.h>
#include <radio.h>

static struct k_thread radio_task_thread;

bool check_spi(){
    if(!device_is_ready(device_config.spi_port)){
        return false;
    }
    //return true
}

bool check_gpio(){
    if(!device_is_ready(device_config.gpio_port)){
        return false;
    }
    //return true
}

bool check_gpio_cs(){
    if(!device_is_ready(device_config.gpio_cs_port)){
        return false;
    }
    //return true
}

bool check_can(){
    if(!device_is_ready(device_config.can_bus)){
        return false;
    }
    //return true
}

//Might not need this function. Call directly to init_radio() from main.cpp instead of this function
int check_init_radio(){
    if(init_radio() != 0){
        return 1;
    }
    return 0;
}

bool check_radio_thread_once(){
    if(radio_task_thread){//placeholder value 
        k_thread_join(&radio_thread, K_MSEC(1000)); //suspend the thread
    }
    //create a new thread
    k_thread_create(
    &radio_task_thread,
    radio_task_stack,
    K_THREAD_STACK_SIZEOF(radio_task_stack),
    radio_queue_entry,
    nullptr, nullptr, nullptr,
    RADIO_TASK_PRIORITY,
    0,
    K_NO_WAIT
);

    return true
}

