#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define MBR_SIZE 512
#define PARTITION_ENTRY_SIZE 16
#define PARTITION_TABLE_OFFSET 446
#define NUM_PARTITIONS 2
#define DEFAULT_NUM_SECTORS 16777216

struct PartitionEntry {
    uint8_t boot_flag;       
    uint8_t chs_start[3];    
    uint8_t partition_type;  
    uint8_t chs_end[3];      
    uint32_t lba_start;      
    uint32_t num_sectors;    
};

void resize(uint32_t sectors) {
    uint64_t size_in_bytes = sectors * 512;
    if (size_in_bytes >= (1ULL << 30)) {
        printf("%.1fG", size_in_bytes / (float)(1ULL << 30));
    } else if (size_in_bytes >= (1ULL << 20)) {
        printf("%.1fM", size_in_bytes / (float)(1ULL << 20));
    } else if (size_in_bytes >= (1ULL << 10)) {
        printf("%.1fK", size_in_bytes / (float)(1ULL << 10));  
    } else {
        printf("%lluB", size_in_bytes);
    }
}

void print_partition(struct PartitionEntry* entry, int index) {
    char boot_flag = (entry->boot_flag == 0x80) ? '*' : ' ';
    printf("/dev/sda%d  %c  %10u  %10u  %10u  ", 
           index + 1, 
           boot_flag, 
           entry->lba_start, 
           entry->lba_start + entry->num_sectors - 1, 
           entry->num_sectors);
    resize(entry->num_sectors);
    printf("  0x%02X  ", entry->partition_type);

    switch (entry->partition_type) {
        case 0x07: printf("HPFS/NTFS/exFAT\n"); break;
        case 0x83: printf("Linux\n"); break;
        case 0x0C: printf("W95 FAT32 (LBA)\n"); break;
        default:   printf("Desconhecido\n"); break;
    }
}

void print_disk(uint64_t disk_size_bytes, uint32_t num_sectors, uint32_t disk_identifier) {
    printf("Disk /dev/sda: ");
    resize(num_sectors);
    printf(", %llu bytes, %u sectors\n", disk_size_bytes, num_sectors);
    printf("Units: sectors of 1 * 512 = 512 bytes\n");
    printf("Sector size (logical/physical): 512 bytes / 512 bytes\n");
    printf("I/O size (minimum/optimal): 512 bytes / 512 bytes\n");
    printf("Disklabel type: dos\n");
    printf("Disk identifier: 0x%08X\n\n", disk_identifier);
}

int main(int argc, char* argv[]) {

    uint32_t num_sectors = DEFAULT_NUM_SECTORS;

    if (argc != 2) {
        printf("Uso: %s <arquivo_MBR>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    uint8_t mbr[MBR_SIZE];

    if (fread(mbr, 1, MBR_SIZE, file) != MBR_SIZE) {
        perror("Erro ao ler a MBR");
        fclose(file);
        return 1;
    }

    fclose(file);

    uint64_t disk_size_bytes = (uint64_t)num_sectors * 512;

    uint32_t disk_identifier = *((uint32_t*)(&mbr[440]));

    print_disk(disk_size_bytes, num_sectors, disk_identifier);

    struct PartitionEntry* partition_table = (struct PartitionEntry*)&mbr[PARTITION_TABLE_OFFSET];

    printf("Device     Boot  Start       End   Sectors   Size Id Type\n");

    for (int i = 0; i < NUM_PARTITIONS; i++) {
        print_partition(&partition_table[i], i);
    }

    return 0;
}
