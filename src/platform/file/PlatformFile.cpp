/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "StarfishConfig.h"
#include "PlatformFile.h"

#include <sys/types.h>
#include <sys/stat.h>

#if defined(OS_WINDOWS)
#include <Windows.h>
#include <locale>
#include <codecvt>
#include <string>

static std::wstring toWideString(const std::string& src)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(src);
}

static std::string toNarrowString(const std::wstring& src)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(src);
}
#endif

#if defined(OS_WINDOWS)
static size_t getline(char** lineptr, size_t* n, FILE* stream)
{
    char* bufptr = NULL;
    char* p = bufptr;
    size_t size;
    int c;

    if (lineptr == NULL) {
        return -1;
    }
    if (stream == NULL) {
        return -1;
    }
    if (n == NULL) {
        return -1;
    }
    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }
    if (bufptr == NULL) {
        bufptr = (char*)malloc(128);
        if (bufptr == NULL) {
            return -1;
        }
        size = 128;
    }
    p = bufptr;
    while (c != EOF) {
        if ((p - bufptr) > (size - 1)) {
            size = size + 128;
            bufptr = (char*)realloc(bufptr, size);
            if (bufptr == NULL) {
                return -1;
            }
        }
        *p++ = c;
        if (c == '\n') {
            break;
        }
        c = fgetc(stream);
    }

    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;

    return p - bufptr - 1;
}
#endif

namespace Starfish {

bool PlatformFileUtil::removeFile(const std::string& filePath)
{
    return remove(filePath.data()) == 0;
}

Nullable<std::string> PlatformFileUtil::absolutePath(
    const std::string& filePath)
{
    std::string prefix("file://");
    if (filePath.find("file://") == 0) {
#if defined(OS_WINDOWS)
        auto s = sizeof("file://");
#else
        auto s = sizeof("file://") - 1;
#endif
        return absolutePath(filePath.substr(s, filePath.length() - s));
    }

#if defined(OS_WINDOWS)
    wchar_t result[MAX_PATH];
    std::wstring wideString = toWideString(filePath);
    DWORD dresult = GetFullPathNameW(wideString.data(), MAX_PATH, result, NULL);
    // TODO convert into longPathString
    if (dresult) {
        for (size_t i = 0; i < wcslen(result); i++) {
            if (result[i] == '\\') {
                result[i] = '/';
            }
        }
        return Nullable<std::string>(toNarrowString(result));
    } else {
        return Nullable<std::string>();
    }
#else
    char* resolved = realpath(filePath.c_str(), NULL);
    if (resolved) {
        std::string result = std::string(resolved);
        free(resolved);
        return Nullable<std::string>(result);
    }
    return Nullable<std::string>();
#endif
}

class PlatformFilePosix : public PlatformFile {
public:
    PlatformFilePosix(FILE* fp, const std::string& filePath)
        : PlatformFile()
        , m_path(filePath)
        , m_fp(fp)
    {
    }

    ~PlatformFilePosix()
    {
        fclose(m_fp);
    }

    size_t size() override
    {
        size_t currentPosition = ftell(m_fp);
        seek(0, Whence::End);
        size_t len = ftell(m_fp);
        seek(currentPosition, Whence::Start);
        return len;
    }

    size_t read(void* buf, size_t size, size_t count) override
    {
        return fread(buf, size, count, m_fp);
    }

    size_t write(void* buf, size_t size, size_t count) override
    {
        return fwrite(buf, size, count, m_fp);
    }

    ssize_t readLine(char** out, size_t* len) override
    {
        return getline(out, len, m_fp);
    }

    int seek(long offset, int whence) override
    {
        return fseek(m_fp, offset, whence);
    }

    int flush() override
    {
        return fflush(m_fp);
    }

    int eof() override
    {
        return feof(m_fp);
    }

    int64_t lastAccessTime() override
    {
        struct stat s;
        memset(&s, 0, sizeof(struct stat));
        stat(m_path.data(), &s);
        return s.st_atime;
    }

    int64_t lastModificationTime() override
    {
        struct stat s;
        memset(&s, 0, sizeof(struct stat));
        stat(m_path.data(), &s);
        return s.st_mtime;
    }

    int64_t lastChangeTime() override
    {
        struct stat s;
        memset(&s, 0, sizeof(struct stat));
        stat(m_path.data(), &s);
        return s.st_ctime;
    }

private:
    std::string m_path;
    FILE* m_fp;
};

#if defined(OS_WINDOWS)
static std::wstring toUtf16(std::string str)
{
    std::wstring ret;
    int len =
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), NULL, 0);
    if (len > 0) {
        ret.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), &ret[0],
                            len);
    }
    return ret;
}
#endif

std::unique_ptr<PlatformFile> PlatformFile::open(const std::string& filePath,
                                                 FileMode mode)
{
#if defined(OS_WINDOWS)
    const wchar_t* m = L"rb";
    if (mode == FileMode::Write) {
        m = L"wb";
    } else if (mode == FileMode::ReadWrite) {
        m = L"wb+";
    }
    FILE* fp = _wfopen(toUtf16(filePath).data(), m);
#else
    const char* m = "rb";
    if (mode == FileMode::Write) {
        m = "wb";
    } else if (mode == FileMode::ReadWrite) {
        m = "wb+";
    }
    FILE* fp = fopen(filePath.data(), m);
#endif
    if (!fp) {
        return nullptr;
    }

    // windows is cannot use utf-8 encoded filename if there is non-ASCII char
    // we should convert it as utf-16
#if defined(OS_WINDOWS)
    struct _stat64i32 s;
    memset(&s, 0, sizeof(struct _stat64i32));
    int r = _wstat(toUtf16(filePath).data(), &s);
#else
    struct stat s;
    memset(&s, 0, sizeof(struct stat));

    int r = fstat(fileno(fp), &s);
#endif
    if (r < 0 && mode == FileMode::Read) {
        fclose(fp);
        return nullptr;
    }

    if ((s.st_mode & S_IFMT) == S_IFDIR) {
        fclose(fp);
        return nullptr;
    }
    return std::unique_ptr<PlatformFile>(new PlatformFilePosix(fp, filePath));
}

} // namespace Starfish
