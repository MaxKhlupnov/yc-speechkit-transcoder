// sk-transcoder.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include "sk-discoverer.h"

#include "yandex/cloud/ai/stt/v2/stt_service.pb.h"


void on_discovered_cb(GstDiscoverer* discoverer, GstDiscovererInfo* info, GError* err, DiscovererData* data);

void on_finished_cb(GstDiscoverer* discoverer, DiscovererData* data);

DiscovererData discovery{};

