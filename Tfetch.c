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

#define BUFFER_SIZE 1024 //размер под буфер или просто под массивы

void print_memory(char *out) //функция вывода памяти
{
    out[0] = '\0';
    char buffer[BUFFER_SIZE]; //создаем массив символов
    FILE *fp = fopen("/proc/meminfo", "r"); //открываем файл в котором лежит описание того, сколько у нас памяти
    if (fp == NULL) return; //проверяем на ошибку открытия

    unsigned long total_kb = 0; //заводим переменную для килобайт всего
    unsigned long avab_kb = 0; //заводим переменную для килобайт свободных

    while (fgets(buffer, sizeof(buffer), fp)) //проходимся по файлу
    {
        if (strncmp(buffer, "MemTotal:", strlen("MemTotal:")) == 0) sscanf(buffer + strlen("MemTotal:"), "%lu", &total_kb); //здесь мы делаем обращение в наш массив и спрашиваем если там строчка, потом мы выполняем запись в переменную 
        if (strncmp(buffer, "MemAvailable:", strlen("MemAvailable:")) == 0) sscanf(buffer + strlen("MemAvailable:"), "%lu", &avab_kb); //здесь по сути делаем тоже самое
    }
    fclose(fp); //закрываем файл

    if (total_kb > 0 && avab_kb > 0) //проверяем не равно ли нулю наша оперативка
    {
        unsigned long used_kb = total_kb - avab_kb; //просто узнаем сколько памяти прям щас используется

        double total_gb = (double)total_kb / 1024.0 / 1024.0; //переводим в гигабайты
        double used_gb = (double)used_kb / 1024.0 / 1024.0; //переводим в гигабайты
        
        snprintf(out, 256, "Memory: %.2f GiB / %.2f GiB", used_gb, total_gb); //собераем строчку вывода
    }
}

void get_archit(char *res_buf, size_t max_size) //здесь нам нужно узнать архитектуру
{
    struct utsname sys_info; //берем готовую структуру
    if (uname(&sys_info) == 0) snprintf(res_buf, max_size, "%s", sys_info.machine); //здесь собираем строчку вывода
    else snprintf(res_buf, max_size, "unknown"); //если вдруг что-то не так
}

void print_os(char *out, char *ext) //выводим операционку
{
    out[0] = '\0';
    char buffer[BUFFER_SIZE]; //создаем массив
    FILE *fp = fopen("/etc/os-release", "r"); //открываем файл
    if (fp == NULL) return; //если не открылся

    while (fgets(buffer, sizeof(buffer), fp)) //идем по файлу
    {
        if (strncmp(buffer, "NAME=", strlen("NAME=")) == 0) //ищем вхождение нужной строчки
        {
            char *os_name = buffer + strlen("NAME="); //создаем переменную и прибавляем отступ нам нужный

            if (*os_name == '"') os_name++; //если там есть ковычки

            int len = strlen(os_name); //длина 
            if (len > 0 && os_name[len - 1] == '\n') os_name[len - 1] = '\0'; //тут мы смотрим, если в нашем массиве os_name последный элемент это перенос строки, то заменяем на конец строки
            len = strlen(os_name); //еще раз передаем длину
            if (len > 0 && os_name[len - 1] == '"') os_name[len - 1] = '\0'; //теперь убираем ковычки

            snprintf(out, 256, "OS: %s %s", os_name, ext); //делаем строку вывода
            break;
        }
    }
    fclose(fp); //не забываем закрыть файл
}

void print_cpu(char *out) //выводим процессор
{
    out[0] = '\0';
    char buffer[BUFFER_SIZE]; //делаем массив
    FILE *fp = fopen("/proc/cpuinfo", "r"); //открываем файл
    if (fp == NULL) return; //если ошибка

    while(fgets(buffer, sizeof(buffer), fp)) //идем по файлу
    {
        if (strncmp(buffer, "model name", strlen("model name")) == 0) //находим вхождение
        {
            char *cpu_name = strchr(buffer, ':'); //создаем массив и добавляем в него чтобы он начинался с двоеточия
            if (cpu_name != NULL)
            {
                cpu_name++; //переводим на само название
                while (*cpu_name == ' ') cpu_name++; //чтобы не выводить пробелы

                int len = strlen(cpu_name); //длина
                if (len > 0 && cpu_name[len - 1] == '\n') cpu_name[len - 1] = '\0'; // заменяем перенос строки на конец строки

                snprintf(out, 256, "CPU: %s", cpu_name); //собираем строчку для вывода 
                break;
            }
        }
    }
    fclose(fp); //закрываем файл
}

void print_gpu(char *out) //вывод видюхи
{
    out[0] = '\0';
    int fd[2]; //создаем массив для трубы
    char buffer[BUFFER_SIZE]; //создаем массив символов

    if (pipe(fd) == -1) return; //тут мы сразу добавляем в наш массив то, какие выходы трубы есть

    pid_t pid = fork(); //здесь мы создаем процесс 

    if (pid == 0) //проверяем что он дочерний
    {
        close(fd[0]); //закрываем вход трубы на чтение

        dup2(fd[1], STDOUT_FILENO); //делаем чтобы он выводил
        close(fd[1]); //закрываем вход трубы на запись

        char *args[] = {"lspci", NULL}; //создаем массив аргументов и передаем в него нужную команду и NULL
        execvp(args[0], args); //запускаем нашу команду она все что мы натворили выше стирает просто

        _exit(1); //если вдруг вышла ошибка
    }
    else //основной процесс
    {
        close(fd[1]); //закрываем вход трубы на запись
        FILE *fp = fdopen(fd[0], "r"); //открываем это как файл, типо чтобы можно было работать как с файлом

        while(fgets(buffer, sizeof(buffer), fp)) //идем по этому типо файлу
        {
            if (strstr(buffer, "VGA") != NULL || strstr(buffer, "3D") != NULL) //тут проверяем что в этом написанно, ищем вхождения
            {
                char *gpu_name = strchr(buffer, ':'); //делаем массив для видюхи и начинаем его с двоеточия
                if (gpu_name != NULL)
                {
                    gpu_name++; //убираем двоеточие
                    gpu_name = strchr(gpu_name, ':'); //начинаем с другого двоеточия
                    if (gpu_name != NULL)
                    {
                        gpu_name++; //убираем это двоеточие
                        while (*gpu_name == ' ') gpu_name++; //пробелы убираем
                        int len = strlen(gpu_name); //длина строки
                        if (len > 0 && gpu_name[len - 1] == '\n') gpu_name[len - 1] = '\0'; //делаем символ конца строки

                        snprintf(out, 256, "GPU: %s", gpu_name); //собираем строчку
                        break;
                    }
                }
            } 
        }
        fclose(fp); //закрываем файл
        wait(NULL); //ждем окончания
    }
}

void print_host(char *out) //выводим хоста
{
    out[0] = '\0';
    char buffer[BUFFER_SIZE]; //массив 
    FILE *fp = fopen("/sys/class/dmi/id/board_name", "r"); //открываем файл
    if (fp == NULL) return;
    fgets(buffer, sizeof(buffer), fp); //читаем файл
    int len = strlen(buffer); //длина
    if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0'; //ставим конец строки
    snprintf(out, 256, "Host: %s", buffer); //собираем строчку
    fclose(fp); //закрываем файл
}

void print_kernel(char *out) //выводим ядро
{
    out[0] = '\0';
    struct utsname sys_info; //готовая структура

    if (uname(&sys_info) == 0) //выводим просто всю инфу
    {
        snprintf(out, 256, "Kernel: %s", sys_info.release); //собираем строчку для ядра
    }
}

void print_uptime(char *out) //сколько система включена
{
    out[0] = '\0';
    struct sysinfo info; //опять готовая структура

    if (sysinfo(&info) == 0) //получаем общие сведенья
    {
        long uptime = info.uptime; //берем сколько система включена
        //все ниже просто перевод чисел
        long days = uptime / 86400;
        uptime = uptime % 86400;
        long hours = uptime / 3600;
        uptime = uptime % 3600;
        long minutes = uptime / 60;
        //здесь собираем строчки
        if (days > 0) snprintf(out, 256, "Uptime: %ld days, %ld hours, %ld minutes", days, hours, minutes);
        else if (hours > 0) snprintf(out, 256, "Uptime: %ld hours, %ld minutes", hours, minutes);
        else if (minutes > 0) snprintf(out, 256, "Uptime: %ld minutes", minutes);
        else snprintf(out, 256, "Uptime: %ld seconds", uptime);
    }
}

void print_de(char *out) //выводим окружения рабочего стола или оконный менеджер
{
    out[0] = '\0';
    char *de = getenv("XDG_CURRENT_DESKTOP"); //возвращаем значение переменного окружения
    if (de == NULL) de = getenv("XDG_SESSION_DESKTOP");
    if (de == NULL) de = getenv("DESKTOP_SESSION");

    if (de != NULL) snprintf(out, 256, "DE/WM: %s", de); //выводим при успехе
    else snprintf(out, 256, "DE/WM: Unknown"); //если что-то не так
}

void print_term(char *out) //выводим терминал
{
    out[0] = '\0';
    char *term = getenv("COLORTERM"); //возвращаем значение переменного окружения
    if (term == NULL || strcmp(term, "truecolor") == 0) term = getenv("TERM"); //ищем вхождение и возвращаем

    if (term != NULL) snprintf(out, 256, "Terminal: %s", term); //собираем строчку
    else snprintf(out, 256, "Terminal: Unknown"); //если что-то не так
}

void print_shell(char *out) //выводим оболочку
{
    out[0] = '\0';
    int fd[2]; //создаем массив для трубы
    char buffer[BUFFER_SIZE]; //создаем массив буфер

    if (pipe(fd) == -1) return; //делаем трубу

    pid_t pid = fork(); //создаем процесс
    if (pid == -1) return;

    if (pid == 0) //дочерний процесс
    {
        close(fd[0]); //закрываем на чтение
        dup2(fd[1], STDOUT_FILENO); //выводим
        close(fd[1]); //закрываем на запись

        char *args[] = {"bash", "--version", NULL}; //передаем команду
        execvp(args[0], args); //исполняем
        _exit(1); //если ошибка
    }
    else //родительный
    {
        close(fd[1]); //закрываем на запись
        FILE *fp = fdopen(fd[0], "r"); //открываем как типо файл

        if (fgets(buffer, sizeof(buffer), fp)) //идем по файлу
        {
            char *version = strstr(buffer, "version "); //ищем вхождение
            if (version != NULL)
            {
                version += strlen("version "); //плюсуем к длине
                char *version_end = strchr(version, ' '); //начинаем с пробела
                if (version_end != NULL) *version_end = '\0'; //добавляем конец строки
            }
            int len = strlen(version); //длина
            if (len > 0 && version[len - 1] == '\n') version[len - 1] = '\0'; //добавляем конец строки
            snprintf(out, 256, "Shell: bash %s", version); //собираем строку
        }
        fclose(fp); //закрываем файл
        wait(NULL); //ждем завершения
    }
}

void print_resolution(char *out) //разрешение
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

        int devnull = open("/dev/null", O_WRONLY); //открываем файл
        if (devnull != -1)
        {
            dup2(devnull, STDERR_FILENO); //для ошибки
            close(devnull); //закрываем
        }
        close(fd[1]);

        char *display_server = getenv("XDG_SESSION_TYPE"); //возвращаем переменное окружение
        if (display_server != NULL && strcmp(display_server, "wayland") == 0) //ищем вхождение для wayland
        {
            char *args[] = {"wlr-randr", NULL};
            execvp(args[0], args);
        }
        //для иксов
        char *args[] = {"xrandr", "--current", NULL};
        execvp(args[0], args);

        _exit(1);
    }
    else
    {
        close(fd[1]);
        FILE *fp = fdopen(fd[0], "r"); //открываем типо файл

        int count_monitor = 1; //счетчик мониторов
        char out_str[512] = ""; //пустой массив

        while (fgets(buffer, sizeof(buffer), fp)) //идем по файлу
        {
            if (strstr(buffer, "connected") != NULL) //ищем вхождение
            {
                char *pos = strstr(buffer, "connected"); //ищем вхождение
                while (*pos && !(*pos >= '0' && *pos <= '9')) pos++;

                if (*pos)
                {
                    //много всего но посути просто много условий
                    char *res_end = pos;
                    while (*res_end && *res_end != ' ' && *res_end != '+' && *res_end != '\n') res_end++;
                    *res_end = '\0';

                    char temp[128];
                    snprintf(temp, sizeof(temp), "%s (Screen %d), ", pos, count_monitor++); //собираем строчку
                    strcat(out_str, temp); //еще соеденяем
                }
            }
            else if (strstr(buffer, "current") != NULL) //много парсинга строк короче
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
            snprintf(out, 256, "Resolution: %s", out_str); //собираем конечную строчку
        }
        else snprintf(out, 256, "Resolution: Unknown");
    }
}

void print_packages(char *out) //пакетный менеджер
{
    out[0] = '\0';
    int count = 0; //для подсчета
    //для arch-based
    if (access("/usr/bin/pacman", F_OK) == 0) //проверяем права
    {
        DIR *d = opendir("/var/lib/pacman/local"); //открываем директорию
        if (d != NULL)
        {
            //читаем кол-во пакетов
            struct dirent *dir;
            while ((dir = readdir(d)) != NULL) if (dir->d_name[0] != '.') count++;
            closedir(d);
            snprintf(out, 256, "Packages: %d (pacman)", count);
            return;
        }
    }
    //для debian-based
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
}//для dnf было лень :). RedHat never existed.

void print_banner(char *out_banner, char *out_underline) //для банера
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
    //char hehe[100] = "I use Arch, btw\n";
    //write(1, hehe, strlen(hehe));
    return 0;
}