#ifndef ERRNO_H_
#define ERRNO_H_

#include <_cheader.h>

_BEGIN_C_HEADER

/* list modified from ToaruOS (https://github.com/klange/toaruos) */
#define EPERM 1               /* Operation not permitted */
#define ENOENT 2              /* No such file or directory */
#define ESRCH 3               /* No such process */
#define EINTR 4               /* Interrupted system call */
#define EIO 5                 /* I/O error */
#define ENXIO 6               /* No such device or address */
#define E2BIG 7               /* Argument list too long */
#define ENOEXEC 8             /* Exec format error */
#define EBADF 9               /* Bad file descriptor */
#define ECHILD 10             /* No child process */
#define EAGAIN 11             /* Resource temporarily unavailable */
#define ENOMEM 12             /* Out of memory */
#define EACCES 13             /* Permission denied */
#define EFAULT 14             /* Bad address */
#define ENOTBLK 15            /* Block device required */
#define EBUSY 16              /* Mount device busy */
#define EEXIST 17             /* File exists */
#define EXDEV 18              /* Cross-device link */
#define ENODEV 19             /* No such device */
#define ENOTDIR 20            /* Not a directory */
#define EISDIR 21             /* Is a directory */
#define EINVAL 22             /* Invalid argument */
#define ENFILE 23             /* Too many open files in system */
#define EMFILE 24             /* No file descriptors available */
#define ENOTTY 25             /* Not a tty */
#define ETXTBSY 26            /* Text file busy */
#define EFBIG 27              /* File too large */
#define ENOSPC 28             /* No space left on device */
#define ESPIPE 29             /* Invalid seek */
#define EROFS 30              /* Read-only file system */
#define EMLINK 31             /* Too many links */
#define EPIPE 32              /* Broken pipe */
#define EDOM 33               /* Domain error */
#define ERANGE 34             /* Result not representable */
#define ENOMSG 35             /* No message of desired type */
#define EIDRM 36              /* Identifier removed */
#define ECHRNG 37             /* Channel number out of range */
#define EL2NSYNC 38           /* Level 2 not synchronized */
#define EL3HLT 39             /* Level 3 halted */
#define EL3RST 40             /* Level 3 reset */
#define ELNRNG 41             /* Link number out of range */
#define EUNATCH 42            /* Protocol driver not attached */
#define ENOCSI 43             /* No CSI structure available */
#define EL2HLT 44             /* Level 2 halted */
#define EDEADLK 45            /* Resource deadlock would occur */
#define ENOLCK 46             /* No locks available */
#define EBADE 50              /* Invalid exchange */
#define EBADR 51              /* Invalid request descriptor */
#define EXFULL 52             /* Exchange full */
#define ENOANO 53             /* No anode */
#define EBADRQC 54            /* Invalid request code */
#define EBADSLT 55            /* Invalid slot */
#define EBFONT 57             /* Bad font file format */
#define ENOSTR 60             /* Device not a stream */
#define ENODATA 61            /* No data available */
#define ETIME 62              /* Device timeout */
#define ENOSR 63              /* Out of streams resources */
#define ENONET 64             /* Machine is not on the network */
#define ENOPKG 65             /* Package not installed */
#define EREMOTE 66            /* Object is remote */
#define ENOLINK 67            /* Link has been severed */
#define EADV 68               /* Advertise error */
#define ESRMNT 69             /* Srmount error */
#define ECOMM 70              /* Communication error on send */
#define EPROTO 71             /* Protocol error */
#define EMULTIHOP 74          /* Multihop attempted */
#define EBADMSG 77            /* Bad message */
#define ENOTUNIQ 80           /* Name not unique on network */
#define EBADFD 81             /* File descriptor in bad state */
#define EREMCHG 82            /* Remote address changed */
#define ELIBACC 83            /* Cannot access a needed shared library */
#define ELIBBAD 84            /* Accessing a corrupted shared library */
#define ELIBSCN 85            /* .lib section in a.out corrupted */
#define ELIBMAX 86            /* Attempting to link in too many shared libraries */
#define ELIBEXEC 87           /* Cannot exec a shared library directly */
#define ENOSYS 88             /* Function not implemented */
#define ENOTEMPTY 90          /* Directory not empty */
#define ENAMETOOLONG 91       /* Filename too long */
#define ELOOP 92              /* Symbolic link loop */
#define EOPNOTSUPP 95         /* Operation not supported */
#define EPFNOSUPPORT 96       /* Protocol family not supported */
#define ECONNRESET 104        /* Connection reset by peer */
#define ENOBUFS 105           /* No buffer space available */
#define EAFNOSUPPORT 106      /* Address family not supported by protocol */
#define EPROTOTYPE 107        /* Protocol wrong type for socket */
#define ENOTSOCK 108          /* Not a socket */
#define ENOPROTOOPT 109       /* Protocol not available */
#define ESHUTDOWN 110         /* Cannot send after socket shutdown */
#define ECONNREFUSED 111      /* Connection refused */
#define EADDRINUSE 112        /* Address in use */
#define ECONNABORTED 113      /* Connection aborted */
#define ENETUNREACH 114       /* Network unreachable */
#define ENETDOWN 115          /* Network is down */
#define ETIMEDOUT 116         /* Operation timed out */
#define EHOSTDOWN 117         /* Host is down */
#define EHOSTUNREACH 118      /* Host is unreachable */
#define EINPROGRESS 119       /* Operation in progress */
#define EALREADY 120          /* Operation already in progress */
#define EDESTADDRREQ 121      /* Destination address required */
#define EMSGSIZE 122          /* Message too large */
#define EPROTONOSUPPORT 123   /* Protocol not supported */
#define ESOCKTNOSUPPORT 124   /* Socket type not supported */
#define EADDRNOTAVAIL 125     /* Address not available */
#define ENETRESET 126         /* Connection reset by network */
#define EISCONN 127           /* Socket is connected */
#define ENOTCONN 128          /* Socket not connected */
#define ETOOMANYREFS 129      /* Too many references: cannot splice */
#define EUSERS 131            /* Too many users */
#define EDQUOT 132            /* Quota exceeded */
#define ESTALE 133            /* Stale file handle */
#define ENOTSUP 134           /* Not supported */
#define EILSEQ 138            /* Illegal byte sequence */
#define EOVERFLOW 139         /* Value too large for data type */
#define ECANCELED 140         /* Operation canceled */
#define ENOTRECOVERABLE 141   /* State not recoverable */
#define EOWNERDEAD 142        /* Previous owner died */
#define ESTRPIPE 143          /* Streams pipe error */

extern int errno;

_END_C_HEADER

#endif // ERRNO_H_
