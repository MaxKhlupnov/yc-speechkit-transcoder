#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>
#include "sk-transcoder.h"


int
main(int argc, char* argv[])
{
    GstElement* pipeline;
    GstBus* bus;
    GstMessage* msg;
    GError* err = NULL;



    gchar* uri = "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm";

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
