/*
 * https://gitlab.com/bztsrc/minix3fs
 *
 * Copyright (C) 2023 bzt (MIT license)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL ANY
 * DEVELOPER OR DISTRIBUTOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * @brief a bare minimum, memory allocation-free, read-only Minix3 filesystem driver
 */

#include <stdint.h>

#ifndef MFS_H
#define MFS_H

/**
 * Sector loader, must be implemented by the host
 */
void loadsec(uint32_t lba, uint32_t cnt, void *buf);

/**
 * The API that we provide
 */
uint32_t mfs_open(const char *fn);
uint32_t mfs_read(uint32_t offs, uint32_t size, void *dst);
void     mfs_close(void);

#ifdef MFS_IMPLEMENTATION

/**
 * Configurable by the host
 */
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#ifndef MFS_DIRSEP
#define MFS_DIRSEP '/'
#endif

/**
 * MFS specific defines
 */
#define MFS_ROOT_INO    1       /* Root inode */

#define MFS_DIRSIZ     60
#define MFS_NR_DZONES   7       /* # direct zone numbers in a V2 inode */
#define MFS_NR_TZONES  10       /* total # zone numbers in a V2 inode */

/* i_mode, file permission bitmasks */
#define S_IXOTH        0x0001   /* execute by others */
#define S_IWOTH        0x0002   /* write by others */
#define S_IROTH        0x0004   /* read by others */
#define S_IXGRP        0x0008   /* execute by group */
#define S_IWGRP        0x0010   /* write by group */
#define S_IRGRP        0x0020   /* read by group */
#define S_IXUSR        0x0040   /* execute by owner */
#define S_IWUSR        0x0080   /* write by owner */
#define S_IRUSR        0x0100   /* read by owner */
#define S_ISUID        0x0400   /* set user on execution */
#define S_ISGID        0x0800   /* set group on execution */

/* i_mode, inode formats */
#define S_IFDIR        0x4000
#define S_IFREG        0x8000
#define S_IFLNK        0xA000
#define MFS_FILETYPE(x) ((x) & 0xF000)

/**
 * The superblock, always located at offset 1024, no matter the sector size or block size
 */
typedef struct {
  uint32_t s_ninodes;           /* # usable inodes on the minor device */
  uint16_t s_nzones;            /* total device size, including bit maps etc */
  uint16_t s_imap_blocks;       /* # of blocks used by inode bit map */
  uint16_t s_zmap_blocks;       /* # of blocks used by zone bit map */
  uint16_t s_firstdatazone_old; /* number of first data zone (small) */
  uint16_t s_log_zone_size;     /* log2 of blocks/zone */
  uint16_t s_flags;             /* FS state flags */
  int32_t s_max_size;           /* maximum file size on this device */
  uint32_t s_zones;             /* number of zones (replaces s_nzones in V2) */
  int16_t s_magic;              /* magic number to recognize super-blocks */
  /* The following items are valid on disk only for V3 and above */
  int16_t s_pad2;               /* try to avoid compiler-dependent padding */
  uint16_t s_block_size;        /* block size in bytes. */
  int8_t s_disk_version;        /* filesystem format sub-version */
} __attribute__((packed)) superblock_t;

/**
 * One inode slot in the inode table, 64 bytes
 * Inode table located at offset (2 + s_imap_blocks + s_zmap_blocks) * s_block_size, and has s_ninodes slots
 */
typedef struct {                /* V2/V3 disk inode */
  uint16_t i_mode;              /* file type, protection, etc. */
  uint16_t i_nlinks;            /* how many links to this file. */
  int16_t i_uid;                /* user id of the file's owner. */
  uint16_t i_gid;               /* group number */
  uint32_t i_size;              /* current file size in bytes */
  uint32_t i_atime;             /* when was file data last accessed */
  uint32_t i_mtime;             /* when was file data last changed */
  uint32_t i_ctime;             /* when was inode data last changed */
  uint32_t i_zone[MFS_NR_TZONES];/* zone nums for direct, ind, and dbl ind */
} __attribute__((packed)) inode_t;

/**
 * Directory entry format, 64 bytes
 */
typedef struct {
  uint32_t d_ino;
  char d_name[MFS_DIRSIZ];
} __attribute__((packed)) direct_t;

/**
 * Private static variables (no memory allocation in this library)
 */
uint32_t ind[2], inode_table, inode_nr, block_size = 0, sec_per_blk, blk_per_block, blk_per_block2, mfs_inode = 0, *blklst;
uint8_t mfs_data[32768], mfs_dir[32768], mfs_buf[65536], mfs_fn[PATH_MAX];
inode_t *ino;

/**
 * Load an inode
 * Returns:
 * - mfs_inode: the loaded inode's number, or 0 on error
 * - ino: points to the inode_t of the loaded inode
 */
static void loadinode(uint32_t inode)
{
    uint32_t block_offs = (inode - 1) * sizeof(inode_t) / block_size;
    uint32_t inode_offs = (inode - 1) * sizeof(inode_t) % block_size;
    /* failsafe, boundary check */
    if(inode < 1 || inode >= inode_nr) { mfs_inode = 0; return; }
    /* load the block with our inode structure */
    loadsec((inode_table + block_offs) * sec_per_blk, sec_per_blk, mfs_buf);
    /* copy inode from the table into the beginning of our buffer */
    __builtin_memcpy(mfs_buf, mfs_buf + inode_offs, sizeof(inode_t));
    /* reset indirect block cache */
    __builtin_memset(ind, 0, sizeof(ind));
    mfs_inode = inode;
}

/**
 * Look up directories, set mfs_inode and return file size
 * Returns:
 * - the opened file's size on success, or
 * - 0 on file not found error and
 * - 0xffffffff if MFS was not recognized on the storage
 */
uint32_t mfs_open(const char *fn)
{
    superblock_t *sb = (superblock_t*)mfs_buf;
    direct_t *de;
    uint32_t offs, i, rem, redir = 0;
    uint8_t *s = mfs_fn, *e;

    if(!fn || !*fn) return 0;
    /* copy the file name into a buffer, because resolving symbolic links might alter it */
    for(e = (uint8_t*)fn; *e && s - mfs_fn + 1 < PATH_MAX; s++, e++) *s = *e;
    s = mfs_fn;
    
    /* initialize file system */
    if(!block_size) {
        __builtin_memset(mfs_buf, 0, 512);
        loadsec(2, 1, mfs_buf);
        if(sb->s_magic != 0x4D5A || sb->s_block_size < 1024 || (sb->s_block_size & 511))
            return -1U;
        /* save values from the superblock */
        block_size = sb->s_block_size;                              /* one block's size */
        sec_per_blk = block_size >> 9;                              /* calculate frequently used block size multiples */
        blk_per_block = block_size / sizeof(uint32_t);
        blk_per_block2 = blk_per_block * blk_per_block;
        inode_table = 2 + sb->s_imap_blocks + sb->s_zmap_blocks;    /* get the start and length of the inode table */
        inode_nr = sb->s_ninodes;
        mfs_inode = 0; ino = (inode_t*)mfs_buf;                     /* set up inode buffer */
        blklst = (uint32_t*)(mfs_buf + sizeof(inode_t));
    }

    /* do path traversal */
again:
    loadinode(MFS_ROOT_INO);                                        /* start from the root directory */
    if(*s == MFS_DIRSEP) s++;                                       /* remove leading directory separator if any */
    for(e = s; *e && *e != MFS_DIRSEP; e++);                        /* find the end of the file name in path string */
    offs = 0;
    while(offs < ino->i_size) {                                     /* iterate on directory entries, read one block at a time */
        /* read in the next block in the directory */
        if(!mfs_read(offs, block_size, mfs_dir)) break;
        rem = ino->i_size - offs; if(rem > block_size) rem = block_size;
        offs += block_size;
        /* check filenames in directory entries */
        for(i = 0, de = (direct_t*)mfs_dir; i < rem; i += sizeof(direct_t), de++) {
            if(de->d_ino && e - s < MFS_DIRSIZ && !__builtin_memcmp(s, (uint8_t*)de->d_name, e - s) && !de->d_name[e - s]) {
                loadinode(de->d_ino);
                if(!mfs_inode) goto err;
                /* symlink */
                if(MFS_FILETYPE(ino->i_mode) == S_IFLNK) {
                    i = ino->i_size; if(i > PATH_MAX - 1) i = PATH_MAX - 1;
                    /* read in the target path */
                    if(redir >= 8 || !mfs_read(0, i, mfs_dir)) goto err;
                    mfs_dir[i] = 0; redir++;
                    /* this is a minimalistic implementation, we just do string manipulations.
                     * you should resolve target path in mfs_dir to mfs_inode / ino instead */
                    if(mfs_dir[0] == MFS_DIRSEP) {
                        /* starts with the directory separator, so an absolute path. Replace the entire string */
                        __builtin_memcpy(mfs_fn, mfs_dir + 1, i);
                    } else {
                        /* relative path, manipulate string, skip "./" and remove last file name when "../" */
                        for(e = mfs_dir; *e && s < &mfs_fn[PATH_MAX - 1]; e++) {
                            if(e[0] == '.' && e[1] == MFS_DIRSEP) e++; else
                            if(e[0] == '.' && e[1] == '.' && e[2] == MFS_DIRSEP) {
                                e += 2; if(s > mfs_fn) for(s--; s > mfs_fn && s[-1] != MFS_DIRSEP; s--);
                            } else *s++ = *e;
                        }
                        *s = 0;
                    }
                    /* restart traverse */
                    s = mfs_fn; goto again;
                }
                /* regular file and end of path */
                if(!*e) { if(MFS_FILETYPE(ino->i_mode) == S_IFREG) { return ino->i_size; } goto err; }
                /* directory and not end of path */
                if(MFS_FILETYPE(ino->i_mode) == S_IFDIR) {
                    /* with directories:
                     * - we simply replace mfs_inode and ino (already done by loadinode above),
                     * - adjust pointer in path to the next file name elment,
                     * - and restart our loop */
                    for(s = e + 1, e = s; *e && *e != MFS_DIRSEP; e++);
                    offs = 0;
                    break; /* break from for, continue while */
                } else
                /* anything else (device, fifo etc.) or file in the middle or directory at the end */
                    goto err;
            }
        }
    }
err:mfs_inode = 0;
    return 0;
}

/**
 * Read from opened file (current inode in mfs_inode)
 * Returns: the number of bytes loaded, or 0 on error
 */
uint32_t mfs_read(uint32_t offs, uint32_t size, void *dst)
{
    uint32_t blk, rem, i, j, k, os = 0, rs = block_size;
    uint8_t *buf = (uint8_t*)dst;

    if(!mfs_inode || !block_size || offs >= ino->i_size || !size || !buf) return 0;
    /* make sure we won't read out of bounds */
    if(offs + size > ino->i_size) size = ino->i_size - offs;
    rem = size;

    /* calculate file block number from offset */
    os = offs % block_size;
    offs /= block_size;
    if(os) rs = block_size - os;

    while(rem) {
        /* convert file offset block number to file system block number */
        /* we keep indirect records cached in blklst, and the block numbers where they were loaded from in ind[X]
         *   ind[0] -> blklst[0 .. blk_per_block-1]
         *   ind[1] -> blklst[blk_per_block .. 2*blk_per_block-1] */
        blk = 0;
        if(offs < MFS_NR_DZONES) { /* direct */ blk = ino->i_zone[offs]; } else
        if(offs >= MFS_NR_DZONES && offs < MFS_NR_DZONES + blk_per_block) {
            /* indirect */
            if(ind[0] != ino->i_zone[MFS_NR_DZONES]) {
                ind[0] = ino->i_zone[MFS_NR_DZONES];
                loadsec(ind[0] * sec_per_blk, sec_per_blk, (uint8_t*)blklst);
            }
            blk = blklst[offs - MFS_NR_DZONES];
        } else
        if(offs >= MFS_NR_DZONES + blk_per_block && offs < MFS_NR_DZONES + blk_per_block + blk_per_block2) {
            /* double indirect */
            i = offs - MFS_NR_DZONES - blk_per_block;
            k = MFS_NR_DZONES + 1 + i / blk_per_block2;
            if(k < MFS_NR_TZONES) {
                if(ind[0] != ino->i_zone[k]) {
                    ind[0] = ino->i_zone[k];
                    loadsec(ind[0] * sec_per_blk, sec_per_blk, (uint8_t*)blklst);
                }
                j = blklst[i / blk_per_block];
                if(ind[1] != j) {
                    ind[1] = j;
                    loadsec(ind[1] * sec_per_blk, sec_per_blk, (uint8_t*)blklst + block_size);
                }
                blk = blklst[blk_per_block + (i % blk_per_block)];
            }
        }
        if(!blk) break;
        /* load data */
        loadsec(blk * sec_per_blk, sec_per_blk, mfs_data);
        /* copy to its final place */
        if(rs > rem) rs = rem;
        __builtin_memcpy(buf, mfs_data + os, rs); os = 0;
        buf += rs; rem -= rs; rs = block_size;
        offs++;
    }

    return (size - rem);
}

/**
 * Close the opened file
 */
void mfs_close(void)
{
    mfs_inode = 0;
}

#endif /* MFS_IMPLEMENTATION */

#endif /* MFS_H */
