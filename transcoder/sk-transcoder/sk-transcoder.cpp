#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>
#include "sk-transcoder.h"


/* This function is called every time the discoverer has information regarding
 * one of the URIs we provided.*/
void on_discovered_cb(GstDiscoverer* discoverer, GstDiscovererInfo* info, GError* err, DiscovererData* data) {
    GstDiscovererResult result;
    const gchar* uri;
    const GstTagList* tags;
    GstDiscovererStreamInfo* sinfo;

    uri = gst_discoverer_info_get_uri(info);
    result = gst_discoverer_info_get_result(info);
    switch (result) {
    case GST_DISCOVERER_URI_INVALID:
        g_print("Invalid URI '%s'\n", uri);
        break;
    case GST_DISCOVERER_ERROR:
        g_print("Discoverer error: %s\n", err->message);
        break;
    case GST_DISCOVERER_TIMEOUT:
        g_print("Timeout\n");
        break;
    case GST_DISCOVERER_BUSY:
        g_print("Busy\n");
        break;
    case GST_DISCOVERER_MISSING_PLUGINS: {
        const GstStructure* s;
        gchar* str;

        s = gst_discoverer_info_get_misc(info);
        str = gst_structure_to_string(s);

        g_print("Missing plugins: %s\n", str);
        g_free(str);
        break;
    }
    case GST_DISCOVERER_OK:
        g_print("Discovered '%s'\n", uri);
        break;
    }

    if (result != GST_DISCOVERER_OK) {
        g_printerr("This URI cannot be played\n");
        return;
    }

    /* If we got no error, show the retrieved information */

    g_print("\nDuration: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS(gst_discoverer_info_get_duration(info)));

    tags = gst_discoverer_info_get_tags(info);
    if (tags) {
        g_print("Tags:\n");
        gst_tag_list_foreach(tags, print_tag_foreach, GINT_TO_POINTER(1));
    }

    g_print("Seekable: %s\n", (gst_discoverer_info_get_seekable(info) ? "yes" : "no"));

    g_print("\n");

    sinfo = gst_discoverer_info_get_stream_info(info);
    if (!sinfo)
        return;

    g_print("Stream information:\n");

    print_topology(sinfo, 1);

    gst_discoverer_stream_info_unref(sinfo);

    g_print("\n");
}

/* This function is called when the discoverer has finished examining
 * all the URIs we provided.*/
void on_finished_cb(GstDiscoverer* discoverer, DiscovererData* data) {
    g_print("Finished discovering\n");

    g_main_loop_quit(data->loop);
}

int
main(int argc, char* argv[])
{
    GstElement* pipeline;
    GstBus* bus;
    GstMessage* msg;
    GError* err = NULL;



    gchar const *uri = "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm";
    //"https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm";
        //"https://rockthecradle.stream.publicradio.org/rockthecradle.mp3"; -- mp3 stream

    /* if a URI was provided, use it instead of the default one */
    if (argc > 1) {
        uri = argv[1];
    }

    /* Initialize cumstom data structure */
    memset(&discovery, 0, sizeof(discovery));

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    g_print("Discovering '%s'\n", uri);

    /* Instantiate the Discoverer */
    discovery.discoverer = gst_discoverer_new(5 * GST_SECOND, &err);
    if (!discovery.discoverer) {
        g_print("Error creating discoverer instance: %s\n", err->message);
        g_clear_error(&err);
        return -1;
    }

    /* Connect to the interesting signals */
    g_signal_connect(discovery.discoverer, "discovered", G_CALLBACK(on_discovered_cb), &discovery);
    g_signal_connect(discovery.discoverer, "finished", G_CALLBACK(on_finished_cb), &discovery);

    /* Start the discoverer process (nothing to do yet) */
    gst_discoverer_start(discovery.discoverer);

    /* Add a request to process asynchronously the URI passed through the command line */
    if (!gst_discoverer_discover_uri_async(discovery.discoverer, uri)) {
        g_print("Failed to start discovering URI '%s'\n", uri);
        g_object_unref(discovery.discoverer);
        return -1;
    }

    /* Create a GLib Main Loop and set it to run, so we can wait for the signals */
    discovery.loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(discovery.loop);

    /* Stop the discoverer process */
    gst_discoverer_stop(discovery.discoverer);

    /* Free resources */
    g_object_unref(discovery.discoverer);
    g_main_loop_unref(discovery.loop);

    return 0;

    /* Build the pipeline */
    pipeline =
        gst_parse_launch
        ("souphttpsrc location=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm ! filesink location=/home/makhlu/sintel_trailer-480p.webm",
            NULL);

    /* Start playing */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /* Wait until error or EOS */
    bus = gst_element_get_bus(pipeline);
    msg =
        gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    /* Free resources */
    if (msg != NULL)
        gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}
