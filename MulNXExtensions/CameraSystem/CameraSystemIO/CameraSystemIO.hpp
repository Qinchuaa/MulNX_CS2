#pragma once

#include <MulNX/MulNX.hpp>

//播放模式，分并行和串行两种
enum class PlaybackMode {
	Parallel,	//并行模式（元素按照游戏原初时间线播放）
	Serial		//串行模式（元素按照被分配的相对时间进行播放）
};

//基元素类型
enum class ElementType :int {
	None = -1,//错误类型
	ElementBase = 0,//元素基类
	FreeCameraPath = 1,//自由摄像机轨道
	FirstPersonCameraPath = 2,//第一人称摄像机轨道
	LockedCameraPath = 3//锁定摄像机轨道
};

//待添加的附加元素类型，如景深滤镜

class CameraSystemIO {
public:
	MulNX::Base::Math::Frame Frame;//当前帧数据
	float FrameGameTime;//用于参考的绝对游戏时间
	float SolutionTime;//解决方案时间轴时间
	float ElementTime;//元素时间轴时间，这个时间是由解决方案或预览负责提供的，基于元素的绝对时间轴
	PlaybackMode PlaybackMode;//播放模式

	ElementType CurrentElementType;//这一帧的基元素类型

	float PlayBackRate;//播放速率,解决方案需要这个值
	bool* isPlaying;//当前是否在播放，解决方案会修改这个值
};