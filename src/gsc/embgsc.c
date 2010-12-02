/*==================================================================
** NAME         :embgsc.c
** TYPE         :library
** DESCRIPTION  :search by cone(coordinates) in GSC 
**              :finds regions of the GSC in file gsc_reg.bin
**              :finds objects in selected regions
** INPUT        :RA(deci.h), Dec(dec.d), radius1 [,radius2] (arcmin)
**              :
** OUTPUT       :list of GSC objects
**              :
** AUTHOR       :apm, modified (embedded) adc
** DATE         :9/92;10/92;
** 		:5/96: Adapted for PCs, FO
** 		:Nov. 2000, swap bin files rather than recreate
** LIB          :pm.a; use cca to compile
*=================================================================*/
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <gsc.h>
#define REGBIN	"/regions.bin"
#define REGIND	"/regions.ind"
#define scale 3600000L
#define phir  150.	/* unused */
#define radian (180./M_PI)

int band[] = { 0,1,6,8,10,11,12,13,14,18,4,3,16};
int opt[] = { 0,0,0,0,0,0,0,0,0,0,0,0 };


typedef int (*FCT)();	/* Compare function */
typedef struct lgsc_s {	/* Linked GSC record*/
	struct lgsc_s *prev;
	GSCREC rec ;
} LGSC ;


LGSC *last, *agsc = NULL;
int mgsc, igsc;
//static FCT compare ;
//static int order = 1;  /* sorting in ascending order. -1=descending */

/* static unsigned char table[MAXNSORT*12]; *//* Buffer for reading packed GSC */

/*
static char help[] = "\
How to search the Guide Star Catalogue (GSC1.1) by coordinates.\n\
\n\
	gsc -option parameter\n\
\n\
    option     	parameter(s)	Explanations\n\
    ------	------------	--------------------------------------------\n\
	-c	ra +/-dec 	Equatorial position of the center, either\n\
				  in sexagesimal, or in decimal degrees\n\
	-e[+]	 		Display Epoch of plate rather than plate id\n\
	-f	file 		Equatorial coordinates as above, in a file\n\
	-g	GSC-id		Search from GSC-id\n\
	-h			Prints header for output columns\n\
	-help			Prints this help\n\
	-H			Prints detailed column meaning\n\
	-n	output_lines	Max. number output of sorted lines (30)\n\
	-m	mag1 mag2	Limits in magnitude (mag1<mag2).\n\
	-p	0 		Output as in the GSC (decimal degrees)\n\
		1		Equatorial position displayed in sexagesimal\n\
		2		Short output (no errors/plate info)\n\
		3		Output as -p 2, but position in sexagesimal\n\
	-r	dist[,dist2] 	Maximal search radius in arcmin (def=10')\n\
	-s	[column_no]	Sort field number from 1 to 12 (def. 11),\n\
				  descending order if column_no<0.\n\
	-v			Verbose option\n\
    ------	------------	--------------------------------------------\n\
Input lines starting by # or ! are just echoed (interpreted as comments)\
";
*/
 
/*
  \n\
  If coordinates (or file with coordinates) are not specified, \n\
  the target positions are read from standard input.\n\
  Distance from centre (arcmin) and position angle (degrees East of North)\n\
  are always provided at the end of the printed record.\
*/

/*
static char GSC_intro[] = "\n\
===========================================================================\n\
THE GUIDE STAR CATALOG, Version 1.1 (CDS/ADC catalog <I/220>)\n\
\n\
The original version of this catalog, GSC 1.0, is described in a series\n\
of papers: \n\
I.   Astronomical foundations and image processing \n\
     (Lasker et al.,  =1990AJ.....99.2019L)\n\
II.  Photometric and astrometric models and solutions\n\
     (Russel et al.,  =1990AJ.....99.2059R)\n\
III. Production, database organization, and population statistics\n\
     (Jenkner et al., =1990AJ.....99.2082J)\n\
\n\
Additions and corrections made in GSC 1.1 address:\n\
   - incompleteness, misnomers, artifacts, and other errors due to the\n\
     overexposure of the brighter stars on the Schmidt plates,\n\
   - the identification of blends likely to have been incorrectly resolved,\n\
   - the incorporation of errata reported by the user-community or \n\
     identified by the analysis of HST operational problems.\
" ;

static char GSC_explain[] = "\
The columns have the following meaning:\n\
===========================================================================\n\
GSC-id  = GSC Identification, made of plate number (5 digits) and\n\
          star number on the plate (5 digits); note that, in the\n\
          literature, a dash separates generally the two parts.\n\
ra dec  = Position in J2000 (FK5-based) frame\n\
pos-e   = mean error on position (arcsec)\n\
mag     = photographic magnitude\n\
mag-e   = mean error on photographic magnitude \n\
b       = band (emulsion type), coded as follows:\n\
           magband emulsion filter\n\
           ------- -------- --------\n\
                 0 IIIaJ    GG395\n\
                 1 IIaD     W12\n\
                 6 IIaD     GG495\n\
                 8 103aE    Red Plex\n\
                10 IIaD     GG495\n\
                11 103aO    GG400\n\
                18 IIIaJ    GG385\n\
                 4 (bright star)\n\
           ------- -------- --------\n\
c       = class (0 = star / 3 = non-stellar)\n\
pl      = plate identification\n\
mu      = T if multiple object / F otherwise\n\
d'      = distance (arcmin) from asked position\n\
pa      = position angle (degrees) from asked position\n\
=======================================================================\
";

*/

int swap (array, nint)
/*++++++++++++++++
  .PURPOSE  Swap the bytes in the array of integers if necessary
  .RETURNS  0/1/2 (type of swap)
  .REMARKS  Useful for big-endian machines
  -----------------*/
INT *array ;	/* IN: The array to convert   */
int nint ;	/* IN: The number of integers */
{
	static INT value ;
	char *v, *p, *e ;
	int n ;
  
	value = 0x010203 ;
	v = (char *)(&value) ;
	if ((v[0] == 0) && (v[1] == 1) && (v[2] == 2)) {
		return(0) ;	/* No swap necessary */
	}
    
	p = (char *)array ; e = p + 4*nint ;

	if ((v[0] == 1) && (v[1] == 0) && (v[2] == 3)) {	/* Half-word swap */
		while (p < e) {
			n = p[0] ; p[0] = p[1] ; p[1] = n ;
			p += 2 ;
		}
		if(opt[Ov]) fprintf(stderr, "....HW-swapped %d integers\n", nint) ;
		return (1) ;
	}

	if ((v[0] == 3) && (v[1] == 2) && (v[2] == 1)) {	/* Full-word swap */
		while (p < e) {
			n = p[0] ; p[0] = p[3] ; p[3] = n ;
			n = p[1] ; p[1] = p[2] ; p[2] = n ;
			p += 4 ;
		}
		if(opt[Ov]) fprintf(stderr, "....FW-swapped %d integers\n", nint) ;
		return(2) ;
	}

	fprintf(stderr, "****Irrationnal Byte Swap %02x%02x%02X%02x\n", 
		v[0]&0xff, v[1]&0xff, v[2]&0xff, v[3]&0xff) ;
	exit(1) ;
}

#if 0
static int cmp_1 (a, b)   GSCREC *a, *b ;	/* Sort on GSC Number */
{
	if(a->reg < b->reg) return(-order);
	if(a->reg > b->reg) return(order);
	if(a->id  < b->id ) return(-order);
	if(a->id  > b->id ) return(order);
	if(a->m < b->m) return(-order);		/* Identical GSC-number */
	if(a->m > b->m) return(order);
	return(0);
}

static int cmp_2 (a, b)   GSCREC *a, *b ;	/* Sort on RA  Number */
{
	if(a->ra < b->ra) return(-order);
	if(a->ra > b->ra) return(order);
	return(0);
}

static int cmp_3 (a, b)   GSCREC *a, *b ;	/* Sort on Declination */
{
	if(a->dec < b->dec) return(-order);
	if(a->dec > b->dec) return(order);
	return(0);
}

static int cmp_4 (a, b)   GSCREC *a, *b ;	/* Sort on poserr	*/
{
	if(a->poserr < b->poserr) return(-order);
	if(a->poserr > b->poserr) return(order);
	return(cmp_1(a,b));
}

static int cmp_5 (a, b)   GSCREC *a, *b ;	/* Sort on magnitude	*/
{
	if(a->m < b->m) return(-order);
	if(a->m > b->m) return(order);
	return(cmp_1(a,b));
}

static int cmp_6 (a, b)   GSCREC *a, *b ;	/* Sort on mag.error	*/
{
	if(a->merr < b->merr) return(-order);
	if(a->merr > b->merr) return(order);
	return(cmp_1(a,b));
}

static int cmp_7 (a, b)   GSCREC *a, *b ;	/* Sort on mb		*/
{
	if(a->mb < b->mb) return(-order);
	if(a->mb > b->mb) return(order);
	return(cmp_1(a,b));
}

static int cmp_8 (a, b)   GSCREC *a, *b ;	/* Sort on class	*/
{	
	if(a->cl < b->cl) return(-order);
	if(a->cl > b->cl) return(order);
	return(cmp_1(a,b));
}

static int cmp_9 (a, b)   GSCREC *a, *b ;	/* Sort on plate	*/
{ 
	int diff;
	if ((diff = strcmp(a->plate, b->plate))) return(diff*order);
	return(cmp_1(a,b));
}

static int cmp_10 (a, b)   GSCREC *a, *b ;	/* Sort on multiple	*/
{ 
	if(a->mu < b->mu) return(-order);
	if(a->mu > b->mu) return(order);
	return(cmp_1(a,b));
}

static int cmp_11 (a, b)   GSCREC *a, *b ;	/* Sort on Distance	*/
{ 
	if(a->dist < b->dist) return(-order);
	if(a->dist > b->dist) return(order);
	return(cmp_1(a,b));
}

static int cmp_12 (a, b)   GSCREC *a, *b ;	/* Sort on PosAngle	*/
{ 
	if(a->posang < b->posang) return(-order);
	if(a->posang > b->posang) return(order);
	return(cmp_1(a,b));
}

static int cmp_13 (a, b)   GSCREC *a, *b ;	/* Sort on Epoch	*/
{ 
	if(a->epoch < b->epoch) return(-order);
	if(a->epoch > b->epoch) return(order);
	return(cmp_1(a,b));
}

#endif

#if 0
static void prtOs(a)	GSCREC *a;		/* Print Os parameter	*/
{
	switch(opt[Os]) {
	case 1: printf("     %05d-%05d", a->reg, a->id); break;
	case 2: printf(" %7.3f", a->ra); break;
	case 3: printf(" %+7.3f", a->dec); break;
	case 4: printf("%5.1f", a->poserr); break;
	case 5: printf(" %7.2f", a->m); break;
	case 6: printf("%5.2f", a->merr); break;
	case 7: printf("%4d", a->mb); break;
	case 8: printf("%2d", a->cl); break;
	case 9: printf("%5s", a->plate); break;
	case 10: printf(" %c", a->mu); break;
	case 12: printf("%4.0f", a->posang); break;
	case 13: printf("%10.3f", a->epoch); break;
	default: printf("%5.1f", a->dist); break;
	}
}
#endif

/*-------- Interpret a GSC number / region ---------------------------------*/
static int gscid(str, GSCid)	/* Returns number of bytes scanned */
char *str;
int GSCid[2] ;
{
	char *p; int i;
	GSCid[0] = GSCid[1] = 0;
	for (p=str; isspace(*p); p++) ;
	for (i=5; (--i>=0) && isdigit(*p); p++)  
		GSCid[0] = GSCid[0]*10 + (*p - '0');
	if (*p == '-') p++;
	for (i=5; (--i>=0) && isdigit(*p); p++)
		GSCid[1] = GSCid[1]*10 + (*p - '0');
	return(p-str);
}

#if 0
/*-------- Addition of a new record in the list, ordered by compare --------*/
static int add_rec(new)			/* Returns 0 if record ignored */
GSCREC *new;
{
	LGSC *gc, *gp, *gn;
  	/* (1) Find where in the list we've to insert the new record */
  	for (gc=last, gp=(LGSC *)0; gc && ((*compare)(new, &(gc->rec)) < 0);
  	     gp=gc, gc=gc->prev) ;
  	     
     	/* (2) If max attained, remove or return */
     	if (igsc == mgsc) {
		if (!gp) return(0);
		gn = last;
		if (gp == last) gp = (LGSC *)0;
		else last = last->prev ;
     	}
     	else gn = agsc + igsc++ ;

     	/* (3) Insert the new record, and set the links */
     	*(&(gn->rec)) = *new ;
     	gn->prev = gc;
     	if (gp) gp->prev = gn;
     	else last = gn;

	/* Debugging */
	if(opt[Ov] > 1) {
		printf("!...("); prtOs(new); printf(")");
		for (gc=last; gc; gc=gc->prev) prtOs(&(gc->rec));
		printf("\n");
      	}

     	return(1);
}

/*==========================================================================*/

static FCT cmp[] = { cmp_1, cmp_2, cmp_3, cmp_4, cmp_5, cmp_6, cmp_7,
		     cmp_8, cmp_9, cmp_10, cmp_11, cmp_12, cmp_13 };

#endif

/*==========================================================================*/

#define GSC_PATH "/usr/local/share/catalogs/gsc"

int getgsc(float ra,         // ra of center (in degrees)
	   float dec,        // dec of center (in degrees)
	   float radius,     // radius (in minutes)
	   float maxmag,     // maximum magnitude
	   int *regs,        // star regions
	   int *ids,         // star ids (to be printed with %05d)
	   float *ras,       // ra of each gsc object, returned
	   float *decs,      // dec of each gsc object, returned
	   float *mags,      // magnitude of each gsc object, returned
           int n,		// maximum objects to return
	   char *catpath)            

{
	char *GSCDAT;
/* Default, superseded by $GSCDAT or the catpath argument*/
	char *GSCBIN  = NULL;
	int bin_swapped = 0 ;		/* BIN files are swapped */
	tr_regions rec[NREC];
	FILE *fin /*= stdin */;
	INT *ind2 = NULL;
	int gid[2] = {0,0};


	char *ptr; int centers;
	char line[256], *s, region[256], path[256];
	int sum,k,i,stat=0,id;
	INT size,x1,x2,z1=0,z2,xx1,xx2,zz1,zz2;
	INT alpha,hscale=1,hscad2=1,xr1=0,xr2=0;
	int np,fz,f2,fr,n2,cc,nrec,tester;
	INT da,dd,da1,da2,dd1,dd2,nout,tested;
	double rar,der,a1,a2,d1,d2,a,phi,phi1=0.0,phi2=10.0;
	double da0,dd0,dist;
	double cosdec,ra0;
	float magmin=0.0, magmax=25.0;
	long p;

	HEADER header;
	GSCREC gscrec;
	unsigned char *c, *table = NULL;

	nout = 0;

//   opt[Ov] = 2;

//   printf("ra=%f dec=%f radius %f maxn %d\n", ra, dec, radius, n);


	fin = stdin ;
	c = table = (unsigned char*)malloc(12*MAXNSORT);

//	printf("catpath is %s\n", catpath);

/* ------ Find out the required path variables --------------------------*/
	if (catpath == NULL || catpath[0] == 0) { /* use default path */
		GSCDAT = getenv("GSCDAT") ;
		if (!GSCDAT) 
			GSCDAT = GSC_PATH ;
	} else {
		GSCDAT = catpath;
	}

	/* Get the GSCBIN environment variable */
	GSCBIN = getenv("GSCBIN");
	if (! GSCBIN) {
		strcpy(path, GSCDAT);
		strcat(path, "/bin");
		GSCBIN = strdup(path);
	}

/* ------ parse command-line options ------------------------------------*/

	line[0] = '\0';
	sum = 0;
  
	da0 = ra;
	dd0 = dec;
	opt[Oc] = 1;

	opt[On] = n;

	phi1 = 0.0;
	phi2 = radius;

	opt[Or] = 1;
	magmax = maxmag;
	opt[Om] = 1;

	/* ------ open region list and read region index ------------------- */

	strcpy(path, GSCBIN), strcat(path, REGBIN);
//	printf("path1 %s\n", path);
	fz=open(path, O_BINARY);  
	if(fz < 0) { perror(path); goto err_ret; }

	strcpy(path, GSCBIN), strcat(path, REGIND);
//	printf("path2 %s\n", path);
	f2=open(path, O_BINARY);  
	if(f2 < 0) { perror(path); goto err_ret; }
	size = lseek(f2,0L,2);
	lseek(f2,0L,0);
	ind2 = (INT *)malloc(size);
	cc=read(f2,ind2,size);
	if (cc < 0) { perror(path); goto err_ret; }
	n2 = size/sizeof(INT);
	ind2[0] = 0;
	close(f2);

	if ((ind2[0] > ind2[1]) || (ind2[1] > ind2[2]))
		/* Most likely Byte Swap !! */
		bin_swapped = swap(ind2, n2) ;

	mgsc = opt[On];
	agsc = (LGSC *)malloc(mgsc*sizeof(LGSC));

	/* ------ Loop on Central Coordinates -------------- */

	a1 = a2 = d1 = d2 = 0; 
	for( centers = 1 ; --centers >= 0 ; ) {
		igsc = nout = 0;
		tester = tested = 0;
		last = (LGSC *)0;
		if ((opt[Og]==1) || ((opt[Oc]|opt[Og])==0)) {
			if (!fgets(line,sizeof(line),fin)) continue;
			if (strncasecmp(line, "quit", 4) == 0) continue ;
			centers = 1;
			if ((ptr = strchr(line, '\n'))) *ptr = '\0';
			if (opt[Ov]) printf("!...I got '%s'", line);
			if (ispunct(line[0])) {
				if (opt[Ov]) printf("\t# (ignored)\n");
				else puts(line);
				continue ;
			}
			if (opt[Og]) {
				gscid(line, gid);
				if (opt[Ov]) printf("\t= %05d%05d\n", gid[0], gid[1]);
			}
		}

		if(opt[Oh]) {
			prtgsc(opt[Op],opt[Oe],(GSCREC *)0) ;
			memset(&gscrec, 0, sizeof(gscrec));
			if (!opt[Og]) {
				gscrec.ra = da0;
				gscrec.dec = dd0;
				s = gsc2a(opt[Op],opt[Oe],&gscrec) ;
				s[0] = '#';
				for (i=1; i<10; i++) s[i] = ' ';
				puts(s);
			}
		}
    
		if(opt[Og]) {
			zz1 = z2 = 0;
			goto Declination_LOOP ;
		}
    
		alpha = da0*scale;
		cosdec = cos(dd0/radian);
		ra0 = da0;
    
		/* ------ find r.a. - dec box ------------------------------- */
    
		phi = phi2/60.;
		d1 = dd0-phi;
		d2 = dd0+phi;
		if(d1 < -90.0) d1= -90.0;
		if(d2 >  90.0) d2= 90.0;
		z1 = (d1+90.)*scale;
		z2 = (d2+90.)*scale;
    
		hscale = 360.0*scale;
		hscad2 = 180.0*scale;
		if(fabs(dd0)+phi == 90.000000) {
			x1 = alpha - 90.0*scale;
			x2 = alpha + 90.0*scale;
		}
		else if(fabs(dd0)+phi < 90.000000) {
			a=asin(sin(phi/radian)/cosdec)*radian*scale;
			x1 = alpha - a;
			x2 = alpha + a;
		}
		else {
			x1 = 0;
			x2 = hscale;
		}
		stat = 0;
		if(x2 > hscale) {
			x2 -= hscale;
			x1 -= hscale;
			stat = 1;  
		}
		if(x1 < 0) stat = 1;
		xr1 = x1;
		xr2 = x2;
    
		/* ------ searching regions in index ---------------------------- */
    
		/* printf("====Look in ind2[%d]", n2); */
#if 0
		for(i=0; i<n2; i++) {
			if(ind2[i] < z1-phir/60.*scale)ii=i;
			else break;
		}
		if((i= --ii) < 0)i=0;
#else
		for (i=0; (i<n2) && (z1 < ind2[i]); i++ ) ;
		if (i >= n2) i = n2-1 ;
#endif
		p = i*(100L*sizeof(tr_regions));
		/* printf(" =>%d, seek to %ld\n", i, p); */
		lseek(fz,p,0);
		zz1 = -hscale;
    
	Declination_LOOP:
		while((zz1 <= z2) && (nout <= opt[On])) {
			if (opt[Og]) { nrec = 1 ; goto Regions_LOOP ; }
			cc=read(fz,rec,sizeof(rec));
			if(cc < 1) break; 
			nrec = cc/sizeof(tr_regions);
			if (bin_swapped) swap(rec, cc/4) ;
      
			Regions_LOOP :
				for(i=0;(i<nrec) && (zz1<=z2) && (nout <= opt[On]); i++) {
					if (opt[Og]) { 
						rec[i].nr = gid[0]; 
						zz1++ ;
						goto Region_LOOP ; 
					}
					zz1 =rec[i].dec1;
					zz2 =rec[i].dec2;
					if(zz1 > z2) continue ;
					if(zz2 < z1) continue ;
	  
					xx1 = rec[i].alf1;
					xx2 = rec[i].alf2;
					x1 = xr1;
					x2 = xr2;
					if(xx2 > hscale || (x1<0 && xx2>hscad2)) { 
						xx1 -= hscale;
						xx2 -= hscale;
						if(stat == 0) {
							x1 -= hscale;
							x2 -= hscale; 
						}
					} 
	  
					if(xx1 > x2) continue ;
					if(xx2 < x1) continue ;
					a1 = (double)x1/scale;
					a2 = (double)x2/scale;
					if (x1 < 0 && rec[i].alf2 > hscad2) {
						a1 += 360.0;
						a2 += 360.0;
					}
	  
					/* ------ open and decode selected region ----------- */
	  
				Region_LOOP:
					find_reg(region, rec[i].nr, GSCDAT);
					fr = open(region,O_BINARY);
					if (fr < 0) { perror(region); continue ; }
	  
					if(opt[Ov]) printf("!...Look in file: %s\n", region);
					tester +=1 ;
	  
					s = get_header(fr,&header);
					/* lseek(fr,header.len,0); */
					da1 = (a1-header.amin)*header.scale_ra+0.5;
					da2 = (a2-header.amin)*header.scale_ra+0.5;
					dd1 = (d1-header.dmin)*header.scale_dec+0.5;
					dd2 = (d2-header.dmin)*header.scale_dec+0.5;
	  
					np = k = 1;
					while ((np>0) && (nout<=opt[On])) { /* np = records read */
						k-- ; c += 12 ;
						if (k == 0) {
							size = read(fr, table, 12*MAXNSORT);
							if (size < 0) { perror(region); goto err_ret; }
							np = (size/12); tested += np;
							if(opt[Ov] > 1) 
								printf("!....Read %d bytes (%d records)\n", 
								       size, np);
							c = table - 12;
							k = np + 1;
							continue ;
						}
						da = ((c[1]&1)<< 8) | c[2];
						da <<= 8;
						da |= c[3];
						da <<= 8;
						da |= c[4];
						da >>= 3;
	    
						dd = c[4] & 7;
						dd <<= 8;
						dd |= c[5];
						dd <<= 8;
						dd |= c[6];
	    
						if (opt[Og]) {		/* Test GSC-id	*/
							id = ((c[0]&127)<<7)|(c[1]>>1);
							if (gid[1] && id != gid[1]) continue ;
						}
						else  {			/* Test range RA/DE */
							if (da < da1) continue ;
							if (da > da2) continue ;
							if (dd < dd1) continue ;
							if (dd > dd2) continue ;
						}
						s = decode_c(c,&header,&gscrec);
	    
						if (!opt[Og]) {
							rar = gscrec.ra;
							der = gscrec.dec;
							gscrec.posang=dispos(&ra0,&dd0,&rar,&der,&dist);
							gscrec.dist = dist;
	      
							if(dist < phi1) continue ;
							if(dist > phi2) continue ;
						}

						if(gscrec.m < magmin) continue ;
						if(gscrec.m > magmax) continue ;
	    

						ras[nout] = gscrec.ra;
						decs[nout] = gscrec.dec;
						ids[nout] =  gscrec.id;
						regs[nout] = gscrec.reg;
						mags[nout] = gscrec.m;
						nout ++;

					}
					close(fr);
				}
		}
	}
	close(fz);
	if (agsc) {
		free(agsc);
		agsc = NULL;
	}
	if (ind2) {
		free(ind2);
	}
	if (table) {
		free(table);
	}
	if (GSCBIN) {
		free(GSCBIN);
	}
	return nout;
err_ret:
	if (agsc) {
		free(agsc);
		agsc = NULL;
	}
	if (ind2) {
		free(ind2);
	}
	if (table) {
		free(table);
	}
	if (GSCBIN) {
		free(GSCBIN);
	}
	return -1;
}


/*
  main() {

  float ras[1000], decs[1000], mags[1000];
  int regs[1000], ids[1000];
  int nn;

  nn = getgsc(17.9, 11.16, 30.0, regs, ids,ras,decs,mags,1000);
  printf("Found %d stars\n",nn);

  }
*/
