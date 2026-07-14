#include "fault_flag.h"

static volatile bool pending = false;

void fault_request(){
    pending = true;
}

bool fault_take(){
    if(!pending){
        return false;
    }
    pending = false;
    return true;

}

