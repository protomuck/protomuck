
/* Primitives Package */

#include "copyright.h"
#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include "db.h"
#include "tune.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "strings.h"
#include "interp.h"

static struct inst *oper1, *oper2, *oper3, *oper4;
static int result;
static float fresult;
static char buf[BUFFER_LEN];

#define INF (9.9E999)
#define NINF (-9.9E999)

int
no_good(float test)
{
	return (((test == INF) || (test == NINF)));
}

void
prim_inf(PRIM_PROTOTYPE)
{
	CHECKOP(0);
	fresult = INF;
	CHECKOFLOW(1);
	PushFloat(fresult);
}

void
prim_ceil(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (!no_good(oper1->data.fnumber)) {
		fresult = (float) ceil((double) oper1->data.fnumber);
	} else {
		fresult = 0.0;
		fr->error.error_flags.f_bounds = 1;
	}
	CLEAR(oper1);
	PushFloat(fresult);
}

void
prim_floor(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (!no_good(oper1->data.fnumber)) {
		fresult = (float) floor((double) oper1->data.fnumber);
	} else {
		fresult = 0.0;
		fr->error.error_flags.f_bounds = 1;
	}
	CLEAR(oper1);
	PushFloat(fresult);
}

void
prim_sqrt(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
       if ( oper1->type == PROG_INTEGER ) 
          { oper1->type = PROG_FLOAT;
            oper1->data.fnumber = oper1->data.number;
          }
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (!no_good(oper1->data.fnumber)) {
		if (oper1->data.fnumber < 0.0) {
			fresult = 0.0;
			fr->error.error_flags.imaginary = 1;
		} else {
			fresult = (float) sqrt((double) oper1->data.fnumber);
		}
	} else {
		fresult = 0.0;
		fr->error.error_flags.f_bounds = 1;
	}
	CLEAR(oper1);
	PushFloat(fresult);
}

#define F_PI 3.14159265359
#define NF_PI -3.14159265359
#define H_PI 1.5707963268
#define NH_PI -1.5707963268
#define Q_PI 0.7853981634
#define NQ_PI -0.7853981634

void
prim_pi(PRIM_PROTOTYPE)
{
	CHECKOP(0);
	fresult = F_PI;
	CHECKOFLOW(1);
	PushFloat(fresult);
}

void
prim_round(PRIM_PROTOTYPE)
{
	double temp, tshift, tnum, fstore;

	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument. (2)");
	if (oper2->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (oper1->type < 0)
		abort_interp("Precision argument must be a positive integer. (2)");
	if (!no_good(oper2->data.fnumber)) {
		temp = pow(10.0, (double) oper1->data.number);
		tshift = temp * ((double) oper2->data.fnumber);
		tnum = modf(tshift, &fstore);
		if (tnum >= 0.5) {
			fstore = fstore + 1.0;
		} else {
			if (tnum <= -0.5) {
				fstore = fstore - 1.0;
			}
		}
		fstore = fstore / temp;
		fresult = (float) fstore;
	} else {
		fresult = 0.0;
		fr->error.error_flags.f_bounds = 1;
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushFloat(fresult);
}

void
prim_sin(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
       if ( oper1->type == PROG_INTEGER ) 
          { oper1->type = PROG_FLOAT;
            oper1->data.fnumber = oper1->data.number;
          }
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (!no_good(oper1->data.fnumber)) {
		fresult = (float) sin((double) oper1->data.fnumber);
	} else {
		fresult = 0.0;
		fr->error.error_flags.f_bounds = 1;
	}
	CLEAR(oper1);
	PushFloat(fresult);
}

void
prim_cos(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
       if ( oper1->type == PROG_INTEGER ) 
          { oper1->type = PROG_FLOAT;
            oper1->data.fnumber = oper1->data.number;
          }
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (!no_good(oper1->data.fnumber)) {
		fresult = (float) cos((double) oper1->data.fnumber);
	} else {
		fresult = 0.0;
		fr->error.error_flags.f_bounds = 1;
	}
	CLEAR(oper1);
	PushFloat(fresult);
}

void
prim_tan(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
       if ( oper1->type == PROG_INTEGER ) 
          { oper1->type = PROG_FLOAT;
            oper1->data.fnumber = oper1->data.number;
          }
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (!no_good(oper1->data.fnumber)) {
		fresult = fmod((oper1->data.fnumber - H_PI), F_PI);
		if (fresult < 0.000001 || fresult > (F_PI - 0.000001)) {
			fresult = (float) tan((double) oper1->data.fnumber);
		} else {
			fresult = 0.0;
			fr->error.error_flags.nan = 1;
		}
	} else {
		fresult = 0.0;
		fr->error.error_flags.f_bounds = 1;
	}
	CLEAR(oper1);
	PushFloat(fresult);
}

void
prim_asin(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
       if ( oper1->type == PROG_INTEGER ) 
          { oper1->type = PROG_FLOAT;
            oper1->data.fnumber = oper1->data.number;
          }
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (!((oper1->data.fnumber < -1.0) || (oper1->data.fnumber > 1.0))) {
		fresult = (float) asin((double) oper1->data.fnumber);
	} else {
		fresult = 0.0;
		fr->error.error_flags.nan = 1;
	}
	CLEAR(oper1);
	PushFloat(fresult);
}

void
prim_acos(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
       if ( oper1->type == PROG_INTEGER ) 
          { oper1->type = PROG_FLOAT;
            oper1->data.fnumber = oper1->data.number;
          }
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (!((oper1->data.fnumber < -1.0) || (oper1->data.fnumber > 1.0))) {
		fresult = (float) acos((double) oper1->data.fnumber);
	} else {
		fresult = 0.0;
		fr->error.error_flags.nan = 1;
	}
	CLEAR(oper1);
	PushFloat(fresult);
}

void
prim_atan(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
       if ( oper1->type == PROG_INTEGER ) 
          { oper1->type = PROG_FLOAT;
            oper1->data.fnumber = oper1->data.number;
          }
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (!no_good(oper1->data.fnumber)) {
		fresult = (float) atan((double) oper1->data.fnumber);
	} else {
		fresult = H_PI;
	}
	CLEAR(oper1);
	PushFloat(fresult);
}

void
prim_atan2(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper2 = POP();
	oper1 = POP();
       if ( oper1->type == PROG_INTEGER ) 
          { oper1->type = PROG_FLOAT;
            oper1->data.fnumber = oper1->data.number;
          }
       if ( oper2->type == PROG_INTEGER ) 
          { oper2->type = PROG_FLOAT;
            oper2->data.fnumber = oper1->data.number;
          }
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (oper2->type != PROG_FLOAT)
		abort_interp("Non-float argument. (2)");
	fresult = (float) atan2((double) oper1->data.fnumber, (double) oper2->data.fnumber);
	CLEAR(oper1);
	CLEAR(oper2);
	PushFloat(fresult);
}

void
prim_dist3d(PRIM_PROTOTYPE)
{
	float dist;
	double x, y, z;

	CHECKOP(3);
	oper3 = POP();
	oper2 = POP();
	oper1 = POP();
       if ( oper1->type == PROG_INTEGER ) 
          { oper1->type = PROG_FLOAT;
            oper1->data.fnumber = oper1->data.number;
          }
       if ( oper2->type == PROG_INTEGER ) 
          { oper2->type = PROG_FLOAT;
            oper2->data.fnumber = oper1->data.number;
          }
       if ( oper3->type == PROG_INTEGER ) 
          { oper3->type = PROG_FLOAT;
            oper3->data.fnumber = oper1->data.number;
          }
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (oper2->type != PROG_FLOAT)
		abort_interp("Non-float argument. (2)");
	if (oper3->type != PROG_FLOAT)
		abort_interp("Non-float argument. (3)");

	x = oper1->data.fnumber;
	y = oper2->data.fnumber;
	z = oper3->data.fnumber;
	dist = (float) sqrt((x * x) + (y * y) + (z * z));

	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
	PushFloat(dist);
}


void
prim_xyz_to_polar(PRIM_PROTOTYPE)
{
	float dist, theta, phi;
	double x, y, z;

	CHECKOP(3);
	oper3 = POP();
	oper2 = POP();
	oper1 = POP();
       if ( oper1->type == PROG_INTEGER ) 
          { oper1->type = PROG_FLOAT;
            oper1->data.fnumber = oper1->data.number;
          }
       if ( oper2->type == PROG_INTEGER ) 
          { oper2->type = PROG_FLOAT;
            oper2->data.fnumber = oper1->data.number;
          }
       if ( oper3->type == PROG_INTEGER ) 
          { oper3->type = PROG_FLOAT;
            oper3->data.fnumber = oper1->data.number;
          }
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (oper2->type != PROG_FLOAT)
		abort_interp("Non-float argument. (2)");
	if (oper3->type != PROG_FLOAT)
		abort_interp("Non-float argument. (3)");

	x = oper1->data.fnumber;
	y = oper2->data.fnumber;
	z = oper3->data.fnumber;

	if (no_good(x) || no_good(y) || no_good(z)) {
		dist = 0.0;
		theta = 0.0;
		phi = 0.0;
		fr->error.error_flags.nan = 1;
	} else {
		dist = (float) sqrt((x * x) + (y * y) + (z * z));
		if (dist > 0.0) {
			theta = (float) atan2(y, x);
			phi = (float) acos(z / dist);
		} else {
			theta = 0.0;
			phi = 0.0;
		}
	}

	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
	PushFloat(dist);
	PushFloat(theta);
	PushFloat(phi);
}


void
prim_polar_to_xyz(PRIM_PROTOTYPE)
{
	float x, y, z;
	double dist, theta, phi;

	CHECKOP(3);
	oper3 = POP();
	oper2 = POP();
	oper1 = POP();
       if ( oper1->type == PROG_INTEGER ) 
          { oper1->type = PROG_FLOAT;
            oper1->data.fnumber = oper1->data.number;
          }
       if ( oper2->type == PROG_INTEGER ) 
          { oper2->type = PROG_FLOAT;
            oper2->data.fnumber = oper1->data.number;
          }
       if ( oper3->type == PROG_INTEGER ) 
          { oper3->type = PROG_FLOAT;
            oper3->data.fnumber = oper1->data.number;
          }
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (oper2->type != PROG_FLOAT)
		abort_interp("Non-float argument. (2)");
	if (oper3->type != PROG_FLOAT)
		abort_interp("Non-float argument. (3)");

	dist = oper1->data.fnumber;
	theta = oper2->data.fnumber;
	phi = oper3->data.fnumber;

	if (no_good(dist) || no_good(theta) || no_good(phi)) {
		x = 0.0;
		y = 0.0;
		z = 0.0;
		fr->error.error_flags.nan = 1;
	} else {
		x = (float) (dist * cos(theta) * sin(phi));
		y = (float) (dist * sin(theta) * sin(phi));
		z = (float) (dist * cos(phi));
	}

	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
	PushFloat(x);
	PushFloat(y);
	PushFloat(z);
}

void
prim_exp(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
       if ( oper1->type == PROG_INTEGER ) 
          { oper1->type = PROG_FLOAT;
            oper1->data.fnumber = oper1->data.number;
          }
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (!no_good(oper1->data.fnumber)) {
		fresult = (float) exp((double) oper1->data.fnumber);
	} else {
		fresult = 0.0;
		fr->error.error_flags.f_bounds = 1;
	}
	CLEAR(oper1);
	PushFloat(fresult);
}

void
prim_log(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
       if ( oper1->type == PROG_INTEGER ) 
          { oper1->type = PROG_FLOAT;
            oper1->data.fnumber = oper1->data.number;
          }
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (!no_good(oper1->data.fnumber) && oper1->data.fnumber > 0.0) {
		fresult = (float) log((double) oper1->data.fnumber);
	} else {
		fresult = 0.0;
		if (oper1->data.fnumber > 0.0)
			fr->error.error_flags.f_bounds = 1;
		else
			fr->error.error_flags.imaginary = 1;
	}
	CLEAR(oper1);
	PushFloat(fresult);
}

void
prim_log10(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
       if ( oper1->type == PROG_INTEGER ) 
          { oper1->type = PROG_FLOAT;
            oper1->data.fnumber = oper1->data.number;
          }
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (!no_good(oper1->data.fnumber) && oper1->data.fnumber > 0.0) {
		fresult = (float) log10((double) oper1->data.fnumber);
	} else {
		fresult = 0.0;
		if (oper1->data.fnumber > 0.0)
			fr->error.error_flags.f_bounds = 1;
		else
			fr->error.error_flags.imaginary = 1;
	}
	CLEAR(oper1);
	PushFloat(fresult);
}

void
prim_fabs(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
       if ( oper1->type == PROG_INTEGER ) 
          { oper1->type = PROG_FLOAT;
            oper1->data.fnumber = oper1->data.number;
          }
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (!no_good(oper1->data.fnumber)) {
		fresult = (float) fabs((double) oper1->data.fnumber);
	} else {
		fresult = 0.0;
		fr->error.error_flags.f_bounds = 1;
	}
	CLEAR(oper1);
	PushFloat(fresult);
}


void
prim_float(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument. (1)");
	fresult = (float) oper1->data.number;
	CLEAR(oper1);
	PushFloat(fresult);
}

void
prim_pow(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
       if ( oper1->type == PROG_INTEGER ) 
          { oper1->type = PROG_FLOAT;
            oper1->data.fnumber = oper1->data.number;
          }
       if ( oper2->type == PROG_INTEGER ) 
          { oper2->type = PROG_FLOAT;
            oper2->data.fnumber = oper1->data.number;
          }
	if (oper2->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (2)");
	if (!no_good(oper1->data.fnumber) && !no_good(oper2->data.fnumber)) {
		if (((oper2->data.fnumber < SMALL_NUM && oper2->data.fnumber > NSMALL_NUM) &&
			 (oper1->data.fnumber < SMALL_NUM))
			||
			((oper2->data.fnumber < SMALL_NUM) &&
			 (oper1->data.fnumber != (float) ((int) oper1->data.fnumber)))) {
			fresult = 0.0;
			fr->error.error_flags.f_bounds = 1;
		} else {
			fresult = (float)
					pow((double) oper2->data.fnumber, (double) oper1->data.fnumber);
		}
	} else {
		fresult = 0.0;
		fr->error.error_flags.f_bounds = 1;
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushFloat(fresult);
}

void
prim_frand(PRIM_PROTOTYPE)
{
	int tresult;

	CHECKOP(0);
	result = rand();
	tresult = rand();
	if ((result < tresult) && (result != tresult)) {
		fresult = (float) result / (float) tresult;
	} else {
		if (result != 0) {
			fresult = (float) tresult / (float) result;
		} else {
			fresult = 0.0;
			/* 0 is what we want here, no error.error */
		}
	}
	CHECKOFLOW(1);
	PushFloat(fresult);
}

void
prim_fmod(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
       if ( oper1->type == PROG_INTEGER ) 
          { oper1->type = PROG_FLOAT;
            oper1->data.fnumber = oper1->data.number;
          }
       if ( oper2->type == PROG_INTEGER ) 
          { oper2->type = PROG_FLOAT;
            oper2->data.fnumber = oper1->data.number;
          }
	if (oper2->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (2)");
	if (oper1->data.fnumber < SMALL_NUM && oper1->data.fnumber > NSMALL_NUM) {
		fresult = 0.0;
		fr->error.error_flags.div_zero = 1;
	} else {
		fresult = oper2->data.fnumber / oper1->data.fnumber;
		fresult = fresult - (float) ((int) fresult);
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushFloat(fresult);
}

void
prim_modf(PRIM_PROTOTYPE)
{
	float tresult;
	double dresult;

	CHECKOP(1);
	oper1 = POP();
       if ( oper1->type == PROG_INTEGER ) 
          { oper1->type = PROG_FLOAT;
            oper1->data.fnumber = oper1->data.number;
          }
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	if (!no_good(oper1->data.fnumber)) {
		fresult = (float) modf((double) oper1->data.fnumber, &dresult);
	} else {
		fresult = 0.0;
		tresult = 0.0;
		fr->error.error_flags.f_bounds = 1;
	}
	CLEAR(oper1);
	tresult = (float) dresult;
	CHECKOFLOW(2);
	PushFloat(tresult);
	PushFloat(fresult);
}

void
prim_strtof(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (oper1->type != PROG_STRING)
		abort_interp("Non-string argument. (1)");
	fresult = 0.0;
	if (!oper1->data.string || !ifloat(oper1->data.string->data)) {
		fresult = 0.0;
		fr->error.error_flags.nan = 1;
	} else {
		sscanf(oper1->data.string->data, "%g", &fresult);
	}
	CLEAR(oper1);
	PushFloat(fresult);
}

void
prim_ftostr(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
       if ( oper1->type == PROG_INTEGER ) 
          { oper1->type = PROG_FLOAT;
            oper1->data.fnumber = oper1->data.number;
          }
	if (oper1->type != PROG_FLOAT)
		abort_interp("Non-float argument. (1)");
	sprintf(buf, "%#g", oper1->data.fnumber);
	CLEAR(oper1);
	PushString(buf);
}

