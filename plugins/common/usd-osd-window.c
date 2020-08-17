/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * On-screen-display (OSD) window for ukui-settings-daemon's plugins
 *
 * Copyright (C) 2006-2007 William Jon McCann <mccann@jhu.edu> 
 * Copyright (C) 2009 Novell, Inc
 *
 * Authors:
 *   William Jon McCann <mccann@jhu.edu>
 *   Federico Mena-Quintero <federico@novell.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "usd-osd-window.h"

#define DIALOG_TIMEOUT 2000     /* dialog timeout in ms */
#define DIALOG_FADE_TIMEOUT 1500 /* timeout before fade starts */
#define FADE_TIMEOUT 10        /* timeout in ms between each frame of the fade */

#define BG_ALPHA 0.75

#define USD_OSD_WINDOW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), USD_TYPE_OSD_WINDOW, UsdOsdWindowPrivate))

struct UsdOsdWindowPrivate
{
        guint                    is_composited : 1;
        guint                    hide_timeout_id;
        guint                    fade_timeout_id;
        double                   fade_out_alpha;
};

enum {
#if GTK_CHECK_VERSION (3, 0, 0)
        DRAW_WHEN_COMPOSITED,
#else
        EXPOSE_WHEN_COMPOSITED,
#endif
        LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (UsdOsdWindow, usd_osd_window, GTK_TYPE_WINDOW)

static gboolean
fade_timeout (UsdOsdWindow *window)
{
        if (window->priv->fade_out_alpha <= 0.0) {
                gtk_widget_hide (GTK_WIDGET (window));

                /* Reset it for the next time */
                window->priv->fade_out_alpha = 1.0;
                window->priv->fade_timeout_id = 0;

                return FALSE;
        } else {
                GdkRectangle rect;
                GtkWidget *win = GTK_WIDGET (window);
                GtkAllocation allocation;

                window->priv->fade_out_alpha -= 0.10;

                rect.x = 0;
                rect.y = 0;
                gtk_widget_get_allocation (win, &allocation);
                rect.width = allocation.width;
                rect.height = allocation.height;

                gtk_widget_realize (win);
                gdk_window_invalidate_rect (gtk_widget_get_window (win), &rect, FALSE);
        }

        return TRUE;
}

static gboolean
hide_timeout (UsdOsdWindow *window)
{
        if (window->priv->is_composited) {
                window->priv->hide_timeout_id = 0;
                window->priv->fade_timeout_id = g_timeout_add (FADE_TIMEOUT,
                                                               (GSourceFunc) fade_timeout,
                                                               window);
        } else {
                gtk_widget_hide (GTK_WIDGET (window));
        }

        return FALSE;
}

static void
remove_hide_timeout (UsdOsdWindow *window)
{
        if (window->priv->hide_timeout_id != 0) {
                g_source_remove (window->priv->hide_timeout_id);
                window->priv->hide_timeout_id = 0;
        }

        if (window->priv->fade_timeout_id != 0) {
                g_source_remove (window->priv->fade_timeout_id);
                window->priv->fade_timeout_id = 0;
                window->priv->fade_out_alpha = 1.0;
        }
}

static void
add_hide_timeout (UsdOsdWindow *window)
{
        int timeout;

        if (window->priv->is_composited) {
                timeout = DIALOG_FADE_TIMEOUT;
        } else {
                timeout = DIALOG_TIMEOUT;
        }
        window->priv->hide_timeout_id = g_timeout_add (timeout,
                                                       (GSourceFunc) hide_timeout,
                                                       window);
}

void
usd_osd_window_draw_rounded_rectangle (cairo_t* cr,
                                       gdouble  aspect,
                                       gdouble  x,
                                       gdouble  y,
                                       gdouble  corner_radius,
                                       gdouble  width,
                                       gdouble  height)
{
        gdouble radius = corner_radius / aspect;

        cairo_move_to (cr, x + radius, y);

        cairo_line_to (cr,
                       x + width - radius,
                       y);
        cairo_arc (cr,
                   x + width - radius,
                   y + radius,
                   radius,
                   -90.0f * G_PI / 180.0f,
                   0.0f * G_PI / 180.0f);
        cairo_line_to (cr,
                       x + width,
                       y + height - radius);
        cairo_arc (cr,
                   x + width - radius,
                   y + height - radius,
                   radius,
                   0.0f * G_PI / 180.0f,
                   90.0f * G_PI / 180.0f);
        cairo_line_to (cr,
                       x + radius,
                       y + height);
        cairo_arc (cr,
                   x + radius,
                   y + height - radius,
                   radius,
                   90.0f * G_PI / 180.0f,
                   180.0f * G_PI / 180.0f);
        cairo_line_to (cr,
                       x,
                       y + radius);
        cairo_arc (cr,
                   x + radius,
                   y + radius,
                   radius,
                   180.0f * G_PI / 180.0f,
                   270.0f * G_PI / 180.0f);
        cairo_close_path (cr);
}

void
usd_osd_window_color_reverse (const GdkColor *a,
                              GdkColor       *b)
{
        gdouble red;
        gdouble green;
        gdouble blue;
        gdouble h;
        gdouble s;
        gdouble v;

        red = (gdouble) a->red / 65535.0;
        green = (gdouble) a->green / 65535.0;
        blue = (gdouble) a->blue / 65535.0;

        gtk_rgb_to_hsv (red, green, blue, &h, &s, &v);

        v = 0.5 + (0.5 - v);
        if (v > 1.0)
                v = 1.0;
        else if (v < 0.0)
                v = 0.0;

        gtk_hsv_to_rgb (h, s, v, &red, &green, &blue);

        b->red = red * 65535.0;
        b->green = green * 65535.0;
        b->blue = blue * 65535.0;
}

/* This is our expose/draw-event handler when the window is in a compositing manager.
 * We draw everything by hand, using Cairo, so that we can have a nice
 * transparent/rounded look.
 */
static void
#if GTK_CHECK_VERSION (3, 0, 0)
draw_when_composited (GtkWidget *widget, cairo_t *context)
#else
expose_when_composited (GtkWidget *widget, GdkEventExpose *event)
#endif
{
        UsdOsdWindow    *window;
#if !GTK_CHECK_VERSION (3, 0, 0)
        cairo_t         *context;
#endif
        cairo_t         *cr;
        cairo_surface_t *surface;
        int              width;
        int              height;
        GtkStyle        *style;
        GdkColor         color;
        double           r, g, b;

        window = USD_OSD_WINDOW (widget);

#if !GTK_CHECK_VERSION (3, 0, 0)
        context = gdk_cairo_create (gtk_widget_get_window (widget));
#endif

        style = gtk_widget_get_style (widget);
        cairo_set_operator (context, CAIRO_OPERATOR_SOURCE);
        gtk_window_get_size (GTK_WINDOW (widget), &width, &height);

        surface = cairo_surface_create_similar (cairo_get_target (context),
                                                CAIRO_CONTENT_COLOR_ALPHA,
                                                width,
                                                height);

        if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS) {
                goto done;
        }

        cr = cairo_create (surface);
        if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
                goto done;
        }
        cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.0);
        cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
        cairo_paint (cr);

        /* draw a box */
        usd_osd_window_draw_rounded_rectangle (cr, 1.0, 0.5, 0.5, height / 10, width-1, height-1);
        usd_osd_window_color_reverse (&style->bg[GTK_STATE_NORMAL], &color);
        r = (float)color.red / 65535.0;
        g = (float)color.green / 65535.0;
        b = (float)color.blue / 65535.0;
        cairo_set_source_rgba (cr, r, g, b, BG_ALPHA);
        cairo_fill_preserve (cr);

        usd_osd_window_color_reverse (&style->text_aa[GTK_STATE_NORMAL], &color);
        r = (float)color.red / 65535.0;
        g = (float)color.green / 65535.0;
        b = (float)color.blue / 65535.0;
        cairo_set_source_rgba (cr, r, g, b, BG_ALPHA / 2);
        cairo_set_line_width (cr, 1);
        cairo_stroke (cr);

#if GTK_CHECK_VERSION (3, 0, 0)
        g_signal_emit (window, signals[DRAW_WHEN_COMPOSITED], 0, cr);
#else
        g_signal_emit (window, signals[EXPOSE_WHEN_COMPOSITED], 0, cr);
#endif

        cairo_destroy (cr);

        /* Make sure we have a transparent background */
        cairo_rectangle (context, 0, 0, width, height);
        cairo_set_source_rgba (context, 0.0, 0.0, 0.0, 0.0);
        cairo_fill (context);

        cairo_set_source_surface (context, surface, 0, 0);
        cairo_paint_with_alpha (context, window->priv->fade_out_alpha);

 done:
        if (surface != NULL) {
                cairo_surface_destroy (surface);
        }
#if !GTK_CHECK_VERSION (3, 0, 0)
        cairo_destroy (context);
#endif
}

/* This is our expose/draw-event handler when the window is *not* in a compositing manager.
 * We just draw a rectangular frame by hand.  We do this with hardcoded drawing code,
 * instead of GtkFrame, to avoid changing the window's internal widget hierarchy:  in
 * either case (composited or non-composited), callers can assume that this works
 * identically to a GtkWindow without any intermediate widgetry.
 */
static void
#if GTK_CHECK_VERSION (3, 0, 0)
draw_when_not_composited (GtkWidget *widget, cairo_t *cr)
#else
expose_when_not_composited (GtkWidget *widget, GdkEventExpose *event)
#endif
{
#if GTK_CHECK_VERSION (3, 0, 0)
	int width;
	int height;
#else
	GtkAllocation allocation;
#endif


#if GTK_CHECK_VERSION (3, 0, 0)
	width = gtk_widget_get_allocated_width (widget);
	height = gtk_widget_get_allocated_width (widget);
#else
	gtk_widget_get_allocation (widget, &allocation);
#endif

	gtk_paint_shadow (gtk_widget_get_style (widget),
#if GTK_CHECK_VERSION (3, 0, 0)
			  cr,
#else
			  gtk_widget_get_window (widget),
#endif
			  gtk_widget_get_state (widget),
			  GTK_SHADOW_OUT,
#if !GTK_CHECK_VERSION (3, 0, 0)
			  &event->area,
#endif
			  widget,
			  NULL, /* NULL detail -> themes should use the UsdOsdWindow widget name, probably */
			  0,
			  0,
#if GTK_CHECK_VERSION (3, 0, 0)
			  width,
			  height);
#else
			  allocation.width,
			  allocation.height);
#endif
}

static gboolean
#if GTK_CHECK_VERSION (3, 0, 0)
usd_osd_window_draw (GtkWidget *widget,
                     cairo_t *cr)
#else
usd_osd_window_expose_event (GtkWidget          *widget,
                             GdkEventExpose     *event)
#endif
{
	UsdOsdWindow *window;
	GtkWidget *child;

	window = USD_OSD_WINDOW (widget);

#if GTK_CHECK_VERSION (3, 0, 0)
	if (window->priv->is_composited)
		draw_when_composited (widget, cr);
	else
		draw_when_not_composited (widget, cr);
#else
	if (window->priv->is_composited)
		expose_when_composited (widget, event);
	else
		expose_when_not_composited (widget, event);
#endif

	child = gtk_bin_get_child (GTK_BIN (window));
	if (child)
#if GTK_CHECK_VERSION (3, 0, 0)
		gtk_container_propagate_draw (GTK_CONTAINER (window), child, cr);
#else
		gtk_container_propagate_expose (GTK_CONTAINER (window), child, event);
#endif

	return FALSE;
}

static void
usd_osd_window_real_show (GtkWidget *widget)
{
        UsdOsdWindow *window;

        if (GTK_WIDGET_CLASS (usd_osd_window_parent_class)->show) {
                GTK_WIDGET_CLASS (usd_osd_window_parent_class)->show (widget);
        }

        window = USD_OSD_WINDOW (widget);
        remove_hide_timeout (window);
        add_hide_timeout (window);
}

static void
usd_osd_window_real_hide (GtkWidget *widget)
{
        UsdOsdWindow *window;

        if (GTK_WIDGET_CLASS (usd_osd_window_parent_class)->hide) {
                GTK_WIDGET_CLASS (usd_osd_window_parent_class)->hide (widget);
        }

        window = USD_OSD_WINDOW (widget);
        remove_hide_timeout (window);
}

static void
usd_osd_window_real_realize (GtkWidget *widget)
{
#if GTK_CHECK_VERSION (3, 0, 0)
        GdkScreen *screen;
        GdkVisual *visual;
        cairo_region_t *region;
#else
        GdkColormap *colormap;
        GtkAllocation allocation;
        GdkBitmap *mask;
        cairo_t *cr;
#endif

#if GTK_CHECK_VERSION (3, 0, 0)
        screen = gtk_widget_get_screen (widget);
        visual = gdk_screen_get_rgba_visual (screen);

        if (visual == NULL) {
                visual = gdk_screen_get_system_visual (screen);
        }
        gtk_widget_set_visual (widget, visual);
#else
        colormap = gdk_screen_get_rgba_colormap (gtk_widget_get_screen (widget));

        if (colormap != NULL) {
                gtk_widget_set_colormap (widget, colormap);
        }
#endif

        if (GTK_WIDGET_CLASS (usd_osd_window_parent_class)->realize) {
                GTK_WIDGET_CLASS (usd_osd_window_parent_class)->realize (widget);
        }

#if GTK_CHECK_VERSION (3, 0, 0)
        /* make the whole window ignore events */
        region = cairo_region_create ();
        gtk_widget_input_shape_combine_region (widget, region);
        cairo_region_destroy (region);
#else
        gtk_widget_get_allocation (widget, &allocation);
        mask = gdk_pixmap_new (gtk_widget_get_window (widget),
                               allocation.width,
                               allocation.height,
                               1);
        cr = gdk_cairo_create (mask);

        cairo_set_source_rgba (cr, 1., 1., 1., 0.);
        cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
        cairo_paint (cr);

        /* make the whole window ignore events */
        gdk_window_input_shape_combine_mask (gtk_widget_get_window (widget), mask, 0, 0);
        g_object_unref (mask);
        cairo_destroy (cr);
#endif
}

static void
usd_osd_window_style_set (GtkWidget *widget,
                          GtkStyle  *previous_style)
{
        GtkStyle *style;

        GTK_WIDGET_CLASS (usd_osd_window_parent_class)->style_set (widget, previous_style);

        /* We set our border width to 12 (per the MATE standard), plus the
         * thickness of the frame that we draw in our expose/draw handler.  This will
         * make our child be 12 pixels away from the frame.
         */

        style = gtk_widget_get_style (widget);
        gtk_container_set_border_width (GTK_CONTAINER (widget), 12 + MAX (style->xthickness, style->ythickness));
}

#if GTK_CHECK_VERSION (3, 0, 0)
static void
usd_osd_window_get_preferred_width (GtkWidget *widget,
                                    gint *minimum_width,
                                    gint *natural_width)
{
        GtkStyle *style;

        GTK_WIDGET_CLASS (usd_osd_window_parent_class)->get_preferred_width (widget, minimum_width, natural_width);

        /* See the comment in usd_osd_window_style_set() for why we add the thickness here */

        style = gtk_widget_get_style (widget);

        *minimum_width += style->xthickness;
        *natural_width += style->xthickness;
}

static void
usd_osd_window_get_preferred_height (GtkWidget *widget,
                                     gint *minimum_height,
                                     gint *natural_height)
{
        GtkStyle *style;

        GTK_WIDGET_CLASS (usd_osd_window_parent_class)->get_preferred_height (widget, minimum, natural);

        /* See the comment in usd_osd_window_style_updated() for why we add the padding here */

        style = gtk_widget_get_style (widget);

        *minimum_height += style->ythickness;
        *natural_height += style->ythickness;
}
#else
static void
usd_osd_window_size_request (GtkWidget      *widget,
                             GtkRequisition *requisition)
{
        GtkStyle *style;

        GTK_WIDGET_CLASS (usd_osd_window_parent_class)->size_request (widget, requisition);

        /* See the comment in usd_osd_window_style_set() for why we add the thickness here */

        style = gtk_widget_get_style (widget);

        requisition->width  += style->xthickness;
        requisition->height += style->ythickness;
}
#endif

static GObject *
usd_osd_window_constructor (GType                  type,
                            guint                  n_construct_properties,
                            GObjectConstructParam *construct_params)
{
        GObject *object;

        object = G_OBJECT_CLASS (usd_osd_window_parent_class)->constructor (type, n_construct_properties, construct_params);

        g_object_set (object,
                      "type", GTK_WINDOW_POPUP,
                      "type-hint", GDK_WINDOW_TYPE_HINT_NOTIFICATION,
                      "skip-taskbar-hint", TRUE,
                      "skip-pager-hint", TRUE,
                      "focus-on-map", FALSE,
                      NULL);

        return object;
}

static void
usd_osd_window_class_init (UsdOsdWindowClass *klass)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

        gobject_class->constructor = usd_osd_window_constructor;

        widget_class->show = usd_osd_window_real_show;
        widget_class->hide = usd_osd_window_real_hide;
        widget_class->realize = usd_osd_window_real_realize;
        widget_class->style_set = usd_osd_window_style_set;
#if GTK_CHECK_VERSION (3, 0, 0)
        widget_class->get_preferred_width = usd_osd_window_get_preferred_width;
        widget_class->get_preferred_height = usd_osd_window_get_preferred_height;
        widget_class->draw = usd_osd_window_draw;
#else
        widget_class->size_request = usd_osd_window_size_request;
        widget_class->expose_event = usd_osd_window_expose_event;
#endif

#if GTK_CHECK_VERSION (3, 0, 0)
        signals[DRAW_WHEN_COMPOSITED] = g_signal_new ("draw-when-composited",
#else
        signals[EXPOSE_WHEN_COMPOSITED] = g_signal_new ("expose-when-composited",
#endif
                                                        G_TYPE_FROM_CLASS (gobject_class),
                                                        G_SIGNAL_RUN_FIRST,
#if GTK_CHECK_VERSION (3, 0, 0)
                                                        G_STRUCT_OFFSET (UsdOsdWindowClass, draw_when_composited),
#else
                                                        G_STRUCT_OFFSET (UsdOsdWindowClass, expose_when_composited),
#endif
                                                        NULL, NULL,
                                                        g_cclosure_marshal_VOID__POINTER,
                                                        G_TYPE_NONE, 1,
                                                        G_TYPE_POINTER);

        g_type_class_add_private (klass, sizeof (UsdOsdWindowPrivate));
}

/**
 * usd_osd_window_is_composited:
 * @window: a #UsdOsdWindow
 *
 * Return value: whether the window was created on a composited screen.
 */
gboolean
usd_osd_window_is_composited (UsdOsdWindow *window)
{
        return window->priv->is_composited;
}

/**
 * usd_osd_window_is_valid:
 * @window: a #UsdOsdWindow
 *
 * Return value: TRUE if the @window's idea of being composited matches whether
 * its current screen is actually composited.
 */
gboolean
usd_osd_window_is_valid (UsdOsdWindow *window)
{
        GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (window));
        return gdk_screen_is_composited (screen) == window->priv->is_composited;
}

static void
usd_osd_window_init (UsdOsdWindow *window)
{
        GdkScreen *screen;

        window->priv = USD_OSD_WINDOW_GET_PRIVATE (window);

        screen = gtk_widget_get_screen (GTK_WIDGET (window));

        window->priv->is_composited = gdk_screen_is_composited (screen);

        if (window->priv->is_composited) {
                gdouble scalew, scaleh, scale;
                gint size;

                gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
                gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);

                /* assume 130x130 on a 640x480 display and scale from there */
                scalew = gdk_screen_get_width (screen) / 640.0;
                scaleh = gdk_screen_get_height (screen) / 480.0;
                scale = MIN (scalew, scaleh);
                size = 130 * MAX (1, scale);

                gtk_window_set_default_size (GTK_WINDOW (window), size, size);

                window->priv->fade_out_alpha = 1.0;
        } else {
		gtk_container_set_border_width (GTK_CONTAINER (window), 12);
        }
}

GtkWidget *
usd_osd_window_new (void)
{
        return g_object_new (USD_TYPE_OSD_WINDOW, NULL);
}

/**
 * usd_osd_window_update_and_hide:
 * @window: a #UsdOsdWindow
 *
 * Queues the @window for immediate drawing, and queues a timer to hide the window.
 */
void
usd_osd_window_update_and_hide (UsdOsdWindow *window)
{
        remove_hide_timeout (window);
        add_hide_timeout (window);

        if (window->priv->is_composited) {
                gtk_widget_queue_draw (GTK_WIDGET (window));
        }
}