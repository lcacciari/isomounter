#include "config.h"
#include "if_utils.h"
#include <errno.h>
#include <string.h>

/**
 * The file system operations:
 *
 * Most of these should work very similarly to the well known UNIX
 * file system operations.  A major exception is that instead of
 * returning an error in 'errno', the operation should return the
 * negated error value (-errno) directly.
 *
 * All methods are optional, but some are essential for a useful
 * filesystem (e.g. getattr).  Open, flush, release, fsync, opendir,
 * releasedir, fsyncdir, access, create, ftruncate, fgetattr, lock,
 * init and destroy are special purpose methods, without which a full
 * featured filesystem can still be implemented.
 *
 * Almost all operations take a path which can be of any length.
 *
 * Changed in fuse 2.8.0 (regardless of API version)
 * Previously, paths were limited to a length of PATH_MAX.
 *
 * See http://fuse.sourceforge.net/wiki/ for more information.  There
 * is also a snapshot of the relevant wiki pages in the doc/ folder.
 */

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * TODO check that FUSE handle a NULL return as error.
 */
static void * if_init(struct fuse_conn_info *conn) {
  if_status * status = get_status();
  g_debug("opening imagefile %s",status->path);
  status->fh = iso9660_open(status->path);
  if (status->fh == NULL) {
    // TODO check return value
    g_error("Failed to open image at %s",status->path);
    status->phase = IN_ERROR;
    return NULL;
  }
  status->phase = AFTER_MOUNT;
  return status;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
static void if_destroy(void * data) {
  if_status * status = (if_status *) data;
  g_debug("if_destroy called");
  g_debug("closing image at %s",status->path);
  // TODO check errors?
  iso9660_close(status->fh);
  status->phase = AFTER_UMOUNT;
}


/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.	 The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
static int if_getattr(const char * path, struct stat * p_stat) {
  iso9660_t * iso = get_status()->fh;
  g_debug("getatr called for %s",path);
  iso9660_stat_t * info =  iso9660_ifs_stat(iso,path);
  if (info == NULL) {
    // file not found
    g_debug("file not found: %s",path);
    return -ENOENT;
  }
  int result = translate_stat(info,p_stat);
  g_free(info);
  return result;
}

/*
 * Directories handling
 */


/** Open directory
 *
 * Unless the 'default_permissions' mount option is given,
 * this method should check if opendir is permitted for this
 * directory. Optionally opendir may also return an arbitrary
 * filehandle in the fuse_file_info structure, which will be
 * passed to readdir, closedir and fsyncdir.
 *
 * Introduced in version 2.3
 */
static int if_opendir(const char * path, struct fuse_file_info * info) {
  iso9660_t * iso = get_status()->fh;
  iso9660_stat_t * stats = iso9660_ifs_stat(iso,path);  
  if (stats == NULL) {
    return - ENOENT;
  }
  if (!IS_DIRECTORY(stats)) {
    g_free(stats);
    return - ENOTDIR;
  }
  char * path_copy = g_strdup(path);
  if (path_copy == NULL) {
    g_free(stats);
    return -ENOMEM;
  }
  if_dir * data = g_malloc0(sizeof(if_dir));
  if (data == NULL) {
    g_free(path_copy);
    g_free(stats);
    return -ENOMEM;
  }
  data->path = path_copy;
  data->info = stats;
  // we don't realy need this, but maybe we could use it for not
  // reading a directory completly
  info->fh = (intptr_t) data;
  return 0;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
static int if_readdir(const char * path, void * buf, fuse_fill_dir_t filler,
	       off_t offset,struct fuse_file_info * info) {
  g_debug("if_readdir called");
  CdioList_t * list;
  CdioListNode_t * node;
  int result = 0;
  iso9660_t * iso = get_status()->fh;
  if_dir * data = (if_dir *) (uintptr_t) info->fh;
  list = iso9660_ifs_readdir(iso,data->path);
  if (list == NULL) {
    return - EIO;
  }
#ifdef _DIRENT_HAVE_D_TYPE
  // we could use this for retuning the data one at the time
  off_t off = 0;
#endif
  _CDIO_LIST_FOREACH(node,list) {
    struct dirent dh;
    iso9660_stat_t * stats = (iso9660_stat_t *) _cdio_list_node_data(node);
    iso9660_name_translate(stats->filename,dh.d_name);
#ifdef _DIRENT_HAVE_D_TYPE
    dh.d_off = off++;
#endif
    int rc = filler(buf,dh.d_name,NULL,0);
    if (rc != 0) {
      result = - ENOMEM;
      break;
    }
  }
  _cdio_list_free(list,true);
  return result;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
static int if_releasedir(const char * path, struct fuse_file_info * info) {
  g_debug("if_releasedir called");
  iso9660_t * iso = get_status()->fh;
  if_dir * data = (if_dir *) (uintptr_t) info->fh;
  // just destroy the user data
  g_free(data->path);
  g_free(data->info);
  g_free(data);
  return 0;
}

/*
 * File ops
 */
/** File open operation
 *
 * No creation (O_CREAT, O_EXCL) and by default also no
 * truncation (O_TRUNC) flags will be passed to open(). If an
 * application specifies O_TRUNC, fuse first calls truncate()
 * and then open(). Only if 'atomic_o_trunc' has been
 * specified and kernel version is 2.6.24 or later, O_TRUNC is
 * passed on to open.
 *
 * Unless the 'default_permissions' mount option is given,
 * open should check if the operation is permitted for the
 * given flags. Optionally open may also return an arbitrary
 * filehandle in the fuse_file_info structure, which will be
 * passed to all file operations.
 *
 * Changed in version 2.2
 */
static int if_open(const char * path, struct fuse_file_info * info) {
  iso9660_t * iso = get_status()->fh;
  iso9660_stat_t * stats = iso9660_ifs_stat(iso,path);  
  if (stats == NULL) {
    return - ENOENT;
  }
  if (IS_DIRECTORY(stats)) {
    g_free(stats);
    return - EISDIR;
  }
  info->fh = (intptr_t) stats;
  return 0;  
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.	 An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
static int if_read(const char * path,
	    char * buf,size_t size, off_t offset,struct fuse_file_info * info) {
  iso9660_t * iso = get_status()->fh;
  iso9660_stat_t * stats = (iso9660_stat_t *) (uintptr_t) info->fh;
  off_t bk_offset = offset / ISO_BLOCKSIZE;
  off_t off = offset % ISO_BLOCKSIZE; // offset in first bock
  // calculate how many block we must read
  off_t bk_size = (size + off + ISO_BLOCKSIZE - 1) / ISO_BLOCKSIZE;
  size_t bytes_read = 0;
  for (int block = 0; block < bk_size; block++) {
    char data[ISO_BLOCKSIZE];
    const lsn_t lsn = stats->lsn + bk_offset + block;
    size_t read = iso9660_iso_seek_read(iso,data,lsn,1);
    if (read != ISO_BLOCKSIZE) {
      return -EIO;
    }
    /* In this block there are at most ISO_BLOCKSIZE - off
     * of the remaining size - bytes_read bytes to read
     * (note that off is possibly not zero only on first pass (se below)
     */
    size_t remains_to_read = size - bytes_read;
    size_t good_in_block = ISO_BLOCKSIZE - off;
    size_t actually_read =
      remains_to_read < good_in_block ? remains_to_read : good_in_block;
    //     
    memcpy(buf + bytes_read,data + off,actually_read);
    bytes_read += actually_read;
    // blocks following the first are read from the beginning
    off = 0;
  }
  return bytes_read;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.	 It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
static int if_release(const char * path, struct fuse_file_info * info) {
  iso9660_stat_t * stats = (iso9660_stat_t *) (uintptr_t) info->fh;
  g_free(stats);
  info->fh = 0;
  return 0;
}



struct fuse_operations isofuse_ops = {
  .getattr = if_getattr,
  // AFAIK thre is no symlink in a CD
  .readlink = NULL,
  // no .getdir -- that's deprecated
  .getdir = NULL,
  // Can't create or destroy files or directories, nor change names or modes
  // So, all method from here
  .mknod = NULL,
  .mkdir = NULL,
  .unlink = NULL,
  .rmdir = NULL,
  .symlink = NULL,
  .rename = NULL,
  .link = NULL,
  .chmod = NULL,
  .chown = NULL,
  .truncate = NULL,
  // to here, are left undefined
  .utime = NULL,
  .open = if_open,
  .read = if_read,
  // read only filesystem: no write allowed
  .write = NULL,
  /** Just a placeholder, don't set */ // huh???
  .statfs = NULL,
  .flush = NULL,
  .release = if_release,
  .fsync = NULL,
  
#ifdef HAVE_SYS_XATTR_H
  .setxattr = NULL,
  .getxattr = NULL,
  .listxattr = NULL,
  .removexattr = NULL,
#endif
  
  .opendir = if_opendir,
  .readdir = if_readdir,
  .releasedir = if_releasedir,
  // no write, so no sync
  .fsyncdir = NULL,
  .init = if_init,
  .destroy = if_destroy,
  .access = NULL,
  // read only systems do not need create or truncate
  .create = NULL,
  .ftruncate = NULL,
  // this is currently called only after create, which we don't use, thus...
  .fgetattr = NULL
};


/* struct fuse_operations { */
/* 	/\** Get file attributes. */
/* 	 * */
/* 	 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are */
/* 	 * ignored.	 The 'st_ino' field is ignored except if the 'use_ino' */
/* 	 * mount option is given. */
/* 	 *\/ */
/* 	int (*getattr) (const char *, struct stat *); */

/* 	/\** Read the target of a symbolic link */
/* 	 * */
/* 	 * The buffer should be filled with a null terminated string.  The */
/* 	 * buffer size argument includes the space for the terminating */
/* 	 * null character.	If the linkname is too long to fit in the */
/* 	 * buffer, it should be truncated.	The return value should be 0 */
/* 	 * for success. */
/* 	 *\/ */
/* 	int (*readlink) (const char *, char *, size_t); */

/* 	/\* Deprecated, use readdir() instead *\/ */
/* 	int (*getdir) (const char *, fuse_dirh_t, fuse_dirfil_t); */

/* 	/\** Create a file node */
/* 	 * */
/* 	 * This is called for creation of all non-directory, non-symlink */
/* 	 * nodes.  If the filesystem defines a create() method, then for */
/* 	 * regular files that will be called instead. */
/* 	 *\/ */
/* 	int (*mknod) (const char *, mode_t, dev_t); */

/* 	/\** Create a directory  */
/* 	 * */
/* 	 * Note that the mode argument may not have the type specification */
/* 	 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the */
/* 	 * correct directory type bits use  mode|S_IFDIR */
/* 	 * *\/ */
/* 	int (*mkdir) (const char *, mode_t); */

/* 	/\** Remove a file *\/ */
/* 	int (*unlink) (const char *); */

/* 	/\** Remove a directory *\/ */
/* 	int (*rmdir) (const char *); */

/* 	/\** Create a symbolic link *\/ */
/* 	int (*symlink) (const char *, const char *); */

/* 	/\** Rename a file *\/ */
/* 	int (*rename) (const char *, const char *); */

/* 	/\** Create a hard link to a file *\/ */
/* 	int (*link) (const char *, const char *); */

/* 	/\** Change the permission bits of a file *\/ */
/* 	int (*chmod) (const char *, mode_t); */

/* 	/\** Change the owner and group of a file *\/ */
/* 	int (*chown) (const char *, uid_t, gid_t); */

/* 	/\** Change the size of a file *\/ */
/* 	int (*truncate) (const char *, off_t); */

/* 	/\** Change the access and/or modification times of a file */
/* 	 * */
/* 	 * Deprecated, use utimens() instead. */
/* 	 *\/ */
/* 	int (*utime) (const char *, struct utimbuf *); */

/* 	/\** File open operation */
/* 	 * */
/* 	 * No creation (O_CREAT, O_EXCL) and by default also no */
/* 	 * truncation (O_TRUNC) flags will be passed to open(). If an */
/* 	 * application specifies O_TRUNC, fuse first calls truncate() */
/* 	 * and then open(). Only if 'atomic_o_trunc' has been */
/* 	 * specified and kernel version is 2.6.24 or later, O_TRUNC is */
/* 	 * passed on to open. */
/* 	 * */
/* 	 * Unless the 'default_permissions' mount option is given, */
/* 	 * open should check if the operation is permitted for the */
/* 	 * given flags. Optionally open may also return an arbitrary */
/* 	 * filehandle in the fuse_file_info structure, which will be */
/* 	 * passed to all file operations. */
/* 	 * */
/* 	 * Changed in version 2.2 */
/* 	 *\/ */
/* 	int (*open) (const char *, struct fuse_file_info *); */

/* 	/\** Read data from an open file */
/* 	 * */
/* 	 * Read should return exactly the number of bytes requested except */
/* 	 * on EOF or error, otherwise the rest of the data will be */
/* 	 * substituted with zeroes.	 An exception to this is when the */
/* 	 * 'direct_io' mount option is specified, in which case the return */
/* 	 * value of the read system call will reflect the return value of */
/* 	 * this operation. */
/* 	 * */
/* 	 * Changed in version 2.2 */
/* 	 *\/ */
/* 	int (*read) (const char *, char *, size_t, off_t, */
/* 		     struct fuse_file_info *); */

/* 	/\** Write data to an open file */
/* 	 * */
/* 	 * Write should return exactly the number of bytes requested */
/* 	 * except on error.	 An exception to this is when the 'direct_io' */
/* 	 * mount option is specified (see read operation). */
/* 	 * */
/* 	 * Changed in version 2.2 */
/* 	 *\/ */
/* 	int (*write) (const char *, const char *, size_t, off_t, */
/* 		      struct fuse_file_info *); */

/* 	/\** Get file system statistics */
/* 	 * */
/* 	 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored */
/* 	 * */
/* 	 * Replaced 'struct statfs' parameter with 'struct statvfs' in */
/* 	 * version 2.5 */
/* 	 *\/ */
/* 	int (*statfs) (const char *, struct statvfs *); */

/* 	/\** Possibly flush cached data */
/* 	 * */
/* 	 * BIG NOTE: This is not equivalent to fsync().  It's not a */
/* 	 * request to sync dirty data. */
/* 	 * */
/* 	 * Flush is called on each close() of a file descriptor.  So if a */
/* 	 * filesystem wants to return write errors in close() and the file */
/* 	 * has cached dirty data, this is a good place to write back data */
/* 	 * and return any errors.  Since many applications ignore close() */
/* 	 * errors this is not always useful. */
/* 	 * */
/* 	 * NOTE: The flush() method may be called more than once for each */
/* 	 * open().	This happens if more than one file descriptor refers */
/* 	 * to an opened file due to dup(), dup2() or fork() calls.	It is */
/* 	 * not possible to determine if a flush is final, so each flush */
/* 	 * should be treated equally.  Multiple write-flush sequences are */
/* 	 * relatively rare, so this shouldn't be a problem. */
/* 	 * */
/* 	 * Filesystems shouldn't assume that flush will always be called */
/* 	 * after some writes, or that if will be called at all. */
/* 	 * */
/* 	 * Changed in version 2.2 */
/* 	 *\/ */
/* 	int (*flush) (const char *, struct fuse_file_info *); */

/* 	/\** Release an open file */
/* 	 * */
/* 	 * Release is called when there are no more references to an open */
/* 	 * file: all file descriptors are closed and all memory mappings */
/* 	 * are unmapped. */
/* 	 * */
/* 	 * For every open() call there will be exactly one release() call */
/* 	 * with the same flags and file descriptor.	 It is possible to */
/* 	 * have a file opened more than once, in which case only the last */
/* 	 * release will mean, that no more reads/writes will happen on the */
/* 	 * file.  The return value of release is ignored. */
/* 	 * */
/* 	 * Changed in version 2.2 */
/* 	 *\/ */
/* 	int (*release) (const char *, struct fuse_file_info *); */

/* 	/\** Synchronize file contents */
/* 	 * */
/* 	 * If the datasync parameter is non-zero, then only the user data */
/* 	 * should be flushed, not the meta data. */
/* 	 * */
/* 	 * Changed in version 2.2 */
/* 	 *\/ */
/* 	int (*fsync) (const char *, int, struct fuse_file_info *); */

/* 	/\** Set extended attributes *\/ */
/* 	int (*setxattr) (const char *, const char *, const char *, size_t, int); */

/* 	/\** Get extended attributes *\/ */
/* 	int (*getxattr) (const char *, const char *, char *, size_t); */

/* 	/\** List extended attributes *\/ */
/* 	int (*listxattr) (const char *, char *, size_t); */

/* 	/\** Remove extended attributes *\/ */
/* 	int (*removexattr) (const char *, const char *); */

/* 	/\** Open directory */
/* 	 * */
/* 	 * Unless the 'default_permissions' mount option is given, */
/* 	 * this method should check if opendir is permitted for this */
/* 	 * directory. Optionally opendir may also return an arbitrary */
/* 	 * filehandle in the fuse_file_info structure, which will be */
/* 	 * passed to readdir, closedir and fsyncdir. */
/* 	 * */
/* 	 * Introduced in version 2.3 */
/* 	 *\/ */
/* 	int (*opendir) (const char *, struct fuse_file_info *); */

/* 	/\** Read directory */
/* 	 * */
/* 	 * This supersedes the old getdir() interface.  New applications */
/* 	 * should use this. */
/* 	 * */
/* 	 * The filesystem may choose between two modes of operation: */
/* 	 * */
/* 	 * 1) The readdir implementation ignores the offset parameter, and */
/* 	 * passes zero to the filler function's offset.  The filler */
/* 	 * function will not return '1' (unless an error happens), so the */
/* 	 * whole directory is read in a single readdir operation.  This */
/* 	 * works just like the old getdir() method. */
/* 	 * */
/* 	 * 2) The readdir implementation keeps track of the offsets of the */
/* 	 * directory entries.  It uses the offset parameter and always */
/* 	 * passes non-zero offset to the filler function.  When the buffer */
/* 	 * is full (or an error happens) the filler function will return */
/* 	 * '1'. */
/* 	 * */
/* 	 * Introduced in version 2.3 */
/* 	 *\/ */
/* 	int (*readdir) (const char *, void *, fuse_fill_dir_t, off_t, */
/* 			struct fuse_file_info *); */

/* 	/\** Release directory */
/* 	 * */
/* 	 * Introduced in version 2.3 */
/* 	 *\/ */
/* 	int (*releasedir) (const char *, struct fuse_file_info *); */

/* 	/\** Synchronize directory contents */
/* 	 * */
/* 	 * If the datasync parameter is non-zero, then only the user data */
/* 	 * should be flushed, not the meta data */
/* 	 * */
/* 	 * Introduced in version 2.3 */
/* 	 *\/ */
/* 	int (*fsyncdir) (const char *, int, struct fuse_file_info *); */

/* 	/\** */
/* 	 * Initialize filesystem */
/* 	 * */
/* 	 * The return value will passed in the private_data field of */
/* 	 * fuse_context to all file operations and as a parameter to the */
/* 	 * destroy() method. */
/* 	 * */
/* 	 * Introduced in version 2.3 */
/* 	 * Changed in version 2.6 */
/* 	 *\/ */
/* 	void *(*init) (struct fuse_conn_info *conn); */
 


/* 	/\** */
/* 	 * Clean up filesystem */
/* 	 * */
/* 	 * Called on filesystem exit. */
/* 	 * */
/* 	 * Introduced in version 2.3 */
/* 	 *\/ */
/* 	void (*destroy) (void *); */

/* 	/\** */
/* 	 * Check file access permissions */
/* 	 * */
/* 	 * This will be called for the access() system call.  If the */
/* 	 * 'default_permissions' mount option is given, this method is not */
/* 	 * called. */
/* 	 * */
/* 	 * This method is not called under Linux kernel versions 2.4.x */
/* 	 * */
/* 	 * Introduced in version 2.5 */
/* 	 *\/ */
/* 	int (*access) (const char *, int); */

/* 	/\** */
/* 	 * Create and open a file */
/* 	 * */
/* 	 * If the file does not exist, first create it with the specified */
/* 	 * mode, and then open it. */
/* 	 * */
/* 	 * If this method is not implemented or under Linux kernel */
/* 	 * versions earlier than 2.6.15, the mknod() and open() methods */
/* 	 * will be called instead. */
/* 	 * */
/* 	 * Introduced in version 2.5 */
/* 	 *\/ */
/* 	int (*create) (const char *, mode_t, struct fuse_file_info *); */

/* 	/\** */
/* 	 * Change the size of an open file */
/* 	 * */
/* 	 * This method is called instead of the truncate() method if the */
/* 	 * truncation was invoked from an ftruncate() system call. */
/* 	 * */
/* 	 * If this method is not implemented or under Linux kernel */
/* 	 * versions earlier than 2.6.15, the truncate() method will be */
/* 	 * called instead. */
/* 	 * */
/* 	 * Introduced in version 2.5 */
/* 	 *\/ */
/* 	int (*ftruncate) (const char *, off_t, struct fuse_file_info *); */

/* 	/\** */
/* 	 * Get attributes from an open file */
/* 	 * */
/* 	 * This method is called instead of the getattr() method if the */
/* 	 * file information is available. */
/* 	 * */
/* 	 * Currently this is only called after the create() method if that */
/* 	 * is implemented (see above).  Later it may be called for */
/* 	 * invocations of fstat() too. */
/* 	 * */
/* 	 * Introduced in version 2.5 */
/* 	 *\/ */
/* 	int (*fgetattr) (const char *, struct stat *, struct fuse_file_info *); */

/* 	/\** */
/* 	 * Perform POSIX file locking operation */
/* 	 * */
/* 	 * The cmd argument will be either F_GETLK, F_SETLK or F_SETLKW. */
/* 	 * */
/* 	 * For the meaning of fields in 'struct flock' see the man page */
/* 	 * for fcntl(2).  The l_whence field will always be set to */
/* 	 * SEEK_SET. */
/* 	 * */
/* 	 * For checking lock ownership, the 'fuse_file_info->owner' */
/* 	 * argument must be used. */
/* 	 * */
/* 	 * For F_GETLK operation, the library will first check currently */
/* 	 * held locks, and if a conflicting lock is found it will return */
/* 	 * information without calling this method.	 This ensures, that */
/* 	 * for local locks the l_pid field is correctly filled in.	The */
/* 	 * results may not be accurate in case of race conditions and in */
/* 	 * the presence of hard links, but it's unlikely that an */
/* 	 * application would rely on accurate GETLK results in these */
/* 	 * cases.  If a conflicting lock is not found, this method will be */
/* 	 * called, and the filesystem may fill out l_pid by a meaningful */
/* 	 * value, or it may leave this field zero. */
/* 	 * */
/* 	 * For F_SETLK and F_SETLKW the l_pid field will be set to the pid */
/* 	 * of the process performing the locking operation. */
/* 	 * */
/* 	 * Note: if this method is not implemented, the kernel will still */
/* 	 * allow file locking to work locally.  Hence it is only */
/* 	 * interesting for network filesystems and similar. */
/* 	 * */
/* 	 * Introduced in version 2.6 */
/* 	 *\/ */
/* 	int (*lock) (const char *, struct fuse_file_info *, int cmd, */
/* 		     struct flock *); */

/* 	/\** */
/* 	 * Change the access and modification times of a file with */
/* 	 * nanosecond resolution */
/* 	 * */
/* 	 * This supersedes the old utime() interface.  New applications */
/* 	 * should use this. */
/* 	 * */
/* 	 * See the utimensat(2) man page for details. */
/* 	 * */
/* 	 * Introduced in version 2.6 */
/* 	 *\/ */
/* 	int (*utimens) (const char *, const struct timespec tv[2]); */

/* 	/\** */
/* 	 * Map block index within file to block index within device */
/* 	 * */
/* 	 * Note: This makes sense only for block device backed filesystems */
/* 	 * mounted with the 'blkdev' option */
/* 	 * */
/* 	 * Introduced in version 2.6 */
/* 	 *\/ */
/* 	int (*bmap) (const char *, size_t blocksize, uint64_t *idx); */

/* 	/\** */
/* 	 * Flag indicating that the filesystem can accept a NULL path */
/* 	 * as the first argument for the following operations: */
/* 	 * */
/* 	 * read, write, flush, release, fsync, readdir, releasedir, */
/* 	 * fsyncdir, ftruncate, fgetattr, lock, ioctl and poll */
/* 	 * */
/* 	 * If this flag is set these operations continue to work on */
/* 	 * unlinked files even if "-ohard_remove" option was specified. */
/* 	 *\/ */
/* 	unsigned int flag_nullpath_ok:1; */

/* 	/\** */
/* 	 * Flag indicating that the path need not be calculated for */
/* 	 * the following operations: */
/* 	 * */
/* 	 * read, write, flush, release, fsync, readdir, releasedir, */
/* 	 * fsyncdir, ftruncate, fgetattr, lock, ioctl and poll */
/* 	 * */
/* 	 * Closely related to flag_nullpath_ok, but if this flag is */
/* 	 * set then the path will not be calculaged even if the file */
/* 	 * wasn't unlinked.  However the path can still be non-NULL if */
/* 	 * it needs to be calculated for some other reason. */
/* 	 *\/ */
/* 	unsigned int flag_nopath:1; */

/* 	/\** */
/* 	 * Flag indicating that the filesystem accepts special */
/* 	 * UTIME_NOW and UTIME_OMIT values in its utimens operation. */
/* 	 *\/ */
/* 	unsigned int flag_utime_omit_ok:1; */

/* 	/\** */
/* 	 * Reserved flags, don't set */
/* 	 *\/ */
/* 	unsigned int flag_reserved:29; */

/* 	/\** */
/* 	 * Ioctl */
/* 	 * */
/* 	 * flags will have FUSE_IOCTL_COMPAT set for 32bit ioctls in */
/* 	 * 64bit environment.  The size and direction of data is */
/* 	 * determined by _IOC_*() decoding of cmd.  For _IOC_NONE, */
/* 	 * data will be NULL, for _IOC_WRITE data is out area, for */
/* 	 * _IOC_READ in area and if both are set in/out area.  In all */
/* 	 * non-NULL cases, the area is of _IOC_SIZE(cmd) bytes. */
/* 	 * */
/* 	 * If flags has FUSE_IOCTL_DIR then the fuse_file_info refers to a */
/* 	 * directory file handle. */
/* 	 * */
/* 	 * Introduced in version 2.8 */
/* 	 *\/ */
/* 	int (*ioctl) (const char *, int cmd, void *arg, */
/* 		      struct fuse_file_info *, unsigned int flags, void *data); */

/* 	/\** */
/* 	 * Poll for IO readiness events */
/* 	 * */
/* 	 * Note: If ph is non-NULL, the client should notify */
/* 	 * when IO readiness events occur by calling */
/* 	 * fuse_notify_poll() with the specified ph. */
/* 	 * */
/* 	 * Regardless of the number of times poll with a non-NULL ph */
/* 	 * is received, single notification is enough to clear all. */
/* 	 * Notifying more times incurs overhead but doesn't harm */
/* 	 * correctness. */
/* 	 * */
/* 	 * The callee is responsible for destroying ph with */
/* 	 * fuse_pollhandle_destroy() when no longer in use. */
/* 	 * */
/* 	 * Introduced in version 2.8 */
/* 	 *\/ */
/* 	int (*poll) (const char *, struct fuse_file_info *, */
/* 		     struct fuse_pollhandle *ph, unsigned *reventsp); */

/* 	/\** Write contents of buffer to an open file */
/* 	 * */
/* 	 * Similar to the write() method, but data is supplied in a */
/* 	 * generic buffer.  Use fuse_buf_copy() to transfer data to */
/* 	 * the destination. */
/* 	 * */
/* 	 * Introduced in version 2.9 */
/* 	 *\/ */
/* 	int (*write_buf) (const char *, struct fuse_bufvec *buf, off_t off, */
/* 			  struct fuse_file_info *); */

/* 	/\** Store data from an open file in a buffer */
/* 	 * */
/* 	 * Similar to the read() method, but data is stored and */
/* 	 * returned in a generic buffer. */
/* 	 * */
/* 	 * No actual copying of data has to take place, the source */
/* 	 * file descriptor may simply be stored in the buffer for */
/* 	 * later data transfer. */
/* 	 * */
/* 	 * The buffer must be allocated dynamically and stored at the */
/* 	 * location pointed to by bufp.  If the buffer contains memory */
/* 	 * regions, they too must be allocated using malloc().  The */
/* 	 * allocated memory will be freed by the caller. */
/* 	 * */
/* 	 * Introduced in version 2.9 */
/* 	 *\/ */
/* 	int (*read_buf) (const char *, struct fuse_bufvec **bufp, */
/* 			 size_t size, off_t off, struct fuse_file_info *); */
/* 	/\** */
/* 	 * Perform BSD file locking operation */
/* 	 * */
/* 	 * The op argument will be either LOCK_SH, LOCK_EX or LOCK_UN */
/* 	 * */
/* 	 * Nonblocking requests will be indicated by ORing LOCK_NB to */
/* 	 * the above operations */
/* 	 * */
/* 	 * For more information see the flock(2) manual page. */
/* 	 * */
/* 	 * Additionally fi->owner will be set to a value unique to */
/* 	 * this open file.  This same value will be supplied to */
/* 	 * ->release() when the file is released. */
/* 	 * */
/* 	 * Note: if this method is not implemented, the kernel will still */
/* 	 * allow file locking to work locally.  Hence it is only */
/* 	 * interesting for network filesystems and similar. */
/* 	 * */
/* 	 * Introduced in version 2.9 */
/* 	 *\/ */
/* 	int (*flock) (const char *, struct fuse_file_info *, int op); */

/* 	/\** */
/* 	 * Allocates space for an open file */
/* 	 * */
/* 	 * This function ensures that required space is allocated for specified */
/* 	 * file.  If this function returns success then any subsequent write */
/* 	 * request to specified range is guaranteed not to fail because of lack */
/* 	 * of space on the file system media. */
/* 	 * */
/* 	 * Introduced in version 2.9.1 */
/* 	 *\/ */
/* 	int (*fallocate) (const char *, int, off_t, off_t, */
/* 			  struct fuse_file_info *); */
/* }; */


