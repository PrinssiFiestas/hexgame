#include <gpc/gpc.h>

typedef enum base
{
    BASE2,
    BASE10,
    BASE16,
    BASE_LENGTH
} Base;

#define RESET true

static const char* base_strings[BASE_LENGTH] = {
    [BASE2]  = "binary",
    [BASE10] = "decimal",
    [BASE16] = "hexadecimal"
};

double game_time(bool reset)
{
    static __uint128_t start;
    struct timespec t_now;
    if (reset) {
        struct timespec t_start;
        gp_assert(timespec_get(&t_start, TIME_UTC));
        start = (__uint128_t)(t_start.tv_sec)*1000000000 + t_start.tv_nsec;
        return 0.;
    }
    gp_assert(timespec_get(&t_now, TIME_UTC));
    __uint128_t now = (__uint128_t)(t_now.tv_sec)*1000000000 + t_now.tv_nsec;

    double diff = now - start;
    return diff/1000000000;
}

const char* u32toa_binary(uint32_t u)
{
    static char buf[8];
    memset(buf, 0, sizeof buf);
    if (u == 0) {
        buf[0] = '0';
        return buf;
    }
    size_t length = 0;
    for (uint32_t _u = u; _u != 0; _u >>= 1)
        ++length;

    while (u != 0) {
        buf[--length] = (u & 1) + '0';
        u >>= 1;
    }

    return buf;
}

uint32_t atou32_binary(const char* str)
{
    uint32_t result = 0;
    while (*str != '\0') {
        result = (result << 1) | (*str - '0');
        ++str;
    }
    return result;
}

size_t game(size_t round, size_t left_base, size_t right_base)
{
    GPRandomState rs = gp_random_state(time(NULL));

    gp_println(
        "Round", round, ": Convert", base_strings[left_base], "to", base_strings[right_base]);

    // TODO countdown

    size_t score = 0;
    game_time(RESET);
    while (game_time(0) < 60.)
    {
        uint32_t left = gp_random(&rs) & 0xF;
        uint32_t right;

        try_again:
        switch (left_base) {
        case BASE2:  printf("%4s: ", u32toa_binary(left)); break;
        case BASE10: printf("%4u: ", left);                break;
        case BASE16: printf("%4x: ", left);                break;
        default: __builtin_unreachable();
        }
        fflush(stdout);

        char right_base2_buf[8] = "";
        switch (right_base) {
        case BASE2:
            scanf("%4s", right_base2_buf);
            right = atou32_binary(right_base2_buf);
            break;

        case BASE10: scanf("%u", &right); break;
        case BASE16: scanf("%x", &right); break;
        default: __builtin_unreachable();
        }

        while (getchar() != '\n')
            ;

        if (right != left) {
            gp_print(GP_CURSOR_UP(1) GP_CURSOR_FORWARD(6) GP_RED"WRONG!\r"GP_RESET_TERMINAL);
            goto try_again;
        }
        ++score;
    }
    gp_println("Round", round, "score:", score);
    return score;
}

int main(void)
{
    for (size_t round = 0, left_base = BASE2; left_base < BASE_LENGTH; ++left_base)
    {
        for (size_t right_base = BASE2; right_base < BASE_LENGTH; ++right_base, ++round)
        {
            if (left_base == right_base)
                continue;

            game(round, left_base, right_base);
        }
    }
}
