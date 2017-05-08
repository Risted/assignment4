#ifndef LFS_H
#define LFS_H

#define BLOCK_SIZE 256
#define SEGMENT_SIZE 1024

#define MAX_NO_INODES 1024

#define MAX_NAME_LENGTH 8
#define MAX_DIRECT_BLOCKS 16
//#define MAX_INDIRECT_POINTERS 16

typedef struct inode {
  char name[MAX_NAME_LENGTH];
  int ID;
  int type;
  int size;
  int access;
  time_t t_accessed;
  time_t t_modified;
  size_t d_data_pointers[MAX_DIRECT_BLOCKS];
  //size_t id_data_pointers[MAX_INDIRECT_POINTERS];
}inode;

int lfs_getattr( const char *, struct stat * );
int lfs_readdir( const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info * );
int lfs_open( const char *, struct fuse_file_info * );
int lfs_read( const char *, char *, size_t, off_t, struct fuse_file_info * );
int lfs_release(const char *path, struct fuse_file_info *fi);

#endif
