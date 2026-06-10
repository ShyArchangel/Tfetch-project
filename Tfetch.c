#include <linux/sysinfo.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>

#define BUFFER_SIZE 1024

void print_memory(char *out)
{
    out[0] = '\0';
    char buffer[BUFFER_SIZE];
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) return;

    unsigned long total_kb = 0;
    unsigned long avab_kb = 0;

    while (fgets(buffer, sizeof(buffer), fp))
    {
        if (strncmp(buffer, "MemTotal:", strlen("MemTotal:")) == 0) sscanf(buffer + strlen("MemTotal:"), "%lu", &total_kb);
        if (strncmp(buffer, "MemAvailable:", strlen("MemAvailable:")) == 0) sscanf(buffer + strlen("MemAvailable:"), "%lu", &avab_kb);
    }
    fclose(fp);

    if (total_kb > 0 && avab_kb > 0)
    {
        unsigned long used_kb = total_kb - avab_kb;

        double total_gb = (double)total_kb / 1024.0 / 1024.0;
        double used_gb = (double)used_kb / 1024.0 / 1024.0;
        
        snprintf(out, 256, "Memory: %.2f GiB / %.2f GiB", used_gb, total_gb);
    }
}

void get_archit(char *res_buf, size_t max_size)
{
    struct utsname sys_info;
    if (uname(&sys_info) == 0) snprintf(res_buf, max_size, "%s", sys_info.machine);
    else snprintf(res_buf, max_size, "unknown");
}

void print_os(char *out, char *ext)
{
    out[0] = '\0';
    char buffer[BUFFER_SIZE];
    FILE *fp = fopen("/etc/os-release", "r");
    if (fp == NULL) return;

    while (fgets(buffer, sizeof(buffer), fp))
    {
        if (strncmp(buffer, "NAME=", strlen("NAME=")) == 0)
        {
            char *os_name = buffer + strlen("NAME=");

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
}

void print_cpu(char *out)
{
    out[0] = '\0';
    char buffer[BUFFER_SIZE];
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL) return;

    while(fgets(buffer, sizeof(buffer), fp))
    {
        if (strncmp(buffer, "model name", strlen("model name")) == 0)
        {
            char *cpu_name = strchr(buffer, ':');
            if (cpu_name != NULL)
            {
                cpu_name++;
                while (*cpu_name == ' ') cpu_name++;

                int len = strlen(cpu_name);
                if (len > 0 && cpu_name[len - 1] == '\n') cpu_name[len - 1] = '\0';

                snprintf(out, 256, "CPU: %s", cpu_name);
                break;
            }
        }
    }
    fclose(fp);
}

void print_gpu(char *out)
{
    out[0] = '\0';
    int fd[2];
    char buffer[BUFFER_SIZE];

    if (pipe(fd) == -1) return;

    pid_t pid = fork();

    if (pid == 0)
    {
        close(fd[0]);

        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);

        char *args[] = {"lspci", NULL};
        execvp(args[0], args);

        _exit(1);
    }
    else 
    {
        close(fd[1]);
        FILE *fp = fdopen(fd[0], "r");

        while(fgets(buffer, sizeof(buffer), fp))
        {
            if (strstr(buffer, "VGA") != NULL || strstr(buffer, "3D") != NULL)
            {
                char *gpu_name = strchr(buffer, ':');
                if (gpu_name != NULL)
                {
                    gpu_name++;
                    gpu_name = strchr(gpu_name, ':');
                    if (gpu_name != NULL)
                    {
                        gpu_name++;
                        while (*gpu_name == ' ') gpu_name++;
                        int len = strlen(gpu_name);
                        if (len > 0 && gpu_name[len - 1] == '\n') gpu_name[len - 1] = '\0';

                        snprintf(out, 256, "GPU: %s", gpu_name);
                        break;
                    }
                }
            } 
        }
        fclose(fp);
        wait(NULL);
    }
}

void print_host(char *out)
{
    out[0] = '\0';
    char buffer[BUFFER_SIZE];
    FILE *fp = fopen("/sys/class/dmi/id/board_name", "r");
    if (fp == NULL) return;
    fgets(buffer, sizeof(buffer), fp);
    int len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';
    snprintf(out, 256, "Host: %s", buffer);
    fclose(fp);
}

void print_kernel(char *out)
{
    out[0] = '\0';
    struct utsname sys_info;

    if (uname(&sys_info) == 0)
    {
        snprintf(out, 256, "Kernel: %s", sys_info.release);
    }
}

void print_uptime(char *out)
{
    out[0] = '\0';
    struct sysinfo info;

    if (sysinfo(&info) == 0)
    {
        long uptime = info.uptime;

        long days = uptime / 86400;
        uptime = uptime % 86400;
        long hours = uptime / 3600;
        uptime = uptime % 3600;
        long minutes = uptime / 60;

        if (days > 0) snprintf(out, 256, "Uptime: %ld days, %ld hours, %ld minutes", days, hours, minutes);
        else if (hours > 0) snprintf(out, 256, "Uptime: %ld hours, %ld minutes", hours, minutes);
        else if (minutes > 0) snprintf(out, 256, "Uptime: %ld minutes", minutes);
        else snprintf(out, 256, "Uptime: %ld seconds", uptime);
    }
}

void print_de(char *out)
{
    out[0] = '\0';
    char *de = getenv("XDG_CURRENT_DESKTOP");
    if (de == NULL) de = getenv("XDG_SESSION_DESKTOP");
    if (de == NULL) de = getenv("DESKTOP_SESSION");

    if (de != NULL) snprintf(out, 256, "DE/WM: %s", de);
    else snprintf(out, 256, "DE/WM: Unknown");
}

void print_term(char *out)
{
    out[0] = '\0';
    char *term = getenv("COLORTERM");
    if (term == NULL || strcmp(term, "truecolor") == 0) term = getenv("TERM");

    if (term != NULL) snprintf(out, 256, "Terminal: %s", term);
    else snprintf(out, 256, "Terminal: Unknown");
}

void print_shell(char *out)
{
    out[0] = '\0';
    int fd[2];
    char buffer[BUFFER_SIZE];

    if (pipe(fd) == -1) return;

    pid_t pid = fork();
    if (pid == -1) return;

    if (pid == 0)
    {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);

        char *args[] = {"bash", "--version", NULL};
        execvp(args[0], args);
        _exit(1);
    }
    else 
    {
        close(fd[1]);
        FILE *fp = fdopen(fd[0], "r");

        if (fgets(buffer, sizeof(buffer), fp))
        {
            char *version = strstr(buffer, "version ");
            if (version != NULL)
            {
                version += strlen("version ");
                char *version_end = strchr(version, ' ');
                if (version_end != NULL) *version_end = '\0';
            }
            int len = strlen(version);
            if (len > 0 && version[len - 1] == '\n') version[len - 1] = '\0';
            snprintf(out, 256, "Shell: bash %s", version);
        }
        fclose(fp);
        wait(NULL);
    }
}

void print_resolution(char *out)
{
    out[0] = '\0';
    int fd[2];
    char buffer[BUFFER_SIZE];

    if (pipe(fd) == -1) return;
    pid_t pid = fork();
    if (pid == -1) return;

    if (pid == 0)
    {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);

        int devnull = open("/dev/null", O_WRONLY);
        if (devnull != -1)
        {
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        close(fd[1]);

        char *display_server = getenv("XDG_SESSION_TYPE");
        if (display_server != NULL && strcmp(display_server, "wayland") == 0)
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
        close(fd[1]);
        FILE *fp = fdopen(fd[0], "r");

        int count_monitor = 1;
        char out_str[512] = "";

        while (fgets(buffer, sizeof(buffer), fp))
        {
            if (strstr(buffer, "connected") != NULL)
            {
                char *pos = strstr(buffer, "connected");
                while (*pos && !(*pos >= '0' && *pos <= '9')) pos++;

                if (*pos)
                {
                    char *res_end = pos;
                    while (*res_end && *res_end != ' ' && *res_end != '+' && *res_end != '\n') res_end++;
                    *res_end = '\0';

                    char temp[128];
                    snprintf(temp, sizeof(temp), "%s (Screen %d), ", pos, count_monitor++);
                    strcat(out_str, temp);
                }
            }
            else if (strstr(buffer, "current") != NULL)
            {
                char *pos = buffer;
                while (*pos && !(*pos >= '0' && *pos <= '9')) pos++; 

                char *res_end = strstr(pos, " px");
                if (res_end != NULL)
                {
                    *res_end = '\0';
                    char temp[128];
                    snprintf(temp, sizeof(temp), "%s (Screen %d), ", pos, count_monitor++);
                    strcat(out_str, temp);
                }
            }
        }
        fclose(fp);
        wait(NULL);
        if (strlen(out_str) > 0)
        {
            out_str[strlen(out_str) - 2] = '\0';
            snprintf(out, 256, "Resolution: %s", out_str);
        }
        else snprintf(out, 256, "Resolution: Unknown");
    }
}

void print_packages(char *out)
{
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
}

void print_banner(char *out_banner, char *out_underline)
{
    out_banner[0] = '\0';
    out_underline[0] = '\0';
    char *user = getenv("USER");
    if (user == NULL) user = "user";

    char os_name[64] = "Linux";
    FILE *fp = fopen("/etc/os-release", "r");
    if (fp != NULL)
    {
        char line[256];
        while (fgets(line, sizeof(line), fp))
        {
            if (strncmp(line, "NAME=", strlen("NAME=")) == 0)
            {
                char *start = line + strlen("NAME=");
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
    snprintf(out_banner, 256, "%s@%s", user, os_name);
    int len = strlen(out_banner);

    int i;
    for (i = 0; i < len; i++) out_underline[i] = '-';
    out_underline[len] = '\0';
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
        const char *art = (i < 10) ? ascii[i] : "                  "; //тернарный оператор, логика выглядит так: условие ? значение если условие выполняется : значение если условие не выполняется
        /*
        const char *art;
        if (i < 10) art = ascii[i]; Так бы выглядело без этого '?'
        else art = "                  ";
        */
        snprintf(line_out, sizeof(line_out), "%s  %s\n", art, info[i]);
        write(1, line_out, strlen(line_out));
    }
    return 0;
}