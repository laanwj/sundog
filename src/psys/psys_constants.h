/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#ifndef H_PSYS_CONSTANTS
#define H_PSYS_CONSTANTS

/* Endian determinator */
enum psys_endian {
    PSYS_ENDIAN_NATIVE  = 0x0001,
    PSYS_ENDIAN_FLIPPED = 0x0100,
};

/* NIL is 0 as on 68000 */
static const unsigned PSYS_NIL = 0x0;

/* I made the arbitrary decision in the beginning of this project to have all
 * data structure offsets in bytes instead of words. This may come back to bite
 * me.
 */

/*** Segment offsets ***/
enum {
    PSYS_SEG_PROCDICT = 0x00, /* Process dictionary offset (in words) */
    PSYS_SEG_RELOCS   = 0x02, /* Relocation list offset (in words) */
    PSYS_SEG_NAME     = 0x04, /* 8-byte segment name */
    PSYS_SEG_ENDIAN   = 0x0c, /* Endian determinator */
    PSYS_SEG_CPOOLOFS = 0x0e, /* Constant pool offset (in words) */
    PSYS_SEG_REALSIZE = 0x10, /* Realpool size (in words) */
    PSYS_SEG_PARTNUM1 = 0x12, /* 4-byte "SOFtech part number" / reserved */
    PSYS_SEG_PARTNUM2 = 0x14,
    /* (size of segment header) */
    PSYS_SEG_CODESTART = 0x16, /* Code start */
};

/*** MSCW (activation record) ***/
enum {
    PSYS_MSCW_MSSTAT = 0x00, /* pointer to the activation record of the lexical parent */
    PSYS_MSCW_MSDYN  = 0x02, /* pointer to the activation record of the caller */
    PSYS_MSCW_IPC    = 0x04, /* instruction pointer of the caller */
    PSYS_MSCW_MSENV  = 0x06, /* E_Rec pointer of the caller */
    PSYS_MSCW_MPROC  = 0x08, /* Procedure number of the caller */
    PSYS_MSCW_VAROFS = 0x08, /* Offset for globals/locals */
    /* (size of MSCW) */
    PSYS_MSCW_SIZE = 0x0a,
};

/** E_Rec ***/
enum {
    PSYS_EREC_Env_Data   = 0x00, /* Pointer to globals */
    PSYS_EREC_Env_Vect   = 0x02, /* Pointer to EVEC */
    PSYS_EREC_Env_SIB    = 0x04, /* Pointer to SIB */
    PSYS_EREC_Link_Count = 0x06, /* Link count (only principal segments) */
    PSYS_EREC_Next_Rec   = 0x08, /* Next_rec (only principal segments) */
    /* (size of EREC) */
    PSYS_EREC_SIZE = 0x0a,
};

/*** E_Vec ***/
enum {
    PSYS_EVEC_Vec_Length = 0x00, /* Pointer to globals */
    /* Pointer to EREC for segment 1 */
    /* Pointer to EREC for segment 2 */
    /* ... more pointers can follow */
    /* Size is Vec_Length * 2 + 2 */
};

/*** TIB ***/
enum {
    PSYS_TIB_Wait_Q       = 0x00,
    PSYS_TIB_Flags_Prior  = 0x02, /* Flags | Prior */
    PSYS_TIB_SP_Low       = 0x04, /* lower range for SP */
    PSYS_TIB_SP_Upr       = 0x06, /* upper range for SP */
    PSYS_TIB_SP           = 0x08,
    PSYS_TIB_MP           = 0x0a,
    PSYS_TIB_Reserved     = 0x0c,
    PSYS_TIB_IPC          = 0x0e, /* relative to current code segment */
    PSYS_TIB_ENV          = 0x10, /* E_REC pointer */
    PSYS_TIB_IOR_Proc_Num = 0x12, /* Stored IORESULT | Procedure number */
    PSYS_TIB_Hang_Ptr     = 0x14,
    PSYS_TIB_M_Depend     = 0x16,
    PSYS_TIB_Main_Task    = 0x18,
    PSYS_TIB_Start_MSCW   = 0x1a,
    /* (size of TIB) */
    PSYS_TIB_SIZE = 0x1c,
};

/*** SIB ***/
enum {
    PSYS_SIB_Seg_Pool   = 0x00,
    PSYS_SIB_Seg_Base   = 0x02,
    PSYS_SIB_Seg_Refs   = 0x04,
    PSYS_SIB_Time_Stamp = 0x06,
    PSYS_SIB_Link_Count = 0x08,
    PSYS_SIB_Residency  = 0x0a,
    PSYS_SIB_Seg_Name   = 0x0c, /* 8-byte segment name */
    PSYS_SIB_Seg_Leng   = 0x14,
    PSYS_SIB_Seg_Addr   = 0x16,
    PSYS_SIB_Vol_Info   = 0x18,
    PSYS_SIB_Data_Size  = 0x1a,
    PSYS_SIB_Next_Sib   = 0x1c,
    PSYS_SIB_Prev_Sib   = 0x1e,
    PSYS_SIB_Scratch    = 0x20,
    PSYS_SIB_M_Type     = 0x22,
    /* (size of SIB) */
    PSYS_SIB_SIZE = 0x24,
};

/*** PoolDesc ***/
enum {
    PSYS_PD_PoolBase   = 0x00, /* Base of code pool - 32-bit address */
    PSYS_PD_PoolSize   = 0x04, /* Size of code pool in words */
    PSYS_PD_MinOffset  = 0x06, /* Lower boundary of code pool */
    PSYS_PD_MaxOffset  = 0x08, /* Upper boundary of code pool */
    PSYS_PD_Resolution = 0x0a, /* Allocation alignment */
    PSYS_PD_PoolHead   = 0x0c, /* Segment at base of code pool */
    PSYS_PD_Perm_SIB   = 0x0e, /* Always resident SIB */
    PSYS_PD_Extended   = 0x10  /* Using extended memory */
};

/*** Semaphore ***/
enum {
    PSYS_SEM_COUNT = 0x00, /* Semaphore count */
    PSYS_SEM_TIB   = 0x02, /* TIB at head of wait queue */
};

/*** SYSCOM offsets ***/
enum {
    PSYS_SYSCOM_IORSLT      = 0x00, /* Last I/O completion status */
    PSYS_SYSCOM_BOOT_UNIT   = 0x04, /* Disk unit which system booted from */
    PSYS_SYSCOM_GLOBALDIR   = 0x08, /* Global file directory pointer */
    PSYS_SYSCOM_REAL_SEM    = 0x0e, /* Semaphore to start the faulthandler */
    PSYS_SYSCOM_FAULT_TIS   = 0x12, /* TIS of faulting task */
    PSYS_SYSCOM_FAULT_EREC  = 0x14, /* E_REC of segment to read */
    PSYS_SYSCOM_FAULT_WORDS = 0x16, /* Number of words needed on stack */
    PSYS_SYSCOM_FAULT_TYPE  = 0x18, /* PSYS_FAULT_* */
    PSYS_SYSCOM_TIMESTAMP   = 0x2a, /* Increased after each intersegment call */
    PSYS_SYSCOM_EXTM        = 0x34, /* Pointer to EXTM structure */
    PSYS_SYSCOM_REAL_SIZE   = 0x38, /* Size of REAL type in words (0 if no support) */
    /* (size of SYSCOM) */
    PSYS_SYSCOM_SIZE = 0x60,
};

/** EXTM structure offsets. Unsure what this is used for. */
enum {
    PSYS_EXTM_HEAD   = 0x00,
    PSYS_EXTM_BASE   = 0x08,
    PSYS_EXTM_BLOCKS = 0x0c,
    PSYS_EXTM_SIZE   = 0x0e,
};

/*** Execution errors ***/

/* Based on pcode.h from ucsd-psystem-vm,
 * adapted according to P-systems version IV internal reference.
 */
enum psys_exec_err {
    PSYS_ERR_INVNDX = 1,  /* Value range error */
    PSYS_ERR_NOPROC = 2,  /* No proc in segment table */
    PSYS_ERR_INTOVR = 5,  /* Integer overflow */
    PSYS_ERR_DIVZER = 6,  /* Divide by zero */
    PSYS_ERR_UBREAK = 8,  /* Program interrupted by user */
    PSYS_ERR_UIOERR = 10, /* I/O error */
    PSYS_ERR_NOTIMP = 11, /* Unimplemented instruction */
    PSYS_ERR_FPIERR = 12, /* Floating point error */
    PSYS_ERR_S2LONG = 13, /* String overflow */
    PSYS_ERR_BRKPNT = 16, /* Break point */
    PSYS_ERR_SET2LG = 18, /* Set too large */
    PSYS_ERR_SEG2LG = 20, /* Segment too large */
};

/*** I/O errors ***/

/* Page 4-14 I/O completion codes */
enum psys_io_err {
    PSYS_IO_NOERROR   = 0,  /* No error */
    PSYS_IO_BADBLOCK  = 1,  /* Bad block, CRC error (parity) */
    PSYS_IO_BADUNIT   = 2,  /* Bad device number */
    PSYS_IO_BADMODE   = 3,  /* Illegal I/O request */
    PSYS_IO_TIMEOUT   = 4,  /* Data-com timeout */
    PSYS_IO_LOSTUNIT  = 5,  /* Volume is no longer on-line */
    PSYS_IO_LOSTFILE  = 6,  /* File is no longer in directory */
    PSYS_IO_BADTITLE  = 7,  /* Illegal file name */
    PSYS_IO_NOROOM    = 8,  /* No room; insufficient space on disk */
    PSYS_IO_NOUNIT    = 9,  /* No such volume on-line */
    PSYS_IO_NOFILE    = 10, /* No such file name in directory */
    PSYS_IO_DUPFILE   = 11, /* Duplicate file */
    PSYS_IO_NOTCLOSED = 12, /* Not closed: attempt to open an open file */
    PSYS_IO_NOTOPEN   = 13, /* Not open: attempt to access a closed file */
    PSYS_IO_BADFORMAT = 14, /* Error reading real or integer */
    PSYS_IO_BUFOVFL   = 15, /* Ring buffer overflow */
    PSYS_IO_WRITEPROT = 16, /* Write attempt to protected disk */
    PSYS_IO_ILLBLOCK  = 17, /* Illegal block number */
    PSYS_IO_ILLBUF    = 18, /* Illegal buffer address */
    PSYS_IO_ILLSIZE   = 19, /* Bad text file size */
    /* Codes 20 - 127 Reserved for future expansion */
    /* Codes 128 through 255 are available for
       non-predefined, device-dependent errors
     */
};

/*** Faults ***/

/* These are passed in PSYS_SYSCOM_FAULT_TYPE by psys_fault */
enum psys_fault {
    PSYS_FAULT_SEG  = 0x80, /* Segment fault */
    PSYS_FAULT_STK  = 0x81, /* Stack fault */
    PSYS_FAULT_HEAP = 0x82, /* Heap fault */
    PSYS_FAULT_POOL = 0x83, /* Pool fault */
};

#endif
