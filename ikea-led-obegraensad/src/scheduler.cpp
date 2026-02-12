#include "scheduler.h"
#include "websocket.h"

PluginScheduler &PluginScheduler::getInstance()
{
  static PluginScheduler instance;
  return instance;
}

void PluginScheduler::addItem(int pluginId, unsigned long durationSeconds)
{
  // Limit schedule size to prevent memory exhaustion (max 256 items)
  if (schedule.size() >= 256)
  {
    Serial.println("[Scheduler] Schedule limit reached (256 items), skipping new item");
    return;
  }

  // Limit duration to prevent integer overflow (max ~49 days)
  if (durationSeconds > 4294967) 
  {
    Serial.print("[Scheduler] Duration too large, capping to max value. Requested: ");
    Serial.print(durationSeconds);
    durationSeconds = 4294967;
  }
  
  ScheduleItem item = {
      .pluginId = pluginId,
      .duration = durationSeconds * 1000 // Convert to milliseconds
  };
  schedule.push_back(item);
}

void PluginScheduler::clearSchedule(bool emptyStorage)
{
  Serial.print("Clearing schedule, emptyStorage=");
  Serial.println(emptyStorage);
  
  currentIndex = 0;
  isActive = false;
#ifdef ENABLE_STORAGE
  if (emptyStorage)
  {
    schedule.clear();
    storage.begin("led-wall");
    storage.putString("schedule", "");
    storage.putInt("scheduleactive", 0);
    storage.end();
  }
#else
  if (emptyStorage)
  {
    schedule.clear();
  }
#endif
}

void PluginScheduler::start()
{
  if (!schedule.empty())
  {
    currentIndex = 0;
    lastSwitch = millis();
    isActive = true;
    requestPersist();
#ifdef ENABLE_STORAGE
    storage.begin("led-wall", false);
    storage.putInt("schedidx", 0);
    storage.end();
#endif
    switchToCurrentPlugin();
  }
}

void PluginScheduler::stop()
{
  isActive = false;
  requestPersist();
}

void PluginScheduler::requestPersist()
{
  needsPersist = true;
  lastPersistRequest = millis();
}

void PluginScheduler::checkAndPersist()
{
#ifdef ENABLE_STORAGE
  if (needsPersist && (millis() - lastPersistRequest >= PERSIST_DELAY_MS))
  {
    storage.begin("led-wall", false);
    storage.putInt("scheduleactive", isActive ? 1 : 0);
    storage.end();
    needsPersist = false;
  }
#else
  needsPersist = false;
#endif
}

void PluginScheduler::update()
{
  checkAndPersist();

  if (!isActive || schedule.empty())
    return;

  unsigned long currentTime = millis();
  
  // Safety check for bounds
  if (currentIndex >= schedule.size()) {
    Serial.println("[Scheduler] currentIndex out of bounds, resetting");
    currentIndex = 0;
    lastSwitch = currentTime;
    return;
  }
  
  // Check if it's time to switch to next plugin
  if (currentTime - lastSwitch >= schedule[currentIndex].duration)
  {
    currentIndex = (currentIndex + 1) % schedule.size();
    lastSwitch = currentTime;
    switchToCurrentPlugin();
  }
}

void PluginScheduler::switchToCurrentPlugin()
{
  Serial.print("[Scheduler] Switching to index ");
  Serial.print(currentIndex);
  Serial.print("/");
  Serial.print(schedule.size());
  Serial.print(", plugin ID: ");

  if (currentIndex < schedule.size())
  {
    Serial.println(schedule[currentIndex].pluginId);

    // Save currentIndex to NVS so schedule survives crashes/reboots
#ifdef ENABLE_STORAGE
    storage.begin("led-wall", false);
    storage.putInt("schedidx", (int)currentIndex);
    storage.end();
#endif

    pluginManager.setActivePluginById(schedule[currentIndex].pluginId);
#ifdef ENABLE_SERVER
    sendInfo();
#endif
  }
  else
  {
    Serial.println("Invalid index!");
  }
}

void PluginScheduler::init()
{
#ifdef ENABLE_STORAGE
  storage.begin("led-wall", true);
  int storedActive = storage.getInt("scheduleactive", 0);
  int storedIndex = storage.getInt("schedidx", 0);
  bool scheduleIsSet = setScheduleByJSONString(storage.getString("schedule"));

  isActive = (storedActive == 1);
  storage.end();

  if (isActive && !schedule.empty())
  {
    // Restore saved index (survives crashes/reboots)
    if (storedIndex >= 0 && storedIndex < (int)schedule.size())
    {
      currentIndex = (size_t)storedIndex;
      Serial.printf("[Scheduler] Resuming from index %d/%d\n",
                    currentIndex, schedule.size());
    }
    else
    {
      currentIndex = 0;
      Serial.println("[Scheduler] Stored index invalid, starting from 0");
    }
    lastSwitch = millis();
    switchToCurrentPlugin();
  }
#endif
}

bool PluginScheduler::setScheduleByJSONString(String scheduleJson)
{
  if (scheduleJson.length() == 0)
  {
    return false;
  }
  
  JsonDocument doc;  // Dynamic allocation (ArduinoJson v7)
  DeserializationError error = deserializeJson(doc, scheduleJson);
  
  if (error)
  {
    Serial.print("[Scheduler] JSON parsing failed: ");
    Serial.println(error.c_str());
    return false;
  }

  // Clear old schedule
  currentIndex = 0;
  isActive = false;
  schedule.clear();

#ifdef ENABLE_STORAGE
  storage.begin("led-wall");
  storage.putString("schedule", scheduleJson);
  storage.end();
#endif

  Serial.print("[Scheduler] Parsing schedule, items: ");
  Serial.println(doc.as<JsonArray>().size());
  
  for (const auto &item : doc.as<JsonArray>())
  {
    if (item["pluginId"].is<int>() && item["duration"].is<unsigned long>())
    {
      int pluginId = item["pluginId"].as<int>();
      unsigned long duration = item["duration"].as<unsigned long>();
      addItem(pluginId, duration);
    }
  }
  
  Serial.print("[Scheduler] Total items loaded: ");
  Serial.println(schedule.size());
  return !schedule.empty();
}

PluginScheduler &Scheduler = PluginScheduler::getInstance();
