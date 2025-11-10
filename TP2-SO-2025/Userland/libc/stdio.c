#include <stdio.h>
#include <string.h>
#include <syscalls.h>

#ifdef ANSI_4_BIT_COLOR_SUPPORT
#include <ansiColors.h>
#endif

static char buffer[64] = {0};

static uint32_t uintToBase(uint64_t value, char *buffer, uint32_t base);
static void printBase(int fd, int num, int base);
static void printLongBase(int fd, int64_t num, int base);
static void printUnsignedLongBase(int fd, uint64_t num, int base);

void puts(const char *str) {
  /* Avoid passing user data as format string */
  printf("%s\n", str);
}

void vfprintf(int fd, const char *format, va_list args) {
  int i = 0;
  while (format[i] != 0) {
    switch (format[i]) {
    case '\e':
#ifdef ANSI_4_BIT_COLOR_SUPPORT
      sys_write(fd, &format[i], 0);  
      parseANSI(format, &i);
      break;
#else
      while (format[i] != 'm')
        i++; 
      i++;
      break;
#endif
    case '%':
      i++;
      switch (format[i]) {
      case 'x':
        printBase(fd, va_arg(args, int), 16);
        break;
      case 'd':
        printBase(fd, va_arg(args, int), 10);
        break;
      case 'o':
        printBase(fd, va_arg(args, int), 8);
        break;
      case 'b':
        printBase(fd, va_arg(args, int), 2);
        break;
      case 'l':
        i++;
        if (format[i] == 'd') {
          printLongBase(fd, va_arg(args, int64_t), 10);
        } else if (format[i] == 'u') {
          printUnsignedLongBase(fd, va_arg(args, uint64_t), 10);
        } else if (format[i] == 'x') {
          printUnsignedLongBase(fd, va_arg(args, uint64_t), 16);
        } else {
          sys_write(fd, "l", 1);
          i--;
        }
        break;
      case 'c': {
        char c = (char)va_arg(args, int);
        sys_write(fd, &c, 1);
        break;
      }
      case 's':
        {
          char *s = va_arg(args, char *);
          if (s)
            sys_write(fd, s, strlen(s));
        }
        break;
      case '%':
        sys_write(fd, "%", 1);
        break;
      }
      i++;
      break;
    default:
      sys_write(fd, &format[i], 1);
      i++;
      break;
    }
  }
}

void printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(FD_STDOUT, format, args);
  va_end(args);
}

void fprintf(int fd, const char *str, ...) {
  va_list args;
  va_start(args, str);
  vfprintf(fd, str, args);
  va_end(args);
}

int vscanf(const char *format, va_list args) {
  int i = 0;
  int args_read = 0;
  char c;
  while (format[i] != 0) {
    switch (format[i]) {
    case '%':
      i++;
      switch (format[i]) {
      case 'd':
        int64_t num = 0;
        uint8_t negative = 0;

        c = getchar();

        if (c != '-' && (c < '0' || c > '9')) {
          while ((c = getchar()) != '\n')
            ; 
          break;
        };

        do {
          if (c == '-')
            negative = 1;
          else
            num = num * 10 + c - '0';
        } while (((c = getchar()) >= '0' && c <= '9') ||
                 (num == 0 && c == '-'));

        *va_arg(args, int *) = num * (negative ? -1 : 1);
        args_read++;
        break;
      case 'c':
        *va_arg(args, char *) = getchar();
        args_read++;
        break;
      case 's':
        char *str = va_arg(args, char *);
        while ((c = getchar()) != ' ' && c != '\n') {
          *str = c;
          str++;
        }
        *str = 0;
        args_read++;
        break;
      }
    }
    i++;
  }
  return args_read;
}

int vsscanf(const char *buffer, const char *format, va_list args) {
  int form_i = 0;
  int buf_i = 0;
  int args_read = 0;
  while (format[form_i] != 0) {
    switch (format[form_i]) {
    case '%':
      form_i++;
      switch (format[form_i]) {
      case 'd': {
        int num = 0;
        uint8_t read_num = 0, negative = 0;

        if (buffer[buf_i] != '-' &&
            (buffer[buf_i] < '0' || buffer[buf_i] > '9'))
          break;

        if (buffer[buf_i] == '-') {
          negative = 1;
          buf_i++;
        };

        while (buffer[buf_i] >= '0' && buffer[buf_i] <= '9') {
          num = num * 10 + buffer[buf_i] - '0';
          buf_i++;
          read_num = 1;
        }

        *va_arg(args, int *) = num * (negative ? -1 : 1);
        args_read += read_num;
        break;
      }
      case 'c': {
        *va_arg(args, char *) = buffer[buf_i++];
        args_read++;
        break;
      }
      case 's': {
        char *str = va_arg(args, char *);
        while (buffer[buf_i] != ' ' && buffer[buf_i] != '\n' &&
               buffer[buf_i] != 0) {
          *(str++) = buffer[buf_i];
          str++;
          buf_i++;
        }
        *str = 0;
        args_read++;
        break;
      }
      default:
        break;
      }
    }
    form_i++;
  }
  return args_read;
}

int sscanf(const char *str, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int aux = vsscanf(str, format, args);
  va_end(args);
  return aux;
}

int scanf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  int aux = vscanf(format, args);
  va_end(args);
  return aux;
}

int getchar(void) {
  unsigned char c;
  int n;
  while ((n = sys_read(FD_STDIN, &c, 1)) == -1) {
  }
  if (n == 0) {
    return EOF; 
  }
  return (int)c;
}

void putchar(const char c) { sys_write(FD_STDOUT, &c, 1); };

static uint32_t uintToBase(uint64_t value, char *buffer, uint32_t base) {
  char *p = buffer;
  char *p1, *p2;
  uint32_t digits = 0;

  do {
    uint32_t remainder = value % base;
    *p++ = (remainder < 10) ? remainder + '0' : remainder + 'A' - 10;
    digits++;
  } while (value /= base);

  *p = 0;

  p1 = buffer;
  p2 = p - 1;
  while (p1 < p2) {
    char tmp = *p1;
    *p1 = *p2;
    *p2 = tmp;
    p1++;
    p2--;
  }

  return digits;
}

static void printBase(int fd, int num, int base) {
  uint64_t value;
  if (num < 0) {
    sys_write(fd, "-", 1);
    value = (uint64_t)(-num); 
  } else {
    value = (uint64_t)num;
  }
  uintToBase(value, buffer, base);
  sys_write(fd, buffer, strlen(buffer));
}

static void printLongBase(int fd, int64_t num, int base) {
  uint64_t value;
  if (num < 0) {
    sys_write(fd, "-", 1);
    if (num == -9223372036854775807LL - 1) {
      value = 9223372036854775808ULL;
    } else {
      value = (uint64_t)(-num);
    }
  } else {
    value = (uint64_t)num;
  }
  uintToBase(value, buffer, base);
  sys_write(fd, buffer, strlen(buffer));
}

static void printUnsignedLongBase(int fd, uint64_t num, int base) {
  uintToBase(num, buffer, base);
  sys_write(fd, buffer, strlen(buffer));
}
