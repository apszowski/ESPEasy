
// ********************************************************************************
// Web Interface JSON page (no password!)
// ********************************************************************************
void handle_json()
{
  const int  taskNr           = getFormItemInt(F("tasknr"), -1);
  const bool showSpecificTask = taskNr > 0;
  bool showSystem             = true;
  bool showWifi               = true;
  bool showDataAcquisition    = true;
  bool showTaskDetails        = true;
  bool showNodes              = true;
  {
    String view = WebServer.arg("view");

    if (view.length() != 0) {
      if (view == F("sensorupdate")) {
        showSystem          = false;
        showWifi            = false;
        showDataAcquisition = false;
        showTaskDetails     = false;
        showNodes           = false;
      }
    }
  }

  TXBuffer.startJsonStream();

  if (!showSpecificTask)
  {
    TXBuffer += '{';

    if (showSystem) {
      TXBuffer += F("\"System\":{\n");
      stream_next_json_object_value(LabelType::BUILD_DESC);
      stream_next_json_object_value(LabelType::GIT_BUILD);
      stream_next_json_object_value(LabelType::SYSTEM_LIBRARIES);
      stream_next_json_object_value(LabelType::PLUGINS);
      stream_next_json_object_value(LabelType::PLUGIN_DESCRIPTION);
      stream_next_json_object_value(LabelType::LOCAL_TIME);
      stream_next_json_object_value(LabelType::UNIT_NR);
      stream_next_json_object_value(LabelType::UNIT_NAME);
      stream_next_json_object_value(LabelType::UPTIME);
      stream_next_json_object_value(LabelType::BOOT_TYPE);
      stream_next_json_object_value(LabelType::RESET_REASON);

      if (wdcounter > 0)
      {
        stream_next_json_object_value(LabelType::LOAD_PCT);
        stream_next_json_object_value(LabelType::LOOP_COUNT);
      }
      stream_next_json_object_value(LabelType::CPU_ECO_MODE);

      #ifdef CORE_POST_2_5_0
      stream_next_json_object_value(LabelType::HEAP_MAX_FREE_BLOCK);
      stream_next_json_object_value(LabelType::HEAP_FRAGMENTATION);
      #endif // ifdef CORE_POST_2_5_0
      stream_last_json_object_value(LabelType::FREE_MEM);
      TXBuffer += ",\n";
    }

    if (showWifi) {
      TXBuffer += F("\"WiFi\":{\n");
      #if defined(ESP8266)
      stream_next_json_object_value(LabelType::HOST_NAME);
      #endif // if defined(ESP8266)
      stream_next_json_object_value(LabelType::IP_CONFIG);
      stream_next_json_object_value(LabelType::IP_ADDRESS);
      stream_next_json_object_value(LabelType::IP_SUBNET);
      stream_next_json_object_value(LabelType::GATEWAY);
      stream_next_json_object_value(LabelType::STA_MAC);
      stream_next_json_object_value(LabelType::DNS_1);
      stream_next_json_object_value(LabelType::DNS_1);
      stream_next_json_object_value(LabelType::SSID);
      stream_next_json_object_value(LabelType::BSSID);
      stream_next_json_object_value(LabelType::CHANNEL);
      stream_next_json_object_value(LabelType::CONNECTED_MSEC);
      stream_next_json_object_value(LabelType::LAST_DISCONNECT_REASON);
      stream_next_json_object_value(LabelType::LAST_DISC_REASON_STR);
      stream_next_json_object_value(LabelType::NUMBER_RECONNECTS);
      stream_next_json_object_value(LabelType::FORCE_WIFI_BG);
      stream_next_json_object_value(LabelType::RESTART_WIFI_LOST_CONN);
#ifdef ESP8266
      stream_next_json_object_value(LabelType::FORCE_WIFI_NOSLEEP);
#endif // ifdef ESP8266
#ifdef SUPPORT_ARP
      stream_next_json_object_value(LabelType::PERIODICAL_GRAT_ARP);
#endif // ifdef SUPPORT_ARP
      stream_next_json_object_value(LabelType::CONNECTION_FAIL_THRESH);
      stream_last_json_object_value(LabelType::WIFI_RSSI);
      TXBuffer += ",\n";
    }

    if (showNodes) {
      bool comma_between = false;

      for (NodesMap::iterator it = Nodes.begin(); it != Nodes.end(); ++it)
      {
        if (it->second.ip[0] != 0)
        {
          if (comma_between) {
            TXBuffer += ',';
          } else {
            comma_between = true;
            TXBuffer     += F("\"nodes\":[\n"); // open json array if >0 nodes
          }

          TXBuffer += '{';
          stream_next_json_object_value(F("nr"), String(it->first));
          stream_next_json_object_value(F("name"),
                                        (it->first != Settings.Unit) ? it->second.nodeName : Settings.Name);

          if (it->second.build) {
            stream_next_json_object_value(F("build"), String(it->second.build));
          }

          if (it->second.nodeType) {
            String platform = getNodeTypeDisplayString(it->second.nodeType);

            if (platform.length() > 0) {
              stream_next_json_object_value(F("platform"), platform);
            }
          }
          stream_next_json_object_value(F("ip"), it->second.ip.toString());
          stream_last_json_object_value(F("age"), String(it->second.age));
        } // if node info exists
      }   // for loop

      if (comma_between) {
        TXBuffer += F("],\n"); // close array if >0 nodes
      }
    }
  }

  byte firstTaskIndex = 0;
  byte lastTaskIndex  = TASKS_MAX - 1;

  if (showSpecificTask)
  {
    firstTaskIndex = taskNr - 1;
    lastTaskIndex  = taskNr - 1;
  }
  byte lastActiveTaskIndex = 0;

  for (byte TaskIndex = firstTaskIndex; TaskIndex <= lastTaskIndex; TaskIndex++) {
    if (Settings.TaskDeviceNumber[TaskIndex]) {
      lastActiveTaskIndex = TaskIndex;
    }
  }

  if (!showSpecificTask) { TXBuffer += F("\"Sensors\":[\n"); }
  unsigned long ttl_json = 60; // The shortest interval per enabled task (with output values) in seconds

  for (byte TaskIndex = firstTaskIndex; TaskIndex <= lastActiveTaskIndex; TaskIndex++)
  {
    if (Settings.TaskDeviceNumber[TaskIndex])
    {
      byte DeviceIndex                 = getDeviceIndex(Settings.TaskDeviceNumber[TaskIndex]);
      const unsigned long taskInterval = Settings.TaskDeviceTimer[TaskIndex];
      LoadTaskSettings(TaskIndex);
      TXBuffer += F("{\n");

      // For simplicity, do the optional values first.
      if (Device[DeviceIndex].ValueCount != 0) {
        if ((ttl_json > taskInterval) && (taskInterval > 0) && Settings.TaskDeviceEnabled[TaskIndex]) {
          ttl_json = taskInterval;
        }
        TXBuffer += F("\"TaskValues\": [\n");

        for (byte x = 0; x < Device[DeviceIndex].ValueCount; x++)
        {
          TXBuffer += '{';
          stream_next_json_object_value(F("ValueNumber"), String(x + 1));
          stream_next_json_object_value(F("Name"),        String(ExtraTaskSettings.TaskDeviceValueNames[x]));
          stream_next_json_object_value(F("NrDecimals"),  String(ExtraTaskSettings.TaskDeviceValueDecimals[x]));
          stream_last_json_object_value(F("Value"), formatUserVarNoCheck(TaskIndex, x));

          if (x < (Device[DeviceIndex].ValueCount - 1)) {
            TXBuffer += ",\n";
          }
        }
        TXBuffer += F("],\n");
      }

      if (showSpecificTask) {
        stream_next_json_object_value(F("TTL"), String(ttl_json * 1000));
      }

      if (showDataAcquisition) {
        TXBuffer += F("\"DataAcquisition\": [\n");

        for (byte x = 0; x < CONTROLLER_MAX; x++)
        {
          TXBuffer += '{';
          stream_next_json_object_value(F("Controller"), String(x + 1));
          stream_next_json_object_value(F("IDX"),        String(Settings.TaskDeviceID[x][TaskIndex]));
          stream_last_json_object_value(F("Enabled"), jsonBool(Settings.TaskDeviceSendData[x][TaskIndex]));

          if (x < (CONTROLLER_MAX - 1)) {
            TXBuffer += ",\n";
          }
        }
        TXBuffer += F("],\n");
      }

      if (showTaskDetails) {
        stream_next_json_object_value(F("TaskInterval"),     String(taskInterval));
        stream_next_json_object_value(F("Type"),             getPluginNameFromDeviceIndex(DeviceIndex));
        stream_next_json_object_value(F("TaskName"),         String(ExtraTaskSettings.TaskDeviceName));
        stream_next_json_object_value(F("TaskDeviceNumber"), String(Settings.TaskDeviceNumber[TaskIndex]));
      }
      stream_next_json_object_value(F("TaskEnabled"), jsonBool(Settings.TaskDeviceEnabled[TaskIndex]));
      stream_last_json_object_value(F("TaskNumber"), String(TaskIndex + 1));

      if (TaskIndex != lastActiveTaskIndex) {
        TXBuffer += ',';
      }
      TXBuffer += '\n';
    }
  }

  if (!showSpecificTask) {
    TXBuffer += F("],\n");
    stream_last_json_object_value(F("TTL"), String(ttl_json * 1000));
  }

  TXBuffer.endStream();
}

// ********************************************************************************
// JSON formatted timing statistics
// ********************************************************************************

void stream_timing_stats_json(unsigned long count, unsigned long minVal, unsigned long maxVal, float avg) {
  stream_next_json_object_value(F("count"), String(count));
  stream_next_json_object_value(F("min"),   String(minVal));
  stream_next_json_object_value(F("max"),   String(maxVal));
  stream_next_json_object_value(F("avg"),   String(avg));
}

void stream_plugin_function_timing_stats_json(
  const String& object,
  unsigned long count, unsigned long minVal, unsigned long maxVal, float avg) {
  TXBuffer += "{\"";
  TXBuffer += object;
  TXBuffer += "\":{";
  stream_timing_stats_json(count, minVal, maxVal, avg);
  stream_last_json_object_value(F("unit"), F("usec"));
}

void stream_plugin_timing_stats_json(int pluginId) {
  String P_name = "";

  Plugin_ptr[pluginId](PLUGIN_GET_DEVICENAME, NULL, P_name);
  TXBuffer += '{';
  stream_next_json_object_value(F("name"), P_name);
  stream_next_json_object_value(F("id"),   String(pluginId));
  stream_json_start_array(F("function"));
}

void stream_json_start_array(const String& label) {
  TXBuffer += '\"';
  TXBuffer += label;
  TXBuffer += F("\": [\n");
}

void stream_json_end_array_element(bool isLast) {
  if (isLast) {
    TXBuffer += "]\n";
  } else {
    TXBuffer += ",\n";
  }
}

void stream_json_end_object_element(bool isLast) {
  TXBuffer += '}';

  if (!isLast) {
    TXBuffer += ',';
  }
  TXBuffer += '\n';
}

#ifdef WEBSERVER_NEW_UI
void handle_timingstats_json() {
  TXBuffer.startJsonStream();
  TXBuffer += '{';
  jsonStatistics(false);
  TXBuffer += '}';
  TXBuffer.endStream();
}

#endif // WEBSERVER_NEW_UI

#ifdef WEBSERVER_NEW_UI
void handle_nodes_list_json() {
  if (!isLoggedIn()) { return; }
  TXBuffer.startJsonStream();
  json_init();
  json_open(true);

  for (NodesMap::iterator it = Nodes.begin(); it != Nodes.end(); ++it)
  {
    if (it->second.ip[0] != 0)
    {
      json_open();
      bool isThisUnit = it->first == Settings.Unit;

      if (isThisUnit) {
        json_number(F("thisunit"), String(1));
      }

      json_number(F("first"), String(it->first));
      json_prop(F("name"), isThisUnit ? Settings.Name : it->second.nodeName);

      if (it->second.build) { json_prop(F("build"), String(it->second.build)); }
      json_prop(F("type"), getNodeTypeDisplayString(it->second.nodeType));
      json_prop(F("ip"),   it->second.ip.toString());
      json_number(F("age"), String(it->second.age));
      json_close();
    }
  }
  json_close(true);
  TXBuffer.endStream();
}

#endif // WEBSERVER_NEW_UI


/*********************************************************************************************\
   Streaming versions directly to TXBuffer
\*********************************************************************************************/
void stream_to_json_value(const String& value) {
  if ((value.length() == 0) || !isFloat(value)) {
    TXBuffer += '\"';
    TXBuffer += value;
    TXBuffer += '\"';
  } else {
    TXBuffer += value;
  }
}

void stream_to_json_object_value(const String& object, const String& value) {
  TXBuffer += '\"';
  TXBuffer += object;
  TXBuffer += "\":";
  stream_to_json_value(value);
}

String jsonBool(bool value) {
  return toString(value);
}

// Add JSON formatted data directly to the TXbuffer, including a trailing comma.
void stream_next_json_object_value(const String& object, const String& value) {
  TXBuffer += to_json_object_value(object, value);
  TXBuffer += ",\n";
}

// Add JSON formatted data directly to the TXbuffer, including a closing '}'
void stream_last_json_object_value(const String& object, const String& value) {
  TXBuffer += to_json_object_value(object, value);
  TXBuffer += "\n}";
}

void stream_next_json_object_value(LabelType::Enum label) {
  stream_next_json_object_value(getLabel(label), getValue(label));
}

void stream_last_json_object_value(LabelType::Enum label) {
  stream_last_json_object_value(getLabel(label), getValue(label));
}
