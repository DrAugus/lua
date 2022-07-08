//
// main.cpp
// Created by AUGUS on 2022/7/8.
//

#include <string>
#include <utility>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include <mutex>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#ifndef _LUAJIT_ARM_
#include "luajit.h"
#endif
}

#include "ELuna.h"

#define ELUNA_ENGINE_VERSION_V_1_0   "V1.0"
#define VAR_NAME(x) #x

class Attribute {
public:
    Attribute() = default;

    ~Attribute() = default;

    Attribute(const Attribute &) = delete;

    Attribute &operator=(const Attribute &) = delete;

public:
    int a{};
    int b{};
    std::string s;
    std::vector<int> vec;
};

using attribute_sptr = std::shared_ptr<Attribute>;

using eluna_table_sptr = std::shared_ptr<ELuna::LuaTable>;

void Attribute2Table(const attribute_sptr &attribute, ELuna::LuaTable &lua_table, lua_State *p_lua) {
    if (nullptr == attribute || nullptr == p_lua) {
        std::cerr << "nullptr\n";
        return;
    }

    //
    lua_table.set(VAR_NAME(a), attribute->a);
    lua_table.set(VAR_NAME(b), attribute->b);
    lua_table.set(VAR_NAME(s), attribute->s);

    //vector
    auto f = [p_lua, &lua_table](std::string attr_name, std::vector<int> &attr_value_vec) {
        eluna_table_sptr inner_table = std::make_shared<ELuna::LuaTable>(p_lua);
        int i = 0;
        for (auto iter: attr_value_vec) {
            i++;
            inner_table->set(i, iter);
        }
        lua_table.set(std::move(attr_name), *inner_table);
    };
    f(VAR_NAME(vec), attribute->vec);
}


class LuaResult {
public:
    LuaResult() = default;

    LuaResult(const int type, std::string str) : result_type(type), result_str(std::move(str)) {};

    ~LuaResult() = default;

public:
    int32_t result_type = 0;
    std::string result_str;
};

using lua_result_sptr = std::shared_ptr<LuaResult>;

class LuaAllResults {
public:
    LuaAllResults() = default;

    ~LuaAllResults() = default;

public:
    std::vector<lua_result_sptr> results_vector;
};

using lua_all_results_sptr = std::shared_ptr<LuaAllResults>;


class LuaHandle {
public:
    explicit LuaHandle(std::string version);

    ~LuaHandle();

    int LoadLuaFile(const std::string &filepath);

    int LoadLuaScript(const std::string &scripts);

    void ResultAddition(LuaHandle &p);

    void ExtraAddResult(LuaHandle &p, const std::string &result_str, int result_type);

    lua_all_results_sptr Handle(const attribute_sptr &foo);

private:
    int Init();

    void UnInit();

private:
    std::string m_version_;
    void *m_lua_ = nullptr;
    std::mutex m_mut_;
    std::vector<lua_result_sptr> m_lua_result_vector_;
    lua_result_sptr m_lua_result_sptr_;
};

using lua_handle_sptr = std::shared_ptr<LuaHandle>;

LuaHandle::LuaHandle(std::string version)
        : m_version_(std::move(version)) {
}

LuaHandle::~LuaHandle() {
    UnInit();
}

int LuaHandle::Init() {
    if (nullptr == m_lua_) {
        m_lua_ = ELuna::openLua();
    }
    auto *p_lua = static_cast<lua_State *>(m_lua_);

    try {
        ELuna::registerClass<LuaHandle>(p_lua, "LuaHandle", ELuna::constructor<LuaHandle, std::string>);
        ELuna::registerMethod<LuaHandle>(p_lua, "ResultAddition", &LuaHandle::ResultAddition);
        ELuna::registerMethod<LuaHandle>(p_lua, "ExtraAddResult", &LuaHandle::ExtraAddResult);
    }
    catch (std::exception &e) {
        std::cerr << "lua register class or method error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}

void LuaHandle::UnInit() {
    if (nullptr != m_lua_)
        ELuna::closeLua(static_cast<lua_State *>(m_lua_));
}

int LuaHandle::LoadLuaFile(const std::string &filepath) {
    if (filepath.empty()) {
        std::cerr << "lua_engine load filepath empty!" << std::endl;
        return -1;
    }

    if (0 != Init()) {
        std::cerr << "eluna engine Init error!" << std::endl;
        return -1;
    }

    m_mut_.lock();
    auto *pLua = static_cast<lua_State *>(m_lua_);
    ELuna::doFile(pLua, filepath.c_str());

    return 0;
}

int LuaHandle::LoadLuaScript(const std::string &scripts) {
    if (scripts.empty()) {
        return -1;
    }

    std::ofstream outfile("/src/test.lua", std::ios_base::out | std::ios::binary);
    if (outfile.is_open()) {
        outfile.write(scripts.c_str(), scripts.size());
        outfile.close();
        return LoadLuaFile("/src/test.lua");
    }
    return -1;
}

void LuaHandle::ResultAddition(LuaHandle &p) {
    if (nullptr != p.m_lua_result_sptr_)
        p.m_lua_result_vector_.push_back(p.m_lua_result_sptr_);
}

void LuaHandle::ExtraAddResult(LuaHandle &p, const std::string &result_str, int result_type) {
    p.m_lua_result_sptr_ = std::make_shared<LuaResult>();
    if (p.m_lua_result_sptr_) {
        p.m_lua_result_sptr_->result_str = result_str;
        p.m_lua_result_sptr_->result_type = result_type;
    }
}

lua_all_results_sptr LuaHandle::Handle(const attribute_sptr &foo) {
    lua_all_results_sptr results_ret_sptr = std::make_shared<LuaAllResults>();

    auto *p_lua = static_cast<lua_State *>(m_lua_);
    ELuna::LuaTable attribute_lua_table = ELuna::LuaTable(p_lua);
    ELuna::LuaTable reserve_lua_table = ELuna::LuaTable(p_lua);

    Attribute2Table(foo, attribute_lua_table, p_lua);

    try {
        ELuna::LuaFunction<void> lua_callback(p_lua, "lua_callback");
        lua_callback(this, attribute_lua_table, reserve_lua_table);
    }
    catch (std::exception &e) {
        std::cerr << "lua_callback function error:=" << e.what() << std::endl;
    }

    for (const auto &v: m_lua_result_vector_) {
        results_ret_sptr->results_vector.push_back(v);
    }
    m_lua_result_vector_.clear();
    return results_ret_sptr;
}

int main() {
    std::string filePath = "/src/test.lua";

    lua_handle_sptr lua_engine_sptr = std::make_shared<LuaHandle>(ELUNA_ENGINE_VERSION_V_1_0);
    if (nullptr == lua_engine_sptr) {
        std::cerr << "Init lua engine failed!" << std::endl;
    }
    if (0 != lua_engine_sptr->LoadLuaFile(filePath)) {
        std::cerr << "lua engine load file failed! filePath=" << filePath << std::endl;
    }

    std::cout << "555";
    return 0;
}