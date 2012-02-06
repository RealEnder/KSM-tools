/* madv_ksm.c
 * by Alex Stanev <alex@stanev.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <sys/syscall.h>

size_t kernel_pagesize = 4096;

//hook functions on load, high priority constructor
void __attribute__ ((constructor (101))) hook_init();

extern void *__libc_malloc (size_t);
extern void *__libc_calloc (size_t, size_t);

//get page beginning and new length aligned to pagesize
static void merge_pages(void *address, size_t length) {
    const size_t raw_address = (size_t) address;
    const size_t page_address = raw_address & -kernel_pagesize;
    const size_t new_length = (((length + raw_address - page_address) / kernel_pagesize) + 1) * kernel_pagesize;
    //printf("raw_address:%d length:%d page_address:%d new_length:%d\n", (int) raw_address, (int) length, (int) page_address, (int) new_length);

    madvise((void *) page_address, new_length, MADV_MERGEABLE);
}

//hooked functions
void *(*real_mmap)    (void *, size_t, int, int, int, off_t) = NULL;
void *(*real_mmap2)   (void *, size_t, int, int, int, off_t) = NULL;
void *(*real_mremap)  (void *, size_t, size_t, int, ...) = NULL;
void *(*real_realloc) (void *, size_t) = NULL;
void *(*real_calloc)  (size_t, size_t) = __libc_calloc;
void *(*real_malloc)  (size_t) = __libc_malloc;

//init 
void hook_init() {
    kernel_pagesize = (size_t) sysconf(_SC_PAGESIZE);

    real_mmap = dlsym(RTLD_NEXT, "mmap");
    real_mmap2 = dlsym(RTLD_NEXT, "mmap2");
    real_mremap = dlsym(RTLD_NEXT, "mremap");
    real_realloc = dlsym(RTLD_NEXT, "realloc");
    real_calloc = dlsym(RTLD_NEXT, "calloc");
    real_malloc = dlsym(RTLD_NEXT, "malloc");    
}

//add madvise with MADV_MERGEABLE
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    void *res = real_mmap(addr, length, prot, flags, fd, offset);
    merge_pages(res, length);

    return res;
}

void *mmap2(void *start, size_t length, int prot, int flags, int fd, off_t pgoffset) {
    void *res = real_mmap2(start, length, prot, flags, fd, pgoffset);
    merge_pages(res, length);

    return res;
}

void *mremap(void *old_address, size_t old_length, size_t new_length, int flags, ...) {
    void *res;
    va_list ap;

    va_start(ap, flags);
    void *new_address = (flags & MREMAP_FIXED) ? va_arg (ap, void *) : NULL;
    va_end(ap);

    res = real_mremap(old_address, old_length, new_length, flags, new_address);
    merge_pages(res, new_length);

    return res;
}

void *realloc(void *addr, size_t length) {
    void *res = real_realloc(addr, length);
    merge_pages(res, length);

    return res;
}

void *malloc(size_t length) {
    void *res = real_malloc(length);
    merge_pages(res, length);

    return res;
}

void *calloc(size_t nmemb, size_t length) {    
    void *res = real_calloc(nmemb, length);
    merge_pages(res, length*nmemb);

    return res;
}

