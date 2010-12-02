/* telescope control stuff */

#ifdef _SCOPE_H_
#else

enum scope_states {
	SCOPE_CLOSED,
	SCOPE_IDLE,
	SCOPE_ABORTED,
	SCOPE_ERR,
	SCOPE_SLEW_START,
	SCOPE_SLEW_GET_DEC,
	SCOPE_SLEW_WAIT_END,
	SCOPE_STABILIZE,
	SCOPE_GEAR_PLAY1,
	SCOPE_GEAR_PLAY2,
	SCOPE_CENTER1,
	SCOPE_CENTER2,
};

struct scope {
	int state;		/* state of telescope */
	int fd;			/* descriptor used to talk to the telescope */
	char *name;		/* name of connection (serial port) */
	struct timeval op_tv;	/* timer set at beginning of last operation */
	struct timeval tv;	/* general purpose timer */
	int errcount;		/* error counter */
	double last_epoch;	/* target coordinates as set by the caller */
	double last_ra;
	double last_dec;
	double target_ra;	/* target coordinates as sent to the scope */
	double target_dec;	/* (precessed to the epoch of the day) */
	double ra;		/* coordinates last reported by the telescope */
	double dec;
	int abort;		/* flag requesting an abort of the current operation */
	double last_dra;	/* internal persistent deltas */
	double last_ddec;
	int waitms;		/* amount we have to wait in the current state (ms) */
	int dec_time;		/* moving times in dec/ra (ms) */
	int ra_time;
	int dec_reversal;	/* indicates a move north actually decreases the declination */
	int ra_move_active;
	int dec_move_active;
};

int scope_sync_coords(struct scope *scope, double ra, double dec, double epoch);
int scope_timed_center(struct scope *scope, double dra, double ddec);
int scope_slew(struct scope *scope);
int scope_set_object(struct scope *scope, double ra, double dec, double epoch, char *name);
void scope_close(struct scope *scope);
struct scope * scope_open(char *name);
void scope_abort(struct scope *scope);
void scope_status_string(struct scope *scope, char *buf, int size);










#endif
