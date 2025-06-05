// ======================
// ANSI 转义序列颜色宏
// ======================

// 重置所有样式
#define CONSOLE_RESET          "\033[0m"

// 亮色前景色（90-97）                                // 标准前景色（30-37）
#define CONSOLE_BRIGHT_BLACK   "\033[90m"          // #define CONSOLE_BLACK        "\033[30m"
#define CONSOLE_BRIGHT_RED     "\033[91m"          // #define CONSOLE_RED          "\033[31m"
#define CONSOLE_BRIGHT_GREEN   "\033[92m"          // #define CONSOLE_GREEN        "\033[32m"
#define CONSOLE_BRIGHT_YELLOW  "\033[93m"          // #define CONSOLE_YELLOW       "\033[33m"
#define CONSOLE_BRIGHT_BLUE    "\033[94m"          // #define CONSOLE_BLUE         "\033[34m"
#define CONSOLE_BRIGHT_MAGENTA "\033[95m"          // #define CONSOLE_MAGENTA      "\033[35m"
#define CONSOLE_BRIGHT_CYAN    "\033[96m"          // #define CONSOLE_CYAN         "\033[36m"
#define CONSOLE_BRIGHT_WHITE   "\033[97m"          // #define CONSOLE_WHITE        "\033[37m"

// 文字样式
#define CONSOLE_BOLD           "\033[1m"    // 粗体
#define CONSOLE_DIM            "\033[2m"    // 暗淡
#define CONSOLE_ITALIC         "\033[3m"    // 斜体
#define CONSOLE_UNDERLINE      "\033[4m"    // 下划线
#define CONSOLE_BLINK          "\033[5m"    // 闪烁
#define CONSOLE_REVERSE        "\033[7m"    // 反色
#define CONSOLE_HIDDEN         "\033[8m"    // 隐藏

// 高贵预定义
#define CONSOLE_256_FG(n)      "\033[38;5;" #n "m"                 // 256色模式（前景）
#define CONSOLE_RGB_FG(r,g,b)  "\033[38;2;" #r ";" #g ";" #b "m"   // RGB真彩色模式（前景）
