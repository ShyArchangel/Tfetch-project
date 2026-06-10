#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#ifdef __linux__
    #include <linux/sysinfo.h>
    #include <sys/utsname.h>
    #include <sys/sysinfo.h>
    #include <sys/wait.h>
    #include <fcntl.h>
    #include <dirent.h>
    //gcc Tfetchcross.c -o Tfetch - для компиляции
#elif defined(_WIN32)
    #define _WIN32_WINNT 0x0600
    #include <windows.h>
    //x86_64-w64-mingw32-gcc Tfetchcross.c -o Tfetch.exe -D_WIN32_WINNT=0x0600 -ladvapi32 -luser32 - для компиляции
    //gcc Tfetchcross.c -o Tfetch -D_WIN32_WINNT=0x0600 -ladvapi32 -luser32
#elif defined(__APPLE__)
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <mach/mach.h>
    #include <sys/utsname.h>
    //zig cc -target aarch64-macos Tfetchcross.c -o Tfetch_mac_m1 - для компиляции на процы (M1, M2, M3)
    //zig cc -target x86_64-macos Tfetchcross.c -o Tfetch_mac_intel - для компиляции на процы (Intel)
#endif

#define BUFFER_SIZE 1024

void print_banner(char *out_banner, char *out_underline)
{
    out_banner[0] = '\0';
    out_underline[0] = '\0';
    
    char *user;
    #ifdef _WIN32
        user = getenv("USERNAME");
    #else
        user = getenv("USER");
    #endif
    
    if (user == NULL) user = "user";

    char os_name[64] = "Unknown OS";

    #ifdef __linux__
        FILE *fp = fopen("/etc/os-release", "r");
        if (fp != NULL) {
            char line[256];
            while (fgets(line, sizeof(line), fp)) 
            {
                if (strncmp(line, "NAME=", 5) == 0) 
                {
                    char *start = line + 5;
                    if (*start == '"') start++;
                    char *end = start;
                    while (*end && *end != '"' && *end != '\n') end++;
                    *end = '\0';
                    strncpy(os_name, start, sizeof(os_name) - 1);
                    break;
                }
            }
            fclose(fp);
        }
    #elif defined(_WIN32)
        strncpy(os_name, "Windows", sizeof(os_name));
    #elif defined(__APPLE__)
        strncpy(os_name, "macOS", sizeof(os_name));
    #endif

    snprintf(out_banner, 256, "%s@%s", user, os_name);
    int len = strlen(out_banner);

    int i;
    for (i = 0; i < len; i++) out_underline[i] = '-';
    out_underline[len] = '\0';
}
void get_archit(char *res_buf, size_t max_size)
{
    #ifdef __linux__
        struct utsname sys_info;
        if (uname(&sys_info) == 0) snprintf(res_buf, max_size, "%s", sys_info.machine);
        else snprintf(res_buf, max_size, "unknown");
    #elif defined(_WIN32)
        SYSTEM_INFO sysInfo;
        GetNativeSystemInfo(&sysInfo);
        if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) snprintf(res_buf, max_size, "x86_64");
        else if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) snprintf(res_buf, max_size, "arm64");
        else snprintf(res_buf, max_size, "x86");
    #elif defined(__APPLE__)
        size_t size = max_size;
        sysctlbyname("hw.machine", res_buf, &size, NULL, 0);
    #endif
}
void print_os(char *out, char *ext)
{
    out[0] = '\0';
    #ifdef __linux__
        char buffer[BUFFER_SIZE];
        FILE *fp = fopen("/etc/os-release", "r");
        if (fp == NULL) return;
        while (fgets(buffer, sizeof(buffer), fp)) 
        {
            if (strncmp(buffer, "NAME=", 5) == 0) 
            {
                char *os_name = buffer + 5;
                if (*os_name == '"') os_name++;
                int len = strlen(os_name);
                if (len > 0 && os_name[len - 1] == '\n') os_name[len - 1] = '\0';
                len = strlen(os_name);
                if (len > 0 && os_name[len - 1] == '"') os_name[len - 1] = '\0';
                snprintf(out, 256, "OS: %s %s", os_name, ext);
                break;
            }
        }
        fclose(fp);
    #elif defined(_WIN32)
        snprintf(out, 256, "OS: Windows %s", ext);
    #elif defined(__APPLE__)
        snprintf(out, 256, "OS: macOS %s", ext);
    #endif
}
void print_uptime(char *out)
{
    out[0] = '\0';
    long uptime_sec = 0;

    #ifdef __linux__
        struct sysinfo info;
        if (sysinfo(&info) == 0) uptime_sec = info.uptime;
    #elif defined(_WIN32)
        uptime_sec = GetTickCount64() / 1000;
    #elif defined(__APPLE__)
        struct timeval boottime;
        size_t len = sizeof(boottime);
        int mib[2] = {CTL_KERN, KERN_BOOTTIME};
        if (sysctl(mib, 2, &boottime, &len, NULL, 0) == 0) 
        {
            time_t bsec = boottime.tv_sec, csec = time(NULL);
            uptime_sec = difftime(csec, bsec);
        }
    #endif

    if (uptime_sec > 0) 
    {
        long days = uptime_sec / 86400;
        uptime_sec = uptime_sec % 86400;
        long hours = uptime_sec / 3600;
        uptime_sec = uptime_sec % 3600;
        long minutes = uptime_sec / 60;

        if (days > 0) snprintf(out, 256, "Uptime: %ld days, %ld hours, %ld mins", days, hours, minutes);
        else if (hours > 0) snprintf(out, 256, "Uptime: %ld hours, %ld mins", hours, minutes);
        else if (minutes > 0) snprintf(out, 256, "Uptime: %ld mins", minutes);
        else snprintf(out, 256, "Uptime: %ld seconds", uptime_sec);
    } 
    else snprintf(out, 256, "Uptime: Unknown");
}
void print_memory(char *out)
{
    out[0] = '\0';
    #ifdef __linux__
        char buffer[BUFFER_SIZE];
        FILE *fp = fopen("/proc/meminfo", "r");
        if (fp == NULL) return;
        unsigned long total_kb = 0, avab_kb = 0;
        while (fgets(buffer, sizeof(buffer), fp)) 
        {
            if (strncmp(buffer, "MemTotal:", 9) == 0) sscanf(buffer + 9, "%lu", &total_kb);
            if (strncmp(buffer, "MemAvailable:", 13) == 0) sscanf(buffer + 13, "%lu", &avab_kb);
        }
        fclose(fp);
        if (total_kb > 0 && avab_kb > 0) 
        {
            double total_gb = (double)total_kb / 1024.0 / 1024.0;
            double used_gb = (double)(total_kb - avab_kb) / 1024.0 / 1024.0;
            snprintf(out, 256, "Memory: %.2f GiB / %.2f GiB", used_gb, total_gb);
        }
    #elif defined(_WIN32)
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        double total_gb = (double)memInfo.ullTotalPhys / (1024 * 1024 * 1024);
        double used_gb = (double)(memInfo.ullTotalPhys - memInfo.ullAvailPhys) / (1024 * 1024 * 1024);
        snprintf(out, 256, "Memory: %.2f GiB / %.2f GiB", used_gb, total_gb);
    #elif defined(__APPLE__)
        int64_t memsize;
        size_t len = sizeof(memsize);
        sysctlbyname("hw.memsize", &memsize, &len, NULL, 0);
        double total_gb = (double)memsize / (1024 * 1024 * 1024);
        snprintf(out, 256, "Memory: Total %.2f GiB", total_gb);
    #endif
}
void print_host(char *out) 
{
    #ifdef __linux__
        out[0] = '\0';
        char buffer[BUFFER_SIZE];
        FILE *fp = fopen("/sys/class/dmi/id/board_name", "r");
        if (fp != NULL) 
        {
            fgets(buffer, sizeof(buffer), fp);
            int len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';
            snprintf(out, 256, "Host: %s", buffer);
            fclose(fp);
        }
    #else
        snprintf(out, 256, "Host: PC");
    #endif
}
void print_packages(char *out) 
{
    #ifdef __linux__
        out[0] = '\0';
        int count = 0;

        if (access("/usr/bin/pacman", F_OK) == 0)
        {
            DIR *d = opendir("/var/lib/pacman/local");
            if (d != NULL)
            {
                struct dirent *dir;
                while ((dir = readdir(d)) != NULL) if (dir->d_name[0] != '.') count++;
                closedir(d);
                snprintf(out, 256, "Packages: %d (pacman)", count);
                return;
            }
        }
        if (access("/usr/bin/dpkg", F_OK) == 0)
        {
            FILE *fp = popen("dpkg-query -f '.\\n' -W | wc -l", "r");
            if (fp != NULL)
            {
                char buffer[64];
                if (fgets(buffer, sizeof(buffer), fp))
                {
                    count = atoi(buffer);
                    snprintf(out, 256, "Packages: %d (dpkg)", count);
                }
                pclose(fp);
                return;
            }
        }
        snprintf(out, 256, "Packages: Unknown");
    #elif defined(_WIN32)
        snprintf(out, 256, "Packages: winget/choco");
    #elif defined(__APPLE__)
        snprintf(out, 256, "Packages: brew");
    #endif
}
void print_cpu(char *out) 
{
    out[0] = '\0';
    #ifdef __linux__
    char buffer[BUFFER_SIZE];
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) { snprintf(out, 256, "CPU: Unknown"); return; }
    while(fgets(buffer, sizeof(buffer), fp)) 
    {
        if (strncmp(buffer, "model name", 10) == 0) 
        {
            char *cpu_name = strchr(buffer, ':');
            if (cpu_name) 
            {
                cpu_name++; while (*cpu_name == ' ') cpu_name++;
                int len = strlen(cpu_name);
                if (len > 0 && cpu_name[len - 1] == '\n') cpu_name[len - 1] = '\0';
                snprintf(out, 256, "CPU: %s", cpu_name);
                fclose(fp); return;
            }
        }
    }
    fclose(fp);
    snprintf(out, 256, "CPU: Unknown");
    #elif defined(_WIN32)
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) 
        {
            char cpu_name[128];
            DWORD bufferSize = sizeof(cpu_name);
            if (RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL, (LPBYTE)cpu_name, &bufferSize) == ERROR_SUCCESS) 
            {
                snprintf(out, 256, "CPU: %s", cpu_name);
            } 
            else snprintf(out, 256, "CPU: Unknown");
            RegCloseKey(hKey);
        } 
        else snprintf(out, 256, "CPU: Unknown");
    #elif defined(__APPLE__)
        char cpu_name[128];
        size_t size = sizeof(cpu_name);
        if (sysctlbyname("machdep.cpu.brand_string", &cpu_name, &size, NULL, 0) == 0) 
        {
            snprintf(out, 256, "CPU: %s", cpu_name);
        } 
        else snprintf(out, 256, "CPU: Unknown");
    #endif
}
void print_gpu(char *out) 
{
    out[0] = '\0';
#ifdef __linux__
    int fd[2]; char buffer[BUFFER_SIZE];
    if (pipe(fd) == -1) { snprintf(out, 256, "GPU: Unknown"); return; }
    pid_t pid = fork();
    if (pid == 0) 
    {
        close(fd[0]); dup2(fd[1], STDOUT_FILENO); close(fd[1]);
        char *args[] = {"lspci", NULL};
        execvp(args[0], args);
        _exit(1);
    } else {
        close(fd[1]); FILE *fp = fdopen(fd[0], "r");
        int found = 0;
        while(fgets(buffer, sizeof(buffer), fp)) {
            if (strstr(buffer, "VGA") != NULL || strstr(buffer, "3D") != NULL) 
            {
                char *gpu_name = strchr(buffer, ':');
                if (gpu_name != NULL) 
                {
                    gpu_name = strchr(gpu_name + 1, ':');
                    if (gpu_name != NULL) 
                    {
                        gpu_name++; while (*gpu_name == ' ') gpu_name++;
                        int len = strlen(gpu_name);
                        if (len > 0 && gpu_name[len - 1] == '\n') gpu_name[len - 1] = '\0';
                        snprintf(out, 256, "GPU: %s", gpu_name);
                        found = 1; break;
                    }
                }
            } 
        }
        fclose(fp); wait(NULL);
        if (!found) snprintf(out, 256, "GPU: Unknown");
    }
#elif defined(_WIN32)
    snprintf(out, 256, "GPU: Windows DXGI Device");
#elif defined(__APPLE__)
    snprintf(out, 256, "GPU: Apple Metal Device");
#endif
}
void print_kernel(char *out) 
{
    out[0] = '\0';
#if defined(__linux__) || defined(__APPLE__)
    struct utsname sys_info;
    if (uname(&sys_info) == 0) snprintf(out, 256, "Kernel: %s", sys_info.release);
    else snprintf(out, 256, "Kernel: Unknown");
#elif defined(_WIN32)
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS) 
    {
        char build[64];
        DWORD bufferSize = sizeof(build);
        if (RegQueryValueExA(hKey, "CurrentBuild", NULL, NULL, (LPBYTE)build, &bufferSize) == ERROR_SUCCESS) 
        {
            snprintf(out, 256, "Kernel: Windows NT Build %s", build);
        } 
        else snprintf(out, 256, "Kernel: Windows NT");
        RegCloseKey(hKey);
    } 
    else snprintf(out, 256, "Kernel: Windows NT");
#endif
}
void print_de(char *out) 
{
    out[0] = '\0';
#ifdef __linux__
    char *de = getenv("XDG_CURRENT_DESKTOP");
    if (!de) de = getenv("XDG_SESSION_DESKTOP");
    if (!de) de = getenv("DESKTOP_SESSION");
    if (de) snprintf(out, 256, "DE/WM: %s", de);
    else snprintf(out, 256, "DE/WM: Unknown");
#elif defined(_WIN32)
    snprintf(out, 256, "DE/WM: Windows Explorer");
#elif defined(__APPLE__)
    snprintf(out, 256, "DE/WM: Aqua");
#endif
}
void print_term(char *out) 
{
    out[0] = '\0';
    char *term = getenv("TERM_PROGRAM");
    if (!term) term = getenv("COLORTERM");
    if (!term || strcmp(term, "truecolor") == 0) term = getenv("TERM");

    if (term) snprintf(out, 256, "Terminal: %s", term);
    else 
    {
#ifdef _WIN32
        snprintf(out, 256, "Terminal: Windows Console");
#else
        snprintf(out, 256, "Terminal: Unknown");
#endif
    }
}
void print_shell(char *out) 
{
    out[0] = '\0';
#ifdef __linux__
    int fd[2]; char buffer[BUFFER_SIZE];
    if (pipe(fd) == -1) { snprintf(out, 256, "Shell: Unknown"); return; }
    pid_t pid = fork();
    if (pid == 0) 
    {
        close(fd[0]); dup2(fd[1], STDOUT_FILENO); close(fd[1]);
        char *args[] = {"bash", "--version", NULL};
        execvp(args[0], args); _exit(1);
    } 
    else 
    {
        close(fd[1]); FILE *fp = fdopen(fd[0], "r");
        int found = 0;
        if (fgets(buffer, sizeof(buffer), fp)) 
        {
            char *ver = strstr(buffer, "version ");
            if (ver) 
            {
                ver += 8;
                char *end = strchr(ver, ' ');
                if (end) *end = '\0';
                int len = strlen(ver);
                if (len > 0 && ver[len - 1] == '\n') ver[len - 1] = '\0';
                snprintf(out, 256, "Shell: bash %s", ver);
                found = 1;
            }
        }
        fclose(fp); wait(NULL);
        if (!found) snprintf(out, 256, "Shell: Unknown");
    }
#elif defined(_WIN32)
    char *comspec = getenv("COMSPEC");
    if (comspec) 
    {
        char *name = strrchr(comspec, '\\');
        if (name) name++; else name = comspec;
        snprintf(out, 256, "Shell: %s", name);
    } 
    else snprintf(out, 256, "Shell: cmd.exe");
#elif defined(__APPLE__)
    char *shell = getenv("SHELL");
    if (shell) 
    {
        char *name = strrchr(shell, '/');
        if (name) name++; else name = shell;
        snprintf(out, 256, "Shell: %s", name);
    } 
    else snprintf(out, 256, "Shell: zsh");
#endif
}
void print_resolution(char *out) {
    out[0] = '\0';
#ifdef __linux__
    int fd[2]; char buffer[BUFFER_SIZE];
    if (pipe(fd) == -1) { snprintf(out, 256, "Resolution: Unknown"); return; }
    pid_t pid = fork();
    if (pid == 0) 
    {
        close(fd[0]); dup2(fd[1], STDOUT_FILENO);
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull != -1) { dup2(devnull, STDERR_FILENO); close(devnull); }
        close(fd[1]);
        char *ds = getenv("XDG_SESSION_TYPE");
        if (ds && strcmp(ds, "wayland") == 0) 
        {
            char *args[] = {"wlr-randr", NULL};
            execvp(args[0], args);
        }
        char *args[] = {"xrandr", "--current", NULL};
        execvp(args[0], args);
        _exit(1);
    } 
    else 
    {
        close(fd[1]); FILE *fp = fdopen(fd[0], "r");
        int count_monitor = 1; char out_str[512] = "";
        while (fgets(buffer, sizeof(buffer), fp)) 
        {
            if (strstr(buffer, "connected") && !strstr(buffer, "disconnected")) 
            {
                char *pos = strstr(buffer, "connected");
                while (*pos && !(*pos >= '0' && *pos <= '9')) pos++;
                if (*pos) 
                {
                    char *res_end = pos;
                    while (*res_end && *res_end != ' ' && *res_end != '+' && *res_end != '\n') res_end++;
                    *res_end = '\0';
                    char temp[128]; snprintf(temp, sizeof(temp), "%s (Screen %d), ", pos, count_monitor++);
                    strcat(out_str, temp);
                }
            } 
            else if (strstr(buffer, "current")) 
            {
                char *pos = buffer;
                while (*pos && !(*pos >= '0' && *pos <= '9')) pos++; 
                char *res_end = strstr(pos, " px");
                if (res_end) 
                {
                    *res_end = '\0';
                    char temp[128]; snprintf(temp, sizeof(temp), "%s (Screen %d), ", pos, count_monitor++);
                    strcat(out_str, temp);
                }
            }
        }
        fclose(fp); wait(NULL);
        if (strlen(out_str) > 0) 
        {
            out_str[strlen(out_str) - 2] = '\0';
            snprintf(out, 256, "Resolution: %s", out_str);
        } 
        else snprintf(out, 256, "Resolution: Unknown");
    }
#elif defined(_WIN32)
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    if (width > 0 && height > 0) 
    {
        snprintf(out, 256, "Resolution: %dx%d", width, height);
    } 
    else snprintf(out, 256, "Resolution: Unknown");
#elif defined(__APPLE__)
    snprintf(out, 256, "Resolution: Apple Display");
#endif
}

int main()
{
    const char *ascii[] = {
    "      //\\         ",
    "     //##\\        ",
    "    // ## \\       ",
    "   //  ##  \\      ",
    "  //________\\     ",
    "  \\        //     ",
    "   \\  ##  //      ",
    "    \\ ## //       ",
    "     \\##//        ",
    "      \\//         "
    };

    char arch[BUFFER_SIZE];
    get_archit(arch, sizeof(arch));

    char info[14][256];
    for (int i = 0; i < 14; i++) info[i][0] = '\0';

    print_banner(info[0], info[1]);
    print_os(info[2], arch);
    print_host(info[3]);
    print_kernel(info[4]);
    print_uptime(info[5]);
    print_packages(info[6]);
    print_shell(info[7]);
    print_resolution(info[8]);
    print_de(info[9]);
    print_term(info[10]);
    print_cpu(info[11]);
    print_gpu(info[12]);
    print_memory(info[13]);
    
    char line_out[512];
    for (int i = 0; i < 14; i++)
    {
        const char *art = (i < 10) ? ascii[i] : "                  "; 
        snprintf(line_out, sizeof(line_out), "%s  %s\n", art, info[i]);
        printf("%s", line_out); 
    }
    return 0;
}