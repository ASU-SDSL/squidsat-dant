
// #include "radio.h"
// #include "hal/STMRadioHal.h"
// #include <zephyr/drivers/gpio.h>
// #include <zephyr/drivers/spi.h>

// // Radio instances
// static SX1268 radio_sx = nullptr;
// static RFM98 radio_rfm = nullptr;
// static STMRadioHal* hal_sx = nullptr;
// static STMRadioHal* hal_rfm = nullptr;

// // Current active radio state
// static uint8_t current_radio = 0; // 0 = SX1268, 1 = RFM98
// static bool sx_initialized = false;
// static bool rfm_initialized = false;

// // State tracking
// static int16_t rfm_state = RADIOLIB_ERR_NONE;
// static int16_t sx_state = RADIOLIB_ERR_NONE;

// // Message queue for radio operations
// K_MSGQ_DEFINE(radio_msgq, sizeof(radio_queue_operations_t), RADIO_MAX_QUEUE_ITEMS, 4);

// // Device pointers (to be initialized)
// static const struct device* gpio_dev = nullptr;
// static const struct device* spi_dev = nullptr;

// // Forward declarations
// static void configure_radio_pins();
// static void set_rf_switch(uint8_t radio);
// static int16_t transmit_packet(uint8_t* data, size_t size);
// static int16_t get_packet_stats(int16_t* rssi, int8_t* snr);

// void init_radio() {
//     // Get GPIO and SPI devices from device tree
//     gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpioa));
//     if (!device_is_ready(gpio_dev)) {
//         LOG_ERR("GPIO device not ready");
//         return;
//     }

//     spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi1));
//     if (!device_is_ready(spi_dev)) {
//         LOG_ERR("SPI device not ready");
//         return;
//     }

//     // Initialize HAL instances
//     hal_sx = new STMRadioHal(spi_dev, gpio_dev, 2000000);
//     hal_rfm = new STMRadioHal(spi_dev, gpio_dev, 2000000);

//     // Configure radio pins
//     configure_radio_pins();

//     // Initialize SX1268 radio
//     radio_sx = new Module(RADIO_SX_NSS_PIN, RADIO_SX_DIO1_PIN, RADIO_SX_NRST_PIN, RADIO_SX_BUSY_PIN, *hal_sx);
//     sx_state = radio_sx.beginFSK();
//     if (sx_state == RADIOLIB_ERR_NONE) {
//         sx_initialized = true;
//         LOG_INF("SX1268 initialized successfully");
//     } else {
//         LOG_ERR("SX1268 initialization failed: %d", sx_state);
//     }

//     // Initialize RFM98 radio
//     radio_rfm = new Module(RADIO_RFM_NSS_PIN, RADIO_RFM_DIO0_PIN, RADIO_RFM_NRST_PIN, RADIO_RFM_DIO1_PIN, *hal_rfm);
//     rfm_state = radio_rfm.beginLoRa();
//     if (rfm_state == RADIOLIB_ERR_NONE) {
//         rfm_initialized = true;
//         LOG_INF("RFM98 initialized successfully");
//     } else {
//         LOG_ERR("RFM98 initialization failed: %d", rfm_state);
//     }

//     // Set default configuration
//     if (sx_initialized) {
//         radio_sx.setFrequency(RADIO_FREQ);
//         radio_sx.setBandwidth(RADIO_BW);
//         radio_sx.setSpreadingFactor(RADIO_SF);
//         radio_sx.setCodingRate(RADIO_CR);
//         radio_sx.setSyncWord(RADIO_SYNC_WORD);
//         radio_sx.setPreambleLength(RADIO_PREAMBLE_LEN);
//     }

//     if (rfm_initialized) {
//         radio_rfm.setFrequency(RADIO_FREQ);
//         radio_rfm.setBandwidth(RADIO_BW);
//         radio_rfm.setSpreadingFactor(RADIO_SF);
//         radio_rfm.setCodingRate(RADIO_CR);
//         radio_rfm.setSyncWord(RADIO_SYNC_WORD);
//         radio_rfm.setPreambleLength(RADIO_PREAMBLE_LEN);
//         radio_rfm.setOutputPower(RADIO_RFM_GAIN);
//     }

//     LOG_INF("Radio initialization complete");
// }

// static void configure_radio_pins() {
//     // Configure NSS pins as output
//     gpio_pin_configure(gpio_dev, RADIO_SX_NSS_PIN, GPIO_OUTPUT);
//     gpio_pin_configure(gpio_dev, RADIO_RFM_NSS_PIN, GPIO_OUTPUT);

//     // Configure RF switch control pin
//     gpio_pin_configure(gpio_dev, RADIO_RF_SWITCH_PIN, GPIO_OUTPUT);

//     // Configure power enable pins
//     gpio_pin_configure(gpio_dev, RADIO_SX_POWER_PIN, GPIO_OUTPUT);
//     gpio_pin_configure(gpio_dev, RADIO_RFM_POWER_PIN, GPIO_OUTPUT);

//     // Set initial states
//     gpio_pin_set(gpio_dev, RADIO_SX_NSS_PIN, 1);
//     gpio_pin_set(gpio_dev, RADIO_RFM_NSS_PIN, 1);
//     gpio_pin_set(gpio_dev, RADIO_SX_POWER_PIN, 0);
//     gpio_pin_set(gpio_dev, RADIO_RFM_POWER_PIN, 0);
// }

// static void set_rf_switch(uint8_t radio) {
//     gpio_pin_set(gpio_dev, RADIO_RF_SWITCH_PIN, radio == 0 ? RADIO_RF_SWITCH_SX : RADIO_RF_SWITCH_RFM);
// }

// void radio_task_cpp() {
//     radio_queue_operations_t op;

//     while (1) {
//         // Wait for operation from queue
//         k_msgq_get(&radio_msgq, &op, K_FOREVER);

//         switch (op.operation_type) {
//             case TRANSMIT:
//                 if (current_radio == 0 && sx_initialized) {
//                     set_rf_switch(0);
//                     sx_state = transmit_packet(op.data_buffer, op.data_size);
//                 } else if (current_radio == 1 && rfm_initialized) {
//                     set_rf_switch(1);
//                     rfm_state = transmit_packet(op.data_buffer, op.data_size);
//                 }
//                 break;

//             case SET_OUTPUT_POWER:
//                 if (current_radio == 0 && sx_initialized) {
//                     sx_state = radio_sx.setOutputPower(op.data_size); // data_size used as power value
//                 } else if (current_radio == 1 && rfm_initialized) {
//                     // RFM98 has +14dBm offset
//                     rfm_state = radio_rfm.setOutputPower(op.data_size - 14);
//                 }
//                 break;

//             case ENABLE_RFM98:
//                 if (rfm_initialized) {
//                     current_radio = 1;
//                     set_rf_switch(1);
//                     LOG_INF("Switched to RFM98 radio");
//                 }
//                 break;

//             case ENABLE_SX1268:
//                 if (sx_initialized) {
//                     current_radio = 0;
//                     set_rf_switch(0);
//                     LOG_INF("Switched to SX1268 radio");
//                 }
//                 break;

//             case RETURN_STATS:
//                 {
//                     int16_t rssi = 0;
//                     int8_t snr = 0;
//                     if (current_radio == 0 && sx_initialized) {
//                         sx_state = get_packet_stats(&rssi, &snr);
//                         if (sx_state == RADIOLIB_ERR_NONE) {
//                             telemetry_send_radio_stats(rssi, snr, 0);
//                         }
//                     } else if (current_radio == 1 && rfm_initialized) {
//                         rfm_state = get_packet_stats(&rssi, &snr);
//                         if (rfm_state == RADIOLIB_ERR_NONE) {
//                             telemetry_send_radio_stats(rssi, snr, 1);
//                         }
//                     }
//                 }
//                 break;

//             default:
//                 LOG_WRN("Unknown operation type: %d", op.operation_type);
//                 break;
//         }

//         // Free data buffer if allocated
//         if (op.data_buffer != nullptr) {
//             k_free(op.data_buffer);
//         }
//     }
// }

// void radio_task(void *unused_arg) {
//     ARG_UNUSED(unused_arg);
//     init_radio();
//     radio_task_cpp();
// }

// static int16_t transmit_packet(uint8_t* data, size_t size) {
//     if (current_radio == 0) {
//         return radio_sx.transmit(data, size);
//     } else {
//         return radio_rfm.transmit(data, size);
//     }
// }

// static int16_t get_packet_stats(int16_t* rssi, int8_t* snr) {
//     if (current_radio == 0) {
//         *rssi = radio_sx.getRSSI();
//         *snr = radio_sx.getSNR();
//         return radio_sx.getPacketLength();
//     } else {
//         *rssi = radio_rfm.getRSSI();
//         *snr = radio_rfm.getSNR();
//         return radio_rfm.getPacketLength();
//     }
// }

// void radio_queue_message(char *buffer, size_t size) {
//     radio_queue_operations_t op;
//     op.operation_type = TRANSMIT;
//     op.data_buffer = (uint8_t*)k_malloc(size);
//     if (op.data_buffer == nullptr) {
//         LOG_ERR("Failed to allocate memory for radio message");
//         return;
//     }
//     memcpy(op.data_buffer, buffer, size);
//     op.data_size = size;

//     k_msgq_put(&radio_msgq, &op, K_NO_WAIT);
// }

// void radio_set_transmit_power(uint8_t output_power) {
//     radio_queue_operations_t op;
//     op.operation_type = SET_OUTPUT_POWER;
//     op.data_buffer = nullptr;
//     op.data_size = output_power; // Reusing data_size field for power value

//     k_msgq_put(&radio_msgq, &op, K_NO_WAIT);
// }

// void radio_set_module(radio_operation_type_t op) {
//     radio_queue_operations_t queue_op;
//     queue_op.operation_type = op;
//     queue_op.data_buffer = nullptr;
//     queue_op.data_size = 0;

//     k_msgq_put(&radio_msgq, &queue_op, K_NO_WAIT);
// }

// void radio_queue_stat_response() {
//     radio_queue_operations_t op;
//     op.operation_type = RETURN_STATS;
//     op.data_buffer = nullptr;
//     op.data_size = 0;

//     k_msgq_put(&radio_msgq, &op, K_NO_WAIT);
// }

// uint8_t radio_which() {
//     return current_radio;
// }

// int16_t radio_get_RFM_state() {
//     return rfm_state;
// }

// int16_t radio_get_SX_state() {
//     return sx_state;
// }
