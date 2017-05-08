#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int lfs_getattr( const char *, struct stat * );
int lfs_readdir( const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info * );
int lfs_open( const char *, struct fuse_file_info * );
int lfs_read( const char *, char *, size_t, off_t, struct fuse_file_info * );
int lfs_release(const char *path, struct fuse_file_info *fi);

static struct fuse_operations lfs_oper = {
	.getattr	= lfs_getattr,
	.readdir	= lfs_readdir,
	.mknod = lfs_mknod,
	.mkdir = lfs_mkdir,
	.unlink = lfs_unlink,
	.rmdir = lfs_rmdir,
	.truncate = lfs_truncate,
	.open	= lfs_open,
	.read	= lfs_read,
	.release = lfs_release,
	.write = lfs_write,
	.rename = lfs_rename,
	.utime = lfs_utime
};
int *inodeMap;
/*
int lfs_symlink(const char *from, const char *to)
{
	int res;

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}
*/
int lfs_utime(const char *path, const struct timespec ts[2],
		       struct fuse_file_info *fi){
	(void) fi;
	int res;

	/* don't use utime/utimes since they follow symlinks */
	res = utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
	if (res == -1)
		return -errno;

	return 0;
}

int lfs_mknod(const char *path, mode_t mode, dev_t rdev){
	int res;

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(path, mode);
	else
		res = mknod(path, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

int lfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	int fd;
	int res;

	(void) fi;
	if(fi == NULL)
		fd = open(path, O_WRONLY);
	else
		fd = fi->fh;

	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	if(fi == NULL)
		close(fd);
	return res;
}

int lfs_truncate(const char *path, off_t size, struct fuse_file_info *fi){
	int res;

	if (fi != NULL)
		res = ftruncate(fi->fh, size);
	else
		res = truncate(path, size);
	if (res == -1)
		return -errno;

	return 0;
}

int lfs_rename(const char *from, const char *to, unsigned int flags){
	int res;

	if (flags)
		return -EINVAL;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

int lfs_rmdir(const char *path){
	int res;

	res = rmdir(path);
	if (res == -1)
		return -errno;

	return 0;
}

int lfs_unlink(const char *path){
	int res;
	res = unlink(path);
	if (res == -1){
		return -errno;
	}
	return 0;
}

int lfs_getattr( const char *path, struct stat *stbuf ) {
	memset(stbuf, 0, sizeof(struct stat));
	printf("getattr: (path=%s)\n", path);
	inode *node = malloc(sizeof(inode));
	if(strcmp("/", path) == 0){
		memcpy()
	}
	return 0;
}
int lfs_mkdir(const char *path, mode_t mode){
	int res;
	res = mkdir(path, mode);
	if (res == -1)
		return -errno;
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
int init(){
	int i, j;
	//TODO determne MAX_INODES
	inodeArray = malloc( MAX_INODES * sizeof(int));
	current = 0;
	disk = malloc(SEGMENT_SIZE * NR_INODES);

		
	return 0;
}
int main( int argc, char *argv[] ) {
	init();
	fuse_main( argc, argv, &lfs_oper );
	return 0;
}
