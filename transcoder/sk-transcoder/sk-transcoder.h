// sk-transcoder.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include "sk-discoverer.h"

void on_discovered_cb(GstDiscoverer* discoverer, GstDiscovererInfo* info, GError* err, DiscovererData* data);

void on_finished_cb(GstDiscoverer* discoverer, DiscovererData* data);

DiscovererData discovery{};

/* Structure to contain all our information, so we can pass it to callbacks */
struct CustomData {
    GstElement* pipeline;
    GstElement* source;
    GstElement* convert;
    GstElement* resample;
    GstElement* sink;
};
