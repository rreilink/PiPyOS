#include "dirent_os.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "initfs.h"

typedef struct {
    unsigned int name_offs;
    unsigned int data_offs;
    unsigned int size;

} initfs_rec_t;

extern char _binary_initfs_bin_start;

int PiPyOS_initfs_open(initfs_openfile_t *file, const char *pathname, int flags) {
    initfs_rec_t *r = (initfs_rec_t *)&_binary_initfs_bin_start;
    char *s = &_binary_initfs_bin_start;
    
    for(int i=0;r[i].data_offs;i++) {
        if (strcmp(&s[r[i].name_offs], pathname)==0) {
            if (r[i].size==0xffffffff) {
                return -1; // Trying to open a directory
            } else {
                file->data = &s[r[i].data_offs];
                file->pos = 0;
                file->size = r[i].size;
                return 0;
            }
        
        }
    }

    errno = ENOENT;
    return -1;

}

int PiPyOS_initfs_read(initfs_openfile_t *file, void *buf, size_t count) {
    int n = file->size - file->pos;
    
    if (n>((int)count)) n = count;
    
    memcpy(buf, file->data + file->pos, n);
    file->pos += n;

    return n;
}


int PiPyOS_initfs_opendir(const char *pathname, DIR *dir) {
    initfs_rec_t *r = (initfs_rec_t *)&_binary_initfs_bin_start;
    char *s = &_binary_initfs_bin_start;

    int length;

    // strip trailing slashes
    length = strlen(pathname);
    while(length && pathname[length-1] == '/') length--;
    
    // find the directory
    for(int i=0; r[i].data_offs; i++) {
        if (strncmp(&s[r[i].name_offs], pathname, length)==0) {
            if (r[i].size==0xffffffff) {
                dir->initfs.idx_parent = i;
                dir->initfs.idx_cur = i+1;
                return 0;                 
            }
        }
    }
    errno = ENOENT;
    return -1;
    
}

/* Find the next item in the directory listing
 *
 * Returns 0 on succes, 1 on end-of-directory
 * On failure, -1 is returned and errno is set appropiately
 */

int PiPyOS_initfs_readdir(DIR *dirp) {
    initfs_rec_t *r = (initfs_rec_t *)&_binary_initfs_bin_start;
    char *s = &_binary_initfs_bin_start;
    char *parent_name = &s[r[dirp->initfs.idx_parent].name_offs];

    int l = strlen(parent_name);

    // Loop over all remaining directory entries
    for (; r[dirp->initfs.idx_cur].data_offs; dirp->initfs.idx_cur++) {
    
        char *current_name = &s[r[dirp->initfs.idx_cur].name_offs];

        // if this dir or file is not a subdir of the parent, we are done
        if (strncmp(current_name, parent_name, l)!=0) {
            break;
        }
        
        char *subpath = current_name + l; // the part below the parent
        if (*subpath=='/') { // should always be true, but check to be sure
            subpath++; // strip leading /
            if (!strchr(subpath, '/')) {
                // No / in subpath, so it is a direct child (no file in a sub-dir)
                            
                l = strlen(subpath);
                if (l >= NAME_MAX) {
                    errno = ENAMETOOLONG;
                    return -1; //filename too long
                }
                
                // Copy info of the current listing into dirp->dirent
                memcpy(dirp->dirent.d_name, subpath, l+1);
                dirp->dirent.d_ino = dirp->initfs.idx_cur;
                dirp->initfs.idx_cur++;
                return 0;                
                
            }
        } 

    }

    return 1;
}


int PiPyOS_initfs_stat(const char *pathname, struct stat *buf) {

    initfs_rec_t *r = (initfs_rec_t *)&_binary_initfs_bin_start;
    char *s = &_binary_initfs_bin_start;
    int i=0;
    
    while(r[i].data_offs) {
        if (strcmp(&s[r[i].name_offs], pathname)==0) {
            if (r[i].size==0xffffffff) {
                buf->st_size = 0;
                buf->st_blocks = 0;
                buf->st_mode = S_IFDIR | S_IRUSR | S_IXUSR;
                goto found;
            } else {
                buf->st_size = r[i].size;
                buf->st_blocks = (r[i].size+511)/512;
                buf->st_mode = S_IFREG | S_IRUSR;
                goto found;
            }
        
        }
        i++;
    }
    errno = ENOENT;
    return -1;
found:

    buf->st_dev = 0;
    buf->st_ino = 0;

    buf->st_nlink = 1;
    buf->st_uid = 0;
    buf->st_gid = 0;
    buf->st_rdev = 0;
    buf->st_atime = 0;
    buf->st_mtime = 0;
    buf->st_ctime = 0;
    buf->st_blksize = 512;


    return 0;
}
