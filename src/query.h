#ifndef _QUERY_H_
#define _QUERY_H_

#define CDS_MIRROR "cds"

enum {
	QUERY_UCAC2,
	QUERY_GSC2,
	QUERY_USNOB,
	QUERY_GSC_ACT,
	QUERY_TYCHO2,
	QUERY_CATALOGS,
};

#define CAT_QUERY_NAMES {"ucac2", "gsc2", "usnob", "gsc-act", "tycho2", NULL}
extern char *query_catalog_names[];

int make_cat_rcp(char *obj, char *catalog, double box, FILE *outf, double mag_limit) ;
void cds_query_cb(gpointer data, guint action, GtkWidget *menu_item);

#endif
