#include "radio_commands.h"

#include <string.h>

int pack_ack(const struct ack_s *const src, uint8_t *const out_buf, const size_t out_buf_size)
{
    if (!out_buf || out_buf_size < STATUS_REP_SIZE) {
        return -1;
    }

    memcpy(out_buf, src, STATUS_REP_SIZE);
    return 0;
}
