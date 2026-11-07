#include "sd_card.h"
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

#define SD_CMD0_GO_IDLE_STATE            (0U)
#define SD_CMD8_SEND_IF_COND             (8U)
#define SD_ACMD41_SD_SEND_OP_COND        (41U)
#define SD_CMD55_APP_CMD                 (55U)
#define SD_CMD2_ALL_SEND_CID             (2U)
#define SD_CMD3_SEND_RELATIVE_ADDR       (3U)
#define SD_CMD9_SEND_CSD                 (9U)
#define SD_CMD7_SELECT_CARD              (7U)
#define SD_ACMD6_SET_BUS_WIDTH           (6U)
#define SD_CMD6_SWITCH_FUNC              (6U)
#define SD_CMD13_SEND_STATUS             (13U)
#define SD_CMD12_STOP_TRANSMISSION       (12U)
#define SD_CMD17_READ_SINGLE_BLOCK       (17U)
#define SD_CMD18_READ_MULTIPLE_BLOCK     (18U)
#define SD_ACMD23_SET_WR_BLK_ERASE_COUNT (23U)
#define SD_CMD24_WRITE_BLOCK             (24U)
#define SD_CMD25_WRITE_MULTIPLE_BLOCK    (25U)
#define SD_CMD16_SET_BLOCKLEN            (16U)

#define SD_CMD8_VHS_POS                     (8U)
#define SD_CMD8_VHS_MASK                    (0xFU << SD_CMD8_VHS_POS)
#define SD_CMD8_VHS_27_36                   (1U << SD_CMD8_VHS_POS)
#define SD_CMD8_VHS_LOW_VOLTAGE_RANGE       (2U << SD_CMD8_VHS_POS)
#define SD_CMD8_CHECK_PATTERN_POS           (0U)
#define SD_CMD8_CHECK_PATTERN_MASK          (0xFFU << SD_CMD8_CHECK_PATTERN_POS)
#define SD_CMD8_CHECK_PATTERN               (0xAAU << SD_CMD8_CHECK_PATTERN_POS)
#define SD_CMD8_RESP0_VHS_POS               (8U)
#define SD_CMD8_RESP0_VHS_MASK              (0xFU << SD_CMD8_RESP0_VHS_POS)
#define SD_CMD8_RESP0_VHS_27_36             (1U << SD_CMD8_RESP0_VHS_POS)
#define SD_CMD8_RESP0_VHS_LOW_VOLTAGE_RANGE (2U << SD_CMD8_RESP0_VHS_POS)
#define SD_CMD8_RESP0_CHECK_PATTERN_POS     (0U)
#define SD_CMD8_RESP0_CHECK_PATTERN_MASK    (0xFFU << SD_CMD8_RESP0_CHECK_PATTERN_POS)
#define SD_CMD8_RESP0_CHECK_PATTERN         (0xAAU << SD_CMD8_RESP0_CHECK_PATTERN_POS)

#define SD_CMD55_RCA_POS  (16U)
#define SD_CMD55_RCA_MASK (0xFFFFU << SD_CMD55_RCA_POS)

#define SD_ACMD41_HCS_POS                            (30U)
#define SD_ACMD41_HCS_MASK                           (0x1U << SD_ACMD41_HCS_POS)
#define SD_ACMD41_HCS_SDSC                           (0U << SD_ACMD41_HCS_POS)
#define SD_ACMD41_HCS_SDHC_SDXC                      (1U << SD_ACMD41_HCS_POS)
#define SD_ACMD41_XPC_POS                            (28U)
#define SD_ACMD41_XPC_MASK                           (0x1U << SD_ACMD41_XPC_POS)
#define SD_ACMD41_XPC_POWER_SAVING                   (0U << SD_ACMD41_XPC_POS)
#define SD_ACMD41_XPC_MAXIMUM_PERFORMANCE            (1U << SD_ACMD41_XPC_POS)
#define SD_ACMD41_S18R_POS                           (24U)
#define SD_ACMD41_S18R_MASK                          (0x1U << SD_ACMD41_S18R_POS)
#define SD_ACMD41_S18R_USE_CURRENT_SIGNAL_VOLTAGE    (0U << SD_ACMD41_S18R_POS)
#define SD_ACMD41_S18R_SWITCH_TO_18_SIGNAL_VOLTAGE   (1U << SD_ACMD41_S18R_POS)
#define SD_ACMD41_VOLTAGE_WINDOW_POS                 (15U)
#define SD_ACMD41_VOLTAGE_WINDOW_MASK                (0x1FFU << SD_ACMD41_VOLTAGE_WINDOW_POS)
#define SD_ACMD41_VOLTAGE_WINDOW_27_28               (0x1U << SD_ACMD41_VOLTAGE_WINDOW_POS)
#define SD_ACMD41_VOLTAGE_WINDOW_28_29               (0x2U << SD_ACMD41_VOLTAGE_WINDOW_POS)
#define SD_ACMD41_VOLTAGE_WINDOW_29_30               (0x4U << SD_ACMD41_VOLTAGE_WINDOW_POS)
#define SD_ACMD41_VOLTAGE_WINDOW_30_31               (0x8U << SD_ACMD41_VOLTAGE_WINDOW_POS)
#define SD_ACMD41_VOLTAGE_WINDOW_31_32               (0x10U << SD_ACMD41_VOLTAGE_WINDOW_POS)
#define SD_ACMD41_VOLTAGE_WINDOW_32_33               (0x20U << SD_ACMD41_VOLTAGE_WINDOW_POS)
#define SD_ACMD41_VOLTAGE_WINDOW_33_34               (0x40U << SD_ACMD41_VOLTAGE_WINDOW_POS)
#define SD_ACMD41_VOLTAGE_WINDOW_34_35               (0x80U << SD_ACMD41_VOLTAGE_WINDOW_POS)
#define SD_ACMD41_VOLTAGE_WINDOW_35_36               (0x100U << SD_ACMD41_VOLTAGE_WINDOW_POS)
#define SD_ACMD41_VOLTAGE_ALL                        (SD_ACMD41_VOLTAGE_WINDOW_27_28 | \
                                                      SD_ACMD41_VOLTAGE_WINDOW_28_29 | \
                                                      SD_ACMD41_VOLTAGE_WINDOW_29_30 | \
                                                      SD_ACMD41_VOLTAGE_WINDOW_30_31 | \
                                                      SD_ACMD41_VOLTAGE_WINDOW_31_32 | \
                                                      SD_ACMD41_VOLTAGE_WINDOW_32_33 | \
                                                      SD_ACMD41_VOLTAGE_WINDOW_33_34 | \
                                                      SD_ACMD41_VOLTAGE_WINDOW_34_35 | \
                                                      SD_ACMD41_VOLTAGE_WINDOW_35_36)
#define SD_ACMD41_RESP0_BUSY_POS                     (31U)
#define SD_ACMD41_RESP0_BUSY_MASK                    (0x1U << SD_ACMD41_RESP0_BUSY_POS)
#define SD_ACMD41_RESP0_BUSY_ON_INITIALIZATION       (0U << SD_ACMD41_RESP0_BUSY_POS)
#define SD_ACMD41_RESP0_BUSY_INITIALIZATION_COMPLETE (1U << SD_ACMD41_RESP0_BUSY_POS)
#define SD_ACMD41_RESP0_CCS_POS                      (30U)
#define SD_ACMD41_RESP0_CCS_MASK                     (0x1U << SD_ACMD41_RESP0_CCS_POS)
#define SD_ACMD41_RESP0_CCS_SDSC                     (0U << SD_ACMD41_RESP0_CCS_POS)
#define SD_ACMD41_RESP0_CCS_SDHC_SDXC                (1U << SD_ACMD41_RESP0_CCS_POS)

#define SD_CMD3_RCA_POS  (16U)
#define SD_CMD3_RCA_MASK (0xFFFFU << SD_CMD3_RCA_POS)

#define SD_CMD9_RCA_POS  (16U)
#define SD_CMD9_RCA_MASK (0xFFFFU << SD_CMD9_RCA_POS)

#define SD_CMD7_RCA_POS  (16U)
#define SD_CMD7_RCA_MASK (0xFFFFU << SD_CMD7_RCA_POS)

#define SD_ACMD6_BUS_WIDTH_POS  (0U)
#define SD_ACMD6_BUS_WIDTH_MASK (0x3U << SD_ACMD6_BUS_WIDTH_POS)
#define SD_ACMD6_BUS_WIDTH_1B   (0U << SD_ACMD6_BUS_WIDTH_POS)
#define SD_ACMD6_BUS_WIDTH_4B   (2U << SD_ACMD6_BUS_WIDTH_POS)

#define SD_CMD6_MODE_POS                                                (31U)
#define SD_CMD6_MODE_MASK                                               (0x1U << SD_CMD6_MODE_POS)
#define SD_CMD6_MODE_CHECK                                              (0U << SD_CMD6_MODE_POS)
#define SD_CMD6_MODE_SWITCH                                             (1U << SD_CMD6_MODE_POS)
#define SD_CMD6_RESERVED_POS                                            (16U)
#define SD_CMD6_RESERVED_MASK                                           (0x7FFFU << SD_CMD6_RESERVED_POS)
#define SD_CMD6_RESERVED                                                (0U << SD_CMD6_RESERVED_POS)
#define SD_CMD6_POWER_LIMIT_POS                                         (12U)
#define SD_CMD6_POWER_LIMIT_MASK                                        (0xFU << SD_CMD6_POWER_LIMIT_POS)
#define SD_CMD6_POWER_LIMIT_DONT_CARE                                   (0xFU << SD_CMD6_POWER_LIMIT_POS)
#define SD_CMD6_DRIVE_STRENGTH_POS                                      (8U)
#define SD_CMD6_DRIVE_STRENGTH_MASK                                     (0xFU << SD_CMD6_DRIVE_STRENGTH_POS)
#define SD_CMD6_DRIVE_STRENGTH_DONT_CARE                                (0xFU << SD_CMD6_DRIVE_STRENGTH_POS)
#define SD_CMD6_COMMAND_SYSTEM_POS                                      (4U)
#define SD_CMD6_COMMAND_SYSTEM_MASK                                     (0xFU << SD_CMD6_COMMAND_SYSTEM_POS)
#define SD_CMD6_COMMAND_SYSTEM_DONT_CARE                                (0xFU << SD_CMD6_COMMAND_SYSTEM_POS)
#define SD_CMD6_ACCESS_MODE_POS                                         (0U)
#define SD_CMD6_ACCESS_MODE_MASK                                        (0xFU << SD_CMD6_ACCESS_MODE_POS)
#define SD_CMD6_ACCESS_MODE_HS                                          (1U << SD_CMD6_ACCESS_MODE_POS)
#define SD_CMD6_ACCESS_MODE_DONT_CARE                                   (0xFU << SD_CMD6_ACCESS_MODE_POS)
#define SD_CMD6_STATUS_BIT_SIZE                                         (512U)
#define SD_CMD6_STATUS_SUPPORT_ACCESS_MODE_BIT_POS                      (400U)
#define SD_CMD6_STATUS_SUPPORT_ACCESS_MODE_HS_BIT_POS                   (1U + SD_CMD6_STATUS_SUPPORT_ACCESS_MODE_BIT_POS)
#define SD_CMD6_STATUS_FUNCTION_SELECTION_ACCESS_MODE_BIT_POS           (376U)
#define SD_CMD6_STATUS_BYTE_SIZE                                        (SD_CMD6_STATUS_BIT_SIZE / 8U)
#define SD_CMD6_STATUS_SUPPORT_ACCESS_MODE_HS_BYTE_POS                  ((SD_CMD6_STATUS_BIT_SIZE - 1U - SD_CMD6_STATUS_SUPPORT_ACCESS_MODE_HS_BIT_POS) / 8U)
#define SD_CMD6_STATUS_SUPPORT_ACCESS_MODE_HS_BYTE_MASK                 (1U << (SD_CMD6_STATUS_SUPPORT_ACCESS_MODE_HS_BIT_POS % 8U))
#define SD_CMD6_STATUS_FUNCTION_SELECTION_ACCESS_MODE_BYTE_POS          ((SD_CMD6_STATUS_BIT_SIZE - 1U - SD_CMD6_STATUS_FUNCTION_SELECTION_ACCESS_MODE_BIT_POS) / 8U)
#define SD_CMD6_STATUS_FUNCTION_SELECTION_ACCESS_MODE_BYTE_MASK         (0xFU << (SD_CMD6_STATUS_FUNCTION_SELECTION_ACCESS_MODE_BIT_POS % 8U))
#define SD_CMD6_STATUS_FUNCTION_SELECTION_ACCESS_MODE_HS                (1U << (SD_CMD6_STATUS_FUNCTION_SELECTION_ACCESS_MODE_BIT_POS % 8U))

#define SD_CMD13_RCA_POS                        (16U)
#define SD_CMD13_RCA_MASK                       (0xFFFFU << SD_CMD13_RCA_POS)
#define SD_CMD13_SEND_STATUS_POS                (15U)
#define SD_CMD13_SEND_STATUS_MASK               (0x1U << SD_CMD13_SEND_STATUS_POS)
#define SD_CMD13_SEND_STATUS_CARD               (0U << SD_CMD13_SEND_STATUS_POS)
#define SD_CMD13_SEND_STATUS_TASK               (1U << SD_CMD13_SEND_STATUS_POS)
#define SD_CMD13_RESP0_CARD_CURRENT_STATE_POS   (9U)
#define SD_CMD13_RESP0_CARD_CURRENT_STATE_MASK  (0xFU << SD_CMD13_RESP0_CARD_CURRENT_STATE_POS)
#define SD_CMD13_RESP0_CARD_CURRENT_STATE_IDLE  (0U << SD_CMD13_RESP0_CARD_CURRENT_STATE_POS)
#define SD_CMD13_RESP0_CARD_CURRENT_STATE_READY (1U << SD_CMD13_RESP0_CARD_CURRENT_STATE_POS)
#define SD_CMD13_RESP0_CARD_CURRENT_STATE_IDENT (2U << SD_CMD13_RESP0_CARD_CURRENT_STATE_POS)
#define SD_CMD13_RESP0_CARD_CURRENT_STATE_STBY  (3U << SD_CMD13_RESP0_CARD_CURRENT_STATE_POS)
#define SD_CMD13_RESP0_CARD_CURRENT_STATE_TRAN  (4U << SD_CMD13_RESP0_CARD_CURRENT_STATE_POS)
#define SD_CMD13_RESP0_CARD_CURRENT_STATE_DATA  (5U << SD_CMD13_RESP0_CARD_CURRENT_STATE_POS)
#define SD_CMD13_RESP0_CARD_CURRENT_STATE_RCV   (6U << SD_CMD13_RESP0_CARD_CURRENT_STATE_POS)
#define SD_CMD13_RESP0_CARD_CURRENT_STATE_PRG   (7U << SD_CMD13_RESP0_CARD_CURRENT_STATE_POS)
#define SD_CMD13_RESP0_CARD_CURRENT_STATE_DIS   (8U << SD_CMD13_RESP0_CARD_CURRENT_STATE_POS)
#define SD_CMD13_RESP0_READY_FOR_DATA_POS       (8U)
#define SD_CMD13_RESP0_READY_FOR_DATA_MASK      (0x1U << SD_CMD13_RESP0_READY_FOR_DATA_POS)
#define SD_CMD13_RESP0_READY_FOR_DATA_NOT_READY (0U << SD_CMD13_RESP0_READY_FOR_DATA_POS)
#define SD_CMD13_RESP0_READY_FOR_DATA_READY     (1U << SD_CMD13_RESP0_READY_FOR_DATA_POS)

#define SD_ACMD23_NUMBER_OF_BLOCKS_POS  (0U)
#define SD_ACMD23_NUMBER_OF_BLOCKS_MASK (0x7FFFFFU << SD_ACMD23_NUMBER_OF_BLOCKS_POS)

#define SD_INITIALIZATION_APP_CMD_MAX_FAILURES (8U)

#define SD_INITIALIZATION_TIMEOUT_MS (1000U)
#define SD_TRANSFER_READY_TIMEOUT_MS (1000U)

#define SD_POLL_SLEEP_THRESHOLD_MS (2U)
#define SD_INITIALIZATION_SLEEP_MS (10U)
#define SD_TRANSFER_READY_SLEEP_MS (1U)

#define SD_CSD_CSD_STRUCTURE_START_BIT  (126U)
#define SD_CSD_CSD_STRUCTURE_WIDTH      (2U)
#define SD_CSD_CSD_STRUCTURE_V1         (0U)
#define SD_CSD_CSD_STRUCTURE_V2         (1U)
#define SD_CSD_CCC_START_BIT            (84U)
#define SD_CSD_CCC_WIDTH                (12U)
#define SD_CSD_CCC_SWITCH               (1U << 10)
#define SD_CSD_TRAN_SPEED_START_BIT     (96U)
#define SD_CSD_TRAN_SPEED_WIDTH         (8U)
#define SD_CSD_TRAN_SPEED_25MHZ         (0x32U)
#define SD_CSD_TRAN_SPEED_50MHZ         (0x5AU)
#define SD_CSD_TRAN_SPEED_100MHZ        (0x0BU)
#define SD_CSD_TRAN_SPEED_200MHZ        (0x2BU)
#define SD_CSD_READ_BL_LEN_START_BIT    (80U)
#define SD_CSD_READ_BL_LEN_WIDTH        (4U)
#define SD_CSD_READ_BL_LEN_V1_512B      (9U)
#define SD_CSD_READ_BL_LEN_V1_1024B     (10U)
#define SD_CSD_READ_BL_LEN_V1_2048B     (11U)
#define SD_CSD_READ_BL_LEN_V2_512B      (9U)
#define SD_CSD_C_SIZE_V1_START_BIT      (62U)
#define SD_CSD_C_SIZE_V1_WIDTH          (12U)
#define SD_CSD_C_SIZE_V2_START_BIT      (48U)
#define SD_CSD_C_SIZE_V2_WIDTH          (22U)
#define SD_CSD_C_SIZE_V2_SDHC_MIN       (0x1010U)
#define SD_CSD_C_SIZE_V2_SDHC_MAX       (0xFF5FU)
#define SD_CSD_C_SIZE_V2_SDXC_MIN       (0xFFFFU)
#define SD_CSD_C_SIZE_V2_SDXC_MAX       (0x3FFEFFU)
#define SD_CSD_C_SIZE_MULT_V1_START_BIT (47U)
#define SD_CSD_C_SIZE_MULT_V1_WIDTH     (3U)
#define SD_CSD_C_SIZE_MULT_V1_4         (0U)
#define SD_CSD_C_SIZE_MULT_V1_8         (1U)
#define SD_CSD_C_SIZE_MULT_V1_16        (2U)
#define SD_CSD_C_SIZE_MULT_V1_32        (3U)
#define SD_CSD_C_SIZE_MULT_V1_64        (4U)
#define SD_CSD_C_SIZE_MULT_V1_128       (5U)
#define SD_CSD_C_SIZE_MULT_V1_256       (6U)
#define SD_CSD_C_SIZE_MULT_V1_512       (7U)
#define SD_CSD_WRITE_BL_LEN_START_BIT   (22U)
#define SD_CSD_WRITE_BL_LEN_WIDTH       (4U)
#define SD_CSD_WRITE_BL_LEN_V1_512B     (9U)
#define SD_CSD_WRITE_BL_LEN_V1_1024B    (10U)
#define SD_CSD_WRITE_BL_LEN_V1_2048B    (11U)
#define SD_CSD_WRITE_BL_LEN_V2_512B     (9U)

#define SD_BLOCK_SIZE           (512U)
#define SD_SDSC_MAX_BLOCK_COUNT ((2028U << 20) / SD_BLOCK_SIZE)
#define SD_SDHC_MAX_BLOCK_COUNT ((SD_CSD_C_SIZE_V2_SDHC_MAX + 1U) << (19U - 9U))

static void sd_card_init_object(sd_card_t *card)
{
    card->state = SD_CARD_STATE_NO_CARD;
    card->ocr = 0U;
    card->cid[0] = 0U;
    card->cid[1] = 0U;
    card->cid[2] = 0U;
    card->cid[3] = 0U;
    card->rca = 0U;
    card->csd[0] = 0U;
    card->csd[1] = 0U;
    card->csd[2] = 0U;
    card->csd[3] = 0U;
    card->capacity_type = SD_CARD_CAPACITY_TYPE_UNKNOWN;
    card->block_count = 0U;
    card->block_size = 0U;
    card->max_clock_frequency = 0U;
}

static sd_card_status_t sd_card_go_idle_state(const sd_card_t *card)
{
    sd_card_host_cmd_t cmd;

    cmd.data = NULL;
    cmd.data_size = 0U;
    cmd.index = SD_CMD0_GO_IDLE_STATE;
    cmd.argument = 0U;
    cmd.response_type = SD_CARD_HOST_RESPONSE_NONE;
    if (card->ops->host_send_cmd(&cmd) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    return SD_CARD_OK;
}

static sd_card_status_t sd_card_send_if_cond(const sd_card_t *card,
                                             uint32_t argument,
                                             uint32_t *response)
{
    sd_card_host_cmd_t cmd;

    cmd.data = NULL;
    cmd.data_size = 0U;
    cmd.index = SD_CMD8_SEND_IF_COND;
    cmd.argument = argument;
    cmd.response_type = SD_CARD_HOST_RESPONSE_SHORT;
    if (card->ops->host_send_cmd(&cmd) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    if (response != NULL) {
        *response = cmd.response[0];
    }

    return SD_CARD_OK;
}

static sd_card_status_t sd_card_send_app_cmd(const sd_card_t *card,
                                             uint32_t argument)
{
    sd_card_host_cmd_t cmd;

    cmd.data = NULL;
    cmd.data_size = 0U;
    cmd.index = SD_CMD55_APP_CMD;
    cmd.argument = argument;
    cmd.response_type = SD_CARD_HOST_RESPONSE_SHORT;
    if (card->ops->host_send_cmd(&cmd) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    return SD_CARD_OK;
}

static sd_card_status_t sd_card_send_op_cond(const sd_card_t *card,
                                             uint32_t argument,
                                             uint32_t *response)
{
    sd_card_host_cmd_t cmd;

    cmd.data = NULL;
    cmd.data_size = 0U;
    cmd.index = SD_ACMD41_SD_SEND_OP_COND;
    cmd.argument = argument;
    cmd.response_type = SD_CARD_HOST_RESPONSE_SHORT_NO_CRC;
    if (card->ops->host_send_cmd(&cmd) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    if (response != NULL) {
        *response = cmd.response[0];
    }

    return SD_CARD_OK;
}

static sd_card_status_t sd_card_send_all_send_cid(const sd_card_t *card,
                                                  uint32_t *response)
{
    sd_card_host_cmd_t cmd;

    cmd.data = NULL;
    cmd.data_size = 0U;
    cmd.index = SD_CMD2_ALL_SEND_CID;
    cmd.argument = 0U;
    cmd.response_type = SD_CARD_HOST_RESPONSE_LONG;
    if (card->ops->host_send_cmd(&cmd) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    if (response != NULL) {
        response[0] = cmd.response[0];
        response[1] = cmd.response[1];
        response[2] = cmd.response[2];
        response[3] = cmd.response[3];
    }

    return SD_CARD_OK;
}

static sd_card_status_t sd_card_send_relative_addr(const sd_card_t *card,
                                                   uint32_t *response)
{
    sd_card_host_cmd_t cmd;

    cmd.data = NULL;
    cmd.data_size = 0U;
    cmd.index = SD_CMD3_SEND_RELATIVE_ADDR;
    cmd.argument = 0U;
    cmd.response_type = SD_CARD_HOST_RESPONSE_SHORT;
    if (card->ops->host_send_cmd(&cmd) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    if (response != NULL) {
        response[0] = cmd.response[0];
    }

    return SD_CARD_OK;
}

static sd_card_status_t sd_card_send_csd(const sd_card_t *card,
                                         uint32_t argument,
                                         uint32_t *response)
{
    sd_card_host_cmd_t cmd;

    cmd.data = NULL;
    cmd.data_size = 0U;
    cmd.index = SD_CMD9_SEND_CSD;
    cmd.argument = argument;
    cmd.response_type = SD_CARD_HOST_RESPONSE_LONG;
    if (card->ops->host_send_cmd(&cmd) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    if (response != NULL) {
        response[0] = cmd.response[0];
        response[1] = cmd.response[1];
        response[2] = cmd.response[2];
        response[3] = cmd.response[3];
    }

    return SD_CARD_OK;
}

static sd_card_status_t sd_card_select_card(const sd_card_t *card,
                                            uint32_t argument)
{
    sd_card_host_cmd_t cmd;

    cmd.data = NULL;
    cmd.data_size = 0U;
    cmd.index = SD_CMD7_SELECT_CARD;
    cmd.argument = argument;
    cmd.response_type = SD_CARD_HOST_RESPONSE_SHORT;
    if (card->ops->host_send_cmd(&cmd) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    return SD_CARD_OK;
}

static sd_card_status_t sd_card_set_bus_width(const sd_card_t *card,
                                              uint32_t argument)
{
    sd_card_host_cmd_t cmd;

    cmd.data = NULL;
    cmd.data_size = 0U;
    cmd.index = SD_ACMD6_SET_BUS_WIDTH;
    cmd.argument = argument;
    cmd.response_type = SD_CARD_HOST_RESPONSE_SHORT;
    if (card->ops->host_send_cmd(&cmd) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    return SD_CARD_OK;
}

static sd_card_status_t sd_card_switch_function(const sd_card_t *card,
                                                uint32_t argument,
                                                uint8_t *status)
{
    sd_card_host_cmd_t cmd;

    cmd.data = status;
    cmd.data_size = SD_CMD6_STATUS_BYTE_SIZE;
    cmd.index = SD_CMD6_SWITCH_FUNC;
    cmd.argument = argument;
    cmd.response_type = SD_CARD_HOST_RESPONSE_SHORT;
    if (card->ops->host_send_cmd(&cmd) != SD_CARD_OK) {
        card->ops->host_abort_transfer();

        return SD_CARD_ERROR;
    }

    return SD_CARD_OK;
}

static sd_card_status_t sd_card_send_status(const sd_card_t *card,
                                            uint32_t argument,
                                            uint32_t *response)
{
    sd_card_host_cmd_t cmd;

    cmd.data = NULL;
    cmd.data_size = 0U;
    cmd.index = SD_CMD13_SEND_STATUS;
    cmd.argument = argument;
    cmd.response_type = SD_CARD_HOST_RESPONSE_SHORT;
    if (card->ops->host_send_cmd(&cmd) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    if (response != NULL) {
        response[0] = cmd.response[0];
    }

    return SD_CARD_OK;
}

static sd_card_status_t sd_card_stop_transmission(const sd_card_t *card)
{
    sd_card_host_cmd_t cmd;

    cmd.data = NULL;
    cmd.data_size = 0U;
    cmd.index = SD_CMD12_STOP_TRANSMISSION;
    cmd.argument = 0U;
    cmd.response_type = SD_CARD_HOST_RESPONSE_SHORT;
    if (card->ops->host_send_cmd(&cmd) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    return SD_CARD_OK;
}

static sd_card_status_t sd_card_read_single_block(const sd_card_t *card,
                                                  uint32_t argument)
{
    sd_card_host_cmd_t cmd;

    cmd.data = NULL;
    cmd.data_size = 0U;
    cmd.index = SD_CMD17_READ_SINGLE_BLOCK;
    cmd.argument = argument;
    cmd.response_type = SD_CARD_HOST_RESPONSE_SHORT;
    if (card->ops->host_send_cmd(&cmd) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    return SD_CARD_OK;
}

static sd_card_status_t sd_card_read_multiple_block(const sd_card_t *card,
                                                    uint32_t argument)
{
    sd_card_host_cmd_t cmd;

    cmd.data = NULL;
    cmd.data_size = 0U;
    cmd.index = SD_CMD18_READ_MULTIPLE_BLOCK;
    cmd.argument = argument;
    cmd.response_type = SD_CARD_HOST_RESPONSE_SHORT;
    if (card->ops->host_send_cmd(&cmd) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    return SD_CARD_OK;
}

static sd_card_status_t sd_card_set_wr_blk_erase_count(const sd_card_t *card,
                                                       uint32_t argument)
{
    sd_card_host_cmd_t cmd;

    cmd.data = NULL;
    cmd.data_size = 0U;
    cmd.index = SD_ACMD23_SET_WR_BLK_ERASE_COUNT;
    cmd.argument = argument;
    cmd.response_type = SD_CARD_HOST_RESPONSE_SHORT;
    if (card->ops->host_send_cmd(&cmd) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    return SD_CARD_OK;
}

static sd_card_status_t sd_card_write_block(const sd_card_t *card,
                                            uint32_t argument)
{
    sd_card_host_cmd_t cmd;

    cmd.data = NULL;
    cmd.data_size = 0U;
    cmd.index = SD_CMD24_WRITE_BLOCK;
    cmd.argument = argument;
    cmd.response_type = SD_CARD_HOST_RESPONSE_SHORT;
    if (card->ops->host_send_cmd(&cmd) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    return SD_CARD_OK;
}

static sd_card_status_t sd_card_write_multiple_block(const sd_card_t *card,
                                                     uint32_t argument)
{
    sd_card_host_cmd_t cmd;

    cmd.data = NULL;
    cmd.data_size = 0U;
    cmd.index = SD_CMD25_WRITE_MULTIPLE_BLOCK;
    cmd.argument = argument;
    cmd.response_type = SD_CARD_HOST_RESPONSE_SHORT;
    if (card->ops->host_send_cmd(&cmd) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    return SD_CARD_OK;
}

static sd_card_status_t sd_card_set_blocklen(const sd_card_t *card,
                                             uint32_t argument)
{
    sd_card_host_cmd_t cmd;

    cmd.data = NULL;
    cmd.data_size = 0U;
    cmd.index = SD_CMD16_SET_BLOCKLEN;
    cmd.argument = argument;
    cmd.response_type = SD_CARD_HOST_RESPONSE_SHORT;
    if (card->ops->host_send_cmd(&cmd) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    return SD_CARD_OK;
}

static uint32_t sd_card_get_csd_field(const uint32_t *csd,
                                      uint8_t start_bit,
                                      uint8_t width)
{
    uint32_t mask;
    uint8_t low_index;
    uint8_t low_offset;
    uint32_t low_part;
    uint32_t high_mask;
    uint8_t high_index;
    uint32_t high_part;
    uint32_t value;

    mask = (1U << width) - 1U;
    low_index = (127U - start_bit) >> 5;
    low_offset = start_bit & 0x1FU;
    low_part = csd[low_index] >> low_offset;

    if ((low_offset + width) > 32U) {
        high_mask = (1U << (width - (32U - low_offset))) - 1U;
        high_index = low_index - 1U;
        high_part = csd[high_index] & high_mask;

        value = ((high_part << (32U - low_offset)) | low_part) & mask;
    } else {
        value = low_part & mask;
    }

    return value;
}

static sd_card_status_t sd_card_parse_csd(sd_card_t *card,
                                          const uint32_t *csd)
{
    uint32_t csd_structure;
    uint32_t tran_speed;
    uint32_t read_bl_len;
    uint32_t c_size;
    uint32_t write_bl_len;
    uint32_t c_size_mult;

    csd_structure = sd_card_get_csd_field(csd, SD_CSD_CSD_STRUCTURE_START_BIT, SD_CSD_CSD_STRUCTURE_WIDTH);
    if ((csd_structure != SD_CSD_CSD_STRUCTURE_V1) &&
        (csd_structure != SD_CSD_CSD_STRUCTURE_V2)) {
        return SD_CARD_ERROR;
    }

    if (((csd_structure == SD_CSD_CSD_STRUCTURE_V1) &&
         ((card->ocr & SD_ACMD41_RESP0_CCS_MASK) != SD_ACMD41_RESP0_CCS_SDSC)) ||
        ((csd_structure == SD_CSD_CSD_STRUCTURE_V2) &&
         ((card->ocr & SD_ACMD41_RESP0_CCS_MASK) != SD_ACMD41_RESP0_CCS_SDHC_SDXC))) {
        return SD_CARD_ERROR;
    }

    tran_speed = sd_card_get_csd_field(csd, SD_CSD_TRAN_SPEED_START_BIT, SD_CSD_TRAN_SPEED_WIDTH);
    if (csd_structure == SD_CSD_CSD_STRUCTURE_V1) {
        if ((tran_speed != SD_CSD_TRAN_SPEED_25MHZ) &&
            (tran_speed != SD_CSD_TRAN_SPEED_50MHZ)) {
            return SD_CARD_ERROR;
        }
    } else {
        if ((tran_speed != SD_CSD_TRAN_SPEED_25MHZ) &&
            (tran_speed != SD_CSD_TRAN_SPEED_50MHZ) &&
            (tran_speed != SD_CSD_TRAN_SPEED_100MHZ) &&
            (tran_speed != SD_CSD_TRAN_SPEED_200MHZ)) {
            return SD_CARD_ERROR;
        }
    }

    read_bl_len = sd_card_get_csd_field(csd, SD_CSD_READ_BL_LEN_START_BIT, SD_CSD_READ_BL_LEN_WIDTH);
    if (csd_structure == SD_CSD_CSD_STRUCTURE_V1) {
        if ((read_bl_len != SD_CSD_READ_BL_LEN_V1_512B) &&
            (read_bl_len != SD_CSD_READ_BL_LEN_V1_1024B) &&
            (read_bl_len != SD_CSD_READ_BL_LEN_V1_2048B)) {
            return SD_CARD_ERROR;
        }
    } else {
        if (read_bl_len != SD_CSD_READ_BL_LEN_V2_512B) {
            return SD_CARD_ERROR;
        }
    }

    write_bl_len = sd_card_get_csd_field(csd, SD_CSD_WRITE_BL_LEN_START_BIT, SD_CSD_WRITE_BL_LEN_WIDTH);
    if (csd_structure == SD_CSD_CSD_STRUCTURE_V1) {
        if ((write_bl_len != SD_CSD_WRITE_BL_LEN_V1_512B) &&
            (write_bl_len != SD_CSD_WRITE_BL_LEN_V1_1024B) &&
            (write_bl_len != SD_CSD_WRITE_BL_LEN_V1_2048B)) {
            return SD_CARD_ERROR;
        }
    } else {
        if (write_bl_len != SD_CSD_WRITE_BL_LEN_V2_512B) {
            return SD_CARD_ERROR;
        }
    }

    if (csd_structure == SD_CSD_CSD_STRUCTURE_V1) {
        if (((read_bl_len == SD_CSD_READ_BL_LEN_V1_512B) && (write_bl_len != SD_CSD_WRITE_BL_LEN_V1_512B)) ||
            ((read_bl_len == SD_CSD_READ_BL_LEN_V1_1024B) && (write_bl_len != SD_CSD_WRITE_BL_LEN_V1_1024B)) ||
            ((read_bl_len == SD_CSD_READ_BL_LEN_V1_2048B) && (write_bl_len != SD_CSD_WRITE_BL_LEN_V1_2048B))) {
            return SD_CARD_ERROR;
        }
    }

    if (csd_structure == SD_CSD_CSD_STRUCTURE_V1) {
        c_size = sd_card_get_csd_field(csd, SD_CSD_C_SIZE_V1_START_BIT, SD_CSD_C_SIZE_V1_WIDTH);
        c_size_mult = sd_card_get_csd_field(csd, SD_CSD_C_SIZE_MULT_V1_START_BIT, SD_CSD_C_SIZE_MULT_V1_WIDTH);
        if ((c_size_mult != SD_CSD_C_SIZE_MULT_V1_4) &&
            (c_size_mult != SD_CSD_C_SIZE_MULT_V1_8) &&
            (c_size_mult != SD_CSD_C_SIZE_MULT_V1_16) &&
            (c_size_mult != SD_CSD_C_SIZE_MULT_V1_32) &&
            (c_size_mult != SD_CSD_C_SIZE_MULT_V1_64) &&
            (c_size_mult != SD_CSD_C_SIZE_MULT_V1_128) &&
            (c_size_mult != SD_CSD_C_SIZE_MULT_V1_256) &&
            (c_size_mult != SD_CSD_C_SIZE_MULT_V1_512)) {
            return SD_CARD_ERROR;
        }
    } else {
        c_size = sd_card_get_csd_field(csd, SD_CSD_C_SIZE_V2_START_BIT, SD_CSD_C_SIZE_V2_WIDTH);
        if (((c_size < SD_CSD_C_SIZE_V2_SDHC_MIN) || (c_size > SD_CSD_C_SIZE_V2_SDHC_MAX)) &&
            ((c_size < SD_CSD_C_SIZE_V2_SDXC_MIN) || (c_size > SD_CSD_C_SIZE_V2_SDXC_MAX))) {
            return SD_CARD_ERROR;
        }
    }

    if (csd_structure == SD_CSD_CSD_STRUCTURE_V1) {
        card->block_count = (c_size + 1U) * (1U << (c_size_mult + 2U)) * (1U << (read_bl_len - 9U));
        if (card->block_count > SD_SDSC_MAX_BLOCK_COUNT) {
            return SD_CARD_ERROR;
        }
    } else {
        card->block_count = (c_size + 1U) << (19U - 9U);
    }

    card->block_size = SD_BLOCK_SIZE;

    switch (tran_speed) {
        case SD_CSD_TRAN_SPEED_25MHZ: card->max_clock_frequency = SD_CARD_CLOCK_FREQUENCY_25000000HZ; break;
        case SD_CSD_TRAN_SPEED_50MHZ: card->max_clock_frequency = SD_CARD_CLOCK_FREQUENCY_50000000HZ; break;
        case SD_CSD_TRAN_SPEED_100MHZ: card->max_clock_frequency = SD_CARD_CLOCK_FREQUENCY_100000000HZ; break;
        case SD_CSD_TRAN_SPEED_200MHZ: card->max_clock_frequency = SD_CARD_CLOCK_FREQUENCY_200000000HZ; break;
    }

    return SD_CARD_OK;
}

static sd_card_status_t sd_card_wait_transfer_ready(const sd_card_t *card)
{
    uint32_t argument;
    uint32_t response[1];
    uint32_t start_tick;
    uint32_t tick;

    argument = ((((uint32_t)card->rca) << SD_CMD13_RCA_POS) & SD_CMD13_RCA_MASK) |
                SD_CMD13_SEND_STATUS_CARD;

    start_tick = card->ops->get_tick_ms();

    for (;;) {
        tick = card->ops->get_tick_ms();

        if ((tick - start_tick) >= SD_TRANSFER_READY_TIMEOUT_MS) {
            return SD_CARD_ERROR;
        }

        if (sd_card_send_status(card, argument, response) != SD_CARD_OK) {
            return SD_CARD_ERROR;
        }

        if (((response[0] & SD_CMD13_RESP0_READY_FOR_DATA_MASK) == SD_CMD13_RESP0_READY_FOR_DATA_READY) &&
            ((response[0] & SD_CMD13_RESP0_CARD_CURRENT_STATE_MASK) == SD_CMD13_RESP0_CARD_CURRENT_STATE_TRAN)) {
            break;
        }

        if ((card->ops->sleep_ms != NULL) &&
            ((tick - start_tick) >= SD_POLL_SLEEP_THRESHOLD_MS)) {
            card->ops->sleep_ms(SD_TRANSFER_READY_SLEEP_MS);
        }
    }

    return SD_CARD_OK;
}

static uint32_t sd_card_get_block_address(const sd_card_t *card, uint32_t block)
{
    if (card->capacity_type == SD_CARD_CAPACITY_TYPE_SDSC) {
        return block * SD_BLOCK_SIZE;
    }

    return block;
}

sd_card_status_t sd_card_init(sd_card_t *card,
                              const sd_card_ops_t *ops)
{
    if ((card == NULL) ||
        (ops == NULL) ||
        ((ops->host_send_cmd == NULL) ||
         (ops->host_set_bus_width == NULL) ||
         (ops->host_set_clock_frequency == NULL) ||
         (ops->host_abort_transfer == NULL) ||
         (ops->host_prepare_read == NULL) ||
         (ops->host_read == NULL) ||
         (ops->host_write == NULL) ||
         (ops->get_tick_ms == NULL))) {
        return SD_CARD_ERROR;
    }

    sd_card_init_object(card);

    card->ops = ops;

    return SD_CARD_OK;
}

sd_card_status_t sd_card_probe(sd_card_t *card)
{
    uint32_t response[4];
    bool cmd8_responded;
    uint32_t app_cmd_failures;
    uint32_t op_cond_argument;
    uint32_t start_tick;
    uint32_t tick;

    if ((card == NULL) ||
        (card->ops == NULL)) {
        return SD_CARD_ERROR;
    }

    sd_card_init_object(card);

    if (sd_card_go_idle_state(card) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    if (sd_card_send_if_cond(card,
                             SD_CMD8_VHS_27_36 | SD_CMD8_CHECK_PATTERN,
                             response) == SD_CARD_OK) {
        if ((response[0] & (SD_CMD8_RESP0_VHS_MASK | SD_CMD8_RESP0_CHECK_PATTERN_MASK)) !=
            (SD_CMD8_RESP0_VHS_27_36 | SD_CMD8_RESP0_CHECK_PATTERN)) {
            return SD_CARD_ERROR;
        }

        cmd8_responded = true;
    } else {
        cmd8_responded = false;
    }

    op_cond_argument = SD_ACMD41_XPC_POWER_SAVING | SD_ACMD41_S18R_USE_CURRENT_SIGNAL_VOLTAGE | SD_ACMD41_VOLTAGE_ALL;
    if (cmd8_responded == true) {
        op_cond_argument |= SD_ACMD41_HCS_SDHC_SDXC;
    } else {
        op_cond_argument |= SD_ACMD41_HCS_SDSC;
    }

    start_tick = card->ops->get_tick_ms();
    app_cmd_failures = 0U;

    for (;;) {
        tick = card->ops->get_tick_ms();

        if ((tick - start_tick) >= SD_INITIALIZATION_TIMEOUT_MS) {
            return SD_CARD_ERROR;
        }

        if (sd_card_send_app_cmd(card, 0U) != SD_CARD_OK) {
            if (++app_cmd_failures >= SD_INITIALIZATION_APP_CMD_MAX_FAILURES) {
                return SD_CARD_ERROR;
            }

            continue;
        } else {
            app_cmd_failures = 0U;
        }

        if (sd_card_send_op_cond(card, op_cond_argument, response) != SD_CARD_OK) {
            continue;
        }

        if ((response[0] & SD_ACMD41_RESP0_BUSY_MASK) == SD_ACMD41_RESP0_BUSY_INITIALIZATION_COMPLETE) {
            if ((response[0] & SD_ACMD41_VOLTAGE_ALL) == 0U) {
                return SD_CARD_ERROR;
            }

            card->ocr = response[0];

            break;
        }

        if ((card->ops->sleep_ms != NULL) &&
            ((tick - start_tick) >= SD_POLL_SLEEP_THRESHOLD_MS)) {
            card->ops->sleep_ms(SD_INITIALIZATION_SLEEP_MS);
        }
    }

    if (sd_card_send_all_send_cid(card, response) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    } else {
        card->cid[0] = response[0];
        card->cid[1] = response[1];
        card->cid[2] = response[2];
        card->cid[3] = response[3];
    }

    if (sd_card_send_relative_addr(card, response) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    } else {
        card->rca = (uint16_t)((response[0] & SD_CMD3_RCA_MASK) >> SD_CMD3_RCA_POS);
    }

    if (sd_card_send_csd(card,
                         (((uint32_t)card->rca) << SD_CMD9_RCA_POS) & SD_CMD9_RCA_MASK,
                         response) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    } else {
        if (sd_card_parse_csd(card, response) != SD_CARD_OK) {
            return SD_CARD_ERROR;
        }

        card->csd[0] = response[0];
        card->csd[1] = response[1];
        card->csd[2] = response[2];
        card->csd[3] = response[3];
    }

    if (sd_card_select_card(card,
                            (((uint32_t)card->rca) << SD_CMD7_RCA_POS) & SD_CMD7_RCA_MASK) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    if ((card->ocr & SD_ACMD41_RESP0_CCS_MASK) == SD_ACMD41_RESP0_CCS_SDHC_SDXC) {
        if (card->block_count <= SD_SDHC_MAX_BLOCK_COUNT) {
            card->capacity_type = SD_CARD_CAPACITY_TYPE_SDHC;
        } else {
            card->capacity_type = SD_CARD_CAPACITY_TYPE_SDXC;
        }
    } else {
        card->capacity_type = SD_CARD_CAPACITY_TYPE_SDSC;
    }

    if (card->capacity_type == SD_CARD_CAPACITY_TYPE_SDSC) {
        if (sd_card_set_blocklen(card, SD_BLOCK_SIZE) != SD_CARD_OK) {
            return SD_CARD_ERROR;
        }
    }

    card->state = SD_CARD_STATE_READY;

    return SD_CARD_OK;
}

sd_card_status_t sd_card_reset(sd_card_t *card)
{
    if (card == NULL) {
        return SD_CARD_ERROR;
    }

    sd_card_init_object(card);

    return SD_CARD_OK;
}

sd_card_status_t sd_card_config_bus_width(const sd_card_t *card,
                                          sd_card_bus_width_t bus_width)
{
    uint32_t argument;
    sd_card_host_bus_width_t host_bus_width;

    if ((card == NULL) ||
        (bus_width >= SD_CARD_BUS_WIDTH_COUNT)) {
        return SD_CARD_ERROR;
    }

    if (card->state != SD_CARD_STATE_READY) {
        return SD_CARD_ERROR;
    }

    switch (bus_width) {
        case SD_CARD_BUS_WIDTH_1B:
            argument = SD_ACMD6_BUS_WIDTH_1B;
            host_bus_width = SD_CARD_HOST_BUS_WIDTH_1;
            break;

        case SD_CARD_BUS_WIDTH_4B:
            argument = SD_ACMD6_BUS_WIDTH_4B;
            host_bus_width = SD_CARD_HOST_BUS_WIDTH_4;
            break;

        default:
            argument = SD_ACMD6_BUS_WIDTH_1B;
            host_bus_width = SD_CARD_HOST_BUS_WIDTH_1;
            break;
    }

    if (sd_card_send_app_cmd(card,
                             (((uint32_t)card->rca) << SD_CMD55_RCA_POS) & SD_CMD55_RCA_MASK) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    if (sd_card_set_bus_width(card, argument) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    if (card->ops->host_set_bus_width(host_bus_width) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    return SD_CARD_OK;
}

sd_card_status_t sd_card_config_high_speed(sd_card_t *card)
{
    uint32_t ccc;
    uint8_t status[SD_CMD6_STATUS_BYTE_SIZE];

    if (card == NULL) {
        return SD_CARD_ERROR;
    }

    if (card->state != SD_CARD_STATE_READY) {
        return SD_CARD_ERROR;
    }

    ccc = sd_card_get_csd_field(card->csd, SD_CSD_CCC_START_BIT, SD_CSD_CCC_WIDTH);
    if ((ccc & SD_CSD_CCC_SWITCH) != SD_CSD_CCC_SWITCH) {
        return SD_CARD_ERROR;
    }

    (void)memset(status, 0U, sizeof(status));
    if (sd_card_switch_function(card,
                                SD_CMD6_MODE_CHECK | SD_CMD6_RESERVED | SD_CMD6_POWER_LIMIT_DONT_CARE | SD_CMD6_DRIVE_STRENGTH_DONT_CARE | SD_CMD6_COMMAND_SYSTEM_DONT_CARE | SD_CMD6_ACCESS_MODE_HS,
                                status) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    if ((status[SD_CMD6_STATUS_SUPPORT_ACCESS_MODE_HS_BYTE_POS] & SD_CMD6_STATUS_SUPPORT_ACCESS_MODE_HS_BYTE_MASK) == 0U) {
        return SD_CARD_ERROR;
    }

    (void)memset(status, 0U, sizeof(status));
    if (sd_card_switch_function(card,
                                SD_CMD6_MODE_SWITCH | SD_CMD6_RESERVED | SD_CMD6_POWER_LIMIT_DONT_CARE | SD_CMD6_DRIVE_STRENGTH_DONT_CARE | SD_CMD6_COMMAND_SYSTEM_DONT_CARE | SD_CMD6_ACCESS_MODE_HS,
                                status) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    if ((status[SD_CMD6_STATUS_FUNCTION_SELECTION_ACCESS_MODE_BYTE_POS] & SD_CMD6_STATUS_FUNCTION_SELECTION_ACCESS_MODE_BYTE_MASK) != SD_CMD6_STATUS_FUNCTION_SELECTION_ACCESS_MODE_HS) {
        return SD_CARD_ERROR;
    }

    card->max_clock_frequency = SD_CARD_CLOCK_FREQUENCY_50000000HZ;

    return SD_CARD_OK;
}

sd_card_status_t sd_card_config_clock_frequency(const sd_card_t *card,
                                                uint32_t clock_frequency)
{
    if ((card == NULL) ||
        (clock_frequency == 0U)) {
        return SD_CARD_ERROR;
    }

    if (card->state != SD_CARD_STATE_READY) {
        return SD_CARD_ERROR;
    }

    if (clock_frequency > card->max_clock_frequency) {
        return SD_CARD_ERROR;
    }

    if (card->ops->host_set_clock_frequency(clock_frequency) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    return SD_CARD_OK;
}

sd_card_state_t sd_card_get_state(const sd_card_t *card)
{
    if (card == NULL) {
        return SD_CARD_STATE_NO_CARD;
    }

    return card->state;
}

sd_card_capacity_type_t sd_card_get_capacity_type(const sd_card_t *card)
{
    if (card == NULL) {
        return SD_CARD_CAPACITY_TYPE_UNKNOWN;
    }

    if (card->state != SD_CARD_STATE_READY) {
        return SD_CARD_CAPACITY_TYPE_UNKNOWN;
    }

    return card->capacity_type;
}

uint32_t sd_card_get_block_size(const sd_card_t *card)
{
    if (card == NULL) {
        return 0U;
    }

    if (card->state != SD_CARD_STATE_READY) {
        return 0U;
    }

    return card->block_size;
}

uint32_t sd_card_get_block_count(const sd_card_t *card)
{
    if (card == NULL) {
        return 0U;
    }

    if (card->state != SD_CARD_STATE_READY) {
        return 0U;
    }

    return card->block_count;
}

uint32_t sd_card_get_max_clock_frequency(const sd_card_t *card)
{
    if (card == NULL) {
        return 0U;
    }

    if (card->state != SD_CARD_STATE_READY) {
        return 0U;
    }

    return card->max_clock_frequency;
}

sd_card_status_t sd_card_read_blocks(const sd_card_t *card,
                                     uint8_t *buffer,
                                     uint32_t block_start,
                                     uint32_t block_count)
{
    sd_card_status_t ret;
    uint32_t size;
    bool is_multiple;

    if ((card == NULL) ||
        (buffer == NULL) ||
        (block_count == 0U)) {
        return SD_CARD_ERROR;
    }

    if (card->state != SD_CARD_STATE_READY) {
        return SD_CARD_ERROR;
    }

    if ((block_start >= card->block_count) ||
        (block_count > (card->block_count - block_start))) {
        return SD_CARD_ERROR;
    }

    if (block_count > (UINT32_MAX / SD_BLOCK_SIZE)) {
        return SD_CARD_ERROR;
    }

    size = block_count * SD_BLOCK_SIZE;
    is_multiple = (block_count > 1U) ? true : false;

    if (card->ops->host_prepare_read(buffer, size) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    if (is_multiple != false) {
        ret = sd_card_read_multiple_block(card, sd_card_get_block_address(card, block_start));
    } else {
        ret = sd_card_read_single_block(card, sd_card_get_block_address(card, block_start));
    }

    if (ret != SD_CARD_OK) {
        card->ops->host_abort_transfer();
        return SD_CARD_ERROR;
    }

    if (card->ops->host_read(buffer, size) != SD_CARD_OK) {
        ret = SD_CARD_ERROR;
    } else {
        ret = SD_CARD_OK;
    }

    if (is_multiple != false) {
        if (sd_card_stop_transmission(card) != SD_CARD_OK) {
            card->ops->host_abort_transfer();
            return SD_CARD_ERROR;
        }
    }

    if (ret != SD_CARD_OK) {
        card->ops->host_abort_transfer();
        return SD_CARD_ERROR;
    }

    return SD_CARD_OK;
}

sd_card_status_t sd_card_write_blocks(const sd_card_t *card,
                                      const uint8_t *buffer,
                                      uint32_t block_start,
                                      uint32_t block_count)
{
    sd_card_status_t ret;
    uint32_t size;
    bool is_multiple;

    if ((card == NULL) ||
        (buffer == NULL) ||
        (block_count == 0U)) {
        return SD_CARD_ERROR;
    }

    if (card->state != SD_CARD_STATE_READY) {
        return SD_CARD_ERROR;
    }

    if ((block_start >= card->block_count) ||
        (block_count > (card->block_count - block_start))) {
        return SD_CARD_ERROR;
    }

    if (block_count > (UINT32_MAX / SD_BLOCK_SIZE)) {
        return SD_CARD_ERROR;
    }

    size = block_count * SD_BLOCK_SIZE;
    is_multiple = (block_count > 1U) ? true : false;

    if (sd_card_wait_transfer_ready(card) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    if (is_multiple != false) {
        if (sd_card_send_app_cmd(card,
                                 (((uint32_t)card->rca) << SD_CMD55_RCA_POS) & SD_CMD55_RCA_MASK) == SD_CARD_OK) {
            (void)sd_card_set_wr_blk_erase_count(card,
                                                 (block_count << SD_ACMD23_NUMBER_OF_BLOCKS_POS) & SD_ACMD23_NUMBER_OF_BLOCKS_MASK);
        }
        ret = sd_card_write_multiple_block(card, sd_card_get_block_address(card, block_start));
    } else {
        ret = sd_card_write_block(card, sd_card_get_block_address(card, block_start));
    }

    if (ret != SD_CARD_OK) {
        card->ops->host_abort_transfer();
        return SD_CARD_ERROR;
    }

    if (card->ops->host_write(buffer, size) != SD_CARD_OK) {
        ret = SD_CARD_ERROR;
    } else {
        ret = SD_CARD_OK;
    }

    if (is_multiple != false) {
        if (sd_card_stop_transmission(card) != SD_CARD_OK) {
            card->ops->host_abort_transfer();
            return SD_CARD_ERROR;
        }
    }

    if (ret != SD_CARD_OK) {
        card->ops->host_abort_transfer();
        return SD_CARD_ERROR;
    }

    if (sd_card_wait_transfer_ready(card) != SD_CARD_OK) {
        return SD_CARD_ERROR;
    }

    return SD_CARD_OK;
}
