#include "easy/base/Config.h"
#include "easy/base/Env.h"
#include "easy/base/Logger.h"

#include <yaml-cpp/yaml.h>
#include <iostream>

easy::ConfigVar<int>::ptr g_int_value_config =
    easy::Config::Lookup("system.port", static_cast<int>(8080), "system port");

easy::ConfigVar<float>::ptr g_int_valuex_config =
    easy::Config::Lookup("system.port",
                         static_cast<float>(8080),
                         "system port");

easy::ConfigVar<float>::ptr g_float_value_config =
    easy::Config::Lookup("system.value", 10.2f, "system value");

easy::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config =
    easy::Config::Lookup("system.int_vec",
                         std::vector<int>{1, 2},
                         "system int vec");

easy::ConfigVar<std::list<int>>::ptr g_int_list_value_config =
    easy::Config::Lookup("system.int_list",
                         std::list<int>{1, 2},
                         "system int list");

easy::ConfigVar<std::set<int>>::ptr g_int_set_value_config =
    easy::Config::Lookup("system.int_set",
                         std::set<int>{1, 2},
                         "system int set");

easy::ConfigVar<std::unordered_set<int>>::ptr g_int_uset_value_config =
    easy::Config::Lookup("system.int_uset",
                         std::unordered_set<int>{1, 2},
                         "system int uset");

easy::ConfigVar<std::map<std::string, int>>::ptr g_str_int_map_value_config =
    easy::Config::Lookup("system.str_int_map",
                         std::map<std::string, int>{{"k", 2}},
                         "system str int map");

easy::ConfigVar<std::unordered_map<std::string, int>>::ptr
    g_str_int_umap_value_config =
        easy::Config::Lookup("system.str_int_umap",
                             std::unordered_map<std::string, int>{{"k", 2}},
                             "system str int map");

void test_loadconf()
{
  easy::Config::LoadFromConfDir("conf");
}

int main(int argc, char **argv)
{
  easy::EnvMgr::GetInstance()->init(argc, argv);
  test_loadconf();
  std::cout << " ==== " << std::endl;
  sleep(1);
  test_loadconf();
  easy::Config::Visit(
      [](easy::ConfigVarBase::ptr var)
      {
        EASY_LOG_INFO(EASY_LOG_ROOT())
            << "name=" << var->name()
            << " description=" << var->description()
            << " typename=" << var->typeName()
            << " value=" << var->toString();
      });

  return 0;
}
