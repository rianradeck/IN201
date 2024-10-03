#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct fs_header {
    uint8_t identifier[8];
    uint8_t fullsize[4], checksum[4];
    char volume_name[];
};

struct file_header {
    // uint32_t next_offset, spec_info, size, checksum;
    uint8_t next_offset[4];
    uint8_t spec_info[4];
    uint8_t size[4];
    uint8_t checksum[4];
    char filename[];
};

uint32_t read32(uint8_t ptr[4]) {
    uint32_t val = 0;

    // printf("%x %x %x %x\n", ptr[0], ptr[1], ptr[2], ptr[3]);

    val |= ptr[0] << 24;
    val |= ptr[1] << 16;
    val |= ptr[2] << 8;
    val |= ptr[3] << 0;

    return val;
}

// aligns the address to the next 16-bytes boundary
long ceil_16(long x) {
    if (x % 16) x += 16 - (x % 16);
    return x;
}

struct file_header* next(struct fs_header *fs, struct file_header *dir){
    return fs + (read32(dir->next_offset) & !(15)) / sizeof(struct fs_header);
}

// lists the files in a directory on stdout.
void ls(struct fs_header *fs, struct file_header *dir) {
    // assert(ft(dir) == ft_directory);
    printf("Listing content of directory %s\n", dir->filename);
    printf("TYPE          SIZE      NAME\n");
    struct file_header *file = dir;
    while (file) {
        
        pp_file(file);
        file = next(fs, file);
    }
}

// pretty prints a file to stdout (name, type, size, but not content).
void pp_file(struct file_header *file) {
    // printf("next_fh: %u\n", read32(file->next_offset));
    // printf("spec.info: %u\n", read32(file->spec_info));
    // printf("size: %u\n", read32(file->size));
    // printf("checksum: %u\n", read32(file->checksum));
    printf("filename: %s\n", file->filename);
}

void decode(struct fs_header *p, size_t size) {
    for (int i = 0; i < 8; i++) printf("%c", p->identifier[i]);
    puts("");

    for (int i = 0; i < 8; i++) printf("%x ", p->identifier[i]);
    puts("");

    printf("%x\n", read32(p->identifier));

    printf("fullsize is %u\n", read32(p->fullsize));
    assert(read32(p->fullsize) < size);

    // ceil_16(16);
    // ceil_16(63);

    printf("cheksum: %u\n", read32(p->checksum));

    uint32_t fh_offset = 16 + ceil_16(strlen(p->volume_name));
    printf("%lu -> %s\n", strlen(p->volume_name), p->volume_name);
    printf("Memory offset: %u\n", fh_offset);

    // Get the root file offset
    struct file_header *root =
        (struct file_header *)p + fh_offset / sizeof(struct fs_header);

    printf("Root directory: %s (%d bytes)\n", root->filename,
           read32(root->size));

    // Question 8
    puts("Root directory:\n");
    pp_file(root);
    printf("------\n");
    ls(p, root);
    printf("------\n");
}

int main(void) {
    int fd = open("fs.romfs", O_RDONLY);
    assert(fd != -1);
    off_t fsize;
    fsize = lseek(fd, 0, SEEK_END);
    printf("size is %ld\n", fsize);
    struct fs_header *addr = mmap(addr, fsize, PROT_READ, MAP_SHARED, fd, 0);
    assert(addr != MAP_FAILED);
    decode(addr, fsize);
    return 0;
}