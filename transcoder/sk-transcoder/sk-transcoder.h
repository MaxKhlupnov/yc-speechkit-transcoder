// sk-transcoder.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include "sk-discoverer.h"

DiscovererData discovery{};

/* Structure to contain all our information, so we can pass it to callbacks */
struct CustomData {
    GstElement* pipeline;
    GstElement* source;
    GstElement* convert;
    GstElement* resample;
    GstElement* sink;
};
