#include <atomic>
#include <memory>
#include <utility>

// 引用计数基类
class RefCountBase
{
public:
	RefCountBase() noexcept
		: Uses(0), Weaks(0)
	{
	}

	virtual ~RefCountBase() noexcept
	{
	}

	virtual void Dispose() noexcept = 0;

	virtual void Destroy() noexcept
	{
		delete this;
	}

public:
	// 原子增加强引用
	void Incref()
	{
		Uses.fetch_add(1, std::memory_order_relaxed);
	}

	// 原子减少强引用
	void Decref()
	{
		// 归0时，删除ptr
		if (Uses.fetch_sub(1, std::memory_order_acq_rel) == 1)
		{
			Dispose();
			WeakRelease();
		}
	}


	// 原子增加弱引用
	void Incwref()
	{
		Weaks.fetch_add(1, std::memory_order_relaxed);
	}


	// 原子减少弱引用
	void Decwref()
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
	RefCountBase(RefCountBase const&) = delete;
	RefCountBase& operator=(RefCountBase const&) = delete;

	// 强引用计数
	std::atomic<unsigned long> Uses = 0;
	// 弱引用计数
	std::atomic<unsigned long> Weaks = 0;
};


// SharedPtr基类
template <typename Tp>
class PtrBase
{
public:
	unsigned long UseCount() const noexcept
	{
	}

	// 禁止拷贝构造和赋值
	PtrBase(const PtrBase&) = delete;
	PtrBase& operator=(const PtrBase&) = delete;

protected:
	Tp* Get() const noexcept
	{
		return Ptr;
	}

	constexpr PtrBase() noexcept = default;
	~PtrBase() = default;

	void Incref() const noexcept
	{
		if (Rep)
		{
			Rep->Incref();
		}
	}

	void Decref() noexcept
	{
		if (Rep)
		{
			Rep->Decref();
		}
	}

	void Swap(PtrBase& r) noexcept
	{
		std::swap(Ptr, r.Ptr);
		std::swap(Rep, r.Rep);
	}

	void Incwref() const noexcept
	{
		if (Rep)
		{
			Rep->Incwref();
		}
	}

	void Decwref() noexcept
	{
		if (Rep)
		{
			Rep->Decwref();
		}
	}

private:
	Tp* Ptr{nullptr};
	RefCountBase* Rep{nullptr};
};

template <class Tp>
class SharedPtr : public PtrBase<Tp>
{
private:
	using Mybase = PtrBase<Tp>;

public:
	constexpr SharedPtr() noexcept = default;
	// 创建空SharedPtr
	constexpr SharedPtr(nullptr_t) noexcept
	{
	}
};
