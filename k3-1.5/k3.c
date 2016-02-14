/* k3 - a simple prettifier for /proc/meminfo.
 * Public domain by Russell Marks 1993,1996,1999,2001.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define K3_VERSION	"1.5"


int buf_free=1;


#define BAR_WIDTH	40

#define USEDCHAR '#'
#define DBUFCHAR '%'
#define FREECHAR ':'


void showbar(char *title,int availk,int buffk,int totalk)
{
int limit1,limit2,count,pcnt;

pcnt=((availk+buf_free*buffk)*100)/totalk;

printf("%s ",title);
printf("%3d%% free (%8dk/%8dk) [",pcnt,availk+buf_free*buffk,totalk);

limit1=BAR_WIDTH-((availk*BAR_WIDTH)/totalk);
limit2=BAR_WIDTH-(((availk+buffk)*BAR_WIDTH)/totalk);

for(count=0;count<BAR_WIDTH;count++)
  putchar((count<limit2)?USEDCHAR:((count<limit1)?DBUFCHAR:FREECHAR));

puts("]");
}


/* works for 2.0?, 2.2, 2.4 */
void readmeminfo_old(int *pmf,int *pmt,int *pmb,int *pms,int *psf,int *pst)
{
FILE *in;
char buf[128],junk[128];
int waste,cached;

if((in=fopen("/proc/meminfo","r"))==NULL)
  fprintf(stderr,"k3: couldn't read /proc/meminfo.\n"),exit(1);

fgets(buf,sizeof(buf),in);
fgets(buf,sizeof(buf),in);
sscanf(buf,"%s%d%d%d%d%d%d",junk,pmt,&waste,pmf,pms,pmb,&cached);
/* we bundle together buffers/cached like `free' does with its "+/-"... */
*pmb+=cached;
fgets(buf,sizeof(buf),in);
sscanf(buf,"%s%d%d%d",junk,pst,&waste,psf);
fclose(in);

/* actually want it in k, not bytes */
*pmf>>=10;
*pmt>>=10;
*pmb>>=10;
*pms>>=10;
*psf>>=10;
*pst>>=10;
}


/* works for 2.2?, 2.4; reads the "kB" fields.
 * The advantage is that this copes with >=2GB (or is it 4GB?) RAM or
 * swap values correctly on 32-bit systems.
 * Returns 0 if it failed and we should try the old scheme.
 */
int readmeminfo_new(int *pmf,int *pmt,int *pmb,int *pms,int *psf,int *pst)
{
FILE *in;
char buf[128],*ptr;
int cached=0,val;

if((in=fopen("/proc/meminfo","r"))==NULL)
  fprintf(stderr,"k3: couldn't read /proc/meminfo.\n"),exit(1);

*pmt=0;

while(fgets(buf,sizeof(buf),in)!=NULL)
  {
  ptr=strchr(buf,':');
  if(!ptr) continue;
  *ptr=0;
  val=atoi(ptr+1);
  
  if(strcmp(buf,"MemTotal")==0)
    *pmt=val;
  else if(strcmp(buf,"MemFree")==0)
    *pmf=val;
  else if(strcmp(buf,"MemShared")==0)
    *pms=val;
  else if(strcmp(buf,"Buffers")==0)
    *pmb=val;
  else if(strcmp(buf,"Cached")==0)
    cached=val;
  else if(strcmp(buf,"SwapTotal")==0)
    *pst=val;
  else if(strcmp(buf,"SwapFree")==0)
    *psf=val;
  }

fclose(in);

if(*pmt==0) return(0);		/* failed */

/* we bundle together buffers/cached like `free' does with its "+/-"... */
*pmb+=cached;

return(1);
}


void usage(void)
{
printf("k3 " K3_VERSION " - public domain by Russell Marks.\n\n");
printf("\
usage: k3 [-r]

	-r	show real amount free (in numeric totals), rather than
		regarding buffers/cached as more-or-less `free'.
");
}


int main(int argc,char *argv[])
{
int mf,mt,mb,ms,sf,st;

if(argc==2 && strcmp(argv[1],"-r")==0)
  buf_free=0,argc--,argv++;

if(argc!=1) usage(),exit(1);

if(!readmeminfo_new(&mf,&mt,&mb,&ms,&sf,&st))
  readmeminfo_old(&mf,&mt,&mb,&ms,&sf,&st);

showbar("Phys",mf,mb,mt);
if(st>0)
  showbar("Swap",sf,0,st);

exit(0);
}
