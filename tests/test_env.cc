#include "easy/base/Env.h"

#include <unistd.h>
#include <iostream>

int main(int argc, char** argv) {
  std::cout << "argc=" << argc << std::endl;
  easy::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
  easy::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
  easy::EnvMgr::GetInstance()->addHelp("p", "print help");
  if (!easy::EnvMgr::GetInstance()->init(argc, argv)) {
    easy::EnvMgr::GetInstance()->printHelp();
    return 0;
  }

  std::cout << "exe=" << easy::EnvMgr::GetInstance()->exe() << std::endl;
  std::cout << "cwd=" << easy::EnvMgr::GetInstance()->cwd() << std::endl;

  std::cout << "path=" << easy::EnvMgr::GetInstance()->env("PATH", "xxx")
            << std::endl;
  std::cout << "test=" << easy::EnvMgr::GetInstance()->env("TEST", "")
            << std::endl;
  std::cout << "set env " << easy::EnvMgr::GetInstance()->setEnv("TEST", "test")
            << std::endl;
  std::cout << "test=" << easy::EnvMgr::GetInstance()->env("TEST", "")
            << std::endl;
  if (easy::EnvMgr::GetInstance()->has("p")) {
    easy::EnvMgr::GetInstance()->printHelp();
  }
  return 0;
}
