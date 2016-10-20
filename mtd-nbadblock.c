#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/limits.h>
#include <mtd/mtd-user.h>

char *exe_name;
char *mtd_device;

const char *mtd_type_label[] = {
	"MTD_ABSENT",
	"MTD_RAM",
	"MTD_ROM",
	"MTD_NORFLASH",
	"MTD_NANDFLASH",
	"Unknown",
	"MTD_DATAFLASH",
	"MTD_UBIVOLUME",
	"MTD_MLCNANDFLASH",
};

int main(int argc, char *argv[])
{
    int md_fd;
    int nBlocks = 0, nBadBlocks = 0;
	struct mtd_info_user mtd_meminfo;

	exe_name = argv[0];
	if (argc >= 2) {
		mtd_device = argv[1];
	} else {
		mtd_device = "/dev/mtd0";
	}
	
    if ((md_fd = open(mtd_device, O_RDONLY)) < 0) {
        printf("\n\n%s: %s: %s\n", exe_name, mtd_device, strerror(errno));
        exit(1);
    }

    if (ioctl(md_fd, MEMGETINFO, &mtd_meminfo) != 0) {
        printf("\n\n%s: %s: unable to get MTD device info\n",
			   exe_name, mtd_device);
        exit(1);
    }
	printf("MEMGETINFO for %s:\n type: %s\n flags: %d\n size: %x\n erasesize: %x\n writesize: %d\n oobsize: %d\n",
		   mtd_device,
		   mtd_type_label[mtd_meminfo.type],
		   mtd_meminfo.flags,
		   mtd_meminfo.size,
		   mtd_meminfo.erasesize,
		   mtd_meminfo.writesize,
		   mtd_meminfo.oobsize
		   );
    nBadBlocks = get_bb_number( md_fd, &mtd_meminfo);
	nBlocks = mtd_meminfo.size/mtd_meminfo.erasesize;
	printf("Number of the bad blocks is %d (%.1f%%)\n", nBadBlocks, 100.0*nBadBlocks/nBlocks);
    return 0;
}


 int get_bb_number(int fd, const struct mtd_info_user *meminfo)
{
    int isNAND = meminfo->type == MTD_NANDFLASH ? 1 : 0;
    int ibbCounter = 0;
    erase_info_t erase;
    erase.length = meminfo->erasesize;

    for (erase.start = 0;
         erase.start < meminfo->size;
         erase.start += meminfo->erasesize)
    {

        loff_t offset = erase.start;
        int ret = ioctl(fd, MEMGETBADBLOCK, &offset);
        if (ret > 0)
        {
            ibbCounter++;
            continue;
        }
        else if (ret < 0)
        {
            if (errno == EOPNOTSUPP)
            {
                if (isNAND)
                {
                    printf("\n\n%s: %s: Bad block check not available\n",
						   exe_name, mtd_device);
                    exit(1);
                }
            }
            else
            {
                printf("\n%s: %s: MTD get bad block failed: %s\n",
					   exe_name, mtd_device, strerror(errno));
                exit(1);
            }
        }
    }
    return ibbCounter;
}
