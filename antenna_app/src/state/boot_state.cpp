#include <boot_state.h>
#include <state_system.h>
#include <radio.h>


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

bool init_radio(){
    //Might not need this function
}

bool check_radio_thread_once(){
    if(radio_thread_running){//placeholder value
        k_thread_join(&radio_thread, K_MSEC(1000)); //suspend the thread
    }
    k_tid_t new_radio_thread = k_thread_create(&radio_thread, radio_stack, 
                                               K_THREAD_SIZE_OF(radio_stack), 
                                               radio_thread_entry, NULL, NULL, NULL, 
                                               RADIO_PRIORITY,0,K_NO_WAIT);
                                               //create a new thread
    return true
}

