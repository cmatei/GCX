
#include "gsc.h"

static float ra = 0.0, dec = 0.0, radius = 30.0, maglimit = 12.0, dummy = 0.0;
static int maxobjects = 200;

static int get_integer(int argc, char **argv, int idx, int *ret)
{
	if (idx >= argc)
		return -1;

	*ret = atoi(argv[idx]);
	return 0;
}

static float get_float(int argc, char **argv, int idx, float *ret)
{
	if (idx >= argc)
		return -1;

	*ret = atof(argv[idx]);
	return 0;
}

static int get_options(int argc, char **argv)
{
	int i;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-c")) {
			if (get_float(argc, argv, i + 1, &ra))
				return -1;

			if (get_float(argc, argv, i + 2, &dec))
				return -1;

			i += 2;
			continue;
		}

		if (!strcmp(argv[i], "-r")) {
			if (get_float(argc, argv, i + 1, &radius))
				return -1;

			i += 1;
			continue;
		}

		if (!strcmp(argv[i], "-n")) {
			if (get_integer(argc, argv, i + 1, &maxobjects))
				return -1;

			i += 1;
			continue;
		}

		if (!strcmp(argv[i], "-m")) {
			if (get_float(argc, argv, i + 1, &dummy))
				return -1;

			if (get_float(argc, argv, i + 2, &maglimit))
				return -1;

			i += 2;
			continue;
		}

		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	int i, ret;
	int *regs, *ids;
	float *ras, *decs, *mags;

	if (get_options(argc, argv))
		exit(1);

	regs = calloc(sizeof(int), maxobjects);
	ids  = calloc(sizeof(int), maxobjects);

	ras  = calloc(sizeof(float), maxobjects);
	decs = calloc(sizeof(float), maxobjects);
	mags = calloc(sizeof(float), maxobjects);

	ret = getgsc(ra, dec, radius, maglimit,
		     regs, ids, ras, decs, mags,
		     maxobjects - 1,
		     "/usr/share/gcx/catalogs/gsc/");

	//fprintf(stderr, "Found %d recs at radius %.f  around ra %f, dec %f\n", ret, radius, ra, dec);

	for (i = 0; i < ret; i++) {

		/*
		sscanf(line,"%10s %f %f %f %f %f %d %d %4s %2s %f %d",
                            id,&ra,&dec,&pose,&mag,&mage,&band,&c,plate,ob,&dist,&dir);
		*/

		printf("%05d%05d %f %f %f %f %f %d %d %4s %2s %f %d\n",
		       regs[i], ids[i], // id
		       ras[i], decs[i],
		       0.0, // pose
		       mags[i], mags[i],
		       0, 0, //band, c
		       "PLAT", "00",
		       0.0, 1);


	}


	return 0;
}
