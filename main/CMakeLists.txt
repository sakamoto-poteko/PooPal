set(COMPONENT_SRCS
    "main.c"
    "global.h"
    "status.c"
    "status.h"
    "tasks.h"
    "tasks.c"
    "datalink.h"
    "datalink.c"
    "bodydetection.h"
    "bodydetection.c"
    "devicecontrollogic.h"
    "devicecontrollogic.c"
    "wifi.h"
    "wifi.c"
    "led.h"
    "led.c"
    "timeman.h"
    "timeman.c"
    "aziot.h"
    "aziot.c"
    "creddef.h"
    )
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()
