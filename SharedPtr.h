#include <atomic>
#include <memory>
#include <utility>

// 引用计数基类
class SpCountedBase
{
public:
    SpCountedBase() noexcept
        : Uses(1), Weaks(1)
    {
    }

    virtual ~SpCountedBase() noexcept
    {
    }

    virtual void Dispose() noexcept = 0;

    virtual void Destroy() noexcept
    {
        delete this;
    }

public:
    // 原子增加强引用
    void AddRef()
    {
        Uses.fetch_add(1, std::memory_order_relaxed);
    }

    // 原子减少强引用
    void Release()
    {
        // 归0时，删除ptr
        if (Uses.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            Dispose();
            WeakRelease();
        }
    }


    // 原子增加弱引用
    void WeakAddRef()
    {
        Weaks.fetch_add(1, std::memory_order_relaxed);
    }


    // 原子减少弱引用
    void WeakRelease()
    {
        // 归0时删除this
        if (Weaks.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            Destroy();
        }
    }

    // 弱引用提升强引用
    bool AddRefLock()
    {
        // 加载当前强引用计数,compare_exchange_weak实现无锁循环，确保use_count == count时，才会count+1
        unsigned long count = GetUseCount();
        do
        {
            // 对象已经销毁，无法提升
            if (count == 0)
                return false;
        }
        // 成功时内存序,保证后续操作不会重排到CAS之前,失败时内存序,允许重试
        while (!Uses.compare_exchange_weak(count, count + 1, std::memory_order_acquire,
                                           std::memory_order_relaxed));

        return true;
    }

    // 获取强引用数量
    unsigned long GetUseCount() const
    {
        return Uses.load(std::memory_order_relaxed);
    }

private:
    SpCountedBase(SpCountedBase const&) = delete;
    SpCountedBase& operator=(SpCountedBase const&) = delete;

    // 强引用计数
    std::atomic<unsigned long> Uses = 0;
    // 弱引用计数
    std::atomic<unsigned long> Weaks = 0;
};

// Counted ptr with no deleter or allocator
template <typename Tp>
class SpCountedPtr final : public SpCountedBase
{
public:
    explicit SpCountedPtr(Tp p) noexcept
        : ptr(p)
    {
    }

    virtual void Dispose() noexcept override
    {
        delete ptr;
    }

    virtual void Destroy() noexcept override
    {
        delete this;
    }

    SpCountedPtr(const SpCountedPtr&) = delete;
    SpCountedPtr& operator=(const SpCountedPtr&) = delete;

private:
    Tp ptr;
};

template <int Nm, typename Tp, bool use_ebo = !__is_final(Tp) && __is_empty(Tp)>
struct SpEBOHelper;

// Specialization not using EBO
template <int Nm, typename Tp>
struct SpEBOHelper<Nm, Tp, false>
{
    explicit SpEBOHelper(const Tp& tp) : ptr(tp)
    {
    }

    explicit SpEBOHelper(Tp&& tp) : ptr(std::move(tp))
    {
    }

    static Tp& Get(SpEBOHelper& eboh) { return eboh.ptr; }

private:
    Tp ptr;
};

// Support for custom deleter and allocator
template <typename Tp, typename Deleter, typename Alloc>
class SpCountedDeleter final : public SpCountedBase
{
    class Impl : SpEBOHelper<0, Deleter>, SpEBOHelper<1, Alloc>
    {
        typedef SpEBOHelper<0, Deleter> DelBase;
        typedef SpEBOHelper<1, Alloc> AllocBase;

    public:
        Impl(Tp p, Deleter d, const Alloc& a) noexcept
            : DelBase(std::move(d)), AllocBase(a), ptr(p)
        {
        }

        Deleter& Del() noexcept { return DelBase::Get(*this); }
        Alloc& Alloc() noexcept { return AllocBase::Get(*this); }

        Tp ptr;
    };

public:
    SpCountedDeleter(Tp p, Deleter d) noexcept
        : impl(p, std::move(d), Alloc())
    {
    }

    SpCountedDeleter(Tp p, Deleter d, const Alloc& a) noexcept
        : impl(p, std::move(d), a)
    {
    }

    ~SpCountedDeleter() noexcept
    {
        impl.Del()(impl.ptr);
    }

    virtual void Dispose() noexcept override
    {
        impl.Del()(impl.ptr);
    }

    virtual void Destroy() noexcept override
    {
        this->~SpCountedDeleter();
    }

    Impl impl;
};

// SharedPtr基类
template<typename Tp>
class SharedPtrAccess
{
public:
    using ElementType = Tp;
};