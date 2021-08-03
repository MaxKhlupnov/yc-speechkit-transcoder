#pragma once
#include <string.h>
#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

/* Structure to contain all our information, so we can pass it around */
typedef struct _DiscovererData {
	GstDiscoverer* discoverer;
	GMainLoop* loop;
} DiscovererData;


void print_tag_foreach(const GstTagList* tags, const gchar* tag, gpointer user_data);

void print_stream_info(GstDiscovererStreamInfo* info, gint depth);

void print_topology(GstDiscovererStreamInfo* info, gint depth);

