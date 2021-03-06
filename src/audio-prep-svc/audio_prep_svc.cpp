//
// Created by makhlu on 22.09.2021.
//

#include "audio_prep_svc.h"


    audio_preparation_svc::audio_preparation_svc(std::map<std::string, std::string> config, std::shared_ptr<audio_prep_svc_callback> callback)
    {
        _discovery.config = config;
        _discovery.callback = callback;
    }

    void audio_preparation_svc::discover_audio_format(std::string audio_source_uri){

        GError* err = NULL;

        /* Initialize cumstom data structure */
        memset(&_discovery, 0, sizeof(_discovery));

        g_print("Discovering '%s'\n", audio_source_uri.c_str());
        /* Instantiate the Discoverer */
        _discovery.discoverer = gst_discoverer_new(5 * GST_SECOND, &err);
        if (!_discovery.discoverer) {
            g_print("Error creating discoverer instance: %s\n", err->message);
            g_clear_error(&err);
            return;
        }

        /* Connect to the interesting signals */
        g_signal_connect(_discovery.discoverer, "discovered", G_CALLBACK(on_discovered_cb), &_discovery);
        g_signal_connect(_discovery.discoverer, "finished", G_CALLBACK(on_finished_cb), &_discovery);

        /*   Create a GLib Main Loop and set it to run, so we can wait for the signals */
        _discovery.loop = g_main_loop_new(NULL, FALSE);
        g_main_loop_run(_discovery.loop);

        /* Stop the discoverer process */
        gst_discoverer_stop(_discovery.discoverer);

        /* Free discoverer resources */
        g_object_unref(_discovery.discoverer);
        g_main_loop_unref(_discovery.loop);
    }
    void audio_preparation_svc::start_preparation_pipeline(std::string audio_source_uri){
        GstBus* bus;
        GstMessage* msg;
        GError* err = NULL;
        GMainLoop* loop;
        loop = g_main_loop_new(NULL, FALSE);

        std::string str_pipeline = _discovery.config[CFG_PIPELINE_TEMPLATE];

        GstElement* pipeline =  gst_parse_launch(str_pipeline.c_str(), NULL);

        guint watch_id;
        bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));


        //watch_id = gst_bus_add_watch(bus, (gpointer) on_finished_cb , loop);
        g_signal_connect(_discovery.discoverer, "finished", G_CALLBACK(on_finished_cb), &_discovery);
        gst_object_unref(bus);

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
            return;
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
    }

    void audio_preparation_svc::on_finished_cb(GstDiscoverer* discoverer, DiscovererData* data) {
        g_print("Finished discovering\n");

        g_main_loop_quit(data->loop);

        if (!data->callback) {
            g_print("Error. Callback do not set");
        }else{

            JSON_Value *root_value = json_value_init_object();
            JSON_Object *root_object = json_value_get_object(root_value);


            char* serialized_string = json_serialize_to_string_pretty(root_value );

            data->callback->format_detection_result(std::string(serialized_string));
            json_value_free(root_value);
        }
    }

    /* This function is called every time the discoverer has information regarding
    * one of the URIs we provided.*/
    void audio_preparation_svc::on_discovered_cb(GstDiscoverer* discoverer, GstDiscovererInfo* info, GError* err, DiscovererData* data) {
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

/* Print a tag in a human-readable format (name: value) */
void audio_preparation_svc::print_tag_foreach(const GstTagList* tags, const gchar* tag, gpointer user_data) {
    GValue val = { 0, };
    gchar* str;
    gint depth = GPOINTER_TO_INT(user_data);

    gst_tag_list_copy_value(&val, tags, tag);

    if (G_VALUE_HOLDS_STRING(&val))
        str = g_value_dup_string(&val);
    else
        str = gst_value_serialize(&val);

    g_print("%*s%s: %s\n", 2 * depth, " ", gst_tag_get_nick(tag), str);
    g_free(str);

    g_value_unset(&val);
}

/* Print information regarding a stream */
void audio_preparation_svc::print_stream_info(GstDiscovererStreamInfo* info, gint depth) {
    gchar* desc = NULL;
    GstCaps* caps;
    const GstTagList* tags;

    caps = gst_discoverer_stream_info_get_caps(info);

    if (caps) {
        if (gst_caps_is_fixed(caps))
            desc = gst_pb_utils_get_codec_description(caps);
        else
            desc = gst_caps_to_string(caps);
        gst_caps_unref(caps);
    }

    g_print("%*s%s: %s\n", 2 * depth, " ", gst_discoverer_stream_info_get_stream_type_nick(info), (desc ? desc : ""));

    if (desc) {
        g_free(desc);
        desc = NULL;
    }

    tags = gst_discoverer_stream_info_get_tags(info);
    if (tags) {
        g_print("%*sTags:\n", 2 * (depth + 1), " ");
        gst_tag_list_foreach(tags, print_tag_foreach, GINT_TO_POINTER(depth + 2));
    }
}

/* Print information regarding a stream and its substreams, if any */
void audio_preparation_svc::print_topology(GstDiscovererStreamInfo* info, gint depth) {
    GstDiscovererStreamInfo* next;

    if (!info)
        return;

    print_stream_info(info, depth);

    next = gst_discoverer_stream_info_get_next(info);
    if (next) {
        print_topology(next, depth + 1);
        gst_discoverer_stream_info_unref(next);
    }
    else if (GST_IS_DISCOVERER_CONTAINER_INFO(info)) {
        GList* tmp, * streams;

        streams = gst_discoverer_container_info_get_streams(GST_DISCOVERER_CONTAINER_INFO(info));
        for (tmp = streams; tmp; tmp = tmp->next) {
            GstDiscovererStreamInfo* tmpinf = (GstDiscovererStreamInfo*)tmp->data;
            print_topology(tmpinf, depth + 1);
        }
        gst_discoverer_stream_info_list_free(streams);
    }
}










