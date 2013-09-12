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

#include <stdlib.h>
#include <time.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <glib.h>
#include <glib/gprintf.h>

#define PLUG_IN_PROC    "elsamuko-hsv-analysis"
#define PLUG_IN_BINARY  "elsamuko-hsv-analysis"

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
static void      hsvanalysis( GimpDrawable   *drawable );

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
        { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
        { GIMP_PDB_IMAGE,    "image",    "Input image (unused)"         },
        { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"               },
    };

    gimp_install_procedure( PLUG_IN_PROC,
                            "HSV Analysis",
                            "HSV Analysis",
                            "elsamuko <elsamuko@web.de>",
                            "elsamuko",
                            "2010",
                            "_HSV Analysis...",
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

    run_mode = param[0].data.d_int32;
    drawable = gimp_drawable_get (param[2].data.d_drawable);

    /*
     *  Make sure the drawable type is appropriate.
     */
    if (gimp_drawable_is_rgb (drawable->drawable_id) ||
            gimp_drawable_is_gray (drawable->drawable_id))
    {
        gimp_progress_init ("Calculating...");

        hsvanalysis (drawable);

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

static void hsvanalysis( GimpDrawable *drawable ) {
    printf( "\nL%i:****Begin of hsvanalysis.****\n", __LINE__ );

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

    // sums of values
    gdouble HS[256][256];

    // frequencies
    gdouble HS_f[256][256];

    // defaults
    printf( "L%i: Init HS-arrays\n", __LINE__ );
    for ( x = 0; x < 256; x++ ) {
        for ( y = 0; y < 256; y++ ) {
            HS[x][y]   = 0.0;
            HS_f[x][y] = 0.0;
        }
    }

    //read all rgb-values and put them in the hs-histogram
    GimpHSV hsv;
    GimpRGB rgb;
    double one255th = 1.0/255.0;
    for ( y = 0; y < height; y++ ) {
        for ( x = 0; x < width; x++ ) {
            gimp_rgb_set(&rgb,one255th*rectangle[coord( x, y, 0, channels, width )],
                         one255th*rectangle[coord( x, y, 1, channels, width )],
                         one255th*rectangle[coord( x, y, 2, channels, width )]);
            gimp_rgb_to_hsv(&rgb,&hsv);
            HS[(int)(255*hsv.h)][(int)(255*hsv.s)] += hsv.v;
            HS_f[(int)(255*hsv.h)][(int)(255*hsv.s)]++;
        }

        if ( y % 10 == 0 ) {
            gimp_progress_update(( gdouble )( y / ( gdouble ) gimp_image_height( image_ID ) ) );
        }
    }
    //free memory
    g_free( rectangle );


    //calculate average value
    for ( x = 0; x < 256; x++ ) {
        for ( y = 0; y < 256; y++ ) {
            if(HS_f[x][y]) HS[x][y] /= HS_f[x][y];
            if(HS[x][y]>1.0) HS[x][y] = 1.0;
        }
    }

    //paint averaged values in new image
    gint32 hsv_image = gimp_image_new(256,256,0);
    gimp_display_new(hsv_image);
    gint32 hsv_layer = gimp_layer_new( hsv_image,
                                       "HSV",
                                       256,
                                       256,
                                       GIMP_RGB_IMAGE,
                                       100,
                                       GIMP_NORMAL_MODE );
    gimp_image_add_layer( hsv_image, hsv_layer, 0 );
    GimpDrawable *hsv_drawable = gimp_drawable_get( hsv_layer );

    // select region
    gimp_pixel_rgn_init( &region,
                         hsv_drawable,
                         0, 0,
                         256, 256,
                         TRUE, TRUE );
    printf( "L%i: Pixel region initiated.\n", __LINE__ );

    // initialise memory
    rectangle = g_new( guchar, 3 * 256 * 256);
    gimp_pixel_rgn_get_rect( &region,
                             rectangle,
                             0, 0,
                             256, 256 );

    for ( y = 0; y < 256; y++ ) {
        for ( x = 0; x < 256; x++ ) {
            hsv.h = one255th * y;
            hsv.s = one255th * x;
            hsv.v = HS[y][x];
            gimp_hsv_to_rgb(&hsv,&rgb);
            if(HS_f[y][x]) {
                rectangle[coord( x, y, 0, 3, 256 )] = 255*rgb.r;
                rectangle[coord( x, y, 1, 3, 256 )] = 255*rgb.g;
                rectangle[coord( x, y, 2, 3, 256 )] = 255*rgb.b;
            } else {
                rectangle[coord( x, y, 0, 3, 256 )] = 255;
                rectangle[coord( x, y, 1, 3, 256 )] = 255;
                rectangle[coord( x, y, 2, 3, 256 )] = 255;
            }
        }

        if ( y % 10 == 0 ) {
            gimp_progress_update(( gdouble )( y / ( gdouble ) gimp_image_height( image_ID ) ) );
        }
    }
    //save values in array
    gimp_pixel_rgn_set_rect( &region,
                             rectangle,
                             0, 0,
                             256, 256);

    //free memory
    g_free( rectangle );

    //gimp_drawable_flush (gimp_drawable_get(depthmap));
    gimp_drawable_merge_shadow( hsv_layer, TRUE );
    gimp_drawable_update( hsv_layer,
                          0, 0,
                          gimp_image_width( hsv_image ),
                          gimp_image_height( hsv_image ));

    //rotate image 270 deg
    gimp_image_rotate(hsv_image,2);

    //end
    gimp_image_undo_group_end( image_ID );
    printf( "L%i:****End of hsvanalysis.****\n", __LINE__ );
}

