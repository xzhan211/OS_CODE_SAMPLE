#include<errno.h>

/* Write "n" bytes to a descriptor. */
ssize_t writen(int fd, const void *vptr, size_t n){
	size_t nleft;
	size_t nwritten;
	//指针所指的对象是readonly的，但指针自身是可以变化的
	const char *ptr;

	ptr = vptr;
	nleft = n;
	while(nleft > 0){
		if((nwritten = write(fd, ptr, nleft)) <= 0){
			//“Interrupted system call.” An asynchronous signal occurred and prevented completion of the call. 
			//When this happens, you should try the call again.
			//error code macros are defined in errno.h
			if(errno == EINTR)
				nwritten = 0; // call write() again
			else
				return(-1);
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return(n);
}