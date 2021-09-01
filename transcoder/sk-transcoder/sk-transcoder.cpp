#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>
#include "sk-transcoder.h"

// gRPC call

#include <iostream>
#include <string>
#include <thread>
#include <sstream>


#include "yandex/cloud/operation/operation_service.grpc.pb.h"
#include "yandex/cloud/ai/stt/v2/stt_service.grpc.pb.h"
#include "yandex/cloud/ai/stt/v2/stt_service.pb.h"
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/create_channel.h>

using yandex::cloud::ai::stt::v2::LongRunningRecognitionRequest;
using yandex::cloud::ai::stt::v2::LongRunningRecognitionResponse;
using yandex::cloud::ai::stt::v2::RecognitionSpec;
using yandex::cloud::ai::stt::v2::RecognitionSpec_AudioEncoding;
using yandex::cloud::ai::stt::v2::RecognitionConfig;
using yandex::cloud::ai::stt::v2::RecognitionAudio;
using yandex::cloud::operation::Operation;
// services
using yandex::cloud::ai::stt::v2::SttService;
using yandex::cloud::operation::OperationService;

using grpc::CreateChannel;
using grpc::SslCredentials;

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

grpc::ClientContext* make_asr_client_context(){

    grpc::ClientContext* context = new grpc::ClientContext();
    std::string str_bearer = "Bearer t1.";
    context->AddMetadata("authorization", str_bearer.c_str());
    context->AddMetadata("x-data-logging-enabled", "true");
    return context;
}

const char* make_asr_task(std::unique_ptr<SttService::Stub> speech) {

    const char* language_code = "ru-RU";
    const char* model = "general";

    RecognitionSpec* asr_spec = new RecognitionSpec();
    asr_spec->set_language_code(language_code);
    asr_spec->set_audio_encoding(RecognitionSpec_AudioEncoding::RecognitionSpec_AudioEncoding_OGG_OPUS);
    asr_spec->set_sample_rate_hertz(48000);
    asr_spec->set_model(model);
    asr_spec->set_partial_results(false);

    // init recognition config
    RecognitionConfig* asr_config = new RecognitionConfig();
    asr_config->set_allocated_specification(asr_spec);

    // init audio config
    std::string uri = "https://storage.yandexcloud.net/vera/filipp.wav";
    RecognitionAudio* asr_audio = new RecognitionAudio();
    asr_audio->set_allocated_uri(&uri);

    LongRunningRecognitionRequest request{};
    request.set_allocated_audio(asr_audio);
    request.set_allocated_config(asr_config);

    Operation* op = new Operation();

    grpc::ClientContext* con = make_asr_client_context();


    grpc::Status rpc_status = speech->LongRunningRecognize (con, request, op);//(context, request, op);
    const char* asr_task_id;
    if (!rpc_status.ok()) {
        g_print("RPC status '%d'\n", rpc_status.error_code());
        g_print("RPC error message: '%s'\n", rpc_status.error_message().c_str());

    }else{
        asr_task_id = op->id().c_str();
        g_print("RPC call completed successfully. asr_task_id '%s'\n", op->id().c_str());
    }

    request.release_config();
    request.release_audio();

    // release resources
    if (asr_audio != NULL){
        asr_audio->release_uri();
    }

    if (asr_config != NULL){
        asr_config->release_folder_id();
        asr_config->release_specification();
    }
    return asr_task_id;

}

void collect_asr_task_result(std::unique_ptr<OperationService::Stub> asr_task_processing, const char* asr_task_id, LongRunningRecognitionResponse* asr_task_response){

    yandex::cloud::operation::GetOperationRequest asr_task_result_request{};
    asr_task_result_request.set_operation_id(asr_task_id);


    Operation* op = new Operation();
    while(!op->done()){

        g_print("Waiting for asr task '%s' to complete.\n", asr_task_id);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        grpc::ClientContext* context = make_asr_client_context();
        grpc::Status  rpc_status = asr_task_processing->Get(context,asr_task_result_request, op);

        if (!rpc_status.ok()){
            g_print("RPC failure '%s'\n", rpc_status.error_message().c_str());
            return; // error code
        }
    }

    // Extract the response payload.

    if (!op->response().Is<LongRunningRecognitionResponse>()){
        g_print("RPC call for asr task '%s' completed successfully, but didn't return LongRunningRecognitionResponse in payload\n", asr_task_id);

    }else {
        op->response().UnpackTo(asr_task_response);
        g_print("RPC call for asr task '%s' completed successfully. '%ld' bytes received\n", asr_task_id,  asr_task_response->ByteSizeLong());
    }

}


int main(int argc, char* argv[])
{

    const grpc::SslCredentialsOptions credOpt;
    auto channelCreds =  grpc::SslCredentials(credOpt);

    auto asr_task_channel = grpc::CreateChannel("stt.api.cloud.yandex.net:443", channelCreds); //

    std::unique_ptr<SttService::Stub> speech(SttService::NewStub(asr_task_channel));

    const char* asr_task_id = make_asr_task(std::move(speech));
    // Create a long operations stub so we can check the progress of the async request.
    auto asr_task_result_channel = grpc::CreateChannel("operation.api.cloud.yandex.net:443", channelCreds);
    std::unique_ptr<OperationService::Stub> asr_task_processing(OperationService::NewStub(asr_task_result_channel) );

    LongRunningRecognitionResponse response;
    collect_asr_task_result(std::move(asr_task_processing), asr_task_id, &response);
    return 0;

    GstBus* bus;
    GstMessage* msg;
    GError* err = NULL;

    

   // gchar const* uri = "https://storage.yandexcloud.net/m24-speech/01%20Back%20in%20Black.mp3";
        // "https://rockthecradle.stream.publicradio.org/rockthecradle.mp3"; --mp3 stream
        //
        //"https://storage.yandexcloud.net/audio-data/BrandAnalytics/2021-05-03-osoboe-1907-sd-3448912.wav";
        //"https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm";
    //"https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm";
    
    std::string uri = "https://storage.yandexcloud.net/m24-speech/01%20Back%20in%20Black.mp3";;

    /* if a URI was provided, use it instead of the default one */
    if (argc > 1) {
        uri = argv[1];
    }

    /* Initialize cumstom data structure */
    memset(&discovery, 0, sizeof(discovery));

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    g_print("Discovering '%s'\n", uri.c_str());

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
    if (!gst_discoverer_discover_uri_async(discovery.discoverer, uri.c_str())) {
        g_print("Failed to start discovering URI '%s'\n", uri.c_str());
        g_object_unref(discovery.discoverer);
        return -1;
    }

    

    /*
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

    std::string str_pipeline = "souphttpsrc location = \"" + uri 
        + "\" !decodebin !audioconvert !audioresample  quality = 10 !capsfilter caps = \"audio/x-raw,format=S16LE,channels=1,rate=48000\" ! wavenc ! s3sink bucket=\"s3-gst-plugin\"  key=\"audio.wav\" aws-sdk-endpoint=\"storage.yandexcloud.net:443\" content-type=\"audio/wav\"";

    GstElement* pipeline =  gst_parse_launch(str_pipeline.c_str(), NULL);

    guint watch_id;
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));

    watch_id = gst_bus_add_watch(bus, bus_call, loop);
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
