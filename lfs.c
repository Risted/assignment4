#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <libgen.h>

#include "lfs.h"



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
	.rename = NULL,
	.utime = lfs_utime
};

entry **ino_table;

int current_segment;
int current_block;
int blocks_per_segment = SEGMENT_SIZE / BLOCK_SIZE;
void *disc;

int initialize( void ) {
	int i;
	int res;

	printf("INITIALIZING\n");

	current_segment = 0;
	current_block = 0;
	disc = malloc(NO_SEGMENTS*SEGMENT_SIZE);

	if (disc == NULL) {
		return -ERESTART;
	}

	ino_table = malloc(MAX_NO_INODES*sizeof(size_t));

	if (ino_table == NULL) {
		free(disc);
		return -ERESTART;
	}

	// Set all table entries to NULL
	for (i = 0; i < MAX_NO_INODES; i++) {
		ino_table[i] = NULL;
	}

	// Make root
	res = lfs_mkdir("/", S_IRWXO);
	if (res != 0) {
		return res;
	}

	printf("FINISH INITIALIZING\n");
	return 0;
}

int lfs_getattr(const char *path, struct stat *stbuf) {
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

	if (strcmp("/", path) == 0) {
		printf("THIS HAPPENS\n");
	}

	ino = get_ino(path);

	if (ino == NULL) {
		printf("getattr: finish, no entry\n");
		return -ENOENT; // TODO: better error code
	}

	printf("ino_ID: %d\n", ino->ID);

	if(strcmp("/", path) == 0){
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_size = ino->size;
		stbuf->st_atime = ino->t_accessed;
		stbuf->st_mtime = ino->t_modified;
		stbuf->st_ctime = time(NULL);
		//return 0;
		//we in root yo!
	}

	if (ino->type == 0) { 			// Directory
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_size = ino->size;
		stbuf->st_atime = ino->t_accessed;
		stbuf->st_mtime = ino->t_modified;
		stbuf->st_ctime = time(NULL);
	} else if (ino->type == 1) { 										// File
		stbuf->st_mode = S_IFREG | 0777;
		// stbuf->st_nlink = 1;
		stbuf->st_size = ino->size;
		stbuf->st_atime = ino->t_accessed;
		stbuf->st_mtime = ino->t_modified;
		stbuf->st_ctime = time(NULL);
	}

	// else if( strcmp( path, "/hello" ) == 0 ) {
	// 	stbuf->st_mode = S_IFREG | 0777;
	// 	stbuf->st_nlink = 1;
	// 	stbuf->st_size = 12;
	// } else {
	// 	res = -ENOENT;
	// }

	// return res;
	printf("getattr: finish\n");
	return 0;
}

int lfs_readdir( const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ) {
	inode *ino;
	entry *ent;
	char *path_copy;
	int ino_idx;
	int i;
	(void) offset;
	(void) fi;
	printf("readdir: (path=%s)\n", path);

	// if(strcmp(path, "/") != 0) {
	// 	return -ENOENT;
	// }

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	// filler(buf, "hello", NULL, 0);

	ino = get_ino(path);


	if (ino == NULL) {
		return -ENOTDIR;
	}

	path_copy = malloc(strlen(path)+1);
	if (path_copy == NULL) {
		return -ENOMEM;
	}
	// memcpy(path_copy, path, strlen(path)+1);

	// path_copy = basename(path_copy);
	// basename(path_copy);

	//for (i = 0; i < ino->d_pointer_count; i++) {
	for (i = 0; i < 16; i++) {
		ino_idx = ino->d_data_pointers[i];
		printf("Test!! %d\n", ino_idx);

		if (ino_idx != -1) {
			ent = ino_table[ino_idx];
			memcpy(path_copy, ent->path, strlen(ent->path)+1);
			path_copy = basename(path_copy);
			printf("File: %s\n", path_copy);
			filler(buf, path_copy, NULL, 0);
		}
	}

	printf("readdir: finish\n");
	return 0;
}

// Returns the name of the path. e.g. <dir1>/<dir2>/<file> shuld return <file>


// Ask the table for a new entry.
int get_new_ID(const char *path) {
	int ID = 0;
	entry *ent;

	printf("get_new_ID: begin\n");

	while((ent = ino_table[ID]) != NULL) {
		ID++;
		if (ID >= MAX_NO_INODES) {
			return -1; //TODO find error. No free space in ino_table
		}
	}

	printf("get_new_ID: finish\n");
	return ID;
}

int make_ino(const char *path, int type, int access) {
	inode *ino;
	inode *parent_ino;
	entry *ent;
	char *path_copy;
	char *path_copy2;
	char *parent_path;
	int i;

	time_t create_time = time(NULL);

	printf("make_ino: begin\n");


	// Simple checking of parameters
	if (type < 0 || type > 1) {
		return -EINVAL;
	}

	if (access < 0 || access > 2) {
		return -EINVAL;
	}
	// End of checks

	// Allocations
	path_copy = malloc(strlen(path)+1);
	path_copy2 = malloc(strlen(path)+1);

	if (path_copy == NULL) {
		return -ENOMEM;
	}

	memcpy(path_copy, path, strlen(path)+1);
	memcpy(path_copy2, path, strlen(path)+1);

	ino = malloc(sizeof(inode));

	if (ino == NULL) {
		free(path_copy);
		return -ENOMEM;
	}

	memset(ino, 0, sizeof(inode));
	// End of allocations

	// Set up the new inode
	ino->name = basename(path_copy);
	ino->ID = get_new_ID(path);

	if (ino->ID  == -1) {
		free(path_copy);
		free(ino);
		return -ENFILE;
	}

	ino->type = type;
	ino->access = access;
	ino->t_accessed = create_time;
	ino->t_modified = create_time;

	for (i = 0; i < MAX_DIRECT_BLOCKS; i++) {
		ino->d_data_pointers[i] = -1;
	}
	// End of setup

	// Allocation and setup of entry
	ent = malloc(sizeof(entry));

	if (ent == NULL) {
		free(path_copy);
		free(ino);
		return -ENOMEM;
	}

	// new_path = malloc(strlen(path)+1);
	//
	// if (new_path == NULL) {
	// 	free(path_copy);
	// 	free(ino);
	// 	free(ent);
	// 	return -ENOMEM;
	// }
	// memcpy(new_path, path, strlen(path)+1);
	// ent->path = new_path;

	ent->path = path_copy;
	ent->ino = ino;

	ino_table[ino->ID] = ent;
	// End of setup entry

	// Update parent
	parent_path = dirname(path_copy2);
	parent_ino = get_ino(parent_path);

	if (parent_ino == NULL) {
		//FAIL
	}

	for (i = 0; i < MAX_DIRECT_BLOCKS; i++) {
		if (parent_ino->d_data_pointers[i] == -1) {
			parent_ino->d_data_pointers[i] = ino->ID;
			parent_ino->d_pointer_count++;
			break;
		}
	}


	printf("make_ino: finish\n");

	return 0;
}

int rm_ino(const char *path) {
	inode *ino;
	inode *parent_ino;
	entry *ent;
	char *path_copy;
	char *parent_path;
	int i;

	printf("rm_ino: begin\n");

	ino = get_ino(path);
	if (ino == NULL) {
		return -ERESTART;
	}
	ent = ino_table[ino->ID];
	if (ent == NULL) {
		return -ERESTART;
	}
	// If it is a file, free all blocks
	if (ino->type == FILE) {
		for (i = 0; i < MAX_DIRECT_BLOCKS; i++) {
			free((void *) ino->d_data_pointers[i]);
		}
	}
	ino_table[ino->ID] = NULL;

	path_copy = malloc(strlen(path)+1);

	if (path_copy == NULL) {
		return -ENOMEM;
	}

	memcpy(path_copy, path, strlen(path)+1);

	parent_path = dirname(path_copy);
	parent_ino = get_ino(parent_path);
	// remove_kebab();

	if (parent_ino == NULL) {
		return -ENOTDIR;
	}

	for (i = 0; i < MAX_DIRECT_BLOCKS; i++) {
		if (parent_ino->d_data_pointers[i] == ino->ID) {
			parent_ino->d_data_pointers[i] = -1;
			break;
		}
	}

	free(ent->path);
	free(ent);
	free(ino);

	printf("rm_ino: finish\n");

	return 0;
}

inode *get_ino(const char *path) {
	int i;
	entry *e;

	printf("get_ino: begin\n");

	for (i = 0; i < MAX_NO_INODES; i++) {
		e = ino_table[i];

		if (e != NULL) {
			// printf("looking at: %s\n", e->path);
			if (strcmp(e->path, path) == 0) {
				printf("get_ino: finish, found entry\n");
				return e->ino;
			}
		}
	}

	printf("get_ino: finish, no entry\n");

	return NULL;
}
//
// int get_dir_ino() {
//
// }
//
// int get_file_ino() {
//
// }

int lfs_mknod(const char *path, mode_t mode, dev_t dev) {
	int res;

	printf("lfs_mknod: begin\n");

	res = make_ino(path, FILE, READWRITE);



	if (res != 0) {
		printf("lfs_mknod: finish, res: %d\n", res);
		return res;
	}
	printf("lfs_mknod: finish, no problem\n");
	return res;
}

int lfs_mkdir(const char *path, mode_t mode) {
	int res;

	printf("lfs_mkdir: begin\n");

	// TODO: Make sure path is ending with "/"
	res = make_ino(path, DIRECTORY, READWRITE);

	if (res != 0) {
		return res;
	}

	// TODO: Add . and .. to the directory, maybe not though,
	// remember to check if root -> .. is also itself
	printf("lfs_mkdir: finish\n");
	return res;
}

int lfs_unlink(const char *path) {
	return 0;
}

// TODO: new_ino is allocated on the stack, should be in a segment instead
int lfs_rmdir(const char *path) {
	inode *ino;
	int sub_idx;
	entry *sub_ent;
	inode *sub_ino;
	// inode *new_ino;
	int i;
	int res;

	printf("lfs_rmdir: begin\n");

	ino = get_ino(path);
	//free(ino)
	if (ino == NULL || ino->type == FILE) {
		return -ENOTDIR;
	}
	// memcpy(new_ino, ino, sizeof(inode));
	// First call recursively on all sub dirs and simply remove sub files
	for (i = 0; i < MAX_DIRECT_BLOCKS; i++) {
		sub_idx = ino->d_data_pointers[i];
		if (sub_idx != -1) {
			sub_ent = ino_table[sub_idx];
			sub_ino = sub_ent->ino;

			if (sub_ino->type == DIRECTORY) {
				res = lfs_rmdir(sub_ent->path);
				if (res != 0) {
					return res;
				}
			} else if (sub_ino->type == FILE) { // IF might be redundant
				res = rm_ino(sub_ent->path);
				if (res != 0) {
					return res;
				}
			}

			ino->d_data_pointers[i] = -1;
		}
	}

	// Then remove the actual directory
	res = rm_ino(path);
	if (res != 0) {
		return res;
	}

	// new_ino->type = CLEANUP;
	// ino_table[new_ino->ID]->ino = new_ino;
	printf("lfs_rmdir: finish\n");
	return 0;
}

int lfs_truncate(const char *path, off_t size, struct fuse_file_info *fi) {
	return 0;
}

//Permission
int lfs_open(const char *path, struct fuse_file_info *fi) {
  printf("open: (path=%s)\n", path);
	return 0;
}

int lfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	inode *ino;
	int i;

	printf("read: (path=%s)\n", path);

	ino = get_ino(path);

	if (ino == NULL) {
		return -ENOTDIR;
	}

	for (i = 0; i < MAX_DIRECT_BLOCKS; i++) {
		if (ino->d_data_pointers[i] == -1) {
			break;
		}
	}

	memcpy(buf, (void *) ino->d_data_pointers[i-1], size);
	return size;
}

int lfs_release(const char *path, struct fuse_file_info *fi) {
	printf("release: (path=%s)\n", path);
	return 0;
}

int lfs_write(const char *path, const char *data, size_t size, off_t off, struct fuse_file_info *fi) {
	inode *ino;
	void *new_data;
	int res;
	int i;
	// inode *new_ino;

	printf("lfs_write: begin\n");
	ino = get_ino(path);
	if (ino == NULL){
		printf("This is weird, we shouldn't be here.\nFuck you, get outta here.\n");
		res = make_ino(path, FILE, READWRITE);
		if (res != 0) {
			printf("Double weirdness.\n");
			return -ERESTART;
		}
	}

	new_data = malloc(size);

	if (new_data == NULL) {
		return -ENOMEM;
	}

	memcpy(new_data, data, size);

	for(i = 0; i < MAX_DIRECT_BLOCKS; i++) {
		if (ino->d_data_pointers[i] == -1) {
			ino->d_data_pointers[i] = (size_t) new_data;
			ino->size = size;
			ino->d_pointer_count++;
			break;
		}
	}
	if (i >= MAX_DIRECT_BLOCKS) {
		printf("No more blocks\n");
	}

	printf("lfs_write: finish\n");
	return size;
}

int purge (inode *ino){
	printf("purge: begin\n");
	//should free EVERYTHING allocated to this ino including sup pointers
	free(ino->d_data_pointers);
	//free(ino->id_data_pointers);
	free(ino);
	printf("purge: finish\n");
	return 0;
}

int cleaner( void ) {
	int i;
	int res = 0;
	printf("cleaner: begin\n");
	for (i = 0; i< MAX_NO_INODES ; i++){
		inode *temp;
		temp = ino_table[i]->ino;
		if (temp->type == 2){
			res++;
			purge(temp);
		}
	}
	printf("cleaner: finish\n");
	return res;
}

int lfs_utime(const char *path, struct utimbuf *utim) {
	return 0;
}

int main( int argc, char *argv[] ) {
	int res;
	printf("main: begin\n");
	res = initialize();
	if (res != 0) {
		printf("Couldn't initialize. Try again.\n");
		return -ERESTART;
	}
	fuse_main( argc, argv, &lfs_oper );

	printf("main: finish\n");
	return 0;
}
