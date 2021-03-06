#include <string.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <libgepub/gepub.h>

gchar *buf = NULL;
gchar *buf2 = NULL;
gchar *tmpbuf;

GtkTextBuffer *page_buffer;

#define PTEST1(...) printf (__VA_ARGS__)
#define PTEST2(...) buf = g_strdup_printf (__VA_ARGS__);\
                    tmpbuf = buf2;\
                    buf2 = g_strdup_printf ("%s%s", buf2, buf);\
                    g_free (buf);\
                    g_free (tmpbuf)
#define PTEST PTEST2

#define TEST(f,arg...) PTEST ("\n### TESTING " #f " ###\n\n"); f (arg); PTEST ("\n\n");

void
update_text (GepubDoc *doc)
{
    GList *l, *chunks;
    GtkTextIter start, end;

    gtk_text_buffer_get_start_iter (page_buffer, &start);
    gtk_text_buffer_get_end_iter (page_buffer, &end);
    gtk_text_buffer_delete (page_buffer, &start, &end);

    chunks = gepub_doc_get_text (doc);

    for (l=chunks; l; l = l->next) {
        GepubTextChunk *chunk = GEPUB_TEXT_CHUNK (l->data);
        if (chunk->type == GEPUBTextHeader) {
            gtk_text_buffer_insert_at_cursor (page_buffer, "\n", -1);
            gtk_text_buffer_get_end_iter (page_buffer, &end);
            gtk_text_buffer_insert_with_tags_by_name (page_buffer, &end, chunk->text, -1, "head",  NULL);
            gtk_text_buffer_insert_at_cursor (page_buffer, "\n", -1);
        } else if (chunk->type == GEPUBTextNormal) {
            gtk_text_buffer_insert_at_cursor (page_buffer, "\n", -1);
            gtk_text_buffer_insert_at_cursor (page_buffer, chunk->text, -1);
            gtk_text_buffer_insert_at_cursor (page_buffer, "\n", -1);
        } else if (chunk->type == GEPUBTextItalic) {
            gtk_text_buffer_get_end_iter (page_buffer, &end);
            gtk_text_buffer_insert_with_tags_by_name (page_buffer, &end, chunk->text, -1, "italic",  NULL);
        } else if (chunk->type == GEPUBTextBold) {
            gtk_text_buffer_get_end_iter (page_buffer, &end);
            gtk_text_buffer_insert_with_tags_by_name (page_buffer, &end, chunk->text, -1, "bold",  NULL);
        }
    }
}

void
button_pressed (GtkButton *button, GepubDoc *doc)
{
    if (!strcmp (gtk_button_get_label (button), "prev")) {
        gepub_doc_go_prev (doc);
    } else {
        gepub_doc_go_next (doc);
    }
    update_text (doc);
}

void
test_open (const char *path)
{
    GepubArchive *a;
    GList *list_files;
    gint i;
    gint size;

    a = gepub_archive_new (path);
    list_files = gepub_archive_list_files (a);
    if (!list_files) {
        PTEST ("ERROR: BAD epub file");
        g_object_unref (a);
        return;
    }

    size = g_list_length (list_files);
    PTEST ("%d\n", size);
    for (i = 0; i < size; i++) {
        PTEST ("file: %s\n", (char *)g_list_nth_data (list_files, i));
        g_free (g_list_nth_data (list_files, i));
    }

    g_list_free (list_files);

    g_object_unref (a);
}

void
find_xhtml (gchar *key, GepubResource *value, gpointer data)
{
    guchar **d = (guchar **)data;
    if (g_strcmp0 (value->mime, "application/xhtml+xml") == 0) {
        *d = value->uri;
    }
}

void
test_read (const char *path)
{
    GepubArchive *a;
    GList *list_files;
    gint i;
    gint size;
    guchar *buffer;
    guchar *file = NULL;
    gsize bufsize;

    a = gepub_archive_new (path);

    GepubDoc *doc = gepub_doc_new (path);
    GHashTable *ht = (GHashTable*)gepub_doc_get_resources (doc);
    g_hash_table_foreach (ht, (GHFunc)find_xhtml, &file);

    gepub_archive_read_entry (a, file, &buffer, &bufsize);
    if (bufsize)
        PTEST ("doc:%s\n----\n%s\n-----\n", file, buffer);

    g_free (buffer);

    g_list_foreach (list_files, (GFunc)g_free, NULL);
    g_list_free (list_files);

    g_object_unref (a);
}

void
test_root_file (const char *path)
{
    GepubArchive *a;
    gchar *root_file = NULL;

    a = gepub_archive_new (path);

    root_file = gepub_archive_get_root_file (a);
    PTEST ("root file: %s\n", root_file);
    if (root_file)
        g_free (root_file);

    g_object_unref (a);
}

void
test_doc_name (const char *path)
{
    GepubDoc *doc = gepub_doc_new (path);
    gchar *title = gepub_doc_get_metadata (doc, GEPUB_META_TITLE);
    gchar *lang = gepub_doc_get_metadata (doc, GEPUB_META_LANG);
    gchar *id = gepub_doc_get_metadata (doc, GEPUB_META_ID);
    gchar *author = gepub_doc_get_metadata (doc, GEPUB_META_AUTHOR);
    gchar *description = gepub_doc_get_metadata (doc, GEPUB_META_DESC);
    gchar *cover = gepub_doc_get_cover (doc);
    gchar *cover_mime = gepub_doc_get_resource_mime_by_id (doc, cover);

    PTEST ("title: %s\n", title);
    PTEST ("author: %s\n", author);
    PTEST ("id: %s\n", id);
    PTEST ("lang: %s\n", lang);
    PTEST ("desc: %s\n", description);
    PTEST ("cover: %s\n", cover);
    PTEST ("cover mime: %s\n", cover_mime);

    g_free (title);
    g_free (lang);
    g_free (id);
    g_free (author);
    g_free (description);
    g_free (cover);
    g_object_unref (G_OBJECT (doc));
}

void
pk (gchar *key, GepubResource *value, gpointer data)
{
    PTEST ("%s: %s, %s\n", key, value->mime, value->uri);
}

void
test_doc_resources (const char *path)
{
    GepubDoc *doc = gepub_doc_new (path);
    GHashTable *ht = (GHashTable*)gepub_doc_get_resources (doc);
    g_hash_table_foreach (ht, (GHFunc)pk, NULL);
    guchar *ncx;
    gsize size;

    ncx = gepub_doc_get_resource (doc, "ncx", &size);
    PTEST ("ncx:\n%s\n", ncx);
    g_free (ncx);

    g_object_unref (G_OBJECT (doc));
}

void
p (gchar *value, gpointer data)
{
    static int id = 0;
    PTEST ("%d: %s\n", id++, value);
}

void
test_doc_spine (const char *path)
{
    GepubDoc *doc = gepub_doc_new (path);

    GList *spine = gepub_doc_get_spine (doc);
    g_list_foreach (spine, (GFunc)p, NULL);

    g_object_unref (G_OBJECT (doc));
}

int
main (int argc, char **argv)
{
    gtk_init (&argc, &argv);

    GtkWidget *window;
    GtkWidget *vpaned;
    GtkWidget *textview;
    GtkWidget *scrolled;

    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *b_next;
    GtkWidget *b_prev;

    GtkTextBuffer *buffer;

    GepubDoc *doc;
    GtkWidget *textview2;

    if (argc < 2) {
        printf ("you should provide an .epub file\n");
        return 1;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (window, "destroy", (GCallback)gtk_main_quit, NULL);
    gtk_widget_set_size_request (GTK_WIDGET (window), 800, 500);
    vpaned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_container_add (GTK_CONTAINER (window), vpaned);

    doc = gepub_doc_new (argv[1]);
    if (!doc) {
        perror ("BAD epub FILE");
        return -1;
    }

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    textview2 = gtk_text_view_new ();
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (textview2), GTK_WRAP_WORD_CHAR);
    page_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview2));
    gtk_text_buffer_create_tag (page_buffer, "bold", "weight", PANGO_WEIGHT_BOLD, "foreground", "#ff0000", NULL);
    gtk_text_buffer_create_tag (page_buffer, "italic", "style", PANGO_STYLE_ITALIC, "foreground", "#005500", NULL);
    gtk_text_buffer_create_tag (page_buffer, "head", "size-points", 20.0, NULL);
    update_text (doc);
    gtk_container_add (GTK_CONTAINER (scrolled), GTK_WIDGET (textview2));
    gtk_widget_set_size_request (GTK_WIDGET (textview2), 500, 300);

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    b_prev = gtk_button_new_with_label ("prev");
    g_signal_connect (b_prev, "clicked", (GCallback)button_pressed, doc);
    b_next = gtk_button_new_with_label ("next");
    g_signal_connect (b_next, "clicked", (GCallback)button_pressed, doc);
    gtk_container_add (GTK_CONTAINER (hbox), b_prev);
    gtk_container_add (GTK_CONTAINER (hbox), b_next);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);
    gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 5);

    gtk_widget_set_size_request (GTK_WIDGET (vbox), 400, 500);
    gtk_paned_add1 (GTK_PANED (vpaned), vbox);

    textview = gtk_text_view_new ();
    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_add (GTK_CONTAINER (scrolled), textview);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_paned_add2 (GTK_PANED (vpaned), scrolled);

    gtk_widget_show_all (window);


    // Testing all
    TEST(test_open, argv[1])
    TEST(test_read, argv[1])
    TEST(test_root_file, argv[1])
    TEST(test_doc_name, argv[1])
    TEST(test_doc_resources, argv[1])
    TEST(test_doc_spine, argv[1])

    // Freeing the mallocs :P
    if (buf2) {
        buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
        gtk_text_buffer_set_text (buffer, buf2, strlen (buf2));
        g_free (buf2);
    }

    gtk_main ();

    return 0;
}
