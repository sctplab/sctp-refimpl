#include <sys/types.h>
#include <stdio.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>

int
display_sb(char *block)
{
  struct fs *fsp;
  int valid = 0;
  int byte_order = 0;
  fsp = (struct fs *)block;

  if (fsp->fs_magic == FS_UFS1_MAGIC) {
	valid = 1;
	printf("Is host byte order, UFS1 style sb\n");
  } else if (fsp->fs_magic == FS_UFS2_MAGIC) {
	valid = 1;
	printf("Is host byte order, UFS2 style sb\n");
  } else if (ntohl(fsp->fs_magic) == FS_UFS1_MAGIC) {
	valid = 1;
	byte_order = 1;
	printf("UFS1 style sb - REVERSE BYTE ORDER\n");
  } else if (ntohl(fsp->fs_magic) == FS_UFS2_MAGIC) {
	valid = 1;
	byte_order = 1;
	printf("UFS2 style sb - REVERSE BYTE ORDER\n");
  }
  if (valid) {
	printf("fs_sblkno is %d\n",
		   (byte_order == 0) ? fsp->fs_sblkno : ntohl(fsp->fs_sblkno));
  }

}

int
main(int argc, char **argv)
{
  char superblock[SBLOCKSIZE];
  FILE *io=NULL;


  if(argc > 1) {
	io = fopen(argv[1], "r");
  }
  if (io == NULL) {
	printf("Use %s file\n", argv[0]);
	return(-1);
  }
  fseek(io, SBLOCK_UFS1, SEEK_SET);
  if(fread(superblock, SBLOCKSIZE, 1, io) != 1) {
	printf("Can't read UFS1 location\n");
	return(-1);
  }
  if(display_sb(superblock)) {
	printf("Was a UFS1 style file system\n");
	return(0);
  }
  
  fseek(io, SBLOCK_UFS2, SEEK_SET);
  if(fread(superblock, SBLOCKSIZE, 1, io) != 1) {
	printf("Can't read UFS2 location\n");
	return(-1);
  }
  if(display_sb(superblock)) {
	printf("Was a UFS2 style file system\n");
	return(0);
  }

  fseek(io, SBLOCK_PIGGY, SEEK_SET);
  if(fread(superblock, SBLOCKSIZE, 1, io) != 1) {
	printf("Can't read PIGGY location\n");
	return(-1);
  }
  if(display_sb(superblock)) {
	printf("Was a PIGGY style file system\n");
	return(0);
  }
  
  fseek(io, SBLOCK_FLOPPY, SEEK_SET);
  if(fread(superblock, SBLOCKSIZE, 1, io) != 1) {
	printf("Can't read FLOPPY location\n");
	return(-1);
  }
  if(display_sb(superblock)) {
	printf("Was a FLOPPY style file system\n");
	return(0);
  }
  printf("No file system found, sorry\n");
  return(0);
}
