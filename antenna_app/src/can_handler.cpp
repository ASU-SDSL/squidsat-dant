#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

CAN_MSGQ_DEFINE(can_rx_queue, 4);

static void print_can_state(const struct device *dev)
{
    enum can_state state;
    struct can_bus_err_cnt errors;
    const int ret = can_get_state(dev, &state, &errors);

    if (ret == 0) {
        printk("CAN state=%d tx_errors=%u rx_errors=%u\n",
               (int)state, errors.tx_err_cnt, errors.rx_err_cnt);
    }
}

static void can_task_entry(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    const struct device *const can_dev =
        DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

    if (!device_is_ready(can_dev)) {
        printk("CAN device is not ready\n");
        return;
    }

    int ret = can_set_mode(can_dev, CAN_MODE_NORMAL);
    if (ret != 0) {
        printk("CAN mode failed: %d\n", ret);
        return;
    }

    const struct can_filter filter = {
        .id = 0x123,
        .mask = CAN_STD_ID_MASK,
        .flags = 0U,
    };
    const int filter_id = can_add_rx_filter_msgq(
        can_dev, &can_rx_queue, &filter);
    if (filter_id < 0) {
        printk("CAN filter registration failed: %d\n", filter_id);
        return;
    }

    printk("Node %s: RX filter installed for ID 0x123\n",
           IS_ENABLED(CONFIG_DANT_NODE_A) ? "A" : "B");

    ret = can_start(can_dev);
    if (ret != 0) {
        printk("CAN start failed: %d\n", ret);
        return;
    }

    printk("CAN started\n");

    struct can_frame frame = {};

    if (IS_ENABLED(CONFIG_DANT_NODE_A)) {
        frame.id = 0x123;
        frame.dlc = 1;
        frame.data[0] = 42;

        while (true) {
            printk("CAN task alive\n");
            ret = can_send(can_dev, &frame, K_MSEC(100), nullptr, nullptr);
            if (ret != 0) {
                printk("CAN send failed: %d\n", ret);
                print_can_state(can_dev);
            } else {
                printk("CAN frame sent\n");
            }
            k_msleep(1000);
        }
    } else {
        while (true) {
            ret = k_msgq_get(&can_rx_queue, &frame, K_FOREVER);
            if (ret == 0 && frame.dlc >= 1U &&
                (frame.flags & CAN_FRAME_RTR) == 0U) {
                printk("Received ID: 0x%x, data: %u\n",
                       (unsigned int)frame.id,
                       (unsigned int)frame.data[0]);
            }
        }
    }
}

K_THREAD_DEFINE(can_task_id, 1024, can_task_entry,
                nullptr, nullptr, nullptr,
                5, 0, 0);
