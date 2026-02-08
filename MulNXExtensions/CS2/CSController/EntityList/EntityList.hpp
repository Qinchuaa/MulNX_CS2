#pragma once

#include"Entity/Entity.hpp"

#include<vector>
#include<shared_mutex>

class C_EntityList {
private:
    std::vector<C_Entity> Entitys;
public:
    C_EntityList() {
        this->Entitys.resize(64);
    }
    static uintptr_t Address;

    std::ostringstream GetMsg()const;

    static uintptr_t GetEntityBaseFromIndex(int Index);
    static uintptr_t GetEntityControllerFromIndex(int Index);
    static uintptr_t GetEntityPawnFromHandle(uint32_t uHandle);

    mutable std::shared_mutex EntityListMtx{};

	void Update();

    C_Entity& at(size_t index);
    size_t size()const;
    bool empty()const;

    //拷贝获取
    C_Entity GetEntity(size_t Index);
};