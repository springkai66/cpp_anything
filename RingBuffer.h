#include <vector>
#include <mutex>
#include <condition_variable>

// 非线程安全版，不支持自动扩容
template <typename T>
class RingBuffer
{
public:
	explicit RingBuffer(size_t capcity)
		: buffer_(capcity), capacity_(capcity), read_idx_(0), write_idx_(0)
	{
	}

	bool Push(const T& item)
	{
		if (IsFull()) return false;
		buffer_[write_idx_] = item;
		// 通过%模运算实现环形逻辑，读写指针超过容量后自动回绕
		write_idx_ = (write_idx_ + 1) % capacity_;

		return true;
	}

	bool Pop(T& item)
	{
		if (IsEmpty())
		{
			return false;
		}

		item = buffer_[read_idx_];
		read_idx_ = (read_idx_ + 1) % capacity_;
		return true;
	}

	bool IsEmpty() const { return read_idx_ == write_idx_; }
	bool IsFull() const { return (write_idx_ + 1) % capacity_ == read_idx_; }

private:
	// 存储容器
	std::vector<T> buffer_;
	// 容量
	size_t capacity_;
	// 读索引
	size_t read_idx_;
	// 写索引
	size_t write_idx_;
};

// 互斥锁+条件变量实现多线程版
template <typename T>
class ThreadSafeRingBuffer
{
public:
	explicit ThreadSafeRingBuffer(size_t capacity)
		: buffer_(capacity), capacity_(capacity), read_idx_(0), write_idx_(0)
	{
	}

	bool Push(const T& item, bool overwrite = false)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		if (IsFull())
		{
			if (!overwrite) return false;
			// 覆盖旧数据
			read_idx_ = (read_idx_ + 1) % capacity_;
		}

		buffer_[write_idx_] = item;
		write_idx_ = (write_idx_ + 1) % capacity_;
		cv_.notify_one();
		return true;
	}

	bool Pop(T& item)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		cv_.wait(lock, [this]() { return !IsEmpty(); });
		item = buffer_[read_idx_];
		read_idx_ = (read_idx_ + 1) % capacity_;
		return true;
	}

	bool IsEmpty() const { return read_idx_ == write_idx_; }
	bool IsFull() const { return (write_idx_ + 1) % capacity_ == read_idx_; }

private:
	std::vector<T> buffer_;
	size_t capacity_;
	size_t read_idx_;
	size_t write_idx_;
	std::mutex mutex_;
	std::condition_variable cv_;
};

// 无锁版（单生产者单消费者）
template <typename T>
class LockFreeRingBuffer
{
public:
	explicit LockFreeRingBuffer(size_t capacity)
		: buffer_(capacity), capacity_(capacity)
	{
		// 容量强制为2的幂
		if((capacity_ & (capacity_ - 1)) != 0)
		{
			throw std::invalid_argument("Capacity must be power of two");
		}
	}

	bool Push(const T& item)
	{
		size_t write_idx = write_idx_.load(std::memory_order_relaxed);
		size_t next_idx = (write_idx + 1) & (capacity_ - 1);
		if(next_idx == read_idx_.load(std::memory_order_acquire))
		{
			// 缓冲区满
			return false;
		}
		buffer_[write_idx_] = item;
		write_idx_.store(next_idx, std::memory_order_release);
		return true;
	}

	bool Pop(T& item)
	{
		size_t read_idx = read_idx_.load(std::memory_order_relaxed);
		if(read_idx_ == write_idx_.load(std::memory_order_acquire))
		{
			// 缓冲区空
			return false;
		}
		item = buffer_[read_idx_];
		read_idx_.store((read_idx_ + 1) & (capacity_ - 1), std::memory_order_release);
		return true;
	}
	
private:
	std::vector<T> buffer_;
	size_t capacity_;
	std::atomic<size_t> read_idx_{0}, write_idx_{0};
};
