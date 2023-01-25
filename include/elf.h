#ifndef ELF_H_
#define ELF_H_

/* based on glibc elf.h */

#include <sys/stdint.h>

/* Type for a 16-bit quantity. */
typedef uint16_t Elf32_Half;
/* typedef uint16_t Elf64_Half; */

/* Types for signed and unsigned 32-bit quantities. */
typedef uint32_t Elf32_Word;
typedef int32_t  Elf32_Sword;
/* typedef uint32_t Elf64_Word; */
/* typedef int32_t  Elf64_Sword; */

/* Types for signed and unsigned 64-bit quantities. */
typedef uint64_t Elf32_Xword;
typedef int64_t  Elf32_Sxword;
/* typedef uint64_t Elf64_Xword; */
/* typedef int64_t  Elf64_Sxword; */

/* Type of addresses. */
typedef uint32_t Elf32_Addr;
/* typedef uint64_t Elf64_Addr; */

/* Type of file offsets. */
typedef uint32_t Elf32_Off;
/* typedef uint64_t Elf64_Off; */

/* Type for section indices, which are 16-bit quantities. */
typedef uint16_t Elf32_Section;
/* typedef uint16_t Elf64_Section; */

/* Type for version symbol information. */
typedef Elf32_Half Elf32_Versym;
/* typedef Elf64_Half Elf64_Versym; */


/* ELF header */
#define EI_NIDENT 16

typedef struct {
    uint8_t    e_ident[EI_NIDENT];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off  e_phoff;
    Elf32_Off  e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
} Elf32_Ehdr;

/* Fields in the e_ident array. The EI_* macros are indices into the
 * array. The macros under each EI_* macro are the values the byte
 * may have. */

#define EI_MAG0      0
#define ELFMAG0      0x7f

#define EI_MAG1      1
#define ELFMAG1      'E'

#define EI_MAG2      2
#define ELFMAG2      'L'

#define EI_MAG3      3
#define ELFMAG3      'F'

/* Conglomeration of the identification bytes, for easy testing as a word. */
#define ELFMAG       "\177ELF"
#define SELFMAG      4

#define EI_CLASS     4        /* File class byte index */
#define ELFCLASSNONE 0        /* Invalid class */
#define ELFCLASS32   1        /* 32-bit objects */
#define ELFCLASS64   2        /* 64-bit objects */
#define ELFCLASSNUM  3

#define EI_DATA      5        /* Data encoding byte index */
#define ELFDATANONE  0        /* Invalid data encoding */
#define ELFDATA2LSB  1        /* 2's complement, little endian */
#define ELFDATA2MSB  2        /* 2's complement, big endian */
#define ELFDATANUM   3

#define EI_VERSION   6        /* File version byte index */
                              /* Value must be EV_CURRENT */

#define EI_OSABI            7            /* OS ABI identification */
#define ELFOSABI_NONE       0            /* UNIX System V ABI */
#define ELFOSABI_SYSV       0            /* Alias. */
#define ELFOSABI_HPUX       1            /* HP-UX */
#define ELFOSABI_NETBSD     2            /* NetBSD. */
#define ELFOSABI_GNU        3            /* Object uses GNU ELF extensions. */
#define ELFOSABI_LINUX      ELFOSABI_GNU /* Compatibility alias. */
#define ELFOSABI_SOLARIS    6            /* Sun Solaris. */
#define ELFOSABI_AIX        7            /* IBM AIX. */
#define ELFOSABI_IRIX       8            /* SGI Irix. */
#define ELFOSABI_FREEBSD    9            /* FreeBSD. */
#define ELFOSABI_TRU64      10           /* Compaq TRU64 UNIX. */
#define ELFOSABI_MODESTO    11           /* Novell Modesto. */
#define ELFOSABI_OPENBSD    12           /* OpenBSD. */
#define ELFOSABI_ARM_AEABI  64           /* ARM EABI */
#define ELFOSABI_ARM        97           /* ARM */
#define ELFOSABI_STANDALONE 255          /* Standalone (embedded) application */

#define EI_ABIVERSION       8            /* ABI version */

#define EI_PAD              9            /* Byte index of padding bytes */

/* Legal values for e_type (object file type). */

#define ET_NONE   0             /* No file type */
#define ET_REL    1             /* Relocatable file */
#define ET_EXEC   2             /* Executable file */
#define ET_DYN    3             /* Shared object file */
#define ET_CORE   4             /* Core file */
#define ET_NUM    5             /* Number of defined types */
#define ET_LOOS   0xfe00        /* OS-specific range start */
#define ET_HIOS   0xfeff        /* OS-specific range end */
#define ET_LOPROC 0xff00        /* Processor-specific range start */
#define ET_HIPROC 0xffff        /* Processor-specific range end */

/* Legal values for e_machine (architecture). */

#define EM_386    3             /* Intel 80386 */

/* Legal values for e_version (version). */

#define EV_NONE    0            /* Invalid ELF version */
#define EV_CURRENT 1            /* Current version */
#define EV_NUM     2

/* Section header */

typedef struct {
    Elf32_Word sh_name;       /* Section name (string tbl index) */
    Elf32_Word sh_type;       /* Section type */
    Elf32_Word sh_flags;      /* Section flags */
    Elf32_Addr sh_addr;       /* Section virtual addr at execution */
    Elf32_Off  sh_offset;     /* Section file offset */
    Elf32_Word sh_size;       /* Section size in bytes */
    Elf32_Word sh_link;       /* Link to another section */
    Elf32_Word sh_info;       /* Additional section information */
    Elf32_Word sh_addralign;  /* Section alignment */
    Elf32_Word sh_entsize;    /* Entry size if section holds table */
} Elf32_Shdr;

/* Legal values for sh_type (section type). */

#define SHT_NULL          0             /* Section header table entry unused */
#define SHT_PROGBITS      1             /* Program data */
#define SHT_SYMTAB        2             /* Symbol table */
#define SHT_STRTAB        3             /* String table */
#define SHT_RELA          4             /* Relocation entries with addends */
#define SHT_HASH          5             /* Symbol hash table */
#define SHT_DYNAMIC       6             /* Dynamic linking information */
#define SHT_NOTE          7             /* Notes */
#define SHT_NOBITS        8             /* Program space with no data (bss) */
#define SHT_REL           9             /* Relocation entries, no addends */
#define SHT_SHLIB         10            /* Reserved */
#define SHT_DYNSYM        11            /* Dynamic linker symbol table */
#define SHT_INIT_ARRAY    14            /* Array of constructors */
#define SHT_FINI_ARRAY    15            /* Array of destructors */
#define SHT_PREINIT_ARRAY 16            /* Array of pre-constructors */
#define SHT_GROUP         17            /* Section group */
#define SHT_SYMTAB_SHNDX  18            /* Extended section indices */
#define SHT_RELR          19            /* RELR relative relocations */
#define SHT_NUM           20            /* Number of defined types.  */
#define SHT_LOOS          0x60000000    /* Start OS-specific.  */
#define SHT_GNU_ATTRIBUTES 0x6ffffff5   /* Object attributes.  */
#define SHT_GNU_HASH      0x6ffffff6    /* GNU-style hash table.  */
#define SHT_GNU_LIBLIST   0x6ffffff7    /* Prelink library list */
#define SHT_CHECKSUM      0x6ffffff8    /* Checksum for DSO content.  */
#define SHT_LOSUNW        0x6ffffffa    /* Sun-specific low bound.  */
#define SHT_SUNW_move     0x6ffffffa
#define SHT_SUNW_COMDAT   0x6ffffffb
#define SHT_SUNW_syminfo  0x6ffffffc
#define SHT_GNU_verdef    0x6ffffffd    /* Version definition section.  */
#define SHT_GNU_verneed   0x6ffffffe    /* Version needs section.  */
#define SHT_GNU_versym    0x6fffffff    /* Version symbol table.  */
#define SHT_HISUNW        0x6fffffff    /* Sun-specific high bound.  */
#define SHT_HIOS          0x6fffffff    /* End OS-specific type */
#define SHT_LOPROC        0x70000000    /* Start of processor-specific */
#define SHT_HIPROC        0x7fffffff    /* End of processor-specific */
#define SHT_LOUSER        0x80000000    /* Start of application-specific */
#define SHT_HIUSER        0x8fffffff    /* End of application-specific */


/* Legal values for sh_flags (section flags).  */

#define SHF_WRITE            (1 << 0)   /* Writable */
#define SHF_ALLOC            (1 << 1)   /* Occupies memory during execution */
#define SHF_EXECINSTR        (1 << 2)   /* Executable */
#define SHF_MERGE            (1 << 4)   /* Might be merged */
#define SHF_STRINGS          (1 << 5)   /* Contains nul-terminated strings */
#define SHF_INFO_LINK        (1 << 6)   /* `sh_info' contains SHT index */
#define SHF_LINK_ORDER       (1 << 7)   /* Preserve order after combining */
#define SHF_OS_NONCONFORMING (1 << 8)   /* Non-standard OS specific handling
                                         * required */
#define SHF_GROUP            (1 << 9)   /* Section is member of a group. */
#define SHF_TLS              (1 << 10)  /* Section hold thread-local data. */
#define SHF_COMPRESSED       (1 << 11)  /* Section with compressed data. */
#define SHF_MASKOS           0x0ff00000 /* OS-specific. */
#define SHF_MASKPROC         0xf0000000 /* Processor-specific */
#define SHF_GNU_RETAIN       (1 << 21)  /* Not to be GCed by linker. */
#define SHF_ORDERED          (1 << 30)  /* Special ordering requirement
                                         * (Solaris). */
#define SHF_EXCLUDE          (1U << 31) /* Section is excluded unless
                                         * referenced or allocated (Solaris) */

/* Program segment header. */

typedef struct {
    Elf32_Word p_type;          /* Segment type */
    Elf32_Off  p_offset;        /* Segment file offset */
    Elf32_Addr p_vaddr;         /* Segment virtual address */
    Elf32_Addr p_paddr;         /* Segment physical address */
    Elf32_Word p_filesz;        /* Segment size in file */
    Elf32_Word p_memsz;         /* Segment size in memory */
    Elf32_Word p_flags;         /* Segment flags */
    Elf32_Word p_align;         /* Segment alignment */
} Elf32_Phdr;

/* Special value for e_phnum.  This indicates that the real number of
   program headers is too large to fit into e_phnum.  Instead the real
   value is in the field sh_info of section 0.  */

#define PN_XNUM         0xffff

/* Legal values for p_type (segment type).  */

#define PT_NULL         0               /* Program header table entry unused */
#define PT_LOAD         1               /* Loadable program segment */
#define PT_DYNAMIC      2               /* Dynamic linking information */
#define PT_INTERP       3               /* Program interpreter */
#define PT_NOTE         4               /* Auxiliary information */
#define PT_SHLIB        5               /* Reserved */
#define PT_PHDR         6               /* Entry for header table itself */
#define PT_TLS          7               /* Thread-local storage segment */
#define PT_NUM          8               /* Number of defined types */
#define PT_LOOS         0x60000000      /* Start of OS-specific */
#define PT_GNU_EH_FRAME 0x6474e550      /* GCC .eh_frame_hdr segment */
#define PT_GNU_STACK    0x6474e551      /* Indicates stack executability */
#define PT_GNU_RELRO    0x6474e552      /* Read-only after relocation */
#define PT_GNU_PROPERTY 0x6474e553      /* GNU property */
#define PT_LOSUNW       0x6ffffffa
#define PT_SUNWBSS      0x6ffffffa      /* Sun Specific segment */
#define PT_SUNWSTACK    0x6ffffffb      /* Stack segment */
#define PT_HISUNW       0x6fffffff
#define PT_HIOS         0x6fffffff      /* End of OS-specific */
#define PT_LOPROC       0x70000000      /* Start of processor-specific */
#define PT_HIPROC       0x7fffffff      /* End of processor-specific */

/* Legal values for p_flags (segment flags).  */

#define PF_X            (1 << 0)        /* Segment is executable */
#define PF_W            (1 << 1)        /* Segment is writable */
#define PF_R            (1 << 2)        /* Segment is readable */
#define PF_MASKOS       0x0ff00000      /* OS-specific */
#define PF_MASKPROC     0xf0000000      /* Processor-specific */

#endif // ELF_H_
