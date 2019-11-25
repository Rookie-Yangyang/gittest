#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>

typedef unsigned short  UTF16;  /* at least 16 bits */
typedef unsigned char   UTF8;   /* typically 8 bits */

/*
UCS-2编码    UTF-8 字节流(二进制)
0000 - 007F  0xxxxxxx
0080 - 07FF 110xxxxx 10xxxxxx
0800 - FFFF 1110xxxx 10xxxxxx 10xxxxxx
*/
 
#define UTF8_ONE_START      (0x0001)
#define UTF8_ONE_END        (0x007F)
#define UTF8_TWO_START      (0x0080)
#define UTF8_TWO_END        (0x07FF)
#define UTF8_THREE_START    (0x0800)
#define UTF8_THREE_END      (0xFFFF)

void UTF16_To_UTF8 ( UTF16 *inbuf, UTF8 *outbuf, size_t inlen, size_t outlen)
{
		UTF16                *utf16_start = inbuf;
		UTF16                *utf16_end = inbuf + inlen;
		UTF8                 *utf8_start = outbuf;
		UTF8                 *utf8_end = outbuf + outlen;

		while (utf16_start < utf16_end)
		{
			if (*utf16_start <= UTF8_ONE_END && utf8_start + 1 < utf8_end)//一个字节转换
			{
				//0000 - 007F  0xxxxxxx
			    *utf8_start++ = (UTF8)*utf16_start;
			}
			else if (*utf16_start >= UTF8_TWO_START && *utf16_start <= UTF8_TWO_END && utf8_start + 2 < utf8_end)
			{
				//两字节转换
				//0080 - 07FF 110xxxxx 10xxxxxx
				*utf8_start++ = (*utf16_start >> 6) | 0xC0;
				*utf8_start++ = (*utf16_start & 0x3F) | 0x80;
			}
			else if (*utf16_start >= UTF8_THREE_START && *utf16_start <= UTF8_THREE_END && utf8_start + 3 < utf8_end)
			{
				//三字节转换
				//0800 - FFFF 1110xxxx 10xxxxxx 10xxxxxx
				*utf8_start++ = (*utf16_start >> 12) | 0xE0;
				*utf8_start++ = ((*utf16_start >> 6) & 0x3F) | 0x80;
				*utf8_start++ = (*utf16_start & 0x3F) | 0x80;
			}
			else
			{
				break;
			}
			utf16_start++;
		}
		utf8_start = 0;
}

void UTF8_To_UTF16 (UTF8 *inbuf, UTF16 *outbuf, size_t inlen, size_t outlen)
{
		UTF16                *utf16_start = outbuf;
		UTF16                *utf16_end = outbuf + outlen;
		UTF8                 *utf8_start = inbuf;
		UTF8                 *utf8_end = inbuf + inlen;

		while (utf8_start < utf8_end && utf16_start + 1 < utf16_end)
		{
				if (*utf8_start >= 0xE0 && *utf8_start <= 0xEF)
				{
						//三字节格式
						//0800 - FFFF 1110xxxx 10xxxxxx 10xxxxxx
						*utf16_start |= ((*utf8_start++ & 0xEF) << 12);
						*utf16_start |= ((*utf8_start++ & 0x3F) << 6);
						*utf16_start |= (*utf8_start++ & 0x3F);
				}
				else if (*utf8_start >= 0xC0 && *utf8_start <= 0xDF)
				{
						//两字节格式
						//0080 - 07FF 110xxxxx 10xxxxxx
						*utf16_start |= ((*utf8_start++ & 0x1F) << 6);
						*utf16_start |= (*utf8_start++ & 0x3F);
				}
				else if (*utf8_start >= 0 && *utf8_start <= 0x7F)
				{
						//一个字节格式
						//0000 - 007F  0xxxxxxx
						*utf16_start = *utf8_start++;
				}
				else
				{
						break;
				}
				utf16_start++;
		}
		*utf16_start = 0;
}
			

int main(int argc, char **argv)
{
		UTF16 *utf16 = L"Hello你好我的朋友!";
		UTF8 utf8[256];
		
		size_t inlen = wcslen(utf16);
		size_t outlen = 256;

		printf("UTF-16转UTF-8:\n");
		printf("转换前字符：%12s\n长度为：%zd\n", utf16, inlen);
		UTF16_To_UTF8(utf16, utf8, inlen, outlen);
		printf("转换后的字符：%s\n长度为：%zd\n", utf8, strlen(utf8));

		printf("UTF-8转UTF-16:\n");


		return 0;
}
