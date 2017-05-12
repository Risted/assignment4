#ifndef LFS_H
#define LFS_H

#define BLOCK_SIZE 256
#define SEGMENT_SIZE 1024

#define MAX_NO_INODES 16 //TODO: change later
#define NO_SEGMENTS 64

#define MAX_NAME_LENGTH 8
#define MAX_DIRECT_BLOCKS 16

#define DIRECTORY 0
#define FILE 1
#define CLEANUP 2

#define READ 0
#define WRITE 1
#define READWRITE 2
//#define MAX_INDIRECT_POINTERS 16

typedef struct inode {
  //char name[MAX_NAME_LENGTH];
  char *name;
  int ID;
  int type; // 0 is directory, 1 is a file (2 for cleanup)
  int size;
  int d_pointer_count;
  int access; // 0 is read, 1 is write, 2 is read/write
  time_t t_accessed;
  time_t t_modified;
  size_t d_data_pointers[MAX_DIRECT_BLOCKS];
  //size_t id_data_pointers[MAX_INDIRECT_POINTERS];
}inode;

typedef struct entry {
  char *path;
  inode *ino;
}entry;

int lfs_getattr(const char *, struct stat *);
int lfs_mknod(const char *path, mode_t mode, dev_t dev);
int lfs_mkdir(const char *path, mode_t mode);
int lfs_unlink(const char *path);
int lfs_rmdir(const char *path);
int lfs_write(const char *path, const char *data, size_t size, off_t off, struct fuse_file_info *fi);
int lfs_readdir(const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *);
int lfs_open(const char *, struct fuse_file_info *);
int lfs_read(const char *, char *, size_t, off_t, struct fuse_file_info *);
int lfs_release(const char *path, struct fuse_file_info *fi);
int lfs_utime(const char *path, struct utimbuf *utim);

inode *get_ino(const char *path);
char *get_name(const char *path);
int get_new_ID(const char *path);

#endif
