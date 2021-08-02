#pragma once
#include <string.h>
#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

/* Structure to contain all our information, so we can pass it around */
typedef struct _DiscovererData {
	GstDiscoverer* discoverer;
	GMainLoop* loop;
} DiscovererData;


static void print_tag_foreach(const GstTagList* tags, const gchar* tag, gpointer user_data);

static void print_stream_info(GstDiscovererStreamInfo* info, gint depth);

static void print_topology(GstDiscovererStreamInfo* info, gint depth);

void on_discovered_cb(GstDiscoverer* discoverer, GstDiscovererInfo* info, GError* err, DiscovererData* data);

void on_finished_cb(GstDiscoverer* discoverer, DiscovererData* data);