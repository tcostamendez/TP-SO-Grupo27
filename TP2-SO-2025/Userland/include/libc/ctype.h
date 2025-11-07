#ifndef _LIBC_CTYPE_H_
#define _LIBC_CTYPE_H_

#define _tolower(a) ((a) >= 'A' && (a) <= 'Z' ? (a) + ('a' - 'A') : (a) )
#define _toupper(a) ((a) >= 'a' && (a) <= 'z' ? (a) - ('a' - 'A') : (a) )

#endif