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
    char magic[8];
    uint8_t size[4];
    // Can also be 'uint32_t size', but that requires adding "&" to calls to
    // read32 (pass address and not value)
    uint32_t checksum;
    char name[];
} __attribute__((packed));

struct file_header {
    uint8_t next[4];
    uint8_t spec_info[4];
    uint8_t size[4];
    uint32_t checksum;
    char name[];
} __attribute__((packed));

// converts an integer from fs endianness (big endian) to native endianness
// (little endian)
uint32_t read32(uint8_t cast[4]) {
    return (cast[0] << 24) | (cast[1] << 16) | (cast[2] << 8) | cast[3];
}

// aligns the address to the next 16-bytes boundary
long ceil_16(long x) {
    if (x & 15L) {
        return (x | 15L) + 1L;
    } else {
        return x;
    }
}

// returns the file header at the specified offset. Cleans the permission bits
// stored in the 3 least significant bits. Returns NULL if this is the last
// file.
struct file_header *file_from_offset(struct fs_header *fs, uint8_t offset[4]) {
    uint32_t real_offset = read32(offset) & ~15;
    // Checks that the offset is sensible, instead of blindly segfaulting
    assert(real_offset < read32(fs->size));
    if (real_offset) {
        return (struct file_header *)((char *)fs + real_offset);
    } else {
        return NULL;
    };
}

// returns a pointer to the start of the data of the file
char *file_data(struct file_header *file) {
    return (char *)ceil_16((long)&file->name[strlen(file->name) + 1]);
}

// returns the file after the argument, or NULL if this is the last one.
struct file_header *next(struct fs_header *fs, struct file_header *file) {
    return file_from_offset(fs, file->next);
}

// an enum for possible file types
enum file_type {
    ft_hard_link = 0,
    ft_directory,
    ft_regular,
    ft_symlink,
    ft_blockdev,
    ft_chardev,
    ft_socket,
    ft_fifo,
};

// return the file type of the corrsponding header
enum file_type ft(struct file_header *file) {
    // low 3 bits of the last byte in big endian, so low three bits of the first
    // byte in native endianness
    return file->next[3] & 0b0111;
}

char *type(struct file_header *file) {
    switch (ft(file)) {
        case ft_hard_link:
            return "hard link   ";
        case ft_directory:
            return "directory   ";
        case ft_regular:
            return "regular     ";
        case ft_symlink:
            return "symlink     ";
        case ft_blockdev:
            return "block device";
        case ft_chardev:
            return "char device ";
        case ft_socket:
            return "socket      ";
        case ft_fifo:
            return "fifo        ";
        default:
            return "impossible  ";
    }
}

// pretty prints a file to stdout (name, type, size, but not content).
void pp_file(struct file_header *file) {
    char *typ = type(file);
    printf("%s  %6d B  %s\n", typ, read32(file->size), file->name);
}

// lists the files in a directory on stdout.
void ls(struct fs_header *fs, struct file_header *dir) {
    assert(ft(dir) == ft_directory);
    printf("Listing content of directory %s\n", dir->name);
    printf("TYPE          SIZE      NAME\n");
    struct file_header *file = file_from_offset(fs, dir->spec_info); // This seems to be wrong
    while (file) {
        pp_file(file);
        file = next(fs, file);
    }
}

// find the first file with the specified name. Does not follow links. Returns
// NULL if no file matches.
struct file_header *find(struct fs_header *fs, struct file_header *root,
                         char *name) {
    assert(ft(root) == ft_directory);
    struct file_header *file = file_from_offset(fs, root->spec_info);
    while (file) {
        // pas besoin de vÃ©rifier .. parce que .. est un hard_link
        if (ft(file) == ft_directory && strcmp(file->name, ".")) {
            struct file_header *ret = find(fs, file, name);
            if (ret) return ret;
        }
        if (ft(file) == ft_regular && !strcmp(file->name, name)) {
            return file;
        }
        file = next(fs, file);
    }
    return NULL;
}

void decode(struct fs_header *fs, size_t size) {
    // check that this file is really a romfs
    assert(memcmp(fs->magic, "-rom1fs-", sizeof(fs->magic)) == 0);
    // check that the claimed size makes sense.
    uint32_t inner_size = read32(fs->size);
    assert(inner_size <= size);

    // Get first file header
    uint32_t offset = ceil_16(strlen(fs->name) + 1);
    printf("AAAAAA %u", offset);
    struct file_header *root = (struct file_header *)(fs->name + offset);

    printf("Root directory: %s (%d bytes)\n", root->name, read32(root->size));
    printf("------------------------------------\n");
    ls(fs, root);
    printf("------------------------------------\n");

    printf("Looking for message.txt\n");
    struct file_header *message = find(fs, root, "message.txt");
    assert(message);
    printf("Found it!\n");
    pp_file(message);
    int file_size = read32(message->size);
    char *data = file_data(message);
    fwrite(data, 1, file_size, stdout);
}

int main(void) {
    int fd = open("fs.romfs", O_RDONLY);
    assert(fd != -1);
    off_t fsize;
    fsize = lseek(fd, 0, SEEK_END);

    //  printf("size is %d", fsize);
    void *addr = mmap(NULL, fsize, PROT_READ, MAP_SHARED, fd, 0);
    assert(addr != MAP_FAILED);
    decode(addr, fsize);
    return 0;
}