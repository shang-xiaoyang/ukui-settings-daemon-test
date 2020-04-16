#include <QIcon>
#include "keyboard-xkb.h"
#include "clib-syslog.h"

KeyboardManager * KeyboardXkb::manager = KeyboardManager::KeyboardManagerNew();

/*Migrate source files "delayed-dialog.c"*/
static gboolean        delayed_show_timeout (gpointer   data);
static GdkFilterReturn message_filter       (GdkXEvent *xevent,
                                             GdkEvent  *event,
                                             gpointer   data);
static GSList *dialogs = NULL;

QGSettings* settings_desktop;
QGSettings* settings_kbd;

static XklEngine* xkl_engine;
static XklConfigRegistry* xkl_registry = NULL;

static MatekbdDesktopConfig current_desktop_config;
static MatekbdKeyboardConfig current_kbd_config;

/* never terminated */
static MatekbdKeyboardConfig initial_sys_kbd_config;

static gboolean inited_ok = FALSE;

static PostActivationCallback pa_callback = NULL;
static void *pa_callback_user_data = NULL;

static GtkStatusIcon* icon = NULL;

static GHashTable* preview_dialogs = NULL;

static Atom caps_lock;
static Atom num_lock;
static Atom scroll_lock;

static QIcon indicator_icons[3];
static const gchar* indicator_on_icon_names[] = {
    "kbd-scrolllock-on",
    "kbd-numlock-on",
    "kbd-capslock-on"
};

static const gchar* indicator_off_icon_names[] = {
    "kbd-scrolllock-off",
    "kbd-numlock-off",
    "kbd-capslock-off"
};


KeyboardXkb::KeyboardXkb()
{
    CT_SYSLOG(LOG_DEBUG,"Keyboard Xkb initializing!");
}
KeyboardXkb::~KeyboardXkb()
{
    CT_SYSLOG(LOG_DEBUG,"Keyboard Xkb free");
    if(settings_desktop)
        delete settings_desktop;
    if(settings_kbd)
        delete settings_kbd;
}


/*Migrate source files "delayed-dialog.c"*/
void usd_delayed_show_dialog (GtkWidget *dialog)
{
        GdkDisplay *display = gtk_widget_get_display (dialog);
        Display *xdisplay = GDK_DISPLAY_XDISPLAY (display);
        GdkScreen *screen = gtk_widget_get_screen (dialog);
        char selection_name[10];
        Atom selection_atom;

        /* We can't use gdk_selection_owner_get() for this, because
         * it's an unknown out-of-process window.
         */
        snprintf (selection_name, sizeof (selection_name), "WM_S%d",
                  gdk_screen_get_number (screen));
        selection_atom = XInternAtom (xdisplay, selection_name, True);
        if (selection_atom &&
            XGetSelectionOwner (xdisplay, selection_atom) != None) {
                gtk_widget_show (dialog);
                return;
        }

        dialogs = g_slist_prepend (dialogs, dialog);

        gdk_window_add_filter (NULL, message_filter, NULL);

        g_timeout_add (5000, delayed_show_timeout, NULL);
}

static gboolean
delayed_show_timeout (gpointer data)
{
        GSList *l;

        for (l = dialogs; l; l = l->next)
                gtk_widget_show ((GtkWidget *)l->data);
        g_slist_free (dialogs);
        dialogs = NULL;

        /* FIXME: There's no gdk_display_remove_client_message_filter */

        return FALSE;
}

static GdkFilterReturn
message_filter (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
        XClientMessageEvent *evt;
        char *selection_name;
        int screen;
        GSList *l, *next;

        if (((XEvent *)xevent)->type != ClientMessage)
                return GDK_FILTER_CONTINUE;

        evt = (XClientMessageEvent *)xevent;

        if (evt->message_type != XInternAtom (evt->display, "MANAGER", FALSE))
                return GDK_FILTER_CONTINUE;

        selection_name = XGetAtomName (evt->display, evt->data.l[1]);

        if (strncmp (selection_name, "WM_S", 4) != 0) {
                XFree (selection_name);
                return GDK_FILTER_CONTINUE;
        }

        screen = atoi (selection_name + 4);

        for (l = dialogs; l; l = next) {
                GtkWidget *dialog = (GtkWidget *)l->data;
                next = l->next;

                if (gdk_screen_get_number (gtk_widget_get_screen (dialog)) == screen) {
                        gtk_widget_show (dialog);
                        dialogs = g_slist_remove (dialogs, dialog);
                }
        }

        if (!dialogs) {
                gdk_window_remove_filter (NULL, message_filter, NULL);
        }

        XFree (selection_name);

        return GDK_FILTER_CONTINUE;
}



static void usd_keyboard_update_indicator_icons ()
{
    Bool state;
    int new_state, i;
    Display *display = QX11Info::display();
    XkbGetNamedIndicator (display, caps_lock, NULL, &state,
                  NULL, NULL);
    new_state = state ? 1 : 0;
    XkbGetNamedIndicator (display, num_lock, NULL, &state, NULL, NULL);
    new_state <<= 1;
    new_state |= (state ? 1 : 0);
    XkbGetNamedIndicator (display, scroll_lock, NULL, &state,
                  NULL, NULL);
    new_state <<= 1;
    new_state |= (state ? 1 : 0);
    xkl_debug (160, "Indicators state: %d\n", new_state);


    for (i = sizeof (indicator_icons) / sizeof (indicator_icons[0]);
         --i >= 0;) {
        /*gtk_status_icon_set_from_icon_name (indicator_icons[i],
                            (new_state & (1 << i))
                            ?
                            indicator_on_icon_names
                            [i] :
                            indicator_off_icon_names
                            [i]);*/
        QIcon::setThemeName((new_state & (1 << i))
                            ? indicator_on_icon_names[i]
                            : indicator_off_icon_names[i]);
    }
}

static void usd_keyboard_xkb_analyze_sysconfig (void)
{
    if (!inited_ok)
        return;

    matekbd_keyboard_config_init (&initial_sys_kbd_config, xkl_engine);
    matekbd_keyboard_config_load_from_x_initial (&initial_sys_kbd_config,
                          NULL);
}

void KeyboardXkb::apply_desktop_settings (void)
{
    bool show_leds;
    int i;
    if (!inited_ok)
        return;
    manager->usd_keyboard_manager_apply_settings (manager);
    matekbd_desktop_config_load_from_gsettings (&current_desktop_config);
    /* again, probably it would be nice to compare things
       before activating them */
    matekbd_desktop_config_activate (&current_desktop_config);

    show_leds = settings_desktop->get(DUPLICATE_LEDS_KEY).toBool();// g_settings_get_boolean (settings_desktop, DUPLICATE_LEDS_KEY);
    for (i = sizeof (indicator_icons) / sizeof (indicator_icons[0]);
         --i >= 0;) {
        if(show_leds)
            indicator_icons[i].On;
        else
            indicator_icons[i].Off;
    }
}

static gboolean try_activating_xkb_config_if_new (MatekbdKeyboardConfig *
                  current_sys_kbd_config)
{
    /* Activate - only if different! */
    if (!matekbd_keyboard_config_equals
        (&current_kbd_config, current_sys_kbd_config)) {
        if (matekbd_keyboard_config_activate (&current_kbd_config)) {
            if (pa_callback != NULL) {
                (*pa_callback) (pa_callback_user_data);
                return TRUE;
            }
        } else {
            return FALSE;
        }
    }
    return TRUE;
}

static void g_strv_behead (gchar **arr)
{
    if (arr == NULL || *arr == NULL)
        return;

    g_free (*arr);
    memmove (arr, arr + 1, g_strv_length (arr) * sizeof (gchar *));
}


static gboolean filter_xkb_config (void)
{
    XklConfigItem *item;
    gchar *lname;
    gchar *vname;
    gchar **lv;
    gboolean any_change = FALSE;

    xkl_debug (100, "Filtering configuration against the registry\n");
    if (!xkl_registry) {
        xkl_registry =
            xkl_config_registry_get_instance (xkl_engine);
        /* load all materials, unconditionally! */
        if (!xkl_config_registry_load (xkl_registry, TRUE)) {
            g_object_unref (xkl_registry);
            xkl_registry = NULL;
            return FALSE;
        }
    }
    lv = current_kbd_config.layouts_variants;
    item = xkl_config_item_new ();
    while (*lv) {
        xkl_debug (100, "Checking [%s]\n", *lv);
        if (matekbd_keyboard_config_split_items (*lv, &lname, &vname)) {
            gboolean should_be_dropped = FALSE;
            g_snprintf (item->name, sizeof (item->name), "%s",
                    lname);
            if (!xkl_config_registry_find_layout
                (xkl_registry, item)) {
                xkl_debug (100, "Bad layout [%s]\n",
                       lname);
                should_be_dropped = TRUE;
            } else if (vname) {
                g_snprintf (item->name,
                        sizeof (item->name), "%s",
                        vname);
                if (!xkl_config_registry_find_variant
                    (xkl_registry, lname, item)) {
                    xkl_debug (100,
                           "Bad variant [%s(%s)]\n",
                           lname, vname);
                    should_be_dropped = TRUE;
                }
            }
            if (should_be_dropped) {
                g_strv_behead (lv);
                any_change = TRUE;
                continue;
            }
        }
        lv++;
    }
    g_object_unref (item);
    return any_change;
}

static void popup_menu_launch_capplet ()
{
    GAppInfo *info;
    GdkAppLaunchContext *context;
    GError *error = NULL;

    if (g_file_test("/usr/bin/ukui-control-center", G_FILE_TEST_EXISTS))
        info = g_app_info_create_from_commandline ("ukui-control-center -k &", NULL, (GAppInfoCreateFlags)0, &error);
    else
        info = g_app_info_create_from_commandline ("mate-keyboard-properties", NULL, (GAppInfoCreateFlags)0, &error);

    if (info != NULL) {
        context = gdk_display_get_app_launch_context (gdk_display_get_default ());
        g_app_info_launch (info, NULL,
                   G_APP_LAUNCH_CONTEXT (context), &error);

        g_object_unref (info);
        g_object_unref (context);
    }

    if (error != NULL) {
        g_warning
            ("Could not execute keyboard properties capplet: [%s]\n",
             error->message);
        g_error_free (error);
    }
}

static void show_layout_destroy (GtkWidget * dialog, gint group)
{
    g_hash_table_remove (preview_dialogs, GINT_TO_POINTER (group));
}

static void popup_menu_show_layout ()
{
    GtkWidget *dialog;
    Display *dpy = QX11Info::display();
    XklEngine *engine = xkl_engine_get_instance (dpy);
    XklState *xkl_state = xkl_engine_get_current_state (engine);
    gpointer p = g_hash_table_lookup (preview_dialogs,
                      GINT_TO_POINTER
                      (xkl_state->group));
    gchar **group_names = matekbd_status_get_group_names ();

    if (xkl_state->group < 0
        || xkl_state->group >= g_strv_length (group_names)) {
        return;
    }

    if (p != NULL) {
        /* existing window */
        gtk_window_present (GTK_WINDOW (p));
        return;
    }

    dialog = matekbd_keyboard_drawing_new_dialog (xkl_state->group,
                          group_names[xkl_state->group]);
    g_signal_connect (dialog, "destroy",
              G_CALLBACK (show_layout_destroy),
              GINT_TO_POINTER (xkl_state->group));
    g_hash_table_insert (preview_dialogs,
                 GINT_TO_POINTER (xkl_state->group), dialog);
}

static void popup_menu_set_group (GtkMenuItem * item, gpointer param)
{
    gint group_number = GPOINTER_TO_INT (param);
    XklEngine *engine = matekbd_status_get_xkl_engine ();
    XklState st;
    Window cur;

    st.group = group_number;
    xkl_engine_allow_one_switch_to_secondary_group (engine);
    cur = xkl_engine_get_current_window (engine);
    if (cur != (Window) NULL) {
        xkl_debug (150, "Enforcing the state %d for window %lx\n",
               st.group, cur);
        xkl_engine_save_state (engine,
                       xkl_engine_get_current_window
                       (engine), &st);
/*    XSetInputFocus(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), cur, RevertToNone, CurrentTime );*/
    } else {
        xkl_debug (150,
               "??? Enforcing the state %d for unknown window\n",
               st.group);
        /* strange situation - bad things can happen */
    }
    xkl_engine_lock_group (engine, st.group);
}

static void status_icon_popup_menu_cb (GtkStatusIcon * icon, guint button, guint time)
{
    GtkMenu *popup_menu = GTK_MENU (gtk_menu_new ());
    GtkMenu *groups_menu = GTK_MENU (gtk_menu_new ());
    int i = 0;
    gchar **current_name = matekbd_status_get_group_names ();

    GtkWidget *item = gtk_menu_item_new_with_mnemonic (_("_Layouts"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), item);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (item),
                   GTK_WIDGET (groups_menu));

    item = gtk_menu_item_new_with_mnemonic (_("Keyboard _Preferences"));
    gtk_widget_show (item);
    g_signal_connect (item, "activate", popup_menu_launch_capplet,
              NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), item);

    item = gtk_menu_item_new_with_mnemonic (_("Show _Current Layout"));
    gtk_widget_show (item);
    g_signal_connect (item, "activate", popup_menu_show_layout, NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (popup_menu), item);

    for (i = 0; *current_name; i++, current_name++) {
        gchar *image_file = matekbd_status_get_image_filename (i);

        if (image_file == NULL) {
            item =
                gtk_menu_item_new_with_label (*current_name);
        } else {
            GdkPixbuf *pixbuf =
                gdk_pixbuf_new_from_file_at_size (image_file,
                                  24, 24,
                                  NULL);
            GtkWidget *img =
                gtk_image_new_from_pixbuf (pixbuf);
            item =
                gtk_image_menu_item_new_with_label
                (*current_name);
            gtk_widget_show (img);
            gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM
                               (item), img);
            gtk_image_menu_item_set_always_show_image
                (GTK_IMAGE_MENU_ITEM (item), TRUE);
            g_free (image_file);
        }
        gtk_widget_show (item);
        gtk_menu_shell_append (GTK_MENU_SHELL (groups_menu), item);
        g_signal_connect (item, "activate",
                  G_CALLBACK (popup_menu_set_group),
                  GINT_TO_POINTER (i));
    }

    gtk_menu_popup (popup_menu, NULL, NULL,
            gtk_status_icon_position_menu,
            (gpointer) icon, button, time);
}

static void activation_error (void)
{
    Display *dpy = QX11Info::display();
    char const *vendor = ServerVendor (dpy);//GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()));
    int release = VendorRelease (dpy);//GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()));
    GtkWidget *dialog;

    /* VNC viewers will not work, do not barrage them with warnings */
    if (NULL != vendor && NULL != strstr (vendor, "VNC"))
        return;

    dialog = gtk_message_dialog_new_with_markup (NULL,
                             (GtkDialogFlags)0,
                             GTK_MESSAGE_ERROR,
                             GTK_BUTTONS_CLOSE,
                             _
                             ("Error activating XKB configuration.\n"
                              "It can happen under various circumstances:\n"
                              " • a bug in libxklavier library\n"
                              " • a bug in X server (xkbcomp, xmodmap utilities)\n"
                              " • X server with incompatible libxkbfile implementation\n\n"
                              "X server version data:\n%s\n%d\n"
                              "If you report this situation as a bug, please include:\n"
                              " • The result of <b>%s</b>\n"
                              " • The result of <b>%s</b>"),
                             vendor,
                             release,
                             "xprop -root | grep XKB",
                             "gsettings list-keys org.mate.peripherals-keyboard-xkb.kbd");
    g_signal_connect (dialog, "response",
              G_CALLBACK (gtk_widget_destroy), NULL);
    usd_delayed_show_dialog (dialog);
}

static void show_hide_icon ()
{
    if (g_strv_length (current_kbd_config.layouts_variants) > 1) {
        if (icon == NULL) {
            bool disable = settings_desktop->get(DISABLE_INDICATOR_KEY).toBool();//g_settings_get_boolean (settings_desktop, DISABLE_INDICATOR_KEY);
            if (disable)
                return;

            xkl_debug (150, "Creating new icon\n");
            icon = matekbd_status_new ();
            gtk_status_icon_set_name (icon, "keyboard");
            g_signal_connect (icon, "popup-menu",
                      G_CALLBACK
                      (status_icon_popup_menu_cb),
                      NULL);

        }
    } else {
        if (icon != NULL) {
            xkl_debug (150, "Destroying icon\n");
            g_object_unref (icon);
            icon = NULL;
        }
    }
}

static void apply_xkb_settings (void)
{
    MatekbdKeyboardConfig current_sys_kbd_config;

    if (!inited_ok)
        return;

    matekbd_keyboard_config_init (&current_sys_kbd_config, xkl_engine);

    matekbd_keyboard_config_load_from_gsettings (&current_kbd_config,
                          &initial_sys_kbd_config);

    matekbd_keyboard_config_load_from_x_current (&current_sys_kbd_config,
                          NULL);

    if (!try_activating_xkb_config_if_new (&current_sys_kbd_config)) {
        if (filter_xkb_config ()) {
            if (!try_activating_xkb_config_if_new
                (&current_sys_kbd_config)) {
                g_warning
                    ("Could not activate the filtered XKB configuration");
                activation_error ();
            }
        } else {
            g_warning
                ("Could not activate the XKB configuration");
            activation_error ();
        }
    } else
        xkl_debug (100,
               "Actual KBD configuration was not changed: redundant notification\n");

    matekbd_keyboard_config_term (&current_sys_kbd_config);
    show_hide_icon ();
}

void KeyboardXkb::apply_desktop_settings_cb (QString key)
{
    apply_desktop_settings ();
}
void apply_desktop_settings_mate_cb(GSettings *settings, gchar *key, gpointer   user_data)
{
   KeyboardXkb::apply_desktop_settings ();
}

void KeyboardXkb::apply_xkb_settings_cb (QString key)
{
    apply_xkb_settings ();
}
static void apply_xkb_settings_mate_cb (GSettings *settings, gchar *key, gpointer   user_data)
{
    apply_xkb_settings ();
}

GdkFilterReturn usd_keyboard_xkb_evt_filter (GdkXEvent * xev, GdkEvent * event)
{
    XEvent *xevent = (XEvent *) xev;
    xkl_engine_filter_events (xkl_engine, xevent);
    return GDK_FILTER_CONTINUE;
}

/* When new Keyboard is plugged in - reload the settings */
void KeyboardXkb::usd_keyboard_new_device (XklEngine * engine)
{
    apply_desktop_settings ();
    apply_xkb_settings ();
}

static void usd_keyboard_state_changed (XklEngine * engine, XklEngineStateChange type,
                gint new_group, gboolean restore)
{
    xkl_debug (160,"State changed: type %d, new group: %d, restore: %d.\n",
                type, new_group, restore);
    if (type == INDICATORS_CHANGED) {
        usd_keyboard_update_indicator_icons ();
    }
}

void KeyboardXkb::usd_keyboard_xkb_init(KeyboardManager* kbd_manager)
{
        CT_SYSLOG(LOG_DEBUG,"init --- XKB");
        int i;
        Display *display = QX11Info::display();
        QString icon_path = "/usr/share/ukui-settings-daemon/icons,";
        QStringList list1 = icon_path.split(",");
        QIcon::setThemeSearchPaths(list1);

        /*gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
                           "/usr/local/share/ukui-settings-daemon/" G_DIR_SEPARATOR_S
                           "icons");*/

        caps_lock = XInternAtom (display, "Caps Lock", False);
        num_lock = XInternAtom (display, "Num Lock", False);
        scroll_lock = XInternAtom (display, "Scroll Lock", False);

        for (i = sizeof (indicator_icons) / sizeof (indicator_icons[0]);
             --i >= 0;) {
               indicator_icons[i]=QIcon::fromTheme(indicator_off_icon_names[i]);
               //gtk_status_icon_new_from_icon_name(indicator_off_icon_names[i]);
        }

        usd_keyboard_update_indicator_icons ();

        manager = kbd_manager;

        xkl_engine = xkl_engine_get_instance (display);

        if (xkl_engine) {
            inited_ok = TRUE;

            settings_desktop = new QGSettings(MATEKBD_DESKTOP_SCHEMA);
            settings_kbd = new QGSettings(MATEKBD_KBD_SCHEMA);

            matekbd_desktop_config_init (&current_desktop_config,xkl_engine);

            matekbd_keyboard_config_init (&current_kbd_config,xkl_engine);

            xkl_engine_backup_names_prop (xkl_engine);
            usd_keyboard_xkb_analyze_sysconfig ();

            matekbd_desktop_config_start_listen (&current_desktop_config, G_CALLBACK (apply_desktop_settings_mate_cb),NULL);

            matekbd_keyboard_config_start_listen (&current_kbd_config,G_CALLBACK (apply_xkb_settings_mate_cb),NULL);

            QObject::connect(settings_desktop,SIGNAL(changed(QString)),this,SLOT(apply_desktop_settings_cb(QString)));

            QObject::connect(settings_kbd,SIGNAL(changed(QString)),this,SLOT(apply_xkb_settings_cb(QString)));

            gdk_window_add_filter (NULL, (GdkFilterFunc)usd_keyboard_xkb_evt_filter, NULL);

            if (xkl_engine_get_features (xkl_engine) &XKLF_DEVICE_DISCOVERY)
                g_signal_connect (xkl_engine, "X-new-device",
                                  G_CALLBACK(usd_keyboard_new_device), NULL);

            g_signal_connect (xkl_engine, "X-state-changed",
                              G_CALLBACK(usd_keyboard_state_changed), NULL);

            xkl_engine_start_listen (xkl_engine, XKLL_MANAGE_LAYOUTS |XKLL_MANAGE_WINDOW_STATES);

            apply_desktop_settings ();
            apply_xkb_settings ();
        }
        preview_dialogs = g_hash_table_new (g_direct_hash, g_direct_equal);
}

void KeyboardXkb::usd_keyboard_xkb_shutdown (void)
{
    int i;

    pa_callback = NULL;
    pa_callback_user_data = NULL;
    manager = NULL;

    for (i = sizeof (indicator_icons) / sizeof (indicator_icons[0]);
         --i >= 0;) {
       /* g_object_unref (G_OBJECT (indicator_icons[i]));
        indicator_icons[i] = NULL;*/
    }

    g_hash_table_destroy (preview_dialogs);

    if (!inited_ok)
        return;

    xkl_engine_stop_listen (xkl_engine,
                XKLL_MANAGE_LAYOUTS |
                XKLL_MANAGE_WINDOW_STATES);

    gdk_window_remove_filter (NULL, (GdkFilterFunc)
                  usd_keyboard_xkb_evt_filter, NULL);

    if (settings_desktop != NULL) {
        g_object_unref (settings_desktop);
    }

    if (settings_kbd != NULL) {
        g_object_unref (settings_kbd);
    }

    if (xkl_registry) {
        g_object_unref (xkl_registry);
    }

    g_object_unref (xkl_engine);

    xkl_engine = NULL;
    inited_ok = FALSE;
}
