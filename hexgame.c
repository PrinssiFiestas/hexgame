// MIT License
// Copyright (c) 2025 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/hexgame/blob/main/LICENSE.md

#define GPC_IMPLEMENTATION
#include "gpc.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

typedef enum base
{
    BASE2,
    BASE10,
    BASE16,
    BASE_LENGTH
} base_t;

// Probably no reason to make any bigger, but here we go if needed.
typedef uint16_t score_t;

typedef struct leaderboard_entry
{
    char    name[16];
    time_t  timestamp;
    score_t score;
} LeaderBoardEntry;

typedef struct high_score_position
{
    base_t left_base;
    base_t right_base;
    size_t position;
} HighScorePosition;

// -----------------------------
// ▘|▝|▀|▖|▌|▞|▛|▗|▚|▐|▜|▄|▙|▟|█
// -----------------------------
static const char* header = GP_MAGENTA       "\n"
"▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀\n"
"█  █ █▀▀▀ ▜▖  ▗▛   ▟▛▀▀▜▙ ▟▛▀▜▙ ▙      ▟ █▀▀▀\n"
"█  █ █     ▜▖▗▛    █      █   █ █▙    ▟█ █   \n"
"█▀▀█ █▀▀▀   ██     █ ▟▀▜▙ █▀▀▀█ █▝▙  ▟▘█ █▀▀▀\n"
"█  █ █     ▟▘▝▙    █    █ █   █ █ ▝▙▟▘ █ █   \n"
"█  █ █▄▄▄ ▟▘  ▝▙   ▜▙▄▄▄▛ █   █ █  ▝▘  █ █▄▄▄\n"
"▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄" GP_RESET_TERMINAL "\n";

#define ROUND_DURATION 30. // seconds

#define LEADERBOARD_MAX_LENGTH 10

static const char* base_lowercase[BASE_LENGTH] = {
    [BASE2]  = "binary",
    [BASE10] = "decimal",
    [BASE16] = "hexadecimal"
};

static const char* base_titlecase[BASE_LENGTH] = {
    [BASE2]  = "Binary",
    [BASE10] = "Decimal",
    [BASE16] = "Hexadecimal"
};

#define TIME_RESET true
#define TIME_NOW   false

#define SCORE_FIELD_WIDTH 8

#define BASE_COMBINATIONS (BASE_LENGTH * (BASE_LENGTH - 1)) // distinct

static double game_time(bool reset)
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

static const char* u4toa_binary(size_t u)
{
    gp_assert(u <= 0xF, "Argument should be of size uint4_t.");

    char* buf = gp_mem_alloc(&gp_last_scope()->base, 8);
    memset(buf, 0, 8);

    for (size_t i = 4-1; i != (size_t)-1; --i) {
        buf[i] = (u & 1) + '0';
        u >>= 1;
    }
    gp_assert(strlen(buf) == 4);
    return buf;
}

static size_t atou4_binary(const char* str)
{
    while (isspace(*str))
        ++str;
    size_t result = 0;
    size_t length = 0;
    while ('0' <= *str && *str <= '1') {
        result = (result << 1) | (*str - '0');
        ++str;
        ++length;
    }
    if (length == 0 || length > 4)
        return -1; // will be interpreted as wrong answer
    return result;
}

__attribute__((format(scanf, 1, 2)))
static int read_input(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    int result = vscanf(format, args);
    if (result == EOF) { // we'll interpret Ctrl+D as a quit request
        puts("");
        exit(EXIT_SUCCESS);
    }
    while (getchar() != '\n') // flush input in case of failed vscanf()
        ;
    va_end(args);

    return result;
}

static size_t game(size_t round, base_t left_base, base_t right_base)
{
    GPRandomState rs = gp_random_state(time(NULL));
    GPScope* scope = gp_begin(0);

    gp_println(
        "Round", round, ": Convert", base_lowercase[left_base], "to", base_lowercase[right_base]);
    gp_println("Get ready...");

    for (size_t countdown = 5; countdown != 0; --countdown) {
        gp_print(countdown, "\r");
        fflush(stdout);
        game_time(TIME_RESET);
        while (game_time(TIME_NOW) < 1.)
            usleep(10);
    }

    uint32_t last_left = -1;
    size_t score = 0;
    game_time(TIME_RESET);
    while (game_time(TIME_NOW) < ROUND_DURATION)
    {
        uint32_t right;
        uint32_t left; do {
            left = gp_random(&rs) & 0xF;
        } while (left == last_left);
        last_left = left;

        try_again:;
        size_t left_digits;
        switch (left_base) {
        case BASE2:
            printf("%s: ", u4toa_binary(left));
            left_digits = 1 + (left > 1); // we only care if larger than 1
            break;

        case BASE10:
            printf("%4u: ", left);
            left_digits = snprintf(NULL, 0, "%u", left);
            break;

        case BASE16:
            printf(" 0x%X: ", left);
            left_digits = snprintf(NULL, 0, "%x", left);
            break;

        default: __builtin_unreachable();
        }
        gp_assert(0 < left_digits && left_digits <= 4);
        gp_print("\n", GP_CURSOR_UP(1) GP_CURSOR_FORWARD(6)); // empty line to avoid scroll on WRONG
        fflush(stdout);

        size_t right_digits;
        char right_base2_buf[8] = "";
        switch (right_base) {
        case BASE2:
            printf("0b");
            read_input("%5s", right_base2_buf);
            right = atou4_binary(right_base2_buf);
            right_digits = 1 + (right > 1); // we only care if larger than 1
            break;

        case BASE10:
            read_input("%u", &right);
            right_digits = snprintf(NULL, 0, "%u", right);
            break;

        case BASE16:
            printf("0x");
            read_input("%x", &right);
            right_digits = snprintf(NULL, 0, "%x", right);
            break;

        default: __builtin_unreachable();
        }

        if (right != left) {
            printf(GP_CURSOR_UP(1) GP_CURSOR_FORWARD(6));
            if (right_base != BASE10) // skip 0x or 0b
                printf(GP_CURSOR_FORWARD(2));
            printf(GP_RED"WRONG                                              \r"GP_RESET_TERMINAL);
            goto try_again;
        }

        gp_print(GP_GREEN "Correct! ");
        if (left_digits == 1 && right_digits == left_digits) {
            score += 1;
            gp_println(" +1p " GP_RESET_TERMINAL "(trivial conversion) | Score:", score);
        } else {
            score += 2;
            gp_println(" +2p " GP_RESET_TERMINAL "(non-trivial points) | Score:", score);
        }
    } // while(game_time(TIME_NOW) < 30.)

    gp_println("\nRound", round, "score:", score, "\n");
    gp_end(scope);
    return score;
}

static void print_leaderboard_entry(
    LeaderBoardEntry leaderboard[LEADERBOARD_MAX_LENGTH][BASE_LENGTH][BASE_LENGTH],
    size_t leaderboard_length,
    base_t left_base,
    base_t right_base)
{
    puts("-----------------------------------------------------------------");
    if (left_base == 0 && right_base == 0)
        printf("All Rounds Total\n");
    else
        printf("%s to %s\n", base_titlecase[left_base], base_titlecase[right_base]);

    printf("   | %-*s | %-*s | Date\n",
        (int)(sizeof leaderboard[0][0][0].name - sizeof""), "Name",
        SCORE_FIELD_WIDTH, "Score");
    puts("-----------------------------------------------------------------");

    for (size_t i_entry = 0; i_entry < leaderboard_length; ++i_entry) {
        LeaderBoardEntry entry = leaderboard[i_entry][left_base][right_base];
        char date[128] = "";
        gp_assert(strftime(date, sizeof date, "%c", localtime(&entry.timestamp)) != 0);

        printf("%2zu | %-*s | %-*zu | %s\n", i_entry+1,
            (int)(sizeof entry.name - sizeof""),
            entry.name,
            SCORE_FIELD_WIDTH,
            (size_t)entry.score,
            date);
    }
    puts("-----------------------------------------------------------------");
    puts("");
}

static void print_leaderboard(
    LeaderBoardEntry leaderboard[LEADERBOARD_MAX_LENGTH][BASE_LENGTH][BASE_LENGTH],
    size_t           leaderboard_length)
{
    if (leaderboard_length == 0) {
        gp_println("No leaderboard data to show.");
        return;
    }

    puts("\n-----------------------------------------------------------------");
    puts("    HEXGAME LEADERBOARD");
    puts("-----------------------------------------------------------------\n");

    for (base_t left_base = 0; left_base < BASE_LENGTH; ++left_base)
        for (base_t right_base = 0; right_base < BASE_LENGTH; ++right_base)
            if (left_base == right_base)
                continue;
            else
                print_leaderboard_entry(
                    leaderboard, leaderboard_length, left_base, right_base);
    print_leaderboard_entry(leaderboard, leaderboard_length, 0, 0);
}

static void print_score(
    score_t score,
    base_t  left_base,
    base_t  right_base,
    int     high_score_rank)
{
    char round_name[128];

    size_t longest = 0;
    for (size_t base = 0; base < BASE_LENGTH; ++base)
        if (strlen(base_lowercase[base]) > longest)
            longest = strlen(base_lowercase[base]);
    const int round_name_width = 2*longest + (sizeof" to "-1);

    if (left_base == 0 && right_base == 0)
        strcpy(round_name, "All Rounds Total");
    else
        strcat(strcat(strcpy(
            round_name, base_titlecase[left_base]), " to "), base_titlecase[right_base]);

    printf("%-*s : %*zu ", round_name_width, round_name, SCORE_FIELD_WIDTH, (size_t)score);
    if (high_score_rank) {
        printf("(top %i!) ", high_score_rank);
        const char* medals[] = {"", "🥇", "🥈", "🥉"};
        if (high_score_rank <= 3)
            printf("%s", medals[high_score_rank]);
    }
    puts("");
}

int main(int argc, char** argv, char** envp)
{
    // Overlapping indices are empty, so we'll use [0][0] for sum. Also, user
    // has unlimited time for the last question, so they are guaranteed to have
    // at least one point.
    score_t scores[BASE_LENGTH][BASE_LENGTH] = {0};
    LeaderBoardEntry leaderboard[LEADERBOARD_MAX_LENGTH][BASE_LENGTH][BASE_LENGTH] = {0};
    size_t leaderboard_length = 0;
    int leaderboard_fd = -1;

    // --------------------------------
    #if _WIN32 // Enable ANSI Colors
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(console, &mode);
    SetConsoleMode(console, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    #endif

    // --------------------------------
    // Create/Read Leaderboard

    const char* home_path = NULL;
    for (char** home_ptr = envp; *home_ptr != NULL; ++home_ptr) {
        if (strncmp(*home_ptr, "HOME=", sizeof"HOME="-1) == 0) {
            home_path = *home_ptr + sizeof"HOME="-1;
            break;
        }
    }
    gp_assert(home_path != NULL, "HOME environment variable not set.");

    // We'll ignore pedantic bounds checks for now
    char leaderboard_path[4096];
    strcpy(leaderboard_path, home_path);
    strcat(leaderboard_path, "/.hexgame");

    #if _WIN32
    #define mkdir(A, ...) mkdir(A)
    #endif
    if (access(leaderboard_path, F_OK) == -1 && mkdir(leaderboard_path, 0766) == -1)
        gp_file_println(stderr,
            "hexgame: cannot create", leaderboard_path, "for leaderboards:",
            strerror(errno));

    else if (access(strcat(leaderboard_path, "/leaderboard.bin"), F_OK) != -1 &&
        (leaderboard_fd = open(leaderboard_path, O_RDONLY)) == -1)

        fprintf(stderr,
            "hexgame: cannot open %s: %s\n", leaderboard_path, strerror(errno));
    else if (leaderboard_fd != -1) {
        leaderboard_length = read(leaderboard_fd, leaderboard, sizeof leaderboard);
        if (leaderboard_length == (size_t)-1)
            gp_file_println(stderr,
                "hexgame: could not read leaderboard data from %s:",
                leaderboard_path,
                strerror(errno));
        else
            leaderboard_length /= sizeof leaderboard[0];

        gp_assert(close(leaderboard_fd) != -1, strerror(errno));
    }

    // --------------------------------
    // Check Arguments

    if (argc == 2 && strcmp(argv[1], "leaderboard") == 0) {
        print_leaderboard(leaderboard, leaderboard_length);
        exit(EXIT_SUCCESS);
    } else if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        gp_println("hexgame: pass no arguments to play or 'leaderboard' to show leaderboard.");
        exit(EXIT_SUCCESS);
    } else if (argc != 1) {
        gp_file_println(stderr, "hexgame: pass no arguments to play or 'leaderboard' to show leaderboard.");
        exit(EXIT_FAILURE);
    }

    // --------------------------------
    // Start Game

    HighScorePosition new_high_scores[BASE_COMBINATIONS + 1]; // +1 for total

    size_t new_high_scores_length = 0;

    puts(header);
    size_t round = 0;
    for (base_t left_base = 0; left_base < BASE_LENGTH; ++left_base)
    {
        for (base_t right_base = 0; right_base < BASE_LENGTH; ++right_base, ++round)
        {
            if (left_base == right_base)
                continue;

            scores[0][0] += scores[left_base][right_base] = game(round, left_base, right_base);

            for (size_t i = 0; i < LEADERBOARD_MAX_LENGTH; ++i) {
                if (scores[left_base][right_base] >= leaderboard[i][left_base][right_base].score) {
                    new_high_scores[new_high_scores_length++] = (HighScorePosition)
                        { left_base, right_base, i };
                    break;
                }
            }
        }
    }
    for (size_t i = 0; i < LEADERBOARD_MAX_LENGTH; ++i) {
        if (scores[0][0] >= leaderboard[i][0][0].score) {
            new_high_scores[new_high_scores_length++] = (HighScorePosition){0,0,i};
            break;
        }
    }
    time_t timestamp = time(NULL);

    // --------------------------------
    // Update Leaderboard

    char nick[128] = "";

    if (new_high_scores_length > 0) {
        try_again:;
        printf("Enter name (max %zu bytes): ", sizeof leaderboard[0][0][0].name);
        fflush(stdout);
        read_input("%127s", nick);

        if (strlen(nick) > sizeof leaderboard[0][0][0].name) {
            printf("Name too long (%zu bytes).\n", strlen(nick));
            goto try_again;
        }
    }

    for (size_t i = 0; i < new_high_scores_length; ++i)
    {
        HighScorePosition hi_score = new_high_scores[i];

        for (size_t j = leaderboard_length - 1; j+1 >= hi_score.position+1; --j)
            if (j == LEADERBOARD_MAX_LENGTH - 1)
                continue;
            else
                leaderboard[j + 1][hi_score.left_base][hi_score.right_base] =
                leaderboard[j + 0][hi_score.left_base][hi_score.right_base] ;

        LeaderBoardEntry new_entry = { "", timestamp, scores[hi_score.left_base][hi_score.right_base] };
        strcpy(new_entry.name, nick);
        leaderboard[hi_score.position][hi_score.left_base][hi_score.right_base] = new_entry;
    }
    bool should_update_leaderboard = new_high_scores_length > 0;
    if (leaderboard_length < LEADERBOARD_MAX_LENGTH) {
        should_update_leaderboard = true;
        ++leaderboard_length;
    }

    if (should_update_leaderboard)
    {
        if ((leaderboard_fd = open(leaderboard_path, O_CREAT | O_WRONLY, 0644)) == -1)
            fprintf(stderr, "hexgame: could not open %s: %s\n", leaderboard_path, strerror(errno));
        else if (write(leaderboard_fd, leaderboard, leaderboard_length * sizeof leaderboard[0]) == -1)
            fprintf(stderr, "hexgame: could not write to %s: %s\n", leaderboard_path, strerror(errno));

        if (leaderboard_fd != -1)
            gp_assert(close(leaderboard_fd) != -1, strerror(errno));
    }

    // --------------------------------
    // Print Results

    int high_score_ranks[BASE_LENGTH][BASE_LENGTH] = {0};
    for (size_t i = 0; i < new_high_scores_length; ++i)
        high_score_ranks[new_high_scores[i].left_base][new_high_scores[i].right_base] =
            new_high_scores[i].position + 1;

    print_leaderboard(leaderboard, leaderboard_length);

    if (new_high_scores_length > 0)
        gp_println(GP_GREEN "Got", new_high_scores_length, "new high scores!" GP_RESET_TERMINAL);
    puts("-------------------------------------------------");
    puts("Your Scores:");
    puts("-------------------------------------------------");
    for (base_t left_base = 0; left_base < BASE_LENGTH; ++left_base)
        for (base_t right_base = 0; right_base < BASE_LENGTH; ++right_base)
            if (left_base == right_base)
                continue;
            else
                print_score(
                    scores[left_base][right_base],
                    left_base, right_base,
                    high_score_ranks[left_base][right_base]);
    print_score(scores[0][0], 0, 0, high_score_ranks[0][0]);
    puts("");
}
