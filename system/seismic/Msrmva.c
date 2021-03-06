/* 3-D S/R WEMVA with extended split-step */
/*
  Copyright (C) 2006 Colorado School of Mines
  Copyright (C) 2004 University of Texas at Austin

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <rsf.h>
#include "srmva.h"
#include "img2.h"

int main (int argc, char *argv[])
{
    bool adj;             /* forward or adjoint */
    bool twoway;          /* two-way traveltime */
    bool verb;            /* verbosity */
    float eps;            /* dip filter constant */  
    int   nrmax;          /* number of reference velocities */
    float dtmax;          /* time error */
    int   pmx,pmy;        /* padding in the k domain */
    int   tmx,tmy;        /* boundary taper size */

    sf_axis amx,amy,amz;
    sf_axis alx,aly;
    sf_axis aw,ae;

    int n,nz,nw;

    /* I/O files */
    sf_file Bw_s=NULL,Bw_r=NULL; /* wavefield file W ( nx, ny,nw) */
    sf_file Bs=NULL;
    sf_file Ps=NULL;             /*  slowness file S (nlx,nly,nz) */
    sf_file Pi=NULL;             /*     image file R ( nx, ny,nz) */

    /* I/O slices */
    sf_fslice Bwfls=NULL,Bwflr=NULL;
    sf_fslice Bslow=NULL;
    sf_fslice Pslow=NULL;
    sf_fslice Pimag=NULL;

    /*------------------------------------------------------------*/
    sf_init(argc,argv);

    if (!sf_getbool(  "verb",&verb ))  verb =  true; /* verbosity flag */
    if (!sf_getfloat(  "eps",&eps  ))   eps =  0.01; /* stability parameter */
    if (!sf_getbool(   "adj",&adj  ))   adj = false; /* y=ADJ scat; n=FWD scat */
    if (!sf_getbool("twoway",&twoway))twoway=  true; /* two-way traveltime */
    if (!sf_getint(  "nrmax",&nrmax)) nrmax =     1; /* max number of refs */
    if (!sf_getfloat("dtmax",&dtmax)) dtmax = 0.004; /* max time error */
    if (!sf_getint(    "pmx",&pmx  ))   pmx =     0; /* padding on x */
    if (!sf_getint(    "pmy",&pmy  ))   pmy =     0; /* padding on y */
    if (!sf_getint(    "tmx",&tmx  ))   tmx =     0; /* taper on x   */
    if (!sf_getint(    "tmy",&tmy  ))   tmy =     0; /* taper on y   */


    /*------------------------------------------------------------*/
    /* SLOWNESS */

    Bs = sf_input("slo");
    alx = sf_iaxa(Bs,1); sf_setlabel(alx,"lx");
    aly = sf_iaxa(Bs,2); sf_setlabel(aly,"ly");
    amz = sf_iaxa(Bs,3); sf_setlabel(amz,"mz");

    n = sf_n(alx)*sf_n(aly);
    nz = sf_n(amz);

    Bslow = sf_fslice_init(n, nz, sizeof(float));
    sf_fslice_load(Bs,Bslow,SF_FLOAT);

    /*------------------------------------------------------------*/
    /* WAVEFIELD */

    Bw_s = sf_input("swf");
    Bw_r = sf_input("rwf");

    if (SF_COMPLEX != sf_gettype(Bw_s)) sf_error("Need complex   source data");
    if (SF_COMPLEX != sf_gettype(Bw_r)) sf_error("Need complex receiver data");

    amx = sf_iaxa(Bw_s,1); sf_setlabel(amx,"mx");
    amy = sf_iaxa(Bw_s,2); sf_setlabel(amy,"my");
    amz = sf_iaxa(Bw_s,3); sf_setlabel(amz,"mz");
    aw  = sf_iaxa(Bw_s,4); sf_setlabel(aw, "w" );
    ae  = sf_iaxa(Bw_s,5); sf_setlabel(ae, "e" );

    n = sf_n(amx)*sf_n(amy);
    nw = sf_n(aw);

    Bwfls = sf_fslice_init( n,nz*nw,sizeof(sf_complex));
    Bwflr = sf_fslice_init( n,nz*nw,sizeof(sf_complex));

    sf_fslice_load(Bw_s,Bwfls,SF_COMPLEX);
    sf_fslice_load(Bw_r,Bwflr,SF_COMPLEX);

    /*------------------------------------------------------------*/

    if(adj) {
	Pi = sf_input ( "in");
	if (SF_COMPLEX !=sf_gettype(Pi)) 
	    sf_error("Need complex image perturbation");

	Ps = sf_output("out"); sf_settype(Ps,SF_COMPLEX);
	sf_oaxa(Ps,amx,1);
	sf_oaxa(Ps,amy,2);
	sf_oaxa(Ps,amz,3);

	Pslow = sf_fslice_init(n,nz, sizeof(sf_complex));

	Pimag = sf_fslice_init(n,nz, sizeof(sf_complex));
	sf_fslice_load(Pi,Pimag,SF_COMPLEX);
    } else {
	
	Ps = sf_input("in");
	if (SF_COMPLEX !=sf_gettype(Ps)) 
	    sf_error("Need complex slowness perturbation");
	
	Pi = sf_output("out"); sf_settype(Pi,SF_COMPLEX);
	sf_oaxa(Pi,amx,1);
	sf_oaxa(Pi,amy,2);
	sf_oaxa(Pi,amz,3);
	
	Pslow = sf_fslice_init(n,nz, sizeof(sf_complex));
	sf_fslice_load(Ps,Pslow,SF_COMPLEX);
	
	Pimag = sf_fslice_init(n,nz, sizeof(sf_complex));
    }

    /*------------------------------------------------------------*/

    srmva_init (verb,eps,twoway,dtmax,
		aw,
		amx,amy,amz,
		alx,aly,
		tmx,tmy,
		pmx,pmy,
		nrmax,Bslow,Bwfls,Bwflr);
    
/*    srmva_aloc();*/
    srmva(adj,Pslow,Pimag);
/*    srmva_free();*/

    srmva_close();

    /*------------------------------------------------------------*/

    if(adj) sf_fslice_dump(Ps,Pslow,SF_COMPLEX);
    else    sf_fslice_dump(Pi,Pimag,SF_COMPLEX);
    sf_fslice_close(Pimag);
    sf_fslice_close(Pslow);

    sf_fslice_close(Bwfls);
    sf_fslice_close(Bwflr);
    sf_fslice_close(Bslow);

    exit (0);
}
