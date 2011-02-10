#include <stdio.h>
#include <sys/types.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/errno.h>


struct entry {
	long bps;
	long timemark;
};

void
translate_entry(struct entry *ent, char *buf)
{
	char *tm;

	ent->bps = strtol(buf, &tm, 0);
	tm++;
	ent->timemark = strtol(tm, NULL, 0);
}

void
sync_needed(struct entry *le, FILE **leftp,
	    struct entry *re, FILE **rightp,
	    FILE *out)
{
	char localbuf[1024];
	FILE *right, *left;
	right = *rightp;
	left = *leftp;

restart_closed:
	if (left == NULL) {
		/* Left is closed ... read out all the right */
		fprintf(out, "%ld:%ld\n",
			re->bps, re->timemark);
		while(fgets(localbuf, sizeof(localbuf), right) != NULL) {
			fprintf(out, "%s", localbuf);
		}
		fclose(right);
		*rightp = NULL;
		return;
	} else if (right == NULL) {
		/* write is closed ... read out all the left */
		fprintf(out, "%ld:%ld\n",
			le->bps, le->timemark);
		while(fgets(localbuf, sizeof(localbuf), left) != NULL) {
			fprintf(out, "%s", localbuf);
		}
		fclose(left);
		*leftp = NULL;
		return;
	}
again:
	/* Ok we have a valid right and left entry */
	if(le->timemark > re->timemark) {
		/* write out the right entry and read another */
		fprintf(out, "%ld:%ld\n",
			re->bps, re->timemark);
		/* read next right one */
		if (fgets(localbuf, sizeof(localbuf), right) == NULL) {
			fclose(right);
			right = NULL;
			*rightp = NULL;
			goto restart_closed;
		} else {
			translate_entry(re, localbuf);
		}
		goto again;

	} else if (re->timemark > le->timemark) {
		/* write out the left entry and read another */
		fprintf(out, "%ld:%ld\n",
			le->bps, le->timemark);
		if (fgets(localbuf, sizeof(localbuf), left) == NULL) {
			fclose(left);
			left = NULL;
			*leftp = NULL;
			goto restart_closed;
		} else {
			translate_entry(le, localbuf);
		}
		goto again;
	} else {
		/* We are equal - let the main handle it */
		return;
	}
}


int
main(int argc, char **argv)
{

	char buffer_r[1024];
	char buffer_l[1024];
	int i, notdone = 1;
	FILE *left_io, *right_io, *out;
	char *left=NULL, *right=NULL, *outfile = NULL;
	struct entry left_entry;
	struct entry right_entry;

	while ((i = getopt(argc, argv, "l:r:w:?")) != EOF) {
		switch (i) {
		case 'w':
			outfile = optarg;
			break;
		case 'l':
			left = optarg;
			break;
		case 'r':
			right = optarg;
			break;
		case '?':
		default:
		use:
			printf("Use %s -l leftfile -r rightfile -w outfile\n",
			       argv[0]);
		exit(-1);
		break;
		};
	};
	if ((left == NULL) || (right == NULL) || (outfile == NULL)) {
		goto use;
	}
	left_io = fopen(left, "r");
	if (left_io == NULL) {
		printf("Can't open '%s' - err:%d\n", left, errno);
		return (-1);
	}
	right_io = fopen(right, "r");
	if (right_io == NULL) {
		printf("Can't open '%s' - err:%d\n", right, errno);
		return (-1);
	}
	out = fopen(outfile, "w+");
	if (out == NULL) {
		printf("Can't open '%s' for write - err:%d\n", 
		       outfile, errno);
		return (-1);
	}
	/* Initial read */
	if (fgets(buffer_r, sizeof(buffer_r), 
		  right_io) == NULL) {
		fclose(right_io);
		right_io = NULL;
	} else {
		translate_entry(&right_entry, buffer_r);
	}
	if (fgets(buffer_l, sizeof(buffer_l), 
		  left_io) == NULL) {
		fclose(left_io);
		left_io = NULL;
	} else {
		translate_entry(&left_entry, buffer_l);
	}
	/* Now in the loop */
	while (notdone) {
		if ((left_io == NULL) && (right_io == NULL)) {
			notdone = 0;
			continue;
		}
		if (left_entry.timemark != right_entry.timemark) {
			sync_needed(&left_entry, &left_io,
				    &right_entry, &right_io,
				    out);
			continue;
		}
		/* If we reach here we are in sync */
		fprintf(out, "%ld:%ld\n",
			(left_entry.bps+right_entry.bps),
			left_entry.timemark);

		/* Read the next entry */
		if (right_io) {
			if (fgets(buffer_r, sizeof(buffer_r), 
				  right_io) == NULL) {
				fclose(right_io);
				right_io = NULL;
			} else {
				translate_entry(&right_entry, buffer_r);
			}
		} else {
			right_entry.timemark = 0;
		}
		if (left_io) {
			if (fgets(buffer_l, sizeof(buffer_l), 
				  left_io) == NULL) {
				fclose(left_io);
				left_io = NULL;
			} else {
				translate_entry(&left_entry, buffer_l);
			}
		} else {
			left_entry.timemark = 0;
		}
	}
	if (right_io)
		fclose(right_io);	
	if (left_io)
		fclose(left_io);

	fclose(out);
	return (0);
}
