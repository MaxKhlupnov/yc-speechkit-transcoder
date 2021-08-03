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

static gboolean
bus_call(GstBus* bus,
    GstMessage* msg,
    gpointer    data)
{
    GMainLoop* loop = (GMainLoop*) data;

    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS:
        g_print("End-of-stream\n");
        g_main_loop_quit(loop);
        break;
    case GST_MESSAGE_ERROR: {
        gchar* debug = NULL;
        GError* err = NULL;

        gst_message_parse_error(msg, &err, &debug);

        g_print("Error: %s\n", err->message);
        g_error_free(err);

        if (debug) {
            g_print("Debug details: %s\n", debug);
            g_free(debug);
        }

        g_main_loop_quit(loop);
        break;
    }
    default:
        break;
    }

    return TRUE;
}

int
main(int argc, char* argv[])
{
    GstBus* bus;
    GstMessage* msg;
    GError* err = NULL;

    

    gchar const* uri = "https://storage.yandexcloud.net/m24-speech/01%20Back%20in%20Black.mp3";
        //"https://storage.yandexcloud.net/audio-data/BrandAnalytics/2021-05-03-osoboe-1907-sd-3448912.wav";
        //"https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm";
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

    

    /*
    STOP 4 DEBUG
    
    Create a GLib Main Loop and set it to run, so we can wait for the signals */
    discovery.loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(discovery.loop);

    /* Stop the discoverer process */
    gst_discoverer_stop(discovery.discoverer);

    /* Free discoverer resources */
    g_object_unref(discovery.discoverer);
    g_main_loop_unref(discovery.loop);

    GMainLoop* loop;
    loop = g_main_loop_new(NULL, FALSE);

    /* Build the pipeline */
    GstElement *pipeline, *urlsrc, *decoder, * resample, * sink;
    GstElement *encoder, *media_convert, * capsfilter;

    pipeline = gst_pipeline_new("speechkit_pipeline");

    /* watch for messages on the pipeline's bus (note that this will only
   * work like this when a GLib main loop is running) */
    guint watch_id;
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));

    watch_id = gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    urlsrc = gst_element_factory_make("souphttpsrc", "media_source");
    decoder = gst_element_factory_make("decodebin", "media_decoder");


    /* putting an audioconvert element here to convert the output of the
   * decoder into a format that my_filter can handle (we are assuming it
   * will handle any sample rate here though) */
   
        media_convert = gst_element_factory_make("audioconvert", "media_convert");

        resample = gst_element_factory_make("audioresample", "media_resampler");
      /*  g_object_set(G_OBJECT(resample),
            "quality", 10, NULL);*/

        capsfilter = gst_element_factory_make("capsfilter", "caps_filter");
       /* g_object_set(G_OBJECT(capsfilter),
            "caps","audio/x-raw,format=S16LE,channels=1,rate=16000", NULL);*/

        encoder = gst_element_factory_make("wavenc", "encoder");

        sink = gst_element_factory_make("s3sink", "sink");       
       g_object_set(G_OBJECT(sink),
        //   "aws-credentials", "access-key-id=IVztEFxxxxxx|secret-access-key=Abo9xxxxxxx",
           "bucket", "s3-gst-plugin",
           "aws-sdk-endpoint", "storage.yandexcloud.net:443",
           "content-type","audio/wav",
           "key", "out.wav",
           NULL);
        
            

    if (!sink ) {
        g_print("S3 output could not be found - check your install\n");
        return -1;
    }
    else if (!media_convert || !capsfilter) {
        g_print("Could not create audioconvert or audioresample element, "
            "check your installation\n");
        return -1;
    }

    g_object_set(G_OBJECT(urlsrc), "location", uri, NULL);


    gst_bin_add_many(GST_BIN(pipeline), urlsrc, sink, NULL);//media_convert, resample,

    /*we link the elements together* /
    /* souphttpsrc -> ogg-demuxer ~> filesink 
    gst_element_link(urlsrc, sink);
    if (!gst_element_link(urlsrc, decoder)) {
        g_print("Failed to link src and decoder!\n");
        return -1;
    }*/
    

    /* link everything together */
    if (!gst_element_link_many(urlsrc,  sink, NULL)) {//pmedia_convert, resample,
        g_print("Failed to link one or more elements!\n");
        return -1;
    }

    /* run */
    GstStateChangeReturn ret;
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  
    if (ret == GST_STATE_CHANGE_FAILURE) {
        GstMessage* msg;

        g_print("Failed to start up pipeline!\n");

        /* check if there is an error message with details on the bus */
        msg = gst_bus_poll(bus, GST_MESSAGE_ERROR, 0);
        if (msg) {
            GError* err = NULL;

            gst_message_parse_error(msg, &err, NULL);
            g_print("ERROR: %s\n", err->message);
            g_error_free(err);
            gst_message_unref(msg);
        }
        return -1;
    }

    /* Iterate */
    g_print("Running...\n");
    g_main_loop_run(loop);


    /* Out of the main loop, clean up nicely */
    g_print("Returned, stopping...\n");


    /* Free resources */
    gst_element_set_state(pipeline, GST_STATE_NULL);
    g_print("Deleting pipeline\n");
    gst_object_unref(pipeline);
    g_source_remove(watch_id);
    g_main_loop_unref(loop);

    
    return 0;
}
