#pragma once

#include <atomic>
#include <array>

namespace MulNX {
    namespace Base {
        class TripleBufferBase {
        public:
            // 将三个2-bit索引打包到32位中：bits [1:0]=Write, [3:2]=Mid, [5:4]=Read
            std::atomic<uint32_t> IndexState{ (0u) | (1u << 2) | (2u << 4) };

            virtual ~TripleBufferBase() = default;

            virtual void Clear() = 0;
            virtual void* GetWriteBuffer() = 0;
            virtual void* GetReadBuffer() = 0;
            virtual bool SwapWriteBuffer() = 0;  // 返回是否成功
            virtual bool SwapReadBuffer() = 0;   // 返回是否有新数据
            virtual bool HasNewData() const = 0;  // 检查是否有新数据

        protected:
            static constexpr uint32_t Pack(int w, int m, int r) noexcept {
                return (static_cast<uint32_t>(w) & 0x3u)
                    | ((static_cast<uint32_t>(m) & 0x3u) << 2)
                    | ((static_cast<uint32_t>(r) & 0x3u) << 4);
            }

            static constexpr int UnpackWrite(uint32_t s) noexcept {
                return static_cast<int>(s & 0x3u);
            }

            static constexpr int UnpackMid(uint32_t s) noexcept {
                return static_cast<int>((s >> 2) & 0x3u);
            }

            static constexpr int UnpackRead(uint32_t s) noexcept {
                return static_cast<int>((s >> 4) & 0x3u);
            }
        };

        template<typename T>
        class TripleBuffer final : public TripleBufferBase {
        public:
            std::array<T, 3> Information{};

            void Clear() override {
                for (auto& It : Information) {
                    It = T{};
                }
                IndexState = Pack(0, 1, 2);
            }

            void* GetWriteBuffer() override {
                uint32_t s = IndexState.load(std::memory_order_acquire);
                int w = UnpackWrite(s);
                return &Information[w];
            }

            void* GetReadBuffer() override {
                uint32_t s = IndexState.load(std::memory_order_acquire);
                int r = UnpackRead(s);
                return &Information[r];
            }

            // 写入线程：交换 Write 和 Mid
            // 返回true表示成功交换
            bool SwapWriteBuffer() override {
                uint32_t oldState = IndexState.load(std::memory_order_acquire);
                uint32_t newState;

                do {
                    int w = UnpackWrite(oldState);
                    int m = UnpackMid(oldState);
                    int r = UnpackRead(oldState);

                    // 交换 Write 和 Mid
                    // 逻辑：写入完成，将Write缓冲区标记为就绪
                    newState = Pack(m, w, r);  // Write <-> Mid

                } while (!IndexState.compare_exchange_weak(
                    oldState, newState,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire
                ));

                return true;
            }

            // 读取线程：交换 Read 和 Mid（如果有新数据）
            // 返回true表示有新数据并成功交换
            bool SwapReadBuffer() override {
                uint32_t oldState = IndexState.load(std::memory_order_acquire);
                uint32_t newState;

                do {
                    int w = UnpackWrite(oldState);
                    int m = UnpackMid(oldState);
                    int r = UnpackRead(oldState);

                    // 检查是否有新数据（Mid != Read）
                    if (m == r) {
                        return false;  // 没有新数据
                    }

                    // 交换 Read 和 Mid
                    // 逻辑：读取完成，将Mid标记为新的读取缓冲区
                    newState = Pack(w, r, m);  // Read <-> Mid

                } while (!IndexState.compare_exchange_weak(
                    oldState, newState,
                    std::memory_order_acq_rel,
                    std::memory_order_acquire
                ));

                return true;
            }

            // 检查是否有新数据
            bool HasNewData() const override {
                uint32_t s = IndexState.load(std::memory_order_acquire);
                int m = UnpackMid(s);
                int r = UnpackRead(s);
                return m != r;  // Mid != Read 表示有新数据
            }

            // 类型安全的辅助方法
            T& GetWriteData() {
                uint32_t s = IndexState.load(std::memory_order_acquire);
                int w = UnpackWrite(s);
                return Information[w];
            }

            const T& GetWriteData() const {
                uint32_t s = IndexState.load(std::memory_order_acquire);
                int w = UnpackWrite(s);
                return Information[w];
            }

            T& GetReadData() {
                uint32_t s = IndexState.load(std::memory_order_acquire);
                int r = UnpackRead(s);
                return Information[r];
            }

            const T& GetReadData() const {
                uint32_t s = IndexState.load(std::memory_order_acquire);
                int r = UnpackRead(s);
                return Information[r];
            }

            T& GetMidData() {
                uint32_t s = IndexState.load(std::memory_order_acquire);
                int m = UnpackMid(s);
                return Information[m];
            }

            const T& GetMidData() const {
                uint32_t s = IndexState.load(std::memory_order_acquire);
                int m = UnpackMid(s);
                return Information[m];
            }
        };

        //RAII帮助，让上下文使用get获取指针时更方便，离开作用域自动提交

        // RAII写入包装器
        template<typename T>
        class DataWrite {
        public:
            TripleBuffer<T>* pBuffer = nullptr;
            T* pData = nullptr;

            DataWrite(TripleBufferBase* pBuffer)
                : pBuffer(static_cast<TripleBuffer<T>*>(pBuffer)) {
                if (this->pBuffer) {
                    this->pData = static_cast<T*>(this->pBuffer->GetWriteBuffer());
                }
            }

            ~DataWrite() {
                if (this->pBuffer && this->pData) {
                    this->pBuffer->SwapWriteBuffer();
                }
            }

            T* get() { return this->pData; }

            T* operator->() { return this->pData; }
            T& operator*() { return *this->pData; }

            explicit operator bool() const { return pData != nullptr; }
        };

        // RAII读取包装器
        template<typename T>
        class DataRead {
        public:
            TripleBuffer<T>* pBuffer = nullptr;
            T* pData = nullptr;
            bool hasNewData = false;

            DataRead(TripleBufferBase* pBuffer)
                : pBuffer(static_cast<TripleBuffer<T>*>(pBuffer)) {
                if (this->pBuffer) {
                    this->pData = static_cast<T*>(this->pBuffer->GetReadBuffer());
                    this->hasNewData = this->pBuffer->SwapReadBuffer();
                }
            }

            ~DataRead() = default;  // 读取不需要特殊清理

            T* get() { return this->pData; }

            T* operator->() { return this->pData; }
            T& operator*() { return *this->pData; }

            bool hasNew() const { return hasNewData; }

            explicit operator bool() const { return pData != nullptr; }
        };
    }
}