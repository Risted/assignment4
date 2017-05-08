#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "lfs.h"



static struct fuse_operations lfs_oper = {
	.getattr	= lfs_getattr,
	.readdir	= lfs_readdir,
	.mknod = NULL,
	.mkdir = NULL,
	.unlink = NULL,
	.rmdir = NULL,
	.truncate = NULL,
	.open	= lfs_open,
	.read	= lfs_read,
	.release = lfs_release,
	.write = NULL,
	.rename = NULL,
	.utime = NULL
};

entry ino_table[MAX_NO_INODES];

inode *get_ino(const char *path) {
	int i;
	entry e;
	for (i = 0; i < MAX_NO_INODES; i++) {
		e = ino_table[i];
		if (strcmp(e.path, path) == 0) {
			return e.ino;
		}
	}
	return NULL;
}

int lfs_getattr( const char *path, struct stat *stbuf ) {
	// int res = 0;
	inode *ino;

	printf("getattr: (path=%s)\n", path);

	memset(stbuf, 0, sizeof(struct stat));
	// if( strcmp( path, "/" ) == 0 ) {
	// 	// stbuf->st_mode = S_IFDIR | 0755;
	// 	// stbuf->st_nlink = 2;
	// 	stbuf->st_size = ino->size;
	// 	stbuf->st_atime = ino->t_accessed;
	// 	stbuf->st_mtime = ino->t_modified;
	// 	return 0;
	// }

	ino = get_ino(path);

	if (ino == NULL) {
		return -ENOMEM; // TODO: better error code
	}

	if (ino->type == 0) { 			// Directory
		stbuf->st_size = ino->size;
		stbuf->st_atime = ino->t_accessed;
		stbuf->st_mtime = ino->t_modified;
	} else { 										// File
		// stbuf->st_mode = S_IFREG | 0777;
		// stbuf->st_nlink = 1;
		stbuf->st_size = ino->size;
		stbuf->st_atime = ino->t_accessed;
		stbuf->st_mtime = ino->t_modified;
	}

	// else if( strcmp( path, "/hello" ) == 0 ) {
	// 	stbuf->st_mode = S_IFREG | 0777;
	// 	stbuf->st_nlink = 1;
	// 	stbuf->st_size = 12;
	// } else {
	// 	res = -ENOENT;
	// }

	// return res;
	return 0;
}

int lfs_readdir( const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ) {
	(void) offset;
	(void) fi;
	printf("readdir: (path=%s)\n", path);

	if(strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, "hello", NULL, 0);

	return 0;
}

//Permission
int lfs_open( const char *path, struct fuse_file_info *fi ) {
    printf("open: (path=%s)\n", path);
	return 0;
}

int lfs_read( const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi ) {
    printf("read: (path=%s)\n", path);
	memcpy( buf, "Hello\n", 6 );
	return 6;
}

int lfs_release(const char *path, struct fuse_file_info *fi) {
	printf("release: (path=%s)\n", path);
	return 0;
}

int main( int argc, char *argv[] ) {
	fuse_main( argc, argv, &lfs_oper );

	return 0;
}
