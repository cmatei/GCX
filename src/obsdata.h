#ifndef _OBSDATA_H_
#define _OBSDATA_H_


#define mjd_to_jd(x) ((x) + 2400000.5)
#define jd_to_mjd(x) ((x) - 2400000.5)

/* data that we need to adnotate an observation */
struct obs_data {
	int ref_count;
	char *objname; /* malloced designation of object */
	double ra; /* coordinates */
	double dec;
	double rot; /* rotation EofN, in degrees */
	double equinox;
	double airmass;
	double mjd;
	char *filter; /* malloced filter name */
	char *sequence ; 	/* source of the sequence used to reduce the frame */
	char *comment;

	double tele_ra;
	double tele_dec;
};

#define OBS_DATA(x) ((struct obs_data *)(x))


int obs_set_from_object(struct obs_data *obs, char *name);
void obs_data_from_par(struct obs_data *obs);
struct obs_data * obs_data_new(void);
void obs_data_ref(struct obs_data *obs);
void obs_data_release(struct obs_data *obs);
void replace_strval(char **str, char *val);
void ccd_frame_add_obs_info(struct ccd_frame *fr, struct obs_data *obs);
void rescan_fits_exp(struct ccd_frame *fr, struct exp_data *exp);
void rescan_fits_wcs(struct ccd_frame *fr, struct wcs *fim);
double frame_airmass(struct ccd_frame *fr, double ra, double dec) ;
double obs_current_hour_angle(struct obs_data *obs);
double obs_current_airmass(struct obs_data *obs);
void noise_to_fits_header(struct ccd_frame *fr, struct exp_data *exp);
char *extract_catname(char *text, char **oname);
struct cat_star *get_object_by_name(char *name);
void wcs_to_fits_header(struct ccd_frame *fr);
void jdate_to_timeval(double jd, struct timeval *tv);
double timeval_to_jdate(struct timeval *tv);
double frame_jdate(struct ccd_frame *fr);
void date_time_from_jdate(double jd, char *date, int n);
double calculate_airmass(double ra, double dec, double jd, double lat, double lng);


#endif
