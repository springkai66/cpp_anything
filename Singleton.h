#include <mutex>
#include <atomic>
// C++11保证静态局部变量线程安全, C++11标准规定静态局部变量初始化具备原子性
class Singleton1
{
public:
	static Singleton1& GetInstance()
	{
		static Singleton1 instance;
		return instance;
	}

	// 禁用拷贝和赋值
	Singleton1(const Singleton1&) = delete;
	Singleton1& operator = (const Singleton1&) = delete;
private:
	// 私有构造
	Singleton1() = default;
	~Singleton1() = default;
};

// 双检锁模式，通过memory_order防止指令重排
// Acquire-Release语义：通过这对内存序，保证instance的写入对其他线程可见。
// 当写线程执行store（Release），读线程执行load（Acquire）时，写操作前的所有内存修改对读线程可见
class Singleton2
{
public:
	static Singleton2* GetInstance()
	{
		// 读操作，确保后续的读/写操作不会重排到此操作之前
		Singleton2* tmp = instance_.load(std::memory_order_acquire);
		if (tmp == nullptr)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			// 无顺序约束，仅保证原子性
			tmp = instance_.load(std::memory_order_relaxed);
			if (tmp == nullptr)
			{
				tmp = new Singleton2();
				// 写操作，确保之前的读/写操作不会重排到此操作之后
				instance_.store(tmp, std::memory_order_release);
			}
		}

		return tmp;
	}
	
	// 禁用拷贝和赋值
	Singleton2(const Singleton2&) = delete;
	Singleton2& operator = (const Singleton2&) = delete;
private:
	// 私有构造
	Singleton2() = default;
	~Singleton2() = default;

	static std::atomic<Singleton2*> instance_;
	static std::mutex mutex_;
};

// 模版通用实现
template<typename T>
class Singleton
{
public:
	static T& GetInstance()
	{
		static T instance;
		return instance;
	}

	// 禁用拷贝和赋值
	Singleton(const Singleton&) = delete;
	Singleton& operator = (const Singleton&) = delete;
private:
	// 私有构造
	Singleton() = default;
	~Singleton() = default;
};