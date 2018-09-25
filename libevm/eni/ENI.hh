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

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "fork_call.hh"

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

std::vector<std::string> elf_DynamicSymbols(std::string elfPath)
{
    return {};
    // TODO
}

std::map<std::string, std::string> getEniFunctions()
{
    // if runtime.GOOS != "linux" {
    // 	return nil, errors.New("currently ENI is only supported on Linux")
    // }

    auto libPath = getLibPath();
    {
        char* val = getenv("ENI_LIBRARY_PATH");
        if (val)
        {
            libPath = val;
        }
    }

    std::vector<std::string> dynamicLibs;

    char* const fts_arg0[] = {strdupa(libPath.c_str()), 0};
    FTS* fts_handle = fts_open(fts_arg0, FTS_COMFOLLOW | FTS_PHYSICAL, 0);
    FTSENT* parent;
    FTSENT* child;
    while ((parent = fts_read(fts_handle)))
    {
        child = fts_children(fts_handle, 0);
        while (child)
        {
            child = child->fts_link;
            if (child->fts_statp->st_mode == 0 and
                std::string(".so") == (child->fts_name + child->fts_namelen - 3))
            {
                dynamicLibs.emplace_back(child->fts_path);
            }
        }
    }

    std::map<std::string, std::string> functions;
    for (auto& lib : dynamicLibs)
    {
        for (auto& symbol : elf_DynamicSymbols(lib))
        {
            functions[symbol] = lib;
        }
    }

    return functions;
}


class ENI
{
public:
    void InitENI(std::string eniFunction, std::string argsText)
    {
        functions = getEniFunctions();

        std::string gasSym = eniFunction + "_gas";
        std::string runSym = eniFunction + "_run";

        std::string dynamicLibName = functions[gasSym];
        void* handler = dlopen(dynamicLibName.c_str(), RTLD_LAZY);
        if (not handler)
        {
            throw "dlopen failed: ";
        }

        void* gasFunc = dlsym(handler, gasSym.c_str());
        if (not gasFunc)
        {
            throw "dlsym failed: ";
        }

        void* runFunc = dlsym(handler, runSym.c_str());
        if (not runFunc)
        {
            throw "dlsym failed: ";
        }

        opName = eniFunction;
        argsText = argsText;
    }

    uint64_t Gas()
    {
        int status = 87;
        uint64_t gas = fork_gas(gasFunc, argsText.c_str(), &status);
        if (status != 0)
        {
            throw "ENI gas error status";
        }
        return gas;
    }

    std::string ExecuteENI()
    {
        int status = 87;
        char* retCStr = fork_run(runFunc, argsText.c_str(), &status);
        std::string ret = retCStr;
        free(retCStr);
        if (status != 0) {
            throw std::runtime_error(boost::str(boost::format("ENI %s run error, status=%d") % opName % status));
        }
        return ret;
    }

private:
    std::map<std::string, std::string> functions;
    std::string opName;
    void* gasFunc;
    void* runFunc;
    std::string argsText;
    std::string retText;
};
