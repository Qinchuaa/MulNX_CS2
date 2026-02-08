#include"ConVarSystem.hpp"

#include<string.h>


void* C_ConVarSystem::GetFirstCvarIterator(uint64_t& idx)const {
	return vmt::CallVirtual<void*>(12, this->Address, &idx);
}
void* C_ConVarSystem::GetNextCvarIterator(uint64_t& idx)const {
	return vmt::CallVirtual<void*>(13, this->Address, &idx, idx);
}
C_ConVar* C_ConVarSystem::GetCVarByIndex(uint64_t index)const {
	return vmt::CallVirtual<C_ConVar*>(43, this->Address, index);
}
C_ConVar* C_ConVarSystem::GetCVarByName(const char* var_name)const {
	uint64_t i = 0;
	this->GetFirstCvarIterator(i);
	while (i != 0xFFFFFFFF) {
		C_ConVar* pCvar = nullptr;
		pCvar = this->GetCVarByIndex(i);
		if (strcmp(pCvar->szName, var_name) == 0) {
			return pCvar;
		}
		this->GetNextCvarIterator(i);
	}
	return nullptr;
}


void C_ConVarSystem::UnlockHiddenCVars(int& Count)const {
	uint64_t i = 0;
	this->GetFirstCvarIterator(i);
	while (i != 0xFFFFFFFF) {
		C_ConVar* pConVar = this->GetCVarByIndex(i);
		if (pConVar) {
			if (pConVar->IsHidden()) {
				pConVar->Unhide();
				++Count;
			}
		}
		this->GetNextCvarIterator(i);
	}
	return;
}
void C_ConVarSystem::LockAllCvars(int& Count)const {
	uint64_t i = 0;
	this->GetFirstCvarIterator(i);
	while (i != 0xFFFFFFFF) {
		C_ConVar* pConVar = this->GetCVarByIndex(i);
		if (pConVar) {
			if (!pConVar->IsHidden()) {
				pConVar->Hide();
				++Count;
			}
		}
		this->GetNextCvarIterator(i);
	}
	return;
}

C_ConVar* C_ConVarSystem::GetCvar(const std::string& CvarName){
	std::unique_lock lock(this->ConVarSystemMutex);
	auto it = this->CvarMap.find(CvarName);
	if (it != CvarMap.end()) {
		return it->second;
	}
	C_ConVar* pCvar = this->GetCVarByName(CvarName.c_str());
	if (pCvar) {
		this->CvarMap[CvarName] = pCvar;
	}
	return pCvar;
}