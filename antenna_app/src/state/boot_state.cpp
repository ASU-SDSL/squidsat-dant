#include <boot_state.h>
#include <state_system.h>
#include <radio.h>


bool check_spi(){
    if(!device_is_ready(device_config.spi_port)){
        return false;
    }
}

bool check_gpio(){
    if(!device_is_ready(device_config.gpio_port)){
        return false;
    }
}

bool check_gpio_cs(){
    if(!device_is_ready(device_config.gpio_cs_port)){
        return false;
    }
}

bool check_CAN(){
    if(!device_is_ready(device_config.can_bus)){
        return false;
    }
}

bool check_radio_init(){

}

bool check_radio_thread(){

}

