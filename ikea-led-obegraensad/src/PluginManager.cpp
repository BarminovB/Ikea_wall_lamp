#include "PluginManager.h"
#include "scheduler.h"

Plugin::Plugin() : id(-1)
{
}

void Plugin::setId(int id)
{
  this->id = id;
}

int Plugin::getId() const
{
  return id;
}

void Plugin::teardown()
{
}
void Plugin::loop()
{
}
void Plugin::websocketHook(JsonDocument &request)
{
}

PluginManager::PluginManager() : nextPluginId(1)
{
}

PluginManager::~PluginManager()
{
  for (Plugin *plugin : plugins)
  {
    if (plugin)
    {
      delete plugin;
    }
  }
  plugins.clear();
}

void PluginManager::init()
{
  Screen.clear();
  Serial.print("[PluginManager] Initializing with ");
  Serial.print(getNumPlugins());
  Serial.println(" plugins");
  
  activatePersistedPlugin();
  
  if (!activePlugin)
  {
    Serial.println("[PluginManager] CRITICAL: No active plugin after initialization!");
  }
}

void PluginManager::renderPluginId(int pluginId)
{
  if (Scheduler.isActive)
  {
    return;
  }

  Screen.clear();

  std::vector<int> digits;

  if (pluginId >= 10)
  {
    digits.push_back((pluginId - pluginId % 10) / 10);
    digits.push_back(pluginId % 10);
  }
  else
  {
    digits.push_back(pluginId);
  }

  if (pluginId >= 10)
  {
    Screen.drawNumbers(3, 6, digits, MAX_BRIGHTNESS);
  }
  else
  {
    Screen.drawNumbers(6, 6, digits, MAX_BRIGHTNESS);
  }

  // Non-blocking delay using vTaskDelay (ESP32) or delay (ESP8266)
  // Avoids millis() overflow issues by using interval counting method
  int iterations = 80; // 800ms / 10ms per iteration
  while (iterations > 0)
  {
    yield();
#ifdef ESP32
    vTaskDelay(pdMS_TO_TICKS(10));
#else
    delay(10);
#endif
    iterations--;
  }
}

void PluginManager::activatePersistedPlugin()
{
  std::vector<Plugin *> &allPlugins = pluginManager.getAllPlugins();
  if (allPlugins.empty())
  {
    Serial.println("[PluginManager] No plugins registered!");
    return;
  }
#ifdef ENABLE_STORAGE
  storage.begin("led-wall", true);
  persistedPluginId = storage.getInt("current-plugin", allPlugins.at(0)->getId());
  storage.end();
  pluginManager.setActivePluginById(persistedPluginId);
#else
  pluginManager.setActivePluginById(allPlugins.at(0)->getId());
#endif
  if (!activePlugin)
  {
    Serial.println("[PluginManager] Failed to activate persisted plugin, activating first plugin");
    pluginManager.setActivePluginById(allPlugins.at(0)->getId());
  }
}

void PluginManager::persistActivePlugin()
{
#ifdef ENABLE_STORAGE
  storage.begin("led-wall", false);
  if (activePlugin)
  {
    persistedPluginId = activePlugin->getId();
    storage.putInt("current-plugin", persistedPluginId);
  }
  storage.end();
#endif
}

int PluginManager::getPersistedPluginId()
{
  std::vector<Plugin *> &allPlugins = pluginManager.getAllPlugins();
  if (allPlugins.empty())
  {
    Serial.println("[PluginManager] No plugins registered!");
    return -1;
  }
#ifdef ENABLE_STORAGE
  storage.begin("led-wall", true);
  persistedPluginId = storage.getInt("current-plugin", allPlugins.at(0)->getId());
  storage.end();
  return persistedPluginId;
#else
  return -1;
#endif
}

int PluginManager::addPlugin(Plugin *plugin)
{

  plugin->setId(nextPluginId++);
  plugins.push_back(plugin);
  return plugin->getId();
}

void PluginManager::setActivePlugin(const char *pluginName)
{
  Serial.print("Setting active plugin: ");
  Serial.println(pluginName);

#ifdef ESP32
  Serial.printf("[PluginSwitch] Heap before: free=%u maxBlock=%u\n",
                ESP.getFreeHeap(), ESP.getMaxAllocHeap());
#endif

  currentStatus = LOADING; // Block Core 0 FIRST to prevent race condition

  if (activePlugin)
  {
    Serial.print("[PluginSwitch] Tearing down: ");
    Serial.println(activePlugin->getName());
    activePlugin->teardown();
    activePlugin = nullptr;
  }

#ifdef ESP32
  Serial.printf("[PluginSwitch] Heap after teardown: free=%u maxBlock=%u\n",
                ESP.getFreeHeap(), ESP.getMaxAllocHeap());
#endif

  for (Plugin *plugin : plugins)
  {
    if (strcmp(plugin->getName(), pluginName) == 0)
    {
      activePlugin = plugin;
      renderPluginId(activePlugin->getId());
      Serial.print("[PluginSwitch] Setting up: ");
      Serial.println(pluginName);
      activePlugin->setup();
      break;
    }
  }

#ifdef ESP32
  Serial.printf("[PluginSwitch] Heap after setup: free=%u maxBlock=%u\n",
                ESP.getFreeHeap(), ESP.getMaxAllocHeap());
#endif

  currentStatus = NONE; // Unblock Core 0
}

void PluginManager::setActivePluginById(int pluginId)
{
  Serial.print("Setting active plugin by ID: ");
  Serial.println(pluginId);
  
  for (Plugin *plugin : plugins)
  {
    if (plugin->getId() == pluginId)
    {
      Serial.print("Found plugin with ID ");
      Serial.print(pluginId);
      Serial.print(", name: ");
      Serial.println(plugin->getName());
      setActivePlugin(plugin->getName());
      return;
    }
  }
  
  Serial.print("Plugin with ID ");
  Serial.print(pluginId);
  Serial.println(" not found!");
  
  // Fallback: activate first plugin if requested one not found
  if (!plugins.empty() && !activePlugin)
  {
    Serial.println("[PluginManager] Activating first plugin as fallback");
    setActivePluginById(plugins.at(0)->getId());
  }
}

void PluginManager::setupActivePlugin()
{
  if (activePlugin)
  {
    renderPluginId(activePlugin->getId());
    activePlugin->setup();
  }
}

void PluginManager::runActivePlugin()
{
  if (activePlugin && currentStatus != UPDATE && currentStatus != LOADING &&
      currentStatus != WSBINARY)
  {
    activePlugin->loop();
  }
}

Plugin *PluginManager::getActivePlugin() const
{
  return activePlugin;
}

std::vector<Plugin *> &PluginManager::getAllPlugins()
{
  return plugins;
}

size_t PluginManager::getNumPlugins()
{
  return plugins.size();
}

void PluginManager::activateNextPlugin()
{
  if (activePlugin)
  {
    if (activePlugin->getId() <= getNumPlugins() - 1)
    {
      setActivePluginById(activePlugin->getId() + 1);
    }
    else
    {
      setActivePluginById(1);
    }
  }
  else
  {
    setActivePluginById(1);
  }
#ifdef ENABLE_SERVER
  sendInfo();
#endif
}