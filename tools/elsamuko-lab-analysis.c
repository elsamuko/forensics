/*
 * Copyright (C) 1999 Winston Chang
 *                    <winstonc@cs.wisc.edu>
 *                    <winston@stdout.org>
 * Copyright (C) 2010 elsamuko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// CC=g++ LIBS="-lX11" CFLAGS="-O3" gimptool-2.0 --install elsamuko-lab-analysis.c

#include <stdlib.h>
#include <time.h>
#include <vector>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <glib.h>
#include <glib/gprintf.h>

#include "CImg.h"

#define PLUG_IN_PROC    "elsamuko-lab-analysis"
#define PLUG_IN_BINARY  "elsamuko-lab-analysis"

using namespace cimg_library;
using std::vector;

/* local function prototypes */
inline gint coord( gint x, gint y, gint k, gint channels, gint width ) {
    return channels*( width*y + x ) + k;
};

static void      query( void );
static void      run( const gchar      *name,
                      gint              nparams,
                      const GimpParam  *param,
                      gint             *nreturn_vals,
                      GimpParam       **return_vals );
static void      labanalysis( GimpDrawable   *drawable );

/* Setting PLUG_IN_INFO */
const GimpPlugInInfo PLUG_IN_INFO = {
    NULL,  /* init_proc  */
    NULL,  /* quit_proc  */
    query, /* query_proc */
    run,   /* run_proc   */
};

MAIN();

static void
query (void)
{
    static const GimpParamDef args[] =
    {
        { GIMP_PDB_INT32,    (gchar*)"run-mode", (gchar*)"The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
        { GIMP_PDB_IMAGE,    (gchar*)"image",    (gchar*)"Input image (unused)"         },
        { GIMP_PDB_DRAWABLE, (gchar*)"drawable", (gchar*)"Input drawable"               },
    };

    gimp_install_procedure( PLUG_IN_PROC,
                            "LAB Analysis",
                            "LAB Analysis",
                            "elsamuko <elsamuko@web.de>",
                            "elsamuko",
                            "2010",
                            "_LAB Analysis...",
                            "RGB*",
                            GIMP_PLUGIN,
                            G_N_ELEMENTS( args ), 0,
                            args, NULL );

    gimp_plugin_menu_register( PLUG_IN_PROC, "<Image>/Colors" );
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
    GimpDrawable      *drawable;
    GimpRunMode        run_mode;
    GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
    static GimpParam   values[1];

    values[0].type          = GIMP_PDB_STATUS;
    values[0].data.d_status = status;

    *nreturn_vals = 1;
    *return_vals  = values;

    if (strcmp (name, PLUG_IN_PROC) != 0 || nparams < 3)
    {
        values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
        return;
    }

    run_mode = (GimpRunMode)param[0].data.d_int32;
    drawable = gimp_drawable_get (param[2].data.d_drawable);

    /*
     *  Make sure the drawable type is appropriate.
     */
    if (gimp_drawable_is_rgb (drawable->drawable_id) ||
            gimp_drawable_is_gray (drawable->drawable_id))
    {
        gimp_progress_init ("Calculating...");

        labanalysis (drawable);

        if (run_mode != GIMP_RUN_NONINTERACTIVE)
        {
            gimp_displays_flush ();
        }
    }
    else
    {
        status = GIMP_PDB_EXECUTION_ERROR;
    }

    values[0].data.d_status = status;
    gimp_drawable_detach (drawable);
}

static void labanalysis( GimpDrawable *drawable ) {
    printf( "\nL%i:****Begin of labanalysis.****\n", __LINE__ );

    //get drawable properties
    const gint32 image_ID = gimp_drawable_get_image( drawable->drawable_id );
    gimp_image_undo_group_start( image_ID );
    const gint width  = gimp_image_width( image_ID );
    const gint height = gimp_image_height( image_ID );

    // define drawables
    const gint channels = gimp_drawable_bpp( drawable->drawable_id );
    printf( "L%i: Image ID: %i\n",          __LINE__, image_ID );
    printf( "L%i: Number of Channels: %i\n",__LINE__, channels );

    // select region
    GimpPixelRgn region;
    gimp_pixel_rgn_init( &region,
                         drawable,
                         0, 0,
                         width, height,
                         FALSE, FALSE );
    printf( "L%i: Pixel region initiated.\n", __LINE__ );

    // initialise memory
    guchar *rectangle;
    rectangle = g_new( guchar, channels * width * height );
    gimp_pixel_rgn_get_rect( &region,
                             rectangle,
                             0, 0,
                             width, height );

    // algorithm begins here:
    gint x;
    gint y;
    gint c;
    
    CImg<double> cimg(width,height,1,3);
    for ( y = 0; y < height; y++ ) {
        for ( x = 0; x < width; x++ ) {
            for ( c = 0; c < 3; c++ ) {
                cimg(x,y,0,c) = (int)rectangle[coord( x, y, c, channels, width )];
            }
            if(y<1 && x<5) printf("R:%f G:%f B:%f\n",cimg(x,y,0,0),cimg(x,y,0,1),cimg(x,y,0,2));
        }
    }
    printf( "L%i: Convert to Lab\n", __LINE__ );
    cimg.RGBtoLab();

    // sums of values
    vector< vector<gdouble> > AB(1024, vector<gdouble>(1024));

    // frequencies
    vector< vector<gdouble> > AB_f(1024, vector<gdouble>(1024));

    // defaults
    printf( "L%i: Init AB-arrays\n", __LINE__ );
    for ( x = 0; x < 1024; x++ ) {
        for ( y = 0; y < 1024; y++ ) {
            AB[x][y]   = 0.0;
            AB_f[x][y] = 0.0;
        }
    }

    // read all Lab-values and put them in the ab-histogram
    // range from a,b from -128 to 128
    printf( "L%i: Put Lab values into histogram\n", __LINE__ );
    double L,A,B;
    for ( y = 0; y < height; y++ ) {
        for ( x = 0; x < width; x++ ) {
            L = cimg(x,y,0,0);
            A = 4.0*cimg(x,y,0,1)+512;
            B = 4.0*cimg(x,y,0,2)+512;
            if(y<1 && x<5) printf("L:%f A:%f B:%f\n",L,A,B);
            AB[(int)A][(int)B] += L;
            AB_f[(int)A][(int)B]++;
        }
        if ( y % 10 == 0 ) {
            gimp_progress_update(( gdouble )( y / ( gdouble ) gimp_image_height( image_ID ) ) );
        }
    }
    // free memory
    g_free( rectangle );


    // calculate average luminance
    printf( "L%i: Calculate average luminance\n", __LINE__ );
    for ( x = 0; x < 1024; x++ ) {
        for ( y = 0; y < 1024; y++ ) {
            if(AB_f[x][y]) AB[x][y] /= AB_f[x][y];
            if(!AB[x][y])AB[x][y] = -1000;
        }
    }

    // write values into CImg image for back conversion
    printf( "L%i: wite histogram back into CImg image for back conversion\n", __LINE__ );
    CImg<double> cimg_histo(1024,1024,1,3);
    CImg<double> cimg_zero(1024,1024,1,3);
    for ( x = 0; x < 1024; x++ ) {
        for ( y = 0; y < 1024; y++ ) {
            cimg_histo(x,y,0,0) = AB[x][y];
            cimg_histo(x,y,0,1) = x-512;
            cimg_histo(x,y,0,2) = y-512;
        }
    }
    printf( "L%i: Convert back to RGB\n", __LINE__ );
    cimg_histo.LabtoRGB();
    cimg_zero.LabtoRGB();

    // transfer converted CImg histogramm into new GIMP image
    gint32 lab_image = gimp_image_new(1024,1024,GimpImageBaseType(0));
    gimp_display_new(lab_image);
    gint32 lab_layer = gimp_layer_new( lab_image,
                                       "LAB",
                                       1024,
                                       1024,
                                       GIMP_RGB_IMAGE,
                                       100,
                                       GIMP_NORMAL_MODE );
    gimp_image_add_layer( lab_image, lab_layer, 0 );
    GimpDrawable *lab_drawable = gimp_drawable_get( lab_layer );

    // select region
    gimp_pixel_rgn_init( &region,
                         lab_drawable,
                         0, 0,
                         1024, 1024,
                         TRUE, TRUE );
    printf( "L%i: Pixel region initiated.\n", __LINE__ );

    // initialise memory
    rectangle = g_new( guchar, 3 * 1024 * 1024);
    gimp_pixel_rgn_get_rect( &region,
                             rectangle,
                             0, 0,
                             1024, 1024 );

    for ( y = 0; y < 1024; y++ ) {
        for ( x = 0; x < 1024; x++ ) {
            for( c = 0; c < 3; c++ ) {
                rectangle[coord( x, y, c, 3, 1024 )] = cimg_histo(x,y,0,c);
            }
        }
    }
    //save values in array
    gimp_pixel_rgn_set_rect( &region,
                             rectangle,
                             0, 0,
                             1024, 1024);

    //free memory
    g_free( rectangle );

    //gimp_drawable_flush (gimp_drawable_get(depthmap));
    gimp_drawable_merge_shadow( lab_layer, TRUE );
    gimp_drawable_update( lab_layer,
                          0, 0,
                          gimp_image_width( lab_image ),
                          gimp_image_height( lab_image ));

    //end
    gimp_image_undo_group_end( image_ID );
    printf( "L%i:****End of labanalysis.****\n", __LINE__ );
}

