// ucac4 -- local UCAC4 catalogue search

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


/* Based on public domain access functions provided by pluto (at)
   projectpluto.com */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gcx.h"
#include "ucac4.h"
#include "catalogs.h"


/* Note: sizeof( UCAC4_STAR) = 78 bytes */

/* Function to do byte-reversal for non-Intel-order platforms. */
/* NOT ACTUALLY TESTED ON SUCH MACHINES YET.  At least,  not   */
/* to my knowledge... please let me know if you _do_ test it,  */
/* fixes needed,  etc.!                                        */

#ifdef __BYTE_ORDER
#if __BYTE_ORDER == __BIG_ENDIAN
static void swap_32( int32_t *ival)
{
	int8_t temp, *zval = (int8_t *)ival;

	temp = zval[0];
	zval[0] = zval[3];
	zval[3] = temp;
	temp = zval[1];
	zval[1] = zval[2];
	zval[2] = temp;
}

static void swap_16( int16_t *ival)
{
	int8_t temp, *zval = (int8_t *)ival;

	temp = zval[0];
	zval[0] = zval[1];
	zval[1] = temp;
}

void flip_ucac4_star( UCAC4_STAR *star)
{
	int i;

	swap_32( &star->ra);
	swap_32( &star->spd);
	swap_16( &star->mag1);
	swap_16( &star->mag2);
	swap_16( &star->epoch_ra);
	swap_16( &star->epoch_dec);
	swap_32( &star->pm_ra);
	swap_32( &star->pm_dec);
	swap_32( &star->twomass_id);
	swap_16( &star->mag_j);
	swap_16( &star->mag_h);
	swap_16( &star->mag_k);
	for( i = 0; i < 5; i++)
		swap_16( &star->apass_mag[i]);
	swap_32( &star->catalog_flags);
	swap_32( &star->id_number);
	swap_16( &star->ucac2_zone);
	swap_32( &star->ucac2_number);
}
#endif                   // #if __BYTE_ORDER == __BIG_ENDIAN
#endif                   // #ifdef __BYTE_ORDER


#ifdef _WINDOWS
static const char *path_separator = "\\", *read_only_permits = "rb";
#else
static const char *path_separator = "/", *read_only_permits = "r";
#endif


/* The layout of UCAC-4 is such that files for the north (zones 380-900,
corresponding to declinations -14.2 to the north celestial pole) are in
the 'u4n' folder of one DVD.  Those for the south (zones 1-379,
declinations -14.2 and south) are in the 'u4s' folder of the other DVD.
People may copy these retaining the path structure,  or maybe they'll
put all 900 files in one folder.  So if you ask this function for,  say,
zone_number = 314 and files in the folder /data/ucac4,  the function will
look for the data under the following four names:

z314         (i.e.,  all data copied to the current folder)
u4s/z314     (i.e.,  you've copied everything to two subfolders of the current)
/data/ucac4/z314
/data/ucac4/u4s/z314

   ...stopping when it finds a file.  This will,  I hope,  cover all
likely situations.  If you make things any more complicated,  you've
only yourself to blame.   */

static FILE *get_ucac4_zone_file( const int zone_number, const char *path)
{
   FILE *ifile;
   char filename[80];

   sprintf( filename, "u4%c%sz%03d", (zone_number >= 380 ? 'n' : 's'),
                     path_separator, zone_number);
            /* First,  look for file in current path: */
   ifile = fopen( filename + 4, read_only_permits);
   if( !ifile)
      ifile = fopen( filename, read_only_permits);
         /* If file isn't there,  use the 'path' passed in as an argument: */
   if( !ifile && *path)
      {
      char filename2[80], *endptr;
      int i;

      strcpy( filename2, path);
      endptr = filename2 + strlen( filename2);
      if( endptr[-1] != *path_separator)
         *endptr++ = *path_separator;
      for( i = 0; !ifile && i < 2; i++)
         {
         strcpy( endptr, filename + 4 * (1 - i));
         ifile = fopen( filename2, read_only_permits);
         }
      }
   return( ifile);
}

int extract_ucac4_info( const int zone, const long offset, UCAC4_STAR *star,
                     const char *path)
{
   int rval;

   if( zone < 1 || zone > 900)     /* not a valid sequential number */
      rval = -1;
   else
      {
      FILE *ifile = get_ucac4_zone_file( zone, path);

      if( ifile)
         {
         if( fseek( ifile, (offset - 1) * sizeof( UCAC4_STAR), SEEK_SET))
            rval = -2;
         else if( !fread( star, sizeof( UCAC4_STAR), 1, ifile))
            rval = -3;
         else           /* success! */
            {
            rval = 0;
#ifdef __BYTE_ORDER
#if __BYTE_ORDER == __BIG_ENDIAN
            flip_ucac4_star( star);
#endif
#endif
            }
         fclose( ifile);
         }
      else
         rval = -4;
      }
   return( rval);
}

static FILE *get_ucac4_index_file( const char *path)
{
   FILE *index_file;
   const char *idx_filename = "u4index.asc";

                     /* Look for the index file in the local directory... */
   index_file = fopen( idx_filename, read_only_permits);
                     /* ...and if it's not there,  look for it in the same */
                     /* directory as the data: */
   if( !index_file)
      {
      char filename[100];

      strcpy( filename, path);
      if( filename[strlen( filename) - 1] != path_separator[0])
         strcat( filename, path_separator);
      strcat( filename, idx_filename);
      index_file = fopen( filename, read_only_permits);
      }
   return( index_file);
}

/* The layout of the ASCII index is a bit peculiar.  There are 1440
lines per dec zone (of which there are,  of course,  900). Each line
contains 21 bytes,  except for the first,  which includes the dec
and is therefore six bytes longer. */

static long get_index_file_offset( const int zone, const int ra_start)
{
   int rval = (zone - 1) * (1440 * 21 + 6) + ra_start * 21;

   if( ra_start)
      rval += 6;
   return( rval);
}

/* RA, dec, width, height are in degrees */

/* A note on indexing:  within each zone,  we want to locate the stars
within a particular range in RA.  If an index is unavailable,  then
we have things narrowed down to somewhere between the first and
last records.  If an index is available,  our search can take
place within a narrower range.  But in either case,  the range is
refined by doing a secant search which narrows down the starting
point to within 'acceptable_limit' records,  currently set to
40;  i.e.,  it's possible that we will read in forty records that
are before the low end of the desired RA range.  The secant search
is slightly modified to ensure that each iteration knocks off at
least 1/8 of the current range.

*/

int ucac4_search(double ra, const double dec,
		 double width, const double height,
		 void *buf, int sz, char *path)
{
	double dec1 = dec - height / 2., dec2 = dec + height / 2.;
	double ra1 = ra - width / 2., ra2 = ra + width / 2.;
	double zone_height = .2;    /* zones are .2 degrees each */
	int zone = (int)( (dec1  + 90.) / zone_height) + 1;
	int end_zone = (int)( (dec2 + 90.) / zone_height) + 1;
	int index_ra_resolution = 1440;  /* = .25 degrees */
	int ra_start = (int)( ra1 * (double)index_ra_resolution / 360.);
	int rval = 0;
	int buffsize = 400;     /* read this many stars at a try */
	FILE *index_file = NULL;
	struct ucac4_star *starp = buf;

	UCAC4_STAR *stars = (UCAC4_STAR *)calloc( buffsize, sizeof( UCAC4_STAR));
	if( !stars)
		rval = -1;

	if (zone < 1)
		zone = 1;

	if (ra_start < 0)
		ra_start = 0;

	while (rval >= 0 && zone <= end_zone) {
		FILE *ifile = get_ucac4_zone_file( zone, path);

		if (ifile) {
			int keep_going = 1;
			int i, n_read;
			int32_t max_ra  = (int32_t)( ra2 * 3600. * 1000.);
			int32_t min_ra  = (int32_t)( ra1 * 3600. * 1000.);
			int32_t min_spd = (int32_t)( (dec1 + 90.) * 3600. * 1000.);
			int32_t max_spd = (int32_t)( (dec2 + 90.) * 3600. * 1000.);
			uint32_t offset, end_offset;
			uint32_t acceptable_limit = 40;
			long index_file_offset = get_index_file_offset( zone, ra_start);
			static long cached_index_data[5] = {-1L, 0L, 0L, 0L, 0L};
			uint32_t ra_range = (uint32_t)( 360 * 3600 * 1000);
			uint32_t ra_lo = (uint32_t)( ra_start * (ra_range / index_ra_resolution));
			uint32_t ra_hi = ra_lo + ra_range / index_ra_resolution;

			if (index_file_offset == cached_index_data[0]) {
				offset = cached_index_data[1];
				end_offset = cached_index_data[2];
			} else {
				if (!index_file)
					index_file = get_ucac4_index_file(path);

				if (index_file) {
					char ibuff[50];

					fseek(index_file, index_file_offset, SEEK_SET);
					fgets(ibuff, sizeof( ibuff), index_file);
					sscanf( ibuff, "%d%d", &offset, &end_offset);
					end_offset += offset;
					cached_index_data[0] = index_file_offset;
					cached_index_data[1] = offset;
					cached_index_data[2] = end_offset;
				} else {
					/* no index:  binary-search within entire zone: */
					offset = 0;
					fseek( ifile, 0L, SEEK_END);
					end_offset = ftell( ifile) / sizeof( UCAC4_STAR);
					ra_lo = 0;
					ra_hi = ra_range;
				}
			}

			while (end_offset - offset > acceptable_limit) {
				UCAC4_STAR star;
				uint32_t delta = end_offset - offset, toffset;
				uint32_t minimum_bite = delta / 8 + 1;
				uint64_t tval = (uint64_t)delta *
					(uint64_t)( min_ra - ra_lo) / (uint64_t)( ra_hi - ra_lo);

				if (tval < minimum_bite)
					tval = minimum_bite;
				else if (tval > delta - minimum_bite)
					tval = delta - minimum_bite;

				toffset = offset + (uint32_t)tval;
				fseek( ifile, toffset * sizeof( UCAC4_STAR), SEEK_SET);
				fread( &star, 1, sizeof( UCAC4_STAR), ifile);

				if (star.ra < min_ra) {
					offset = toffset;
					ra_lo = star.ra;
				} else {
					end_offset = toffset;
					ra_hi = star.ra;
				}
			}

			fseek (ifile, offset * sizeof( UCAC4_STAR), SEEK_SET);

			while ((n_read = fread( stars, sizeof( UCAC4_STAR), buffsize, ifile)) > 0
			       && keep_going)
				for (i = 0; i < n_read && keep_going; i++) {
					UCAC4_STAR star = stars[i];

#ifdef __BYTE_ORDER
#if __BYTE_ORDER == __BIG_ENDIAN
					flip_ucac4_star( &star);
#endif
#endif
					if (star.ra > max_ra) {
						keep_going = 0;
					} else if (star.ra > min_ra && star.spd > min_spd
						   && star.spd < max_spd) {

						if (sz >= sizeof(struct ucac4_star)) {
							starp->cat  = star;
							starp->zone = zone;
							starp->number = offset + 1; /* 1-based */

							starp++;
							sz -= sizeof(struct ucac4_star);
						}
						rval++;
					}
					offset++;
				}
			fclose (ifile);
		}
		zone++;
	}
	if( index_file)
		fclose( index_file);
	free( stars);

	/* We need some special handling for cases where the area
	   to be extracted crosses RA=0 or RA=24: */
	if (rval >= 0 && ra > 0. && ra < 360.) {

		if( ra1 < 0.) {      /* left side crosses over RA=0h */
			int n = ucac4_search(ra+360., dec, width, height,
					     buf + rval * sizeof(struct ucac4_star),
					     sz - rval * sizeof(struct ucac4_star), path);

			rval += n;
			sz -= n * sizeof(struct ucac4_star);
			if (sz < 0)
				sz = 0;
		}

		if( ra2 > 360.) {
			/* right side crosses over RA=24h */
			rval += ucac4_search(ra-360., dec, width, height,
					     buf + rval * sizeof(struct ucac4_star),
					     sz - rval * sizeof(struct ucac4_star), path);
		}
	}

	return rval;
}
