/*****************************************************************************
CORDIC demo
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: Dec 29, 1999
This code is public domain (no copyright).
You can do whatever you want with it.
*****************************************************************************/
#include <stdio.h>
#include <math.h>

#define AG_CONST	0.6072529350
#define	FIXED(X)	((long int)((X) * 65536.0))
#define	FLOAT(X)	((X) / 65536.0)
#define	DEG2RAD(X)	0.017453 * (X)

typedef long int	fixed; /* 16.16 fixed-point */

static const fixed Angles[]={
	FIXED(45.0),	FIXED(26.565),	FIXED(14.0362),
	FIXED(7.12502),	FIXED(3.57633),	FIXED(1.78991),
	FIXED(0.895174),FIXED(0.447614),FIXED(0.223811),
	FIXED(0.111906),FIXED(0.055953),FIXED(0.027977) };

int main(void)
{	fixed X, Y, TargetAngle, CurrAngle;
	unsigned Step;

	X=FIXED(AG_CONST);  /* AG_CONST * cos(0) */
	Y=0;		    /* AG_CONST * sin(0) */
	TargetAngle=FIXED(28.027);
	CurrAngle=0;
	for(Step=0; Step < 12; Step++)
	{	fixed NewX;

		if(TargetAngle > CurrAngle)
		{	NewX=X - (Y >> Step);
			Y=(X >> Step) + Y;
			X=NewX;
			CurrAngle += Angles[Step]; }
		else
		{	NewX=X + (Y >> Step);
			Y=-(X >> Step) + Y;
			X=NewX;
			CurrAngle -= Angles[Step]; }}
	printf("CORDIC: sin(%7.5f)=%7.5f, cos(%7.5f)=%7.5f\n",
		FLOAT(TargetAngle), FLOAT(Y),
		FLOAT(TargetAngle), FLOAT(X));
	printf("    FP: sin(%7.5f)=%7.5f, cos(%7.5f)=%7.5f\n",
		FLOAT(TargetAngle), sin(DEG2RAD(FLOAT(TargetAngle))),
		FLOAT(TargetAngle), cos(DEG2RAD(FLOAT(TargetAngle))));
	return(0); }
