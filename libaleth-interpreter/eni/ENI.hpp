#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fts.h>
#include <elf.h>

#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <cstdlib>
#include <cstdint>
#include <cstdlib>
#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "fork_call.h"

boost::filesystem::path DefaultDataDir()
{
    auto dir = getenv("HOME");
    if (not dir)
    {
        return "";
    }
    return dir;
}

boost::filesystem::path getLibPath()
{
    return DefaultDataDir() / "eni" / "lib";
}

std::vector<boost::filesystem::path> getEniSharedObjects() {
    auto libPath = getLibPath();
    {
        char* val = getenv("ENI_LIBRARY_PATH");
        if (val)
        {
            libPath = val;
        }
    }

    std::vector<boost::filesystem::path> sharedObjects;

    for (const auto& child: boost::filesystem::recursive_directory_iterator(libPath)) {
        auto path = child.path();
        if (child.status().type() == boost::filesystem::regular_file and path.extension() == ".so") {
            sharedObjects.push_back(child.path());
        }
    }

    return sharedObjects;
}

class ENI
{
public:
    void InitENI(const std::string& eniFunction, const std::string& argsText)
    {
        std::string gasSym = eniFunction + "_gas";
        std::string runSym = eniFunction + "_run";

        auto sharedObjects = getEniSharedObjects();

        if (NULL == dlopen("libeni.so", RTLD_LAZY)) {
            std::cerr << "warning: cannot find libeni.so, trying to find it in automatically\n";
            for (const auto& path: sharedObjects) {
                if (path.filename() == "libeni.so") {
                    if (dlopen(path.c_str(), RTLD_LAZY) != NULL) {
                        std::cerr << "successfully loaded " << path << "\n";
                        break;
                    }
                }
            }
        }

        for (const auto& path: sharedObjects) {
            void* handler = dlopen(path.c_str(), RTLD_LAZY);
            if (not handler) {
                throw std::runtime_error(boost::str(boost::format("dlopen failed: %s\nError: %s") % path % dlerror()));
            }

            gasFunc = dlsym(handler, gasSym.c_str());
            if (not gasFunc) {
                continue;
            }

            runFunc = dlsym(handler, runSym.c_str());
            if (not runFunc) {
                throw std::runtime_error(boost::str(boost::format("dlsym failed: %s\nError: %s") % runSym % dlerror()));
            }

            opName = eniFunction;
            this->argsText = argsText;
            return;
        }
        throw std::runtime_error(boost::str(boost::format("symbol not found: '%s'") % gasSym));
    }

    uint64_t Gas()
    {
        int status = ENI_FAILURE;
        char* argsTextCopy = strdup(argsText.c_str());
        uint64_t gas = fork_gas(gasFunc, argsTextCopy, &status);
        if (status != 0)
        {
            std::runtime_error(boost::str(boost::format("ENI %s gas error, status=%d") % opName % status));
        }
        return gas;
    }

    std::string ExecuteENI()
    {
        int status = ENI_FAILURE;
        char* argsTextCopy = strdup(argsText.c_str());
        char* retCStr = fork_run(runFunc, argsTextCopy, &status);

        if (status != 0) {
            free(argsTextCopy);
            free(retCStr);
            throw std::runtime_error(boost::str(boost::format("ENI %s run error, status=%d") % opName % status));
        }

        std::string ret = retCStr;
        free(argsTextCopy);
        free(retCStr);
        return ret;
    }

private:
    std::string opName;
    void* gasFunc = 0;
    void* runFunc = 0;
    std::string argsText;
    std::string retText;
};
