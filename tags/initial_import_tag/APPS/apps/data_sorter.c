#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/*

A cache line looks as follows. Next points to
the next cache entry for a error rate. Entry points
to the entry at that error rate. The first
entry in the list is the "sum" of all the
others the cnt will be set to the number of
entry's at this cnt. List points to the
linked list of actual entrys (used if you
are doing a mean). Only for the summed entry
is the next pointer used. This points to the
next size in this error rate. For live satellite
measurments no other error rate will be present
but the first one. The code will automatically
switch to live satellite graphing in that case the
user must just make sure the error rate is all the same.

The graphs generated will either be a SET of graphs
mapping transfer time on the y axis and error
rate on the x axis. Each transfer size will have
its own graph. For live satellite only ONE graph
is generated. The y axis will be the time of
transfer and the x axis will be the size of the
transfers.

 ----Cache------>Error Rate
                 next----------->
                 Entry
                 |
                 |
                 |
                 v
          ---------------
          Summed line
          high/low
          size
          Error Rate
          cnt
          sctp (0=tcp, 1=sctp)
          next------------> Next size element
          list
          |
          |
          |
          v
     Individual entries
     that made up summed
     list.

*/


extern int chmod(char*,int);

struct data_entry{
	int size;
	struct timeval time;
	int rcv_buf;
	int count;
	struct timeval highest;
	struct timeval lowest;
	int sctp;
	char error_rate[16];
	struct data_entry *next;
	int cnt;
	struct data_entry *list;
};

struct cache_entry{
    struct cache_entry *next;
    char error_rate[16];
    struct data_entry *recs;
};

struct cache_entry *cache;

static void
add_time(struct data_entry *org,
         struct data_entry *e)
{
    org->time.tv_sec += e->time.tv_sec;
    org->time.tv_usec += e->time.tv_usec;
    if(org->time.tv_usec > 1000000){
        org->time.tv_sec++;
        org->time.tv_usec -= 1000000;
    }
    org->count += e->count;
    /* New highest? */
    if(e->highest.tv_sec > org->highest.tv_sec){
        org->highest = e->highest;
    }else if(e->highest.tv_sec == org->highest.tv_sec){
        if(e->highest.tv_usec > org->highest.tv_usec){
            org->highest = e->highest;
        }
    }
    /* New lowest? */
    if(org->lowest.tv_sec > e->lowest.tv_sec){
        org->lowest = e->lowest;
    }else if(org->lowest.tv_sec == e->lowest.tv_sec){
        if(org->lowest.tv_usec > e->lowest.tv_usec){
            org->lowest = e->lowest;
        }
    }

}

void
add_to_list(struct data_entry *at,
	    struct data_entry *e)
{
	struct data_entry *newone;
	struct data_entry *into,*prev;
	/* allocate one */
	newone = (struct data_entry *)malloc(sizeof(struct data_entry));
	*newone = *e;
	newone->list = NULL;
	/* now add it into the proper place */
	/* for now we just tack it on */
	if(at->list == NULL){
		at->cnt = 1;
		newone->list = NULL;
		at->list = newone;
		return;
	}
	at->cnt++;
	into = at->list;
	prev = NULL;
	while(into){
		if(into->time.tv_sec > newone->time.tv_sec){
			/* insert ahead new is smaller */
		insert:
			newone->list = into;
			if(prev){
				prev->list = newone;
			} else {
				at->list = newone;
			}
			return;
		}
		if(into->time.tv_sec == newone->time.tv_sec) {
			if(into->time.tv_usec > newone->time.tv_sec){
				goto insert;
			}
		}
		prev = into;
		into = prev->list;
	}
	/* onto the end */
	prev->list = newone;
	newone->list = NULL;
	return;
}


static void
sum_to_cache_line(struct cache_entry *c,
                  struct data_entry *e)
{
	struct data_entry *at,*last,*newone;
	/* simple case */
	if(c->recs == NULL){

		c->recs = (struct data_entry *)malloc(sizeof(struct data_entry));
		*c->recs = *e;
		c->recs->next= NULL;
		c->recs->list = NULL;
		add_to_list(c->recs,e);
		return;
	}
	/* sort through the cache to see if we can find this entry */
	at = c->recs;
	while(at){
		if((at->size == e->size) && (at->sctp == e->sctp)){
			/* found it */
			add_time(at,e);
			/* for mean we add on list of entry's */
			add_to_list(at,e);
			return;
		}
		at = at->next;
	}
	newone = (struct data_entry *)malloc(sizeof(struct data_entry));
	*newone = *e;
	newone->next = NULL;
	newone->list = NULL;
	add_to_list(newone,e);

	if(c->recs == NULL){
		c->recs = newone;
		return;
	}
	last = NULL;
	for(at=c->recs;at;at=at->next){
		if(newone->size < at->size){
			newone->next = at;
			if(last){
				last->next = newone;
			}else{
				c->recs = newone;
			}
			return;
		}
		last = at;
	}
	last->next = newone;
}

static void
add_entry_to_cache(struct cache_entry **head,
                   struct data_entry *e)
{
    int err_dec,err_dot,ent_dec,ent_dot;
    char buf[100],*s,*s2;
    char *dot=".";
    struct cache_entry *newone,*thisone,*prev;
    for(thisone=*head;thisone!=NULL;thisone = thisone->next){
        if(strcmp(thisone->error_rate,e->error_rate) == 0){
            sum_to_cache_line(thisone,e);
            return;
        }
    }
    /* New entry */
    newone = (struct cache_entry *)malloc(sizeof(struct cache_entry));
    strcpy(newone->error_rate,e->error_rate);
    newone->next = NULL;
    newone->recs = NULL;
    sum_to_cache_line(newone,e);
    if(*head == NULL){
        /* special case */
        *head = newone;
        return;
    }
    strcpy(buf,newone->error_rate);
    s = strtok(buf,dot);
    if(s == NULL){
        printf("warning no proper error rate %s inserting anywhere\n",
               newone->error_rate);
        
        goto place_at_end;
    }
    err_dec = strtol(s,NULL,0);
    s2 = (char *)((caddr_t)s + strlen(s) + 1);
    err_dot = strtol(s2,NULL,0);
    /* now we must place the new one */
    prev = NULL;
    for(thisone=*head;thisone!=NULL;){
        strcpy(buf,thisone->error_rate);
        s = strtok(buf,dot);
        if(s == NULL){
            printf("warning no proper error rate inserting newone ahead of it\n");
            /* insert */
            newone->next = thisone;
            if(prev){
                prev = newone;
            }else{
                *head = newone;
            }
            return;
        }
        ent_dec = strtol(s,NULL,0);
        s2 = (char *)((caddr_t)s + strlen(s) + 1);
        ent_dot = strtol(s2,NULL,0);
        if(ent_dec > err_dec){
            /* old one is larger */
            /* insert */
            newone->next = thisone;
            if(prev){
                prev->next = newone;
            }else{
                *head = newone;
            }
            return;
        }else if(ent_dec == err_dec){
            if(ent_dot > err_dot){
                /* insert */
                newone->next = thisone;
                if(prev){
                    prev->next = newone;
                }else{
                    *head = newone;
                }
                return;
            }
        }
        prev = thisone;
        thisone = thisone->next;
    }
 place_at_end:
    for(thisone=*head;thisone->next != NULL;thisone=thisone->next){
        ;
    }
    thisone->next = newone;
    return;
}

int
parse_data_input(char *buf,struct data_entry *ent, int lineno)
{

    char *dot=".";
    char *colon=":";
    char *space=" ";
    char *err,*time,*sec,*usec,*rb,*sz,*protocol;
    int secs,usecs,size;
    char timef[200];

/* 3.0 2.882067 # rb:0 size:10000:sctp: */
    err = strtok(buf,space);
    if(err == NULL){
        printf("can't find error rate - line %d skipping\n",
               lineno);
        return(-1);
    }
    time = strtok(NULL,space);
    if(time == NULL){
        printf("can't find time - line %d skipping\n",
               lineno);
        return(-1);
    }
    rb = strtok(NULL,space);
    if(rb == NULL){
        printf("can't find space before rb: - line %d skipping\n",
               lineno);
        return(-1);
    }
    rb = strtok(NULL,space);
    if(rb == NULL){
        printf("can't find space after rb: - line %d skipping\n",
               lineno);
        return(-1);
    }
    sz = strtok(NULL,colon);
    if(sz == NULL){
        printf("can't find colon before size:xxx: - line %d skipping\n",
               lineno);
        return(-1);
    }
    sz = strtok(NULL,colon);
    if(sz == NULL){
        printf("can't find colon after size:xxx: - line %d skipping\n",
               lineno);
        return(-1);
    }
    protocol = strtok(NULL,colon);
    if(sz == NULL){
        printf("can't find protocol after size:num:xxx: - line %d skipping\n",
               lineno);
        return(-1);
    }
    strcpy(timef, time);
    sec = strtok(timef,dot);
    if(sec == NULL){
	    printf("time %s is malformed, not x.xxxxxx - line %d skipping\n", time,
               lineno);
        return(-1);
    }
    if(strncmp(rb,"rb:",3) != 0){
        printf("rec_buf does not begin with rb: (%s) - line %d skipping\n",
               rb,lineno);
        return(-1);
    }
    usec = (char *)((caddr_t)sec + strlen(sec) + 1);
    secs = strtol(sec,NULL,0);
    /* trim up the leading 0's */
    {
	    int blat, blen;
	    blen = strlen(usec);
	    blat=0;
	    while(*usec == '0') {
		    usec++;
		    blat++;
		    if(blat >= blen)
			    break;
	    }
    }
    usecs = strtol(usec,NULL,0);
    if((secs == 0) && (usecs == 0)){
	    printf("bad time '%s' sec:%s usec:%s  measurement, can't be 0.00000 line %d skipping\n",
		   time, sec, usec,
		   lineno);
        return(-1);
    }
    size = strtol(sz,NULL,0);
    if(size == 0){
        printf("can't log a transfer of 0 bytes - line %d skipping\n",
               lineno);
        return(-1);
    }
    /* load entry */
    strcpy(ent->error_rate,err);
    ent->size = size;
    ent->time.tv_sec = secs;
    ent->time.tv_usec = usecs;
    ent->rcv_buf = strtol(&rb[3],NULL,0);
    ent->count = 1;
    ent->highest = ent->time;
    ent->lowest = ent->time;
    ent->next = NULL;
    if(strcmp(protocol,"tcp") == 0){
        ent->sctp = 0;
    }else if(strcmp(protocol,"sctp") == 0){
        ent->sctp = 1;
    }else{
        printf("Warning unknown protocol %s treat as tcp\n",
               protocol);
    }
    return(0);
}
int
print_live_sat_line(char *file,struct cache_entry *c, int *err_cnt, int mean,
		    int do_thruput)
{
	char outfile[256];
	FILE *out;
	struct data_entry *at;
	unsigned int sec,usec;
	int cnt=0;
	int err;
	at = c->recs;

	/* 
	 * Create a file of the name file.protocol
	 * Each output line will be of the form:
	 *        average   lowest    highest
	 * ----------------------------------------------------
	 * size  sec.usec  sec.usec   sec.usec # size:num-bytes protocol
	 *
	 * or avg, lowest, and highest throughputs if enabled
	 * 
	 */
	while(at){
		sprintf(outfile,"%s.%s",
			file,
			((at->sctp) ? "sctp" : "tcp"));
		out = fopen(outfile,"a+");
		if(out == NULL){
			err = *err_cnt;
			err++;
			*err_cnt = err;
			return(cnt);
		}
		cnt++;
		if(mean == 0){
			sec = at->time.tv_sec/at->count;
			usec = (at->time.tv_sec % at->count) * 1000000;
			usec += at->time.tv_usec;
			usec /= at->count;
			if (do_thruput) {
				/* show throughputs */
				double thru_avg, thru_low, thru_high;
				double xfer_time;

				/* calc avg throughput */
				xfer_time = (double) sec + (usec / 1000000);
				thru_avg = (at->size * 8) / xfer_time;
				/* calc lowest throughput */
				xfer_time = (double) at->highest.tv_sec +
					(at->highest.tv_usec / 1000000);
				thru_low = (at->size * 8) / xfer_time;
				/* calc highest throughput */
				xfer_time = (double) at->lowest.tv_sec +
					(at->lowest.tv_usec / 1000000);
				thru_high = (at->size * 8) / xfer_time;
				fprintf(out, "%d %6.6f %6.6f %6.6f # size:%d:%s\n",
					(int)at->size / 1000,
					thru_avg / 1000,
					thru_low / 1000,
					thru_high / 1000,
					(int)at->size,
					((at->sctp) ? "sctp" : "tcp"));
			} else {
				fprintf(out,"%d %d.%6.6d %d.%6.6d %d.%6.6d # size:%d:%s:\n",
					(int)at->size / 1000,
					(int)sec,
					(int)usec,
					(int)at->lowest.tv_sec,
					(int)at->lowest.tv_usec,
					(int)at->highest.tv_sec,
					(int)at->highest.tv_usec,
					(int)at->size,
					((at->sctp) ? "sctp" : "tcp")
				);
			}
		} else {
			struct data_entry *ent;
			int target,i;
			ent = at->list;
			target = at->cnt / 2;
			for(i=0;i<target;i++){
				ent = ent->list;
				if(ent == NULL){
					printf("Failed??\n");
					goto bad2;
				}
			}
			if (do_thruput) {
				/* show throughputs */
				double thru_avg, thru_low, thru_high;
				double xfer_time;

				/* calc avg throughput */
				xfer_time = (double) ent->time.tv_sec +
					(ent->time.tv_usec / 1000000);
				thru_avg = (at->size * 8) / xfer_time;
				/* calc lowest throughput */
				xfer_time = (double) at->highest.tv_sec +
					(at->highest.tv_usec / 1000000);
				thru_low = (at->size * 8) / xfer_time;
				/* calc highest throughput */
				xfer_time = (double) at->lowest.tv_sec +
					(at->lowest.tv_usec / 1000000);
				thru_high = (at->size * 8) / xfer_time;
				fprintf(out, "%d %6.6f %6.6f %6.6f # size:%d:%s\n",
					(int)at->size / 1000,
					thru_avg / 1000,
					thru_low / 1000,
					thru_high / 1000,
					(int)at->size,
					((at->sctp) ? "sctp" : "tcp"));
			} else {
				/* show transfer times */
				fprintf(out,"%d %d.%6.6d %d.%6.6d %d.%6.6d # size:%d:%s:\n",
					(int)at->size / 1000,
					(int)ent->time.tv_sec,
					(int)ent->time.tv_usec,
					(int)at->lowest.tv_sec,
					(int)at->lowest.tv_usec,
					(int)at->highest.tv_sec,
					(int)at->highest.tv_usec,
					(int)at->size,
					((at->sctp) ? "sctp" : "tcp")
					);
			}
		}
	bad2:
		fclose(out);
		at = at->next;
	}
	return(cnt);
}


int
print_cache_line(char *file,struct cache_entry *c, int *err_cnt, int mean)
{
	char outfile[256];
	FILE *out;
	struct data_entry *at;
	unsigned int sec,usec;
	int cnt=0;
	int err;
	at = c->recs;
	/* create or append to a file named file.proto.size 
	 * place either the mean or the average of this cache
	 * entry (for a given error rate) in the form:
	 *      average   lowest    highest
	 * ----------------------------------------------------
	 * ERR sec.usec  sec.usec  sec.usec # size:num-bytes protocol
	 */

	while(at){
		sprintf(outfile,"%s.%s.%d",
			file,
			((at->sctp) ? "sctp" : "tcp"),
			at->size);
		out = fopen(outfile,"a+");
		if(out == NULL){
			err = *err_cnt;
			err++;
			*err_cnt = err;
			return(cnt);
		}
		cnt++;
		if(mean == 0){
			sec = at->time.tv_sec/at->count;
			usec = (at->time.tv_sec % at->count) * 1000000;
			usec += at->time.tv_usec;
			usec /= at->count;
			fprintf(out,"%s %d.%6.6d %d.%6.6d %d.%6.6d # size:%d:%s:\n",
				c->error_rate,
				(int)sec,
				(int)usec,
				(int)at->lowest.tv_sec,
				(int)at->lowest.tv_usec,
				(int)at->highest.tv_sec,
				(int)at->highest.tv_usec,
				(int)at->size,
				((at->sctp) ? "sctp" : "tcp")
				);
		} else {
			struct data_entry *ent;
			int target,i;
			ent = at->list;
			target = at->cnt / 2;
			for(i=0;i<target;i++){
				ent = ent->list;
				if(ent == NULL){
					printf("Failed??\n");
					goto bad;
				}
			}
			fprintf(out,"%s %d.%6.6d %d.%6.6d %d.%6.6d # size:%d:%s:\n",
				c->error_rate,
				(int)ent->time.tv_sec,
				(int)ent->time.tv_usec,
				(int)at->lowest.tv_sec,
				(int)at->lowest.tv_usec,
				(int)at->highest.tv_sec,
				(int)at->highest.tv_usec,
				(int)at->size,
				((at->sctp) ? "sctp" : "tcp")
				);
		}
	bad:
		fclose(out);
		at = at->next;
	}   
	return(cnt);
}

int
is_it_in(int *list,int siz,int len)
{
    int i;
    for(i=0;i<len;i++){
        if(list[i] == siz)
            return(1);
    }
    return(0);
}


void
print_out_live_plots(char *output_file,
		     struct cache_entry *cache,
		     char *symbol,
		     char *do_baseline,
		     int postscript,
		     char *base_dir,
		     int epostscript,        
		     int color,
		     int lower_range,
		     int upper_range,
		     int do_thruput)
{
    FILE *master_out;
    FILE *out;
    char filename[100],outfile[100];
    struct data_entry *at;
    int num_ent;
    num_ent = 0;
    for(at=cache->recs;at;at=at->next){
        num_ent++;
    }
    sprintf(filename,"%s.lplotmaster",output_file);
    master_out = fopen(filename,"w+");
    if(master_out == NULL){
        printf("Can't open %s error %d\n",
               filename,errno);
        return;
    }
    sprintf(filename,"%s.plot_rbscp",
	    output_file);
    out = fopen(filename,"w+");
    if(out == NULL){
	    printf("can't write entry for size %d\n",
		   at->size);
	    return;
    }
    /* Dump the master out */
    if(postscript){
	    sprintf(outfile,"%s.ps",output_file);
    } else if (epostscript) {
	    sprintf(outfile,"%s.eps",output_file);
    }else{
	    sprintf(outfile,"%s.mif",output_file);
    }
    fprintf(master_out,"set nomultiplot\n");
    fprintf(master_out,"set output '%s'\n",outfile);
    if(postscript){
	    fprintf(master_out,"set terminal post landscape 'Times-Roman' 12\n");
	    if(color){
		    fprintf(master_out,"set terminal post color\n");
	    }
	    
    } else if (epostscript) {
	    fprintf(master_out,"set terminal post eps 'Times-Roman' 12\n");
	    if(color){
		    fprintf(master_out,"set terminal post color\n");
	    }
    } else {
	    fprintf(master_out,"set terminal mif color polyline\n");
    }
    fprintf(master_out,"set xlabel 'Size of transfer in kbytes '\n");
    if (do_thruput)	    
	    fprintf(master_out,"set ylabel 'Throughtput (kbps)'\n");
    else
	    fprintf(master_out,"set ylabel 'Time of Transfer (sec)'\n");
    fprintf(master_out,"load '%s'\n",
	    filename);
    fprintf(master_out,"reset\n");
    fflush(master_out);
    if(upper_range < lower_range) {
	    printf("messed up upper_range:%d < lower:%d - to def\n",
		   upper_range, lower_range);
	    lower_range = 0;
	    upper_range = 1000;
    }
    /* now prepare the plot data files */
    fprintf(out,"set xrange [%d:%d]\n", 
	    lower_range,
	    upper_range);
    fprintf(out,"set multiplot\n");
    fprintf(out,"set title 'Live satellite transfer'\n");
    fprintf(out,"set key outside Left title '  Legend' box 1\n");
    fprintf(out,"plot '%s.sctp' smooth be title '  sctp-%s' ,  \\\n",
	    output_file,
	    symbol);
    if(do_baseline == NULL){
	    fprintf(out,"'%s.tcp' smooth be title '  tcp-%s' \n",
		    output_file,
		    symbol); 
    }else{
	    fprintf(out,"'%s.tcp' smooth be title '  tcp-%s' , \\\n",
		    output_file,
		    symbol); 
	    fprintf(out,"'%s/%s.tcp' smooth be title '  tcp-%s' , \\\n",
		    base_dir,
		    do_baseline,
		    do_baseline);
	    fprintf(out,"'%s/%s.sctp' smooth be title '  sctp-%s'\n",
		    base_dir,
		    do_baseline,
		    do_baseline
		    );
    }
    fclose(out);
    fclose(master_out);
    sprintf(filename,"%s.lplotmaster",output_file);
    master_out = fopen("lrun.plot","w+");
    if(master_out == NULL){
        printf("Can't gen shell script\n");
    }
    fprintf(master_out,"#!/bin/sh\n");
    fprintf(master_out,"gnuplot %s\n",filename);
    fclose(master_out);
    chmod("lrun.plot",0755);
    system("lrun.plot");
}



void
print_out_plots(char *output_file,
                struct cache_entry *cache,
                char *symbol,
                char *do_baseline,
                int postscript,
                char *base_dir,
                int epostscript,        
                int color)
{
    FILE *master_out;
    FILE *out;
    char filename[100],outfile[100];
    struct data_entry *at;
    int num_ent,*ary;
    num_ent = 0;
    for(at=cache->recs;at;at=at->next){
        num_ent++;
    }
    sprintf(filename,"%s.plotmaster",output_file);
    master_out = fopen(filename,"w+");
    if(master_out == NULL){
        printf("Can't open %s error %d\n",
               filename,errno);
        return;
    }
    ary = (int *)calloc(num_ent,sizeof(int));
    memset(ary,0,(num_ent * sizeof(int)));
    num_ent = 0;
    for(at=cache->recs;at;at=at->next){
        if(is_it_in(ary,at->size,num_ent) == 0){
            sprintf(filename,"%s.plot_rbscp.%d",
                    output_file,
                    at->size);
            out = fopen(filename,"w+");
            if(out == NULL){
                printf("can't write entry for size %d\n",
                       at->size);
                continue;
            }
            /* Dump the master out */
            if(postscript){
                sprintf(outfile,"%s.%d.ps",output_file,at->size);
            } else if (epostscript) {
                sprintf(outfile,"%s.%d.eps",output_file,at->size);
            }else{
                sprintf(outfile,"%s.%d.mif",output_file,at->size);
            }
            fprintf(master_out,"set nomultiplot\n");
            fprintf(master_out,"set output '%s'\n",outfile);
            if(postscript){
		    fprintf(master_out,"set terminal post landscape 'Times-Roman' 12\n");
		    if(color){
			    fprintf(master_out,"set terminal post color\n");
		    }

            } else if (epostscript) {
		    fprintf(master_out,"set terminal post eps 'Times-Roman' 12\n");
		    if(color){
			    fprintf(master_out,"set terminal post color\n");
		    }

            } else {
                fprintf(master_out,"set terminal mif color polyline\n");
            }
            fprintf(master_out,"set xlabel 'Error Rate in %%'\n");
            fprintf(master_out,"set ylabel 'Time of Transfer'\n");
            fprintf(master_out,"load '%s'\n",
                    filename);
            fprintf(master_out,"reset\n");
            fflush(master_out);
            /* now prepare the plot data files */
            fprintf(out,"set multiplot\n");
            fprintf(out,"set title '%d byte transfer'\n",at->size);
            fprintf(out,"set key outside Left title '  Legend' box 1\n");
            fprintf(out,"plot '%s.sctp.%d' smooth be title '  sctp-%s' ,  \\\n",
                    output_file,
                    at->size,
                    symbol);
            if(do_baseline == NULL){
                fprintf(out,"'%s.tcp.%d' smooth be title '  tcp-%s' \n",
                        output_file,
                        at->size,
                        symbol); 
            }else{
                fprintf(out,"'%s.tcp.%d' smooth be title '  tcp-%s' , \\\n",
                        output_file,
                        at->size,
                        symbol); 
                fprintf(out,"'%s/%s.tcp.%d' smooth be title '  tcp-%s' , \\\n",
                        base_dir,
                        do_baseline,
                        at->size,
                        do_baseline);
                fprintf(out,"'%s/%s.sctp.%d' smooth be title '  sctp-%s'\n",
                        base_dir,
                        do_baseline,
                        at->size,
                        do_baseline
                        );
            }
            fclose(out);
            ary[num_ent] = at->size;
            num_ent++;
        }
    }
    free(ary);
    fclose(master_out);
    sprintf(filename,"%s.plotmaster",output_file);
    master_out = fopen("run.plot","w+");
    if(master_out == NULL){
        printf("Can't gen shell script\n");
    }
    fprintf(master_out,"#!/bin/sh\n");
    fprintf(master_out,"gnuplot %s\n",filename);
    fclose(master_out);
    chmod("run.plot",0755);
    system("run.plot");
}

int
main(int argc, char **argv)
{
	FILE *io;
	char buf[256];
	struct data_entry de;
	int line_cnt=0,i,errs=0,outt=0;
	char *input_file=NULL;
	char *output_file=NULL;
	char *symbol="no.sym";
	char *base_dir = ".";
	char *do_base_line = NULL;
	int postscript=0;
	int epostscript=0;
	int color=0;
	int compare= 0;
	int mean = 0;
	int upper_range = 1000;
	int lower_range = 0;
	int do_thruput = 0;
	struct cache_entry *thisone;

	while((i= getopt(argc,argv,"s:i:o:b:p?B:ecCmitu:l:")) != EOF){
		switch(i){
		case 'l':
			lower_range = strtol(optarg,NULL,0);
			break;
		case 'u':
			upper_range = strtol(optarg,NULL,0);
			break;
		case 'm':
			mean = 1;
			break;
		case 'C':
			compare = 1;
			break;
		case 'c':
			color = 1;
			break;
		case 't':
			do_thruput = 1;
			break;
		case 'e':
			postscript = 0;
			epostscript = 1;
			break;
		case 's':
			symbol = optarg;
			break;
		case 'i':
			input_file = optarg;
			break;
		case 'o':
			output_file = optarg;
			break;
		case 'b':
			do_base_line = optarg;
			break;
		case 'B':
			base_dir=optarg;
			break;
		case 'p':
			epostscript = 0;
			postscript = 1;
			break;
		case '?':
		exit_now:
			printf("Use %s -i input-file -o output-file [-p -s 'symbol' b -B dir -e -c -t] \n",
			       argv[0]);
			printf(" -p - issue postscript output (default is mif)\n");
			printf(" -e - use encapsulated postscript\n");
			printf(" -s 'symbol' - include symbole sctp-'sym' in graph\n");
			printf(" -b - include base.tcp.x and base.sctp.x lines\n");
			printf(" -B - directory of baseline tcp/sctp plots\n");
			printf(" -c - Color on for postscript\n");
			printf(" -t - show throughputs instead of xfer time\n");
			printf(" -u num - Upper range for live sat plot\n");
			printf(" -l num - Lower range for live sat plot\n");
			printf(" For range in upper/lower is val = size/10000\n");
			return(0);
		}
	}
	if(input_file == NULL){
		printf("No input file specified :-<\n");
		goto exit_now;
	}
	if(output_file == NULL){
		printf("No output file specified :-<\n");
		goto exit_now;
	}
	io = fopen(input_file,"r");
	if(io == NULL){
		printf("Can't open %s error: %d\n",
		       argv[1],errno);
		return(-1);
	}
	while(fgets(buf,sizeof(buf),io) != NULL){
		line_cnt++;
		/* skip any comments or empty lines */
		if(buf[0] == '#')
			continue;
		if(buf[0] == '\n')
			continue;
		/* parse it */
		if(parse_data_input(buf,&de,line_cnt) == 0){
			add_entry_to_cache(&cache,&de);
		}
	}
	if(compare == 0){
		if(cache->next){
			for(thisone=cache;thisone!=NULL;thisone=thisone->next){
				/* Dump out the data for later use */
				outt += print_cache_line(output_file, thisone,
							 &errs, mean);
			}
		} else {
			outt += print_live_sat_line(output_file, cache, &errs,
						    mean, do_thruput);
		}
	}
	if(cache->next != NULL)
		print_out_plots(output_file, cache, symbol, do_base_line,
				postscript, base_dir, epostscript, color);
	else {
		print_out_live_plots(output_file, cache, symbol, do_base_line,
				     postscript, base_dir, epostscript, color, 
				     lower_range, upper_range, do_thruput);
		return(0);
	}
	printf("we put out %d lines with %d file-errors\n",
	       outt,errs);
	return(0);
}
