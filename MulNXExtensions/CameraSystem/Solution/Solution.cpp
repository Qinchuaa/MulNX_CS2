#include "Solution.hpp"
#include <MulNXExtensions/CameraSystem/ElementManager/ElementManager.hpp>
#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <fstream>
#include <random>
#include <sstream>

bool Solution::AddElement(const std::shared_ptr<ElementBase> element, const float Offset) {
    if (!element) {
        return false;
    }
    //检查重复
    auto it = std::find_if(this->elements.begin(), this->elements.end(),
        [&element](const SolutionWithOffset& item) {
            if (item.Type != SolutionElementType::Element) {
                return false;
            }
            return item.Element == element;
        });

    if (it != this->elements.end()) {
        return false;
    }

    //创建新元素
    SolutionWithOffset newElement;
    newElement.Type = SolutionElementType::Element;
    newElement.Element = element;
    newElement.Offset = Offset;
    this->elements.push_back(std::move(newElement));

    //计算自身大小同时检验所有元素有效性
    this->SortElements();
    this->InvalidatePlaybackPlan();
    this->Refresh();
    return true;
}

bool Solution::AddElementGroup(const std::string& groupName, const float Offset) {
    if (!this->elementManager || groupName.empty()) {
        return false;
    }
    if (!this->elementManager->ElementGroup_Exists(groupName)) {
        return false;
    }
    if (this->elementManager->GetElementsInGroup(groupName).empty()) {
        return false;
    }

    SolutionWithOffset newElement;
    newElement.Type = SolutionElementType::ElementGroup;
    newElement.GroupName = groupName;
    newElement.Offset = Offset;
    this->elements.push_back(std::move(newElement));

    this->SortElements();
    this->InvalidatePlaybackPlan();
    this->Refresh();
    return true;
}

bool Solution::RemoveElementAt(const size_t Index) {
    if (Index >= this->elements.size()) {
        return false;
    }
    this->elements.erase(this->elements.begin() + Index);
    this->InvalidatePlaybackPlan();
    this->Refresh();
    return true;
}

bool Solution::SetElementOffsetAt(const size_t Index, const float Offset) {
    if (Index >= this->elements.size()) {
        return false;
    }
    this->elements[Index].Offset = Offset;
    this->SortElements();
    this->InvalidatePlaybackPlan();
    this->Refresh();
    return true;
}

float Solution::GetAppendOffset() const {
    float appendOffset = 0.0f;
    for (const auto& item : this->elements) {
        appendOffset = std::max(appendOffset, item.Offset + this->GetEntryDuration(item));
    }
    return appendOffset;
}

void Solution::BindElementManager(ElementManager* elementManager) {
    this->elementManager = elementManager;
    this->InvalidatePlaybackPlan();
    this->Refresh();
}

float Solution::GetEntryDuration(const SolutionWithOffset& item) const {
    if (item.Type == SolutionElementType::Element) {
        return item.Element ? item.Element->GetDurationTime() : 0.0f;
    }
    if (!this->elementManager || item.GroupName.empty()) {
        return 0.0f;
    }
    return this->elementManager->GetElementGroupMaxDuration(item.GroupName);
}

std::string Solution::GetEntryLabel(const SolutionWithOffset& item) const {
    if (item.Type == SolutionElementType::Element) {
        return item.Element ? ("元素: " + item.Element->GetName()) : "元素: <null>";
    }
    return "元素组: " + item.GroupName;
}

void Solution::SortElements() {
    std::sort(this->elements.begin(), this->elements.end(), [](const SolutionWithOffset& lhs, const SolutionWithOffset& rhs) {
        return lhs.Offset < rhs.Offset;
    });
}

void Solution::InvalidatePlaybackPlan() {
    this->playbackPlanReady = false;
    this->playbackPlanDirty = true;
    this->activeEndTime = 0.0f;
    this->resolvedElements.clear();
}

bool Solution::PreparePlaybackPlan() {
    this->Refresh();
    if (!this->safeUse) {
        return false;
    }
    if (!this->playbackPlanDirty && this->playbackPlanReady) {
        return true;
    }

    this->resolvedElements.clear();
    this->activeEndTime = 0.0f;

    static thread_local std::mt19937 rng{ std::random_device{}() };
    float resolvedOffset = 0.0f;

    for (size_t i = 0; i < this->elements.size(); ++i) {
        const auto& item = this->elements[i];
        SolutionWithOffset resolvedItem = item;
        if (item.Type == SolutionElementType::ElementGroup) {
            if (!this->elementManager) {
                return false;
            }
            auto groupElements = this->elementManager->GetElementsInGroup(item.GroupName);
            if (groupElements.empty()) {
                return false;
            }
            std::uniform_int_distribution<size_t> dist(0, groupElements.size() - 1);
            resolvedItem.Element = groupElements[dist(rng)];
        }
        if (!resolvedItem.Element) {
            return false;
        }

        // 以当前随机结果重算实际时间轴，避免元素组按最长时长占位后产生额外停顿。
        if (i == 0) {
            resolvedOffset = item.Offset;
        }
        else {
            const auto& previousPlannedItem = this->elements[i - 1];
            const auto& previousResolvedItem = this->resolvedElements.back();
            const float plannedGap = item.Offset - (previousPlannedItem.Offset + this->GetEntryDuration(previousPlannedItem));
            resolvedOffset = previousResolvedItem.Offset + previousResolvedItem.Element->GetDurationTime() + plannedGap;
        }
        resolvedItem.Offset = resolvedOffset;

        this->activeEndTime = std::max(this->activeEndTime, resolvedItem.Offset + resolvedItem.Element->GetDurationTime());
        this->resolvedElements.push_back(std::move(resolvedItem));
    }

    this->playbackPlanReady = true;
    this->playbackPlanDirty = false;
    return true;
}

void Solution::Refresh() {
    //标记为脏
    this->dirty = true;
    //判空处理
    if (this->elements.empty()) {
        this->safeUse = false;
        this->startTime = 0;
        this->endTime = 0;
        this->totalDurationTime = 0;
        return;
    }

    this->endTime = 0.0f;
    this->startTime = 0.0f;
    bool allEntriesUsable = true;

    //清理过期元素的同时查找整个解决方案的结束时间点
    for (auto It = this->elements.begin(); It != this->elements.end();) {
        bool shouldErase = false;
        if (It->Type == SolutionElementType::Element) {
            shouldErase = !It->Element || It->Element->NeedBeDelete;
        }
        else {
            shouldErase = It->GroupName.empty();
        }

        if (shouldErase) {
            It = this->elements.erase(It);//erase返回下一个有效迭代器
            this->InvalidatePlaybackPlan();
        }
        else {
            const float entryDuration = this->GetEntryDuration(*It);
            if (It->Type == SolutionElementType::ElementGroup && entryDuration <= 0.0f) {
                allEntriesUsable = false;
            }
            float ElementEndTime = It->Offset + entryDuration;
            if (this->endTime < ElementEndTime) {
                this->endTime = ElementEndTime;
            }

            ++It;//未删除时递增
        }
    }

    //更新后判空
    if (this->elements.empty()) {
        this->safeUse = false;
        this->startTime = 0;
        this->endTime = 0;
        this->totalDurationTime = 0;
        return;
    }

    //获取第一个元素的开始时间
    this->startTime = this->elements.front().Offset;

    //计算总持续时间（从第一个元素开始到最后一个元素结束）
    this->totalDurationTime = this->endTime - this->startTime;
    this->safeUse = allEntriesUsable;

    return;
}
bool Solution::TimeLineGenerate() {
    this->Refresh();

    if (this->elements.empty()) {
        return false;
    }
    float TimeReference = this->elements[0].Offset;
    for (int i = 0; i < this->elements.size(); ++i) {
        if (this->elements[i].Type != SolutionElementType::Element || !this->elements[i].Element) {
            continue;
        }
        //先获取最早的元素的开始时间作为参考时间
        if (TimeReference > this->elements[i].Element->GetStartTime()) {
            TimeReference = this->elements[i].Element->GetStartTime();
        }
    }
    //然后设置元素偏移
    for (int i = 0; i < this->elements.size(); ++i) {
        if (this->elements[i].Type != SolutionElementType::Element || !this->elements[i].Element) {
            continue;
        }
        this->elements[i].Offset = this->elements[i].Element->GetStartTime() - TimeReference;
    }

    this->SortElements();
    this->InvalidatePlaybackPlan();
    this->Refresh();

    return true;
}
std::string Solution::GetMsg() {
    this->Refresh();

    if (!this->safeUse)return "不安全的解决方案";

    std::ostringstream oss;
    oss << "解决方案名称：" << this->name
        << "   元素数量：" << this->elements.size()
        << "   总时长：" << this->totalDurationTime
        << "\n详细信息："
        << "\n";

    for (size_t i = 0; i < this->elements.size(); ++i) {
        const auto& item = this->elements.at(i);
        if (item.Type == SolutionElementType::Element && item.Element) {
            oss << i << ".  "
                "  |元素编号：" << i <<
                "  元素名称：" << item.Element->Name <<
                "  元素类型：" << item.Element->TypeGet_String() <<
                "  元素持续时间：" << item.Element->DurationTime <<
                "  元素偏移时间：" << this->elements[i].Offset << "\n";
        }
        else {
            oss << i << ".  "
                << "  |元素组编号：" << i
                << "  元素组名称：" << item.GroupName
                << "  元素组持续时间：" << this->GetEntryDuration(item)
                << "  元素组偏移时间：" << item.Offset << "\n";
        }
    }

    return oss.str();
}
void Solution::Clear() {
    this->elements.clear();
    this->InvalidatePlaybackPlan();
    this->Refresh();
    this->solutionOffset = 0;

    return;
}
void Solution::ResetName(std::string_view NewName) {
    this->name = NewName;
    this->dirty = true;

    return;
}
std::string Solution::GetName()const {
    return this->name;
}
bool Solution::Call(CameraSystemIO* IO) {
    if (!this->safeUse) {
        IO->isPlaying = false;
        return false;
    }
    //对时间依次进行两次偏移：解决方案偏移，具体元素具体偏移

    //判断是否有插值结果。
    //有结果，Manager才可能覆盖
    bool bResult = false;

    //尝试依次调用所有的元素插值
    //编号靠后的元素有更高的决定权
    this->Refresh();//调用前立刻更新，智能共享指针锁定状态

    this->playmode = PlaybackMode::Orchestration;
    //偏移时间轴播放
    float SolutionOffsetedTime = IO->SolutionTime - this->solutionOffset;
    if (!this->PreparePlaybackPlan()) {
        IO->isPlaying = false;
        return false;
    }

    if (this->activeEndTime < SolutionOffsetedTime) {
        //播放结束
        this->solutionOffset = 0;//归位时间
        IO->isPlaying = false;//播放结束
        return false;//无插值结果
    }
    for (size_t i = 0; i < this->resolvedElements.size(); ++i) {
        //这里用减法得到相对于元素的时间
        //尝试该元素插值，如果有结果则代表可以应用
        //这里传入的时间已经是相对时间
        auto& element = this->resolvedElements[i].Element;
        IO->ElementTime = SolutionOffsetedTime - this->resolvedElements[i].Offset + element->GetStartTime();
        bResult = bResult || element->CalculateFrame(IO);
    }

    return bResult;
}
void Solution::SetKeyCheckPack(const MulNX::KeyCheckPack& KCPack) {
    this->KCPack = KCPack;
    this->KCPack.Refresh();
    this->dirty = true;
    return;
}

std::pair<bool, std::string> Solution::Save(const std::filesystem::path& folderPath) {
    // 先刷新确保安全有效
    this->Refresh();
    // 检查文件路径和名称存在性
    if (folderPath.empty()) return { false, "文件夹路径为空，无法保存解决方案！" };

    // 拼接完整路径
    std::filesystem::path filePath = folderPath / (this->name + ".yaml");

    try {
        YAML::Node root;

        root["name"] = this->name;
        root["duration"] = this->totalDurationTime;
        root["KCP"] = this->KCPack;
        root["size"] = this->elements.size();

        YAML::Node elementsNode = root["elements"];
        for (size_t i = 0; i < this->elements.size(); ++i) {
            YAML::Node elemNode;
            elemNode["type"] = this->elements[i].Type == SolutionElementType::Element ? "element" : "group";
            if (this->elements[i].Type == SolutionElementType::Element) {
                std::shared_ptr<ElementBase> element = this->elements[i].Element;
                if (!element) return { false, "疑似有元素在保存过程中被删除，保存终止！" };
                elemNode["name"] = element->Name;
            }
            else {
                elemNode["group"] = this->elements[i].GroupName;
            }
            elemNode["offset"] = this->elements[i].Offset;
            elementsNode.push_back(elemNode);
        }

        std::ofstream fout(filePath);
        fout << root;
        fout.close();

        return { true, std::format("保存成功  解决方案名：{}  元素个数：{}",this->name ,this->elements.size()) };
    }
    catch (const std::exception& e) {
        return { false, "保存失败：" + std::string(e.what()) };
    }
}
std::pair<bool, std::string> Solution::Load(YAML::Node& root, ElementManager* elementManager) {
    this->BindElementManager(elementManager);
    this->KCPack = root["KCP"].as<MulNX::KeyCheckPack>();
    // 获取解决方案名称并检查是否为空
    this->name = root["name"].as<std::string>();
    // 获取持续时长信息
    float TargetDurationTime = root["duration"].as<float>();
    // 元素总量
    size_t AllCount = root["size"].as<size_t>();
    if (root["elements"].size() != AllCount) {
        return { false,std::format("不安全的解决方案！实际元素数量与文件描述不符！ 解决方案名：{}",this->name) };
    }

    // 读取流程
    for (const auto& nodeElement : root["elements"]) {
        // 获取元素偏移
        float ElementOffset = nodeElement["offset"].as<float>();
        const std::string itemType = nodeElement["type"] ? nodeElement["type"].as<std::string>() : "element";
        if (itemType == "group") {
            std::string groupName = nodeElement["group"].as<std::string>();
            if (!this->AddElementGroup(groupName, ElementOffset)) {
                return { false,std::format("无法添加元素组到解决方案   解决方案名：{}  元素组名：{}" ,this->name, groupName) };
            }
            continue;
        }

        // 获取元素名称
        std::string NewElementName = nodeElement["name"].as<std::string>();
        // 得到元素指针
        auto it = elementManager->elements.find(NewElementName);
        // 检验是否找到元素
        if (it==elementManager->elements.end()) {
            return { false,"找不到目标元素   元素名：" + NewElementName };
        }
        // 尝试创建带有时间偏移的弱引用指针并添加进新解决方案并判断是否成功
        if (!this->AddElement(it->second, ElementOffset)) {
            return { false,std::format("无法添加元素到解决方案   解决方案名：{}  元素名：{}" ,this->name, NewElementName) };
        }
    }

    // 刷新
    this->Refresh();
    // 去除脏标记
    this->dirty = false;

    return { true,"解决方案加载成功" };
}

void Solution::SetSolutionOffset(const float Offset) {
    this->solutionOffset = Offset;
    this->InvalidatePlaybackPlan();
    this->PreparePlaybackPlan();
}
