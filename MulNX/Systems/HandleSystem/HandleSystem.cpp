#include "HandleSystem.hpp"

#include "../../Core/Core.hpp"

bool MulNX::HandleSystem::Init(){
	//无需初始化操作
	return true; 
}

MulNXHandle MulNX::HandleSystem::RegisteUnique(MulNX::Base::any_unique_ptr Resource) {
	std::unique_lock lock(this->MapMutex);
	MulNXHandle handle = MulNXHandle::CreateHandle();
	this->UniqueResources[handle] = std::move(Resource);
	return handle;
}
MulNX::Base::any_unique_ptr MulNX::HandleSystem::ReleaseUnique(MulNXHandle Handle) {
	std::unique_lock lock(this->MapMutex);
	auto it = this->UniqueResources.find(Handle);
	if (it == this->UniqueResources.end()) {
		return nullptr;
	}
	MulNX::Base::any_unique_ptr temp = std::move(it->second);
	this->UniqueResources.erase(it);
	return temp;
}
MulNXHandle MulNX::HandleSystem::RegisteShared(MulNX::Base::any_shared_ptr Resource) {
	std::unique_lock lock(this->MapMutex);
	MulNXHandle handle = MulNXHandle::CreateHandle();
	this->SharedResources[handle] = std::move(Resource);
	return handle;
}
MulNX::Base::any_shared_ptr MulNX::HandleSystem::GetShared(MulNXHandle Handle) {
	std::shared_lock lock(this->MapMutex);
	auto it = this->SharedResources.find(Handle);
	if (it == this->SharedResources.end()) {
		return nullptr;
	}
	return it->second;
}