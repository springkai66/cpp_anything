#include <cassert>
#include <iterator>

char* my_strcpy(char* dest, const char*src)
{
	// 检查空指针
	assert(dest != nullptr && src != nullptr);
	// 保存目标地址原始值
	char* ret = dest;
	while ((*dest++ = *src++) != '\0'){}
	return ret;
}

// 判断内存区域是否重叠，决定是正向或逆向复制
void * my_memmove(void* dest, const void* src, size_t n)
{
	char* d = (char*)dest;
	const char* s = (const char*)src;
	if (d < s)
	{
		// 目标地址在源地址之前，正向复制
		for (size_t i = 0; i < n; ++i)
		{
			d[i] = s[i];
		}
	}
	else
	{
		// 目标地址在源地址后，逆向复制
		for (size_t i = n - 1; i >= 0; --i)
		{
			d[i] = s[i];
		}
	}

	return dest;
}
