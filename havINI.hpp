/*
havINI.hpp

ABOUT

Havoc's single-file INI library for C++.

TODO

- Code refactoring.
- Add more error handling code.
- Improve unicode support.

REVISION HISTORY

v0.3 (2024-04-22) - Fixed a small bug which would result into undefined behavior and did some performance optimization.
v0.2 (2024-01-14) - Exclude havUtils functions from the one definition rule.
v0.1 (2024-01-10) - First release.

LICENSE

MIT License

Copyright (c) 2024 René Nicolaus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef HAVINI_HPP
#define HAVINI_HPP

// Add -D_FILE_OFFSET_BITS=64 to CFLAGS for large file support.
// Optionally, you can use #define HAVINI_CASE_SENSITIVE before including the header file to enforce case sensitivity for section names and keys in key/value pairs.

#ifdef _WIN32
#ifdef _MBCS
#error "_MBCS is defined, but only Unicode is supported"
#endif
#undef _UNICODE
#define _UNICODE
#undef UNICODE
#define UNICODE

#undef NOMINMAX
#define NOMINMAX

#undef STRICT
#define STRICT

#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WINXP
#endif
#ifdef _MSC_VER
#include <SDKDDKVer.h>
#endif

#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <cuchar>
#include <cstring>
#include <codecvt>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sstream>
#include <locale>
#include <type_traits>
#include <optional>
#include <memory>
#include <vector>

static_assert(sizeof(signed char) == 1, "expected char to be 1 byte");
static_assert(sizeof(unsigned char) == 1, "expected unsigned char to be 1 byte");
static_assert(sizeof(signed char) == 1, "expected int8 to be 1 byte");
static_assert(sizeof(unsigned char) == 1, "expected uint8 to be 1 byte");
static_assert(sizeof(signed short int) == 2, "expected int16 to be 2 bytes");
static_assert(sizeof(unsigned short int) == 2, "expected uint16 to be 2 bytes");
static_assert(sizeof(signed int) == 4, "expected int32 to be 4 bytes");
static_assert(sizeof(unsigned int) == 4, "expected uint32 to be 4 bytes");
static_assert(sizeof(signed long long) == 8, "expected int64 to be 8 bytes");
static_assert(sizeof(unsigned long long) == 8, "expected uint64 to be 8 bytes");

namespace havINI {
    enum class havINIBOMType : std::uint8_t
    {
        None,
        UTF8,
        UTF16LE,
        UTF16BE,
        UTF32LE,
        UTF32BE
    };

    enum class havINIDataType : std::uint8_t
    {
        Empty,
        Comment,
        Value,
        Array
    };

    enum class havINIPosition : std::uint8_t
    {
        Start,
        End,
        Above,
        Below
    };

    namespace havUtils
    {
        constexpr bool StartsWith(std::string_view sv, std::string_view prefix)
        {
            return sv.size() >= prefix.size() && sv.compare(0, prefix.size(), prefix) == 0;
        }

        constexpr bool EndsWith(std::string_view sv, std::string_view suffix)
        {
            return sv.size() >= suffix.size() && sv.compare(sv.size() - suffix.size(), suffix.size(), suffix) == 0;
        }

        inline std::vector<std::string> Split(const std::string& value, const std::string& delimiter)
        {
            std::vector<std::string> result;

            std::size_t start;
            std::size_t end = 0;

            while ((start = value.find_first_not_of(delimiter, end)) != std::string::npos)
            {
                end = value.find(delimiter, start);
                result.push_back(value.substr(start, end - start));
            }

            if (result.empty() == true)
            {
                result.push_back(value);
            }

            return result;
        }

        inline std::string Join(const std::vector<std::string>& strings, const std::string& delimiter)
        {
            if (strings.empty() == true)
            {
                return "";
            }

            std::string result = "";

            for (std::vector<std::string>::size_type index = 0; index < strings.size(); ++index)
            {
                result += (((index > 0) ? delimiter : "") + strings[index]);
            }

            return result;
        }

        inline std::string ToLower(std::string value, const std::locale& loc)
        {
            auto toLower = std::binder1st(std::mem_fun(&std::ctype<char>::tolower), &std::use_facet<std::ctype<char>>(loc));
            std::transform(value.begin(), value.end(), value.begin(), toLower);
            return value;
        }
    }

    class havINISection;

    class havINIData
    {
        public:
            explicit havINIData(
#ifndef HAVINI_CASE_SENSITIVE
            const std::locale& loc,
#endif
            const std::string& key, const std::string& value, havINIDataType valueType, bool addQuotes = false, std::optional<std::string> inlineComment = std::nullopt, bool hasArrayIndex = false) :
#ifndef HAVINI_CASE_SENSITIVE
            mLocale(loc),
#endif
            mType(valueType), mKey(key), mValue(value), mInlineComment(inlineComment), mAddQuotes(addQuotes), mArrayIndex(0), mHasArrayIndex(hasArrayIndex)
            {
            }

            explicit havINIData(
#ifndef HAVINI_CASE_SENSITIVE
            const std::locale& loc,
#endif
            const std::string& key, havINIDataType valueType = havINIDataType::Empty, bool addQuotes = false, bool hasArrayIndex = false) :
#ifndef HAVINI_CASE_SENSITIVE
            mLocale(loc),
#endif
            mType(valueType), mKey(key), mValue(""), mInlineComment(std::nullopt), mAddQuotes(addQuotes), mArrayIndex(0), mHasArrayIndex(hasArrayIndex)
            {
            }

            explicit havINIData(havINIData&& value) :
#ifndef HAVINI_CASE_SENSITIVE
            mLocale(value.mLocale),
#endif
            mType(value.mType), mKey(value.mKey), mValue(value.mValue), mInlineComment(value.mInlineComment), mAddQuotes(value.mAddQuotes), mArrayIndex(value.mArrayIndex), mHasArrayIndex(value.mHasArrayIndex), mArray(value.mArray)
            {
            }

            explicit havINIData(const havINIData& value) :
#ifndef HAVINI_CASE_SENSITIVE
            mLocale(value.mLocale),
#endif
            mType(value.mType), mKey(value.mKey), mValue(value.mValue), mInlineComment(value.mInlineComment), mAddQuotes(value.mAddQuotes), mArrayIndex(value.mArrayIndex), mHasArrayIndex(value.mHasArrayIndex), mArray(value.mArray)
            {
            }

            havINIData& operator=(havINIData&& value)
            {
#ifndef HAVINI_CASE_SENSITIVE
                mLocale = value.mLocale;
#endif
                mKey = value.mKey;
                mValue = value.mValue;
                mType = value.mType;
                mInlineComment = value.mInlineComment;
                mAddQuotes = value.mAddQuotes;
                mArrayIndex = value.mArrayIndex;
                mHasArrayIndex = value.mHasArrayIndex;
                mArray = value.mArray;

                return *this;
            }

            havINIData& operator=(const havINIData& value)
            {
                if (this != &value)
                {
#ifndef HAVINI_CASE_SENSITIVE
                    mLocale = value.mLocale;
#endif
                    mKey = value.mKey;
                    mValue = value.mValue;
                    mType = value.mType;
                    mInlineComment = value.mInlineComment;
                    mAddQuotes = value.mAddQuotes;
                    mArrayIndex = value.mArrayIndex;
                    mHasArrayIndex = value.mHasArrayIndex;
                    mArray = value.mArray;
                }

                return *this;
            }

            bool operator==(const havINIData& value) const
            {
                return (
#ifndef HAVINI_CASE_SENSITIVE
                    mLocale == value.mLocale &&
#endif
                    mKey == value.mKey &&
                    mValue == value.mValue &&
                    mType == value.mType &&
                    mInlineComment == value.mInlineComment &&
                    mAddQuotes == value.mAddQuotes &&
                    mArrayIndex == value.mArrayIndex &&
                    mHasArrayIndex == value.mHasArrayIndex &&
                    compareVectors(mArray, value.mArray));
            }

            bool compareVectors(const std::vector<havINIData>& vector1, const std::vector<havINIData>& vector2) const
            {
                return vector1.size() == vector2.size() && std::equal(vector1.begin(), vector1.end(), vector2.begin());
            }

            void operator=(const char* value)
            {
                mValue = value;
            }

            void operator=(const std::string& value)
            {
                mValue = value;
            }

            havINIData& operator[](int index)
            {
                if (mType != havINIDataType::Array)
                {
                    throw std::runtime_error("Property is not an array!");
                }

                if (index < 0 || index >= mArray.size())
                {
                    throw std::out_of_range("Index is out of range!");
                }

                return mArray[index];
            }

            havINIData& operator[](char key)
            {
                if (mType != havINIDataType::Array)
                {
                    throw std::runtime_error("Property is not an array!");
                }

                std::string tempKey{ key };

#ifndef HAVINI_CASE_SENSITIVE
                tempKey = havUtils::ToLower(tempKey, mLocale);
#endif

                auto foundKeyValuePair = std::find_if(mArray.begin(), mArray.end(), [&](const havINIData& data) { return data.GetKey() == tempKey; } );

                if (foundKeyValuePair == mArray.end())
                {
#ifdef HAVINI_CASE_SENSITIVE
                    mArray.emplace_back(tempKey, havINIDataType::Value);
#else
                    mArray.emplace_back(mLocale, tempKey, havINIDataType::Value);
#endif
                    foundKeyValuePair = std::find_if(mArray.begin(), mArray.end(), [&](const havINIData& data) { return data.GetKey() == tempKey; } );
                }

                return *foundKeyValuePair;
            }

            havINIData& operator[](const char* key)
            {
                if (mType != havINIDataType::Array)
                {
                    throw std::runtime_error("Property is not an array!");
                }

                std::string tempKey{ key };

#ifndef HAVINI_CASE_SENSITIVE
                tempKey = havUtils::ToLower(tempKey, mLocale);
#endif

                auto foundKeyValuePair = std::find_if(mArray.begin(), mArray.end(), [&](const havINIData& data) { return data.GetKey() == tempKey; } );

                if (foundKeyValuePair == mArray.end())
                {
#ifdef HAVINI_CASE_SENSITIVE
                    mArray.emplace_back(tempKey, havINIDataType::Value);
#else
                    mArray.emplace_back(mLocale, tempKey, havINIDataType::Value);
#endif
                    foundKeyValuePair = std::find_if(mArray.begin(), mArray.end(), [&](const havINIData& data) { return data.GetKey() == tempKey; } );
                }

                return *foundKeyValuePair;
            }

            havINIData& operator[](std::string key)
            {
                if (mType != havINIDataType::Array)
                {
                    throw std::runtime_error("Property is not an array!");
                }

#ifndef HAVINI_CASE_SENSITIVE
                key = havUtils::ToLower(key, mLocale);
#endif

                auto foundKeyValuePair = std::find_if(mArray.begin(), mArray.end(), [&](const havINIData& data) { return data.GetKey() == key; } );

                if (foundKeyValuePair == mArray.end())
                {
#ifdef HAVINI_CASE_SENSITIVE
                    mArray.emplace_back(key, havINIDataType::Value);
#else
                    mArray.emplace_back(mLocale, key, havINIDataType::Value);
#endif
                    foundKeyValuePair = std::find_if(mArray.begin(), mArray.end(), [&](const havINIData& data) { return data.GetKey() == key; } );
                }

                return *foundKeyValuePair;
            }

            std::vector<std::string> Split(const std::string& delimiter)
            {
                if (mType == havINIDataType::Array)
                {
                    throw std::runtime_error("Split is not supported by a property of type array!");
                }

                return havUtils::Split(mValue, delimiter);
            }

            void Join(const std::vector<std::string>& values, const std::string& delimiter)
            {
                if (mType == havINIDataType::Array)
                {
                    throw std::runtime_error("Join is not supported by a property of type array!");
                }

                std::string result = havUtils::Join(values, delimiter);

                SetValue(result);
            }

#ifndef HAVINI_CASE_SENSITIVE
            const std::locale& GetLocale() const { return mLocale; }
#endif
            const std::string& GetKey() const { return mKey; }
            havINIDataType GetType() const { return mType; }
            const std::string& GetValue() const { return mValue; }
            std::string GetInlineComment() const
            {
                if (HasInlineComment() == false)
                {
                    return "";
                }
                return mInlineComment.value();
            }

#ifndef HAVINI_CASE_SENSITIVE
            void SetLocale(const std::locale& loc) { mLocale = loc; }
#endif

            void SetValue(const std::string& value) { mValue = value; }

            void SetArrayEntry(std::string key, const std::string& value, bool addQuotes, bool setInlineComment, const std::string& inlineComment = "")
            {
                if (key.empty() == true)
                {
                    key = std::to_string(GetArrayIndex());
                }

#ifndef HAVINI_CASE_SENSITIVE
                key = havUtils::ToLower(key, mLocale);
#endif

                auto foundKeyValuePair = std::find_if(mArray.begin(), mArray.end(), [&](const havINIData& data) { return data.GetKey() == key; } );

                if (foundKeyValuePair == mArray.end())
                {
#ifdef HAVINI_CASE_SENSITIVE
                    mArray.emplace_back(key, value, havINIDataType::Value);
#else
                    mArray.emplace_back(mLocale, key, value, havINIDataType::Value);
#endif

                    if (setInlineComment == true)
                    {
                        foundKeyValuePair = std::find_if(mArray.begin(), mArray.end(), [&](const havINIData& data) { return data.GetKey() == key; } );
                        foundKeyValuePair->SetInlineComment(inlineComment);
                        foundKeyValuePair->SetAddQuotes(addQuotes);
                    }
                }
                else
                {
                    foundKeyValuePair->SetValue(value);
                    foundKeyValuePair->SetInlineComment(inlineComment);
                    foundKeyValuePair->SetAddQuotes(addQuotes);
                }
            }

            void ArrayClear()
            {
                if (mType == havINIDataType::Array)
                {
                    mArray.clear();

                    return;
                }

                throw std::runtime_error("Data is not of type array!");
            }

            bool ArrayEmpty()
            {
                if (mType == havINIDataType::Array)
                {
                    return mArray.empty();
                }

                throw std::runtime_error("Data is not of type array!");
            }

            void ArrayErase(std::vector<havINIData>::iterator itr)
            {
                if (mType == havINIDataType::Array)
                {
                    mArray.erase(itr);

                    return;
                }

                throw std::runtime_error("Data is not of type array!");
            }

            std::vector<havINIData>::const_iterator ArrayCBegin() const
            {
                if (mType == havINIDataType::Array)
                {
                    return mArray.cbegin();
                }

                throw std::runtime_error("Data is not of type array!");
            }

            std::vector<havINIData>::const_iterator ArrayCEnd() const
            {
                if (mType == havINIDataType::Array)
                {
                    return mArray.cend();
                }

                throw std::runtime_error("Data is not of type array!");
            }

            std::vector<havINIData>::iterator ArrayBegin()
            {
                if (mType == havINIDataType::Array)
                {
                    return mArray.begin();
                }

                throw std::runtime_error("Data is not of type array!");
            }

            std::vector<havINIData>::iterator ArrayEnd()
            {
                if (mType == havINIDataType::Array)
                {
                    return mArray.end();
                }

                throw std::runtime_error("Data is not of type array!");
            }

            havINIData& ArrayFront()
            {
                try
                {
                    if (mType != havINIDataType::Array)
                    {
                        throw std::runtime_error("Data is not of type array!");
                    }

                    return mArray.front();
                }
                catch (const std::out_of_range& ex)
                {
                    std::cout << "Exception thrown after calling front method of array!\n";

                    throw;
                }
            }

            havINIData& ArrayBack()
            {
                try
                {
                    if (mType != havINIDataType::Array)
                    {
                        throw std::runtime_error("Data is not of type array!");
                    }

                    return mArray.back();
                }
                catch (const std::out_of_range& ex)
                {
                    std::cout << "Exception thrown after calling back method of array!\n";

                    throw;
                }
            }

            void ArrayInsert(unsigned int index, const havINIData& newValue)
            {
                if (mType == havINIDataType::Array)
                {
                    auto itr = mArray.begin();

                    mArray.insert(itr + index, newValue);

                    return;
                }

                throw std::runtime_error("Data is not of type array!");
            }

            void ArrayPushBack(const havINIData& newValue)
            {
                if (mType == havINIDataType::Array)
                {
                    mArray.push_back(newValue);

                    return;
                }

                throw std::runtime_error("Data is not of type array!");
            }

            void ArrayPushFront(const havINIData& newValue)
            {
                if (mType == havINIDataType::Array)
                {
                    auto itr = mArray.begin();

                    mArray.insert(itr, newValue);

                    return;
                }

                throw std::runtime_error("Data is not of type array!");
            }

            void ArrayPopBack()
            {
                if (mType == havINIDataType::Array)
                {
                    if (mArray.empty() == false)
                    {
                        mArray.pop_back();
                    }

                    return;
                }

                throw std::runtime_error("Data is not of type array!");
            }

            void ArrayPopFront()
            {
                if (mType == havINIDataType::Array)
                {
                    if (mArray.empty() == false)
                    {
                        auto itr = mArray.begin();

                        mArray.erase(itr);
                    }

                    return;
                }

                throw std::runtime_error("Data is not of type array!");
            }

            void ArrayRemove(unsigned int index)
            {
                if (mType == havINIDataType::Array)
                {
                    if (mArray.empty() == false)
                    {
                        auto itr = mArray.begin();

                        mArray.erase(itr + index);
                    }

                    return;
                }

                throw std::runtime_error("Data is not of type array!");
            }

            void ArrayRemove(std::string keyName)
            {
                if (mType == havINIDataType::Array)
                {
#ifndef HAVINI_CASE_SENSITIVE
                    keyName = havUtils::ToLower(keyName, mLocale);
#endif

                    auto itr = std::find_if(mArray.begin(), mArray.end(), [&](const havINIData& data) { return data.GetKey() == keyName; } );

                    if (itr == mArray.end())
                    {
                        throw std::runtime_error("Provided key could not be found in array!");
                    }

                    mArray.erase(itr);

                    return;
                }

                throw std::runtime_error("Data is not of type array!");
            }

            bool ArrayContains(const havINIData& value)
            {
                if (mType == havINIDataType::Array)
                {
                    return std::find(mArray.begin(), mArray.end(), value) != mArray.end();
                }

                throw std::runtime_error("Data is not of type array!");
            }

            havINIData& ArrayAt(unsigned int index)
            {
                try
                {
                    if (mType != havINIDataType::Array)
                    {
                        throw std::runtime_error("Data is not of type array!");
                    }

                    return mArray.at(index);
                }
                catch (const std::out_of_range& ex)
                {
                    std::cout << "Exception thrown after calling at method of array!\n";

                    throw;
                }
            }

            std::vector<havINIData>::size_type ArraySize()
            {
                if (mType == havINIDataType::Array)
                {
                    return mArray.size();
                }

                throw std::runtime_error("Data is not of type array!");
            }

            bool HasInlineComment() const { return mInlineComment.has_value(); }
            void SetInlineComment(const std::string& inlineComment)
            {
                if (inlineComment.empty() == true)
                {
                    mInlineComment = std::nullopt;

                    return;
                }

                mInlineComment = inlineComment;
            }

            bool GetAddQuotes() const { return mAddQuotes; }
            void SetAddQuotes(bool addQuotes) { mAddQuotes = addQuotes; }

            unsigned int GetArrayIndex()
            {
                unsigned int arrayIndex = 0;

                for (const auto& arrayEntry : mArray)
                {
                    unsigned int arrayKey = std::stol(arrayEntry.GetKey());

                    if (arrayIndex <= arrayKey)
                    {
                        arrayIndex = arrayKey + 1;
                    }
                }

                mArrayIndex = arrayIndex;

                return mArrayIndex;
            }

            void SetHasArrayIndex(bool hasArrayIndex) { mHasArrayIndex = hasArrayIndex; }
            bool HasArrayIndex() const { return mHasArrayIndex; }

        private:
            void SetKey(std::string key)
            {
#ifndef HAVINI_CASE_SENSITIVE
                key = havUtils::ToLower(key, mLocale);
#endif

                mKey = key;
            }

            friend class havINISection;

#ifndef HAVINI_CASE_SENSITIVE
            std::locale mLocale;
#endif
            havINIDataType mType;
            std::string mKey;
            std::string mValue;
            std::optional<std::string> mInlineComment;
            bool mAddQuotes;

            unsigned int mArrayIndex;
            bool mHasArrayIndex;
            std::vector<havINIData> mArray;
    };

    class havINISection
    {
    public:
        explicit havINISection(
#ifndef HAVINI_CASE_SENSITIVE
        const std::locale& loc,
#endif
        const std::string& sectionName, std::optional<std::string> inlineComment = std::nullopt, const std::vector<havINIData>& keyValuePairs = {}) :
#ifndef HAVINI_CASE_SENSITIVE
        mLocale(loc),
#endif
        mSectionName(sectionName), mInlineComment(inlineComment), mKeyValuePairs(keyValuePairs), mCommentLineCount(0), mEmptyLineCount(0)
        {
        }

        explicit havINISection(havINISection&& value) :
#ifndef HAVINI_CASE_SENSITIVE
        mLocale(value.mLocale),
#endif
        mSectionName(value.mSectionName), mInlineComment(value.mInlineComment), mKeyValuePairs(value.mKeyValuePairs), mCommentLineCount(value.mCommentLineCount), mEmptyLineCount(value.mEmptyLineCount)
        {
        }

        explicit havINISection(const havINISection& value) :
#ifndef HAVINI_CASE_SENSITIVE
        mLocale(value.mLocale),
#endif
        mSectionName(value.mSectionName), mInlineComment(value.mInlineComment), mKeyValuePairs(value.mKeyValuePairs), mCommentLineCount(value.mCommentLineCount), mEmptyLineCount(value.mEmptyLineCount)
        {
        }

        havINISection& operator=(havINISection&& value)
        {
#ifndef HAVINI_CASE_SENSITIVE
            mLocale = value.mLocale;
#endif
            mSectionName = value.mSectionName;
            mInlineComment = value.mInlineComment;
            mKeyValuePairs = value.mKeyValuePairs;
            mCommentLineCount = value.mCommentLineCount;
            mEmptyLineCount = value.mEmptyLineCount;

            return *this;
        }

        havINISection& operator=(const havINISection& value)
        {
            if (this != &value)
            {
#ifndef HAVINI_CASE_SENSITIVE
                mLocale = value.mLocale;
#endif
                mSectionName = value.mSectionName;
                mInlineComment = value.mInlineComment;
                mKeyValuePairs = value.mKeyValuePairs;
                mCommentLineCount = value.mCommentLineCount;
                mEmptyLineCount = value.mEmptyLineCount;
            }

            return *this;
        }

        havINIData& operator[](int index)
        {
            if (index < 0 || index >= mKeyValuePairs.size())
            {
                throw std::out_of_range("Index is out of range!");
            }

            return mKeyValuePairs[index];
        }

        havINIData& operator[](char key)
        {
            std::string tempKey{ key };

#ifndef HAVINI_CASE_SENSITIVE
            tempKey = havUtils::ToLower(tempKey, mLocale);
#endif

            auto foundKeyValuePair = std::find_if(mKeyValuePairs.begin(), mKeyValuePairs.end(), [&](const havINIData& data) { return data.GetKey() == tempKey; } );

            if (foundKeyValuePair == mKeyValuePairs.end())
            {
#ifdef HAVINI_CASE_SENSITIVE
                mKeyValuePairs.emplace_back(tempKey, havINIDataType::Value);
#else
                mKeyValuePairs.emplace_back(mLocale, tempKey, havINIDataType::Value);
#endif
                foundKeyValuePair = std::find_if(mKeyValuePairs.begin(), mKeyValuePairs.end(), [&](const havINIData& data) { return data.GetKey() == tempKey; } );
            }

            return *foundKeyValuePair;
        }

        havINIData& operator[](const char* key)
        {
            std::string tempKey{ key };

#ifndef HAVINI_CASE_SENSITIVE
            tempKey = havUtils::ToLower(tempKey, mLocale);
#endif

            auto foundKeyValuePair = std::find_if(mKeyValuePairs.begin(), mKeyValuePairs.end(), [&](const havINIData& data) { return data.GetKey() == tempKey; } );

            if (foundKeyValuePair == mKeyValuePairs.end())
            {
#ifdef HAVINI_CASE_SENSITIVE
                mKeyValuePairs.emplace_back(tempKey, havINIDataType::Value);
#else
                mKeyValuePairs.emplace_back(mLocale, tempKey, havINIDataType::Value);
#endif
                foundKeyValuePair = std::find_if(mKeyValuePairs.begin(), mKeyValuePairs.end(), [&](const havINIData& data) { return data.GetKey() == tempKey; } );
            }

            return *foundKeyValuePair;
        }

        havINIData& operator[](std::string key)
        {
#ifndef HAVINI_CASE_SENSITIVE
            key = havUtils::ToLower(key, mLocale);
#endif

            auto foundKeyValuePair = std::find_if(mKeyValuePairs.begin(), mKeyValuePairs.end(), [&](const havINIData& data) { return data.GetKey() == key; } );

            if (foundKeyValuePair == mKeyValuePairs.end())
            {
#ifdef HAVINI_CASE_SENSITIVE
                mKeyValuePairs.emplace_back(key, havINIDataType::Value);
#else
                mKeyValuePairs.emplace_back(mLocale, key, havINIDataType::Value);
#endif
                foundKeyValuePair = std::find_if(mKeyValuePairs.begin(), mKeyValuePairs.end(), [&](const havINIData& data) { return data.GetKey() == key; } );
            }

            return *foundKeyValuePair;
        }

        void SetSectionName(std::string sectionName)
        {
#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
#endif

            mSectionName = sectionName;
        }

        void SetInlineComment(const std::string& inlineComment)
        {
            if (inlineComment.empty() == true)
            {
                mInlineComment = std::nullopt;

                return;
            }

            mInlineComment = inlineComment;
        }

        bool SetEmptyLine(std::string key, const havINIPosition& position, std::optional<std::string> otherKeyName = std::nullopt)
        {
#ifndef HAVINI_CASE_SENSITIVE
            key = havUtils::ToLower(key, mLocale);

            if (otherKeyName.has_value() == true)
            {
                otherKeyName = havUtils::ToLower(otherKeyName.value(), mLocale);
            }
#endif

            auto foundKeyValuePair = std::find_if(mKeyValuePairs.begin(), mKeyValuePairs.end(), [&](const havINIData& data) { return data.GetKey() == key; } );

            if (foundKeyValuePair == mKeyValuePairs.end())
            {
#ifdef HAVINI_CASE_SENSITIVE
                havINIData newEmptyLine(key, "", havINIDataType::Empty);
#else
                havINIData newEmptyLine(mLocale, key, "", havINIDataType::Empty);
#endif

                std::ptrdiff_t index = 0;

                if (position == havINIPosition::Above || position == havINIPosition::Below)
                {
                    auto foundOtherKeyValuePair = std::find_if(mKeyValuePairs.begin(), mKeyValuePairs.end(), [&](const havINIData& data) { return data.GetKey() == otherKeyName; } );

                    if (foundOtherKeyValuePair != mKeyValuePairs.end())
                    {
                        index = GetIndex((*foundOtherKeyValuePair));
                    }
                    else
                    {
                        throw std::runtime_error("\"" + otherKeyName.value() + "\" could not be found!");
                    }
                }

                switch (position)
                {
                case havINIPosition::Start:
                    mKeyValuePairs.insert(mKeyValuePairs.begin(), newEmptyLine);
                    break;

                case havINIPosition::Above:
                    mKeyValuePairs.insert(mKeyValuePairs.begin() + index, newEmptyLine);
                    break;

                case havINIPosition::Below:
                    mKeyValuePairs.insert(mKeyValuePairs.begin() + index + 1, newEmptyLine);
                    break;

                case havINIPosition::End:
                default:
                    mKeyValuePairs.insert(mKeyValuePairs.end(), newEmptyLine);
                    break;
                }

                return true;
            }

            return false;
        }

        void SetKeyValuePair(std::string key, const std::string& value, bool addQuotes)
        {
#ifndef HAVINI_CASE_SENSITIVE
            key = havUtils::ToLower(key, mLocale);
#endif

            auto foundKeyValuePair = std::find_if(mKeyValuePairs.begin(), mKeyValuePairs.end(), [&](const havINIData& data) { return data.GetKey() == key; } );

            if (foundKeyValuePair != mKeyValuePairs.end())
            {
                foundKeyValuePair->SetValue(value);
                foundKeyValuePair->SetAddQuotes(addQuotes);
            }
            else
            {
#ifdef HAVINI_CASE_SENSITIVE
                mKeyValuePairs.emplace_back(key, value, havINIDataType::Value, addQuotes);
#else
                mKeyValuePairs.emplace_back(mLocale, key, value, havINIDataType::Value, addQuotes);
#endif
            }
        }

        void SetArrayEntry(std::string key, const std::string& value, bool addQuotes, bool setInlineComment, const std::string& inlineComment = "", const std::string& arrayIndex = "", bool hasArrayIndex = false)
        {
#ifndef HAVINI_CASE_SENSITIVE
            key = havUtils::ToLower(key, mLocale);
#endif

            auto foundKeyValuePair = std::find_if(mKeyValuePairs.begin(), mKeyValuePairs.end(), [&](const havINIData& data) { return data.GetKey() == key; } );

            if (foundKeyValuePair != mKeyValuePairs.end())
            {
                if (hasArrayIndex == true)
                {
                    foundKeyValuePair->SetArrayEntry(arrayIndex, value, addQuotes, setInlineComment, inlineComment);
                }
                else
                {
                    foundKeyValuePair->SetArrayEntry("", value, addQuotes, setInlineComment, inlineComment);
                }
            }
            else
            {
#ifdef HAVINI_CASE_SENSITIVE
                mKeyValuePairs.emplace_back(key, havINIDataType::Array, hasArrayIndex);
#else
                mKeyValuePairs.emplace_back(mLocale, key, havINIDataType::Array, hasArrayIndex);
#endif

                auto foundNewKeyValuePair = std::find_if(mKeyValuePairs.begin(), mKeyValuePairs.end(), [&](const havINIData& data) { return data.GetKey() == key; } );

                if (hasArrayIndex == true)
                {
                    foundNewKeyValuePair->SetArrayEntry(arrayIndex, value, addQuotes, setInlineComment, inlineComment);
                }
                else
                {
                    foundNewKeyValuePair->SetArrayEntry("", value, addQuotes, setInlineComment, inlineComment);
                }
            }
        }

        std::ptrdiff_t GetIndex(havINIData& data)
        {
            return std::distance(mKeyValuePairs.data(), std::addressof(data));
        }

        bool SetComment(std::string key, const std::string& value, const havINIPosition& position, std::optional<std::string> otherKeyName = std::nullopt)
        {
#ifndef HAVINI_CASE_SENSITIVE
            key = havUtils::ToLower(key, mLocale);

            if (otherKeyName.has_value() == true)
            {
                otherKeyName = havUtils::ToLower(otherKeyName.value(), mLocale);
            }
#endif

            auto foundKeyValuePair = std::find_if(mKeyValuePairs.begin(), mKeyValuePairs.end(), [&](const havINIData& data) { return data.GetKey() == key; } );

            if (foundKeyValuePair == mKeyValuePairs.end())
            {
#ifdef HAVINI_CASE_SENSITIVE
                havINIData newComment(key, value, havINIDataType::Comment);
#else
                havINIData newComment(mLocale, key, value, havINIDataType::Comment);
#endif

                std::ptrdiff_t index = 0;

                if (position == havINIPosition::Above || position == havINIPosition::Below)
                {
                    auto foundOtherKeyValuePair = std::find_if(mKeyValuePairs.begin(), mKeyValuePairs.end(), [&](const havINIData& data) { return data.GetKey() == otherKeyName; } );

                    if (foundOtherKeyValuePair != mKeyValuePairs.end())
                    {
                        index = GetIndex((*foundOtherKeyValuePair));
                    }
                    else
                    {
                        throw std::runtime_error("\"" + otherKeyName.value() + "\" could not be found!");
                    }
                }

                switch (position)
                {
                case havINIPosition::Start:
                    mKeyValuePairs.insert(mKeyValuePairs.begin(), newComment);
                    break;

                case havINIPosition::Above:
                    mKeyValuePairs.insert(mKeyValuePairs.begin() + index, newComment);
                    break;

                case havINIPosition::Below:
                    mKeyValuePairs.insert(mKeyValuePairs.begin() + index + 1, newComment);
                    break;

                case havINIPosition::End:
                default:
                    mKeyValuePairs.insert(mKeyValuePairs.end(), newComment);
                    break;
                }

                return true;
            }

            return false;
        }

        void SetKey(std::string key, std::vector<havINIData>::iterator it)
        {
#ifndef HAVINI_CASE_SENSITIVE
            key = havUtils::ToLower(key, mLocale);
#endif

            it->SetKey(key);
        }

        const std::string& GetSectionName() const { return mSectionName; }
        std::string GetInlineComment() const
        {
            if (HasInlineComment() == false)
            {
                return "";
            }
            return mInlineComment.value();
        }
        const std::vector<havINIData>& GetKeyValuePairs() const { return mKeyValuePairs; } // Read-only

        std::vector<havINIData>::iterator GetKeyValuePair(std::string key)
        {
#ifndef HAVINI_CASE_SENSITIVE
            key = havUtils::ToLower(key, mLocale);
#endif

            return std::find_if(mKeyValuePairs.begin(), mKeyValuePairs.end(), [&](const havINIData& data) { return data.GetKey() == key; } );
        }

        bool HasInlineComment() const { return mInlineComment.has_value(); }

        bool HasKey(std::string keyName)
        {
#ifndef HAVINI_CASE_SENSITIVE
            keyName = havUtils::ToLower(keyName, mLocale);
#endif

            return std::find_if(mKeyValuePairs.begin(), mKeyValuePairs.end(), [&](const havINIData& data) { return data.GetKey() == keyName; } ) != mKeyValuePairs.end();
        }

        std::vector<havINIData>::size_type GetNumberOfKeys() const
        {
            return mKeyValuePairs.size();
        }

        void RemoveKeyValuePair(std::vector<havINIData>::iterator it)
        {
            mKeyValuePairs.erase(it);
        }

        bool RemoveKeyValuePair(std::string keyName)
        {
#ifndef HAVINI_CASE_SENSITIVE
            keyName = havUtils::ToLower(keyName, mLocale);
#endif

            auto foundKeyValuePair = std::find_if(mKeyValuePairs.begin(), mKeyValuePairs.end(), [&](const havINIData& data) { return data.GetKey() == keyName; } );

            if (foundKeyValuePair != mKeyValuePairs.end())
            {
                mKeyValuePairs.erase(foundKeyValuePair);

                return true;
            }

            return false;
        }

        unsigned int GetCommentLineCount() { return ++mCommentLineCount; }
        unsigned int GetEmptyLineCount() { return ++mEmptyLineCount; }

        std::vector<std::string> GetCommentKeyNames(std::string keyName)
        {
#ifndef HAVINI_CASE_SENSITIVE
            keyName = havUtils::ToLower(keyName, mLocale);
#endif

            std::vector<std::string> commentKeyNames;

            for (const auto& keyValuePair : mKeyValuePairs)
            {
                const std::string& key = keyValuePair.GetKey();

                if (havUtils::StartsWith(key, keyName) == true && keyValuePair.GetType() == havINIDataType::Comment)
                {
                    commentKeyNames.emplace_back(key);
                }
            }

            return commentKeyNames;
        }

        bool RemoveComment(std::string keyName)
        {
#ifndef HAVINI_CASE_SENSITIVE
            keyName = havUtils::ToLower(keyName, mLocale);
#endif

            auto foundComment = std::find_if(mKeyValuePairs.begin(), mKeyValuePairs.end(), [&](const havINIData& data) { return data.GetKey() == keyName && data.GetType() == havINIDataType::Comment; } );

            if (foundComment != mKeyValuePairs.end())
            {
                mKeyValuePairs.erase(foundComment);

                return true;
            }

            return false;
        }

        std::vector<std::string> GetEmptyLineKeyNames(std::string keyName)
        {
#ifndef HAVINI_CASE_SENSITIVE
            keyName = havUtils::ToLower(keyName, mLocale);
#endif

            std::vector<std::string> emptyLineKeyNames;

            for (const auto& keyValuePair : mKeyValuePairs)
            {
                const std::string& key = keyValuePair.GetKey();

                if (havUtils::StartsWith(key, keyName) == true && keyValuePair.GetType() == havINIDataType::Empty)
                {
                    emptyLineKeyNames.emplace_back(key);
                }
            }

            return emptyLineKeyNames;
        }

        bool RemoveEmptyLine(std::string keyName)
        {
#ifndef HAVINI_CASE_SENSITIVE
            keyName = havUtils::ToLower(keyName, mLocale);
#endif

            auto foundEmptyLine = std::find_if(mKeyValuePairs.begin(), mKeyValuePairs.end(), [&](const havINIData& data) { return data.GetKey() == keyName && data.GetType() == havINIDataType::Empty; } );

            if (foundEmptyLine != mKeyValuePairs.end())
            {
                mKeyValuePairs.erase(foundEmptyLine);

                return true;
            }

            return false;
        }

        void Clear()
        {
            mInlineComment.reset();
            mKeyValuePairs.clear();

            mCommentLineCount = 0;
            mEmptyLineCount = 0;
        }

#ifndef HAVINI_CASE_SENSITIVE
        const std::locale& GetLocale() const { return mLocale; }

        void SetLocale(const std::locale& loc) { mLocale = loc; }
#endif

    private:
#ifndef HAVINI_CASE_SENSITIVE
        std::locale mLocale;
#endif
        std::string mSectionName;
        std::optional<std::string> mInlineComment;
        std::vector<havINIData> mKeyValuePairs;

        unsigned int mCommentLineCount;
        unsigned int mEmptyLineCount;
    };

    class havINIStream
    {
    public:
        havINIStream()
        {
#ifdef HAVINI_CASE_SENSITIVE
            mData.emplace_back("HI_Global");
#else
            mData.emplace_back(mLocale, "hi_global");
#endif
        }

        havINISection& operator[](int index)
        {
            if (index < 0 || index >= mData.size())
            {
                throw std::out_of_range("Index is out of range!");
            }

            return mData[index];
        }

        havINISection& operator[](char sectionName)
        {
            std::string tempSectionName{ sectionName };

#ifndef HAVINI_CASE_SENSITIVE
            tempSectionName = havUtils::ToLower(tempSectionName, mLocale);
#endif

            auto foundSection = std::find_if(mData.begin(), mData.end(), [&](const havINISection& data) { return data.GetSectionName() == tempSectionName; } );

            if (foundSection == mData.end())
            {
#ifdef HAVINI_CASE_SENSITIVE
                mData.emplace_back(tempSectionName);
#else
                mData.emplace_back(mLocale, tempSectionName);
#endif
                foundSection = std::find_if(mData.begin(), mData.end(), [&](const havINISection& data) { return data.GetSectionName() == tempSectionName; } );
            }

            return *foundSection;
        }

        havINISection& operator[](const char* sectionName)
        {
            std::string tempSectionName{ sectionName };

#ifndef HAVINI_CASE_SENSITIVE
            tempSectionName = havUtils::ToLower(tempSectionName, mLocale);
#endif

            auto foundSection = std::find_if(mData.begin(), mData.end(), [&](const havINISection& data) { return data.GetSectionName() == tempSectionName; } );

            if (foundSection == mData.end())
            {
#ifdef HAVINI_CASE_SENSITIVE
                mData.emplace_back(tempSectionName);
#else
                mData.emplace_back(mLocale, tempSectionName);
#endif
                foundSection = std::find_if(mData.begin(), mData.end(), [&](const havINISection& data) { return data.GetSectionName() == tempSectionName; } );
            }

            return *foundSection;
        }

        havINISection& operator[](std::string sectionName)
        {
#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
#endif

            auto foundSection = std::find_if(mData.begin(), mData.end(), [&](const havINISection& data) { return data.GetSectionName() == sectionName; } );

            if (foundSection == mData.end())
            {
#ifdef HAVINI_CASE_SENSITIVE
                mData.emplace_back(sectionName);
#else
                mData.emplace_back(mLocale, sectionName);
#endif
                foundSection = std::find_if(mData.begin(), mData.end(), [&](const havINISection& data) { return data.GetSectionName() == sectionName; } );
            }

            return *foundSection;
        }

        std::string CodePointToString(const unsigned int codePoint)
        {
            // Note: null-terminated string
            char value[5] = { '\0' };

            if (codePoint < 0x80)
            {
                value[0] = codePoint;
            }
            else if (codePoint < 0x800)
            {
                value[0] = (codePoint >> 6) | 0xc0;
                value[1] = (codePoint & 0x3f) | 0x80;
            }
            else if (codePoint < 0x10000)
            {
                value[0] = (codePoint >> 12) | 0xe0;
                value[1] = ((codePoint >> 6) & 0x3f) | 0x80;
                value[2] = (codePoint & 0x3f) | 0x80;
            }
            else if (codePoint < 0x110000)
            {
                value[0] = (codePoint >> 18) | 0xf0;
                value[1] = ((codePoint >> 12) & 0x3f) | 0x80;
                value[2] = ((codePoint >> 6) & 0x3f) | 0x80;
                value[3] = (codePoint & 0x3f) | 0x80;
            }

            return std::string(value);
        }

        std::string ConvertToEscapedString(const std::string& value)
        {
            std::string resultValue;

            unsigned int codePoint = 0;
            int numOfBytes = 0;
            bool writeAsHex = false;

            for (std::string::size_type index = 0; index < value.size(); ++index)
            {
                switch (value[index])
                {
                case '"':
                    resultValue += "\\\"";
                    break;

                case '\\':
                    resultValue += "\\\\";
                    break;

                case '\b':
                    resultValue += "\\b";
                    break;

                case '\f':
                    resultValue += "\\f";
                    break;

                case '\n':
                    resultValue += "\\n";
                    break;

                case '\r':
                    resultValue += "\\r";
                    break;

                case '\t':
                    resultValue += "\\t";
                    break;

                case '\v':
                    resultValue += "\\v";
                    break;

                default:
                    {
                        writeAsHex = false;
                        codePoint = 0;

                        if ((value[index] & 0x80) == 0x00)
                        {
                            numOfBytes = 1;

                            if (value[index] < 0x1f)
                            {
                                writeAsHex = true;

                                codePoint = (value[index] & 0x7f);
                            }
                        }
                        else if ((value[index] & 0xe0) == 0xc0)
                        {
                            numOfBytes = 2;

                            writeAsHex = true;

                            codePoint = (value[index] & 0x1f);
                        }
                        else if ((value[index] & 0xf0) == 0xe0)
                        {
                            numOfBytes = 3;

                            writeAsHex = true;

                            codePoint = (value[index] & 0x0f);
                        }
                        else if ((value[index] & 0xf8) == 0xf0)
                        {
                            numOfBytes = 4;

                            writeAsHex = true;

                            codePoint = (value[index] & 0x07);
                        }
                        else
                        {
                            throw std::runtime_error("Invalid UTF-8 sequence!");
                        }

                        for (int byteCount = 1; byteCount < numOfBytes; ++byteCount)
                        {
                            if ((value[++index] & 0xc0) != 0x80)
                            {
                                throw std::runtime_error("Invalid UTF-8 sequence!");
                            }

                            codePoint = (codePoint << 6) | (value[index] & 0x3f);
                        }

                        if (writeAsHex == true)
                        {
                            std::ostringstream outputStringStream;

                            // Code point is UTF-16 surrogate pair
                            if (codePoint >= 0x10000 && codePoint <= 0x10ffff)
                            {
                                codePoint -= 0x10000;

                                unsigned int highSurrogate = (codePoint / 0x400) + 0xd800;
                                unsigned int lowSurrogate = (codePoint % 0x400) + 0xdc00;

                                outputStringStream << "\\x" << std::hex << std::setw(4) << std::setfill('0') << highSurrogate;
                                outputStringStream << "\\x" << std::hex << std::setw(4) << std::setfill('0') << lowSurrogate;

                                resultValue += outputStringStream.str();
                            }
                            else
                            {
                                outputStringStream << "\\x" << std::hex << std::setw(4) << std::setfill('0') << codePoint;

                                resultValue += outputStringStream.str();
                            }
                        }
                        else
                        {
                            resultValue += static_cast<unsigned int>(value[index]);
                        }
                    }
                    break;
                }
            }

            return resultValue;
        }

        bool ParseFile(const std::string& fileName)
        {
#ifdef _WIN32
            std::unique_ptr<std::FILE, decltype(&std::fclose)> fileStream(_wfopen(&ConvertStringToWString(fileName)[0], L"rb"), std::fclose);
#else
            std::unique_ptr<std::FILE, decltype(&std::fclose)> fileStream(std::fopen(fileName.c_str(), "rb"), std::fclose);
#endif

            if (fileStream == nullptr)
            {
                std::cout << "Unable to open INI file: " << fileName << "\n";

                return false;
            }

            // Get file size
            std::fseek(fileStream.get(), 0, SEEK_END);
            long fileSize = std::ftell(fileStream.get());
            std::fseek(fileStream.get(), 0, SEEK_SET);

            if (fileSize == 0)
            {
                std::cout << "INI file is empty!\n";

                return false;
            }

            if (fileSize < 6)
            {
                std::cout << "INI file cannot be smaller than 6 bytes! (Size in bytes: " << fileSize << ")\n";

                return false;
            }

            unsigned char unsignedCurrentChar;

            // Check for BOM (Byte order mark)
            unsigned char bomArray[4] = { '\0' };

            // Read the first four bytes of the file
            for (int index = 0; index < 4; ++index)
            {
                std::fread(&unsignedCurrentChar, sizeof(unsigned char), 1, fileStream.get());

                bomArray[index] = unsignedCurrentChar;
            }

            int bytesToSkip = 0;
            int skippedBytes = 0;
            havINIBOMType bomType = havINIBOMType::None;

            if (bomArray[0] == 0xff && bomArray[1] == 0xfe &&
                bomArray[2] == 0x00 && bomArray[3] == 0x00)
            {
                bomType = havINIBOMType::UTF32LE;

                bytesToSkip = 4;
            }
            else if (bomArray[0] == 0x00 && bomArray[1] == 0x00 &&
                     bomArray[2] == 0xfe && bomArray[3] == 0xff)
            {
                bomType = havINIBOMType::UTF32BE;

                bytesToSkip = 4;
            }
            else if (bomArray[0] == 0xff && bomArray[1] == 0xfe)
            {
                bomType = havINIBOMType::UTF16LE;

                bytesToSkip = 2;
            }
            else if (bomArray[0] == 0xfe && bomArray[1] == 0xff)
            {
                bomType = havINIBOMType::UTF16BE;

                bytesToSkip = 2;
            }
            else if (bomArray[0] == 0xef && bomArray[1] == 0xbb && bomArray[2] == 0xbf)
            {
                bomType = havINIBOMType::UTF8;

                bytesToSkip = 3;
            }

            // If no BOM has been found, we still need to check for the file encoding
            if (bomType == havINIBOMType::None)
            {
                if (bomArray[0] != 0x00 && bomArray[1] == 0x00 &&
                    bomArray[2] == 0x00 && bomArray[3] == 0x00)
                {
                    bomType = havINIBOMType::UTF32LE;
                }
                else if (bomArray[0] == 0x00 && bomArray[1] == 0x00 &&
                         bomArray[2] == 0x00 && bomArray[3] != 0x00)
                {
                    bomType = havINIBOMType::UTF32BE;
                }
                else if (bomArray[0] != 0x00 && bomArray[1] == 0x00 &&
                         bomArray[2] != 0x00 && bomArray[3] == 0x00)
                {
                    bomType = havINIBOMType::UTF16LE;
                }
                else if (bomArray[0] == 0x00 && bomArray[1] != 0x00 &&
                         bomArray[2] == 0x00 && bomArray[3] != 0x00)
                {
                    bomType = havINIBOMType::UTF16BE;
                }
            }

            if (bomType != havINIBOMType::None)
            {
                std::string bomTypeString;

                switch (bomType)
                {
                case havINIBOMType::UTF16LE:
                    bomTypeString = "UTF-16 Little Endian";
                    break;

                case havINIBOMType::UTF16BE:
                    bomTypeString = "UTF-16 Big Endian";
                    break;

                case havINIBOMType::UTF32LE:
                    bomTypeString = "UTF-32 Little Endian";
                    break;

                case havINIBOMType::UTF32BE:
                    bomTypeString = "UTF-32 Big Endian";
                    break;

                default:
                    bomTypeString = "UTF-8";
                    break;
                }

                std::cout << "INI file starts with " << bomTypeString << " BOM! Please note that by default the BOM will be skipped, and removed in case the INI file gets saved and the BOM was not specified!\n";
            }

            std::fseek(fileStream.get(), 0, SEEK_SET);

            std::string fileContents;
            std::u16string fileContentsU16;
            std::u32string fileContentsU32;

            if (bomType == havINIBOMType::None ||
                bomType == havINIBOMType::UTF8)
            {
                char currentChar;
                while (std::fread(&currentChar, sizeof(char), 1, fileStream.get()) > 0)
                {
                    if (bytesToSkip > 0)
                    {
                        if (skippedBytes < bytesToSkip)
                        {
                            ++skippedBytes;
                            continue;
                        }
                    }
                    fileContents += currentChar;
                }
            }
            else if (bomType == havINIBOMType::UTF16LE ||
                     bomType == havINIBOMType::UTF16BE)
            {
                char16_t currentCharU16;
                while (std::fread(&currentCharU16, sizeof(char16_t), 1, fileStream.get()) > 0)
                {
                    if (bytesToSkip > 0)
                    {
                        if (skippedBytes < bytesToSkip)
                        {
                            skippedBytes += sizeof(char16_t);
                            continue;
                        }
                    }
                    fileContentsU16 += currentCharU16;
                }
            }
            else if (bomType == havINIBOMType::UTF32LE ||
                     bomType == havINIBOMType::UTF32BE)
            {
                char32_t currentCharU32;
                while (std::fread(&currentCharU32, sizeof(char32_t), 1, fileStream.get()) > 0)
                {
                    if (bytesToSkip > 0)
                    {
                        if (skippedBytes < bytesToSkip)
                        {
                            skippedBytes += sizeof(char32_t);
                            continue;
                        }
                    }
                    fileContentsU32 += currentCharU32;
                }
            }

            // Convert the file contents to UTF-8, if necessary
            if (bomType == havINIBOMType::UTF16LE)
            {
                fileContents = std::wstring_convert<havINICodeCvt<char16_t, char, std::mbstate_t>, char16_t>{}.to_bytes(fileContentsU16);
            }
            else if (bomType == havINIBOMType::UTF16BE)
            {
                std::u16string u16ConvBE;
                for (char16_t currentChar : fileContentsU16)
                {
                    u16ConvBE += (((currentChar & 0x00ff) << 8) | ((currentChar & 0xff00) >> 8));
                }
                fileContents = std::wstring_convert<havINICodeCvt<char16_t, char, std::mbstate_t>, char16_t>{}.to_bytes(u16ConvBE);
            }
            else if (bomType == havINIBOMType::UTF32LE)
            {
                fileContents = std::wstring_convert<havINICodeCvt<char32_t, char, std::mbstate_t>, char32_t>{}.to_bytes(fileContentsU32);
            }
            else if (bomType == havINIBOMType::UTF32BE)
            {
                std::u32string u32ConvBE;
                for (char32_t currentChar : fileContentsU32)
                {
                    u32ConvBE += (((currentChar & 0x000000ff) << 24) |
                                  ((currentChar & 0x0000ff00) << 8) |
                                  ((currentChar & 0x00ff0000) >> 8) |
                                  ((currentChar & 0xff000000) >> 24));
                }
                fileContents = std::wstring_convert<havINICodeCvt<char32_t, char, std::mbstate_t>, char32_t>{}.to_bytes(u32ConvBE);
            }

            std::vector<std::string> buffer;

            std::string line = "";

            // Read INI file contents
            for (std::string::size_type index = 0; index < fileContents.size(); ++index)
            {
                char currentChar = fileContents[index];

                int extractedCharacter = currentChar;

                switch (extractedCharacter)
                {
                case '\r':
                    {
                        if (++index >= fileContents.size())
                        {
                            break;
                        }

                        currentChar = fileContents[index + 1];

                        // CRLF found!
                        if (currentChar == '\n')
                        {
                            extractedCharacter = currentChar;

                            line += static_cast<char>(extractedCharacter);
                            buffer.push_back(line);
                            line.clear();
                        }
                        // CR found!
                        else
                        {
                            line += '\n';
                            buffer.push_back(line);
                            line.clear();
                        }
                    }
                    break;

                case '\n':
                    // LF found!
                    line += static_cast<char>(extractedCharacter);
                    buffer.push_back(line);
                    line.clear();
                    break;

                case '\\':
                    {
                        if (++index >= fileContents.size())
                        {
                            break;
                        }

                        currentChar = fileContents[index];

                        switch (currentChar)
                        {
                        case 'x':
                            {
                                std::locale loc;

                                std::string hexString;

                                bool isUnicodeEscapeSequence = false;

                                for (int valueIndex = 0; valueIndex < 4; ++valueIndex)
                                {
                                    if (++index >= fileContents.size())
                                    {
                                        throw std::runtime_error("Read beyond end of file, and found invalid unicode escape sequence!");
                                    }

                                    char currentCharInLoop = fileContents[index];

                                    if (std::isxdigit(currentCharInLoop, loc) == false)
                                    {
                                        isUnicodeEscapeSequence = false;

                                        break;
                                    }
                                    else
                                    {
                                        isUnicodeEscapeSequence = true;
                                    }

                                    hexString += currentCharInLoop;
                                }

                                if (isUnicodeEscapeSequence == true)
                                {
                                    unsigned int codePoint = std::strtol(hexString.c_str(), nullptr, 16);

                                    bool endOfFileReached = false;

                                    if (++index >= fileContents.size())
                                    {
                                        endOfFileReached = true;
                                    }

                                    if (fileContents[index] == '\\' && endOfFileReached == false)
                                    {
                                        if (++index >= fileContents.size())
                                        {
                                            endOfFileReached = true;
                                        }

                                        if (fileContents[index] == 'x' && endOfFileReached == false)
                                        {
                                            bool surrogatePair = false;

                                            hexString = "";

                                            for (int valueIndex = 0; valueIndex < 4; ++valueIndex)
                                            {
                                                if (++index >= fileContents.size())
                                                {
                                                    throw std::runtime_error("Read beyond end of file, and found invalid unicode escape sequence!");
                                                }

                                                char currentCharInLoop = fileContents[index];

                                                if (std::isxdigit(currentCharInLoop, loc) == false)
                                                {
                                                    surrogatePair = false;

                                                    break;
                                                }
                                                else
                                                {
                                                    surrogatePair = true;
                                                }

                                                hexString += currentCharInLoop;
                                            }

                                            if (surrogatePair == true)
                                            {
                                                if (codePoint >= 0xd800 && codePoint <= 0xdbff)
                                                {
                                                    unsigned int secondCodePoint = std::strtol(hexString.c_str(), nullptr, 16);

                                                    if (secondCodePoint >= 0xdc00 && secondCodePoint <= 0xdfff)
                                                    {
                                                        // codePoint = high surrogate, secondCodePoint = low surrogate
                                                        unsigned int surrogatePairValue = 0x10000 + ((codePoint - 0xd800) * 0x400) + (secondCodePoint - 0xdc00);

                                                        line += CodePointToString(surrogatePairValue);
                                                    }
                                                    else
                                                    {
                                                        throw std::runtime_error("Invalid code point range for UTF-16 low surrogate pair!");
                                                    }
                                                }
                                                else
                                                {
                                                    // Set the index back by six characters, otherwise the possible next UTF-8 sequence isn't processed!
                                                    index -= 6;

                                                    line += CodePointToString(codePoint);
                                                }
                                            }
                                            else
                                            {
                                                throw std::runtime_error("Invalid code point for UTF-16 low surrogate pair!");
                                            }
                                        }
                                        else
                                        {
                                            // Set the index back by two characters, otherwise the next characters aren't processed!
                                            index -= 2;

                                            line += CodePointToString(codePoint);
                                        }
                                    }
                                    else
                                    {
                                        // Set the index back by one character, otherwise the next character isn't processed!
                                        --index;

                                        line += CodePointToString(codePoint);
                                    }
                                }
                                else
                                {
                                    // Set the index back by one character, otherwise the next character isn't processed!
                                    --index;

                                    // Don't eat the x character
                                    line += currentChar;

                                    if (hexString.empty() == false)
                                    {
                                        line += hexString;
                                    }
                                }
                            }
                            break;

                        default:
                            throw std::runtime_error("Invalid escape character!");
                            break;
                        }
                    }
                    break;

                default:
                    line += static_cast<char>(extractedCharacter);
                    break;
                }
            }

            if (line.empty() == false)
            {
                line += '\n';
                buffer.push_back(line);
                line.clear();
            }

            line.clear();

            std::string errorMessage = "";
#ifdef HAVINI_CASE_SENSITIVE
            std::string sectionName = "HI_Global";
#else
            std::string sectionName = "hi_global";
#endif
            std::string keyName = "";
            std::string value = "";
            std::string inlineComment = "";

            bool newSection = false;
            bool newKey = true;
            bool newValue = false;
            bool hasInlineComment = false;

            bool stringValue = false;

            std::string arrayIndex = "";

            bool isArrayKey = false;
            bool hasArrayIndex = false;

            // Parse INI file contents
            for (std::vector<std::string>::size_type lineIndex = 0; lineIndex < buffer.size(); ++lineIndex)
            {
                if (errorMessage.empty() == false)
                {
                    std::cout << "Error while reading INI file: " << errorMessage << std::endl;

                    errorMessage.clear();

                    break;
                }

                line = buffer[lineIndex];

                // Remove whitespaces
                std::size_t characterIndex = 0;

                std::string tmpLine = "";

                bool commentSection = false;
                bool valueSection = false;

                for (std::size_t cIndex = 0; cIndex < line.size(); ++cIndex)
                {
                    if (line[cIndex] == ';' || line[cIndex] == '#')
                    {
                        commentSection = true;
                    }

                    if (line[cIndex] == '\"' || line[cIndex] == '\'')
                    {
                        valueSection = !valueSection;
                    }

                    if (commentSection == false && valueSection == false)
                    {
                        if (std::isspace(line[cIndex], mLocale) == false)
                        {
                            tmpLine += line[cIndex];
                        }
                    }
                    else
                    {
                        tmpLine += line[cIndex];
                    }
                }

                // Empty line
                if (tmpLine.empty() == true)
                {
#ifdef HAVINI_CASE_SENSITIVE
                    std::string emptyLineKeyStart = "HI_EL_";
#else
                    std::string emptyLineKeyStart = "hi_el_";
#endif

                    if (HasSection(sectionName) == false)
                    {
#ifdef HAVINI_CASE_SENSITIVE
                        havINISection newSectionForEmptyLine(sectionName);
#else
                        havINISection newSectionForEmptyLine(mLocale, sectionName);
#endif
                        unsigned int emptyLineCount = newSectionForEmptyLine.GetEmptyLineCount();
                        newSectionForEmptyLine.SetEmptyLine(emptyLineKeyStart + std::to_string(emptyLineCount), havINIPosition::End);
                        mData.push_back(newSectionForEmptyLine);
                    }
                    else
                    {
                        unsigned int emptyLineCount = GetSection(sectionName)->GetEmptyLineCount();
                        GetSection(sectionName)->SetEmptyLine(emptyLineKeyStart + std::to_string(emptyLineCount), havINIPosition::End);
                    }

                    continue;
                }

                line = tmpLine;

                if (line.back() != '\n')
                {
                    line += '\n';
                }

                // Comment
                if (line[0] == ';' || line[0] == '#')
                {
                    // Remove comment character
                    if (line.front() == ';' || line.front() == '#')
                    {
                        line.erase(line.begin());
                    }

                    // Remove potential space character
                    if (line.front() == ' ')
                    {
                        line.erase(line.begin());
                    }

                    // Remove LF
                    if (line.back() == '\n')
                    {
                        line.pop_back();
                    }

#ifdef HAVINI_CASE_SENSITIVE
                    std::string commentKeyStart = "HI_C_";
#else
                    std::string commentKeyStart = "hi_c_";
#endif

                    if (HasSection(sectionName) == false)
                    {
#ifdef HAVINI_CASE_SENSITIVE
                        havINISection newSectionForComment(sectionName);
#else
                        havINISection newSectionForComment(mLocale, sectionName);
#endif
                        unsigned int commentLineCount = newSectionForComment.GetCommentLineCount();
                        newSectionForComment.SetComment(commentKeyStart + std::to_string(commentLineCount), line, havINIPosition::End);
                        mData.push_back(newSectionForComment);
                    }
                    else
                    {
                        unsigned int commentLineCount = GetSection(sectionName)->GetCommentLineCount();
                        GetSection(sectionName)->SetComment(commentKeyStart + std::to_string(commentLineCount), line, havINIPosition::End);
                    }

                    continue;
                }

                // Section
                if (line[0] == '[')
                {
                    newSection = true;
                    newKey = false;
                    newValue = false;

                    stringValue = false;

                    sectionName.clear();

                    characterIndex = 0;

                    while (line[characterIndex] != '\n')
                    {
                        if (line[characterIndex] == '[' && characterIndex > 0)
                        {
                            errorMessage += "New section started within section tag!\n";
                            break;
                        }

                        if (line[characterIndex] == '[')
                        {
                            ++characterIndex;

                            continue;
                        }

                        if (line[characterIndex] == ' ')
                        {
                            ++characterIndex;

                            continue;
                        }

                        if (line[characterIndex] == ']')
                        {
                            newSection = false;

                            ++characterIndex;

                            continue;
                        }

                        if (line[characterIndex] == ';' || line[characterIndex] == '#')
                        {
                            if (newSection == true)
                            {
                                errorMessage += "Found comment tag within section tag!\n";
                                break;
                            }
                            else
                            {
                                // Section inline comment
                                std::string sectionInlineComment = "";

                                ++characterIndex;

                                while (line[characterIndex] != '\n')
                                {
                                    sectionInlineComment += line[characterIndex];
                                    ++characterIndex;
                                }

                                // Remove potential space character
                                if (sectionInlineComment.empty() == false && sectionInlineComment.front() == ' ')
                                {
                                    sectionInlineComment.erase(sectionInlineComment.begin());
                                }

#ifndef HAVINI_CASE_SENSITIVE
                                sectionName = havUtils::ToLower(sectionName, mLocale);
#endif

                                if (HasSection(sectionName) == false)
                                {
#ifdef HAVINI_CASE_SENSITIVE
                                    havINISection newSectionForInlineComment(sectionName);
#else
                                    havINISection newSectionForInlineComment(mLocale, sectionName);
#endif
                                    newSectionForInlineComment.SetInlineComment(sectionInlineComment);
                                    mData.push_back(newSectionForInlineComment);
                                }
                                else
                                {
                                    GetSection(sectionName)->SetInlineComment(sectionInlineComment);
                                }

                                // Break the loop now... section creation must be done at this point
                                break;
                            }
                        }

                        sectionName += line[characterIndex];

                        ++characterIndex;
                    }

                    if (newSection == true)
                    {
                        errorMessage += "Section end tag was not found!\n";
                    }

                    if (errorMessage.empty() == true)
                    {
                        // Prepare to read first key-value pair
                        newKey = true;
                    }

#ifndef HAVINI_CASE_SENSITIVE
                    sectionName = havUtils::ToLower(sectionName, mLocale);
#endif

                    continue;
                }

                if (newKey == true)
                {
                    keyName.clear();

                    std::size_t foundCharacter = line.find("=");
                    std::size_t foundCharacter2 = line.find(":");

                    if (foundCharacter == std::string::npos && foundCharacter2 == std::string::npos)
                    {
                        errorMessage += "No \"=\" or \":\" sign found!\n";
                    }

                    char endCharacter = '\0';

                    if (foundCharacter != std::string::npos && foundCharacter2 != std::string::npos)
                    {
                        if (foundCharacter > foundCharacter2)
                        {
                            endCharacter = ':';
                        }
                        else
                        {
                            endCharacter = '=';
                        }
                    }
                    else
                    {
                        if (foundCharacter != std::string::npos)
                        {
                            endCharacter = '=';
                        }

                        if (foundCharacter2 != std::string::npos)
                        {
                            endCharacter = ':';
                        }
                    }

                    if (errorMessage.empty() == true)
                    {
                        std::string delimiter{ endCharacter };
                        std::string fullKey(havUtils::Split(line, delimiter)[0]);

                        isArrayKey = false;
                        hasArrayIndex = false;

                        if (havUtils::EndsWith(fullKey, "[]") == true)
                        {
                            isArrayKey = true;
                        }
                        else if (havUtils::StartsWith(fullKey, "[") == false && havUtils::EndsWith(fullKey, "]") == true && fullKey.find("[") != std::string::npos)
                        {
                            arrayIndex = havUtils::Split(fullKey, "[")[1];
                            arrayIndex.pop_back(); // Remove "]" character

                            isArrayKey = true;
                            hasArrayIndex = true;
                        }

                        while (line[characterIndex] != endCharacter)
                        {
                            if (line[characterIndex] == ' ')
                            {
                                ++characterIndex;

                                if (line[characterIndex] == endCharacter)
                                {
                                    newKey = false;
                                }

                                continue;
                            }

                            if (isArrayKey == false)
                            {
                                if (line[characterIndex] == '[')
                                {
                                    errorMessage += "Start section tag within key tag!\n";
                                    break;
                                }

                                if (line[characterIndex] == ']')
                                {
                                    errorMessage += "Close section tag within key tag!\n";
                                    break;
                                }
                            }
                            else
                            {
                                // Key name complete once "[" character has been found
                                if (line[characterIndex] == '[')
                                {
                                    newKey = false;
                                    break;
                                }
                            }

                            if (line[characterIndex] == ';' || line[characterIndex] == '#')
                            {
                                errorMessage += "Found comment tag within key tag!\n";
                                break;
                            }

                            keyName += line[characterIndex];

                            ++characterIndex;

                            if (line[characterIndex] == endCharacter)
                            {
                                newKey = false;
                            }
                        }

                        if (newKey == true)
                        {
                            errorMessage += "Key end tag (\"";
                            errorMessage += endCharacter;
                            errorMessage += "\" sign) was not found!\n";
                        }

                        if (errorMessage.empty() == true)
                        {
#ifndef HAVINI_CASE_SENSITIVE
                            keyName = havUtils::ToLower(keyName, mLocale);
#endif

                            newKey = false;
                            newValue = true;
                        }
                    }
                }

                if (newValue == true)
                {
                    value.clear();

                    std::size_t foundCharacter = line.find("=");
                    std::size_t foundCharacter2 = line.find(":");

                    if (foundCharacter == std::string::npos && foundCharacter2 == std::string::npos)
                    {
                        errorMessage += "No \"=\" or \":\" sign found!\n";
                    }

                    if (foundCharacter != std::string::npos && foundCharacter2 != std::string::npos)
                    {
                        if (foundCharacter > foundCharacter2)
                        {
                            characterIndex = foundCharacter2 + 1;
                        }
                        else
                        {
                            characterIndex = foundCharacter + 1;
                        }
                    }
                    else
                    {
                        if (foundCharacter != std::string::npos)
                        {
                            characterIndex = foundCharacter + 1;
                        }

                        if (foundCharacter2 != std::string::npos)
                        {
                            characterIndex = foundCharacter2 + 1;
                        }
                    }

                    std::size_t quoteCharacterPos = 0;
                    std::size_t allQuoteCharacters = 0;

                    while ((quoteCharacterPos = line.find("\"", quoteCharacterPos + 1)) != std::string::npos &&
                           (quoteCharacterPos = line.find("\'", quoteCharacterPos + 1)) != std::string::npos)
                    {
                        ++allQuoteCharacters;
                    }

                    if (errorMessage.empty() == true)
                    {
                        std::size_t currentQuoteCharacter = 0;

                        bool addQuotes = false;

                        while (line[characterIndex] != '\n')
                        {
                            if (line[characterIndex] == ' ' && stringValue == false)
                            {
                                ++characterIndex;

                                if (line[characterIndex] == '\n')
                                {
                                    newValue = false;
                                }

                                continue;
                            }

                            if (line[characterIndex] == '[')
                            {
                                errorMessage += "Start section tag within value tag!\n";
                                break;
                            }

                            if (line[characterIndex] == ']')
                            {
                                errorMessage += "Close section tag within value tag!\n";
                                break;
                            }

                            if (allQuoteCharacters > 2)
                            {
                                if ((line[characterIndex] == '\"' || line[characterIndex] == '\'') && stringValue == false && currentQuoteCharacter == 0)
                                {
                                    stringValue = true;

                                    ++characterIndex;

                                    ++currentQuoteCharacter;

                                    continue;
                                }

                                if ((line[characterIndex] == '\"' || line[characterIndex] == '\'') && stringValue == true && currentQuoteCharacter < allQuoteCharacters)
                                {
                                    ++currentQuoteCharacter;
                                }

                                if ((line[characterIndex] == '\"' || line[characterIndex] == '\'') && stringValue == true && currentQuoteCharacter == allQuoteCharacters)
                                {
                                    stringValue = false;

                                    ++currentQuoteCharacter;

                                    ++characterIndex;

                                    addQuotes = true;

                                    continue;
                                }
                            }
                            else
                            {
                                if ((line[characterIndex] == '\"' || line[characterIndex] == '\'') && stringValue == false)
                                {
                                    stringValue = true;

                                    ++characterIndex;

                                    continue;
                                }

                                if ((line[characterIndex] == '\"' || line[characterIndex] == '\'') && stringValue == true)
                                {
                                    stringValue = false;

                                    ++characterIndex;

                                    addQuotes = true;

                                    continue;
                                }
                            }

                            if (stringValue == false)
                            {
                                if (line[characterIndex] == ';' || line[characterIndex] == '#')
                                {
                                    // Inline comment
                                    hasInlineComment = true;

                                    ++characterIndex;

                                    while (line[characterIndex] != '\n')
                                    {
                                        inlineComment += line[characterIndex];
                                        ++characterIndex;
                                    }

                                    // Remove potential space character
                                    if (inlineComment.front() == ' ')
                                    {
                                        inlineComment.erase(inlineComment.begin());
                                    }

                                    newValue = false;

                                    break;
                                }
                            }

                            value += line[characterIndex];

                            ++characterIndex;

                            if (line[characterIndex] == '\n' && stringValue == true)
                            {
                                errorMessage += "String end tag not defined!\n";

                                break;
                            }

                            if (line[characterIndex] == '\n')
                            {
                                newValue = false;
                            }
                        }

                        if (newValue == true && stringValue == true)
                        {
                            errorMessage += "Value end tag (New line) was not found!\n";
                        }

                        if (errorMessage.empty() == true)
                        {
                            newValue = false;

                            std::string newInlineComment = "";

                            if (hasInlineComment == true)
                            {
                                newInlineComment = inlineComment;
                            }

                            if (HasSection(sectionName) == false)
                            {
#ifdef HAVINI_CASE_SENSITIVE
                                havINISection newSectionForKeyValuePair(sectionName);
#else
                                havINISection newSectionForKeyValuePair(mLocale, sectionName);
#endif

                                if (isArrayKey == true)
                                {
                                    newSectionForKeyValuePair.SetArrayEntry(keyName, value, addQuotes, hasInlineComment, newInlineComment, arrayIndex, hasArrayIndex);
                                }
                                else
                                {
                                    newSectionForKeyValuePair.SetKeyValuePair(keyName, value, addQuotes);
                                    auto keyValuePair = newSectionForKeyValuePair.GetKeyValuePair(keyName);
                                    keyValuePair->SetInlineComment(newInlineComment);
                                }

                                mData.push_back(newSectionForKeyValuePair);
                            }
                            else
                            {
                                if (isArrayKey == true)
                                {
                                    GetSection(sectionName)->SetArrayEntry(keyName, value, addQuotes, hasInlineComment, newInlineComment, arrayIndex, hasArrayIndex);
                                }
                                else
                                {
                                    GetSection(sectionName)->SetKeyValuePair(keyName, value, addQuotes);
                                    auto keyValuePair = GetSection(sectionName)->GetKeyValuePair(keyName);
                                    keyValuePair->SetInlineComment(newInlineComment);
                                }
                            }

                            hasInlineComment = false;
                            inlineComment.clear();

                            newKey = true;
                        }
                    }
                }
            }

            if (errorMessage.empty() == false)
            {
                std::cout << "Error while reading INI file: " << errorMessage << std::endl;

                errorMessage.clear();
            }

            // A section is missing when a section is at the end of the INI file and doesn't have any key value pairs
            if (HasSection(sectionName) == false)
            {
#ifdef HAVINI_CASE_SENSITIVE
                mData.emplace_back(sectionName);
#else
                mData.emplace_back(mLocale, sectionName);
#endif
            }

            return true;
        }

        bool WriteFile(const std::string& fileName, bool formatted = false, havINIBOMType bomType = havINIBOMType::None)
        {
            // Open file stream
#ifdef _WIN32
            std::unique_ptr<std::FILE, decltype(&std::fclose)> fileStream(_wfopen(&ConvertStringToWString(fileName)[0], L"wb"), std::fclose);
#else
            std::unique_ptr<std::FILE, decltype(&std::fclose)> fileStream(std::fopen(fileName.c_str(), "wb"), std::fclose);
#endif

            if (fileStream == nullptr)
            {
                std::cout << "Unable to write INI file: " << fileName << "\n";

                return false;
            }

            if (bomType != havINIBOMType::None)
            {
                if (bomType == havINIBOMType::UTF8)
                {
                    // Write UTF-8 BOM at the beginning of the INI file
                    const unsigned char bomArray[3] = { 0xef, 0xbb, 0xbf };

                    for (unsigned int index = 0; index < 3; ++index)
                    {
                        std::fwrite(&bomArray[index], sizeof(unsigned char), 1, fileStream.get());
                    }
                }
                else if (bomType == havINIBOMType::UTF16LE)
                {
                    // Write UTF-16 Little Endian BOM at the beginning of the INI file
                    const unsigned char bomArray[2] = { 0xff, 0xfe };

                    for (unsigned int index = 0; index < 2; ++index)
                    {
                        std::fwrite(&bomArray[index], sizeof(unsigned char), 1, fileStream.get());
                    }
                }
                else if (bomType == havINIBOMType::UTF16BE)
                {
                    // Write UTF-16 Big Endian BOM at the beginning of the INI file
                    const unsigned char bomArray[2] = { 0xfe, 0xff };

                    for (unsigned int index = 0; index < 2; ++index)
                    {
                        std::fwrite(&bomArray[index], sizeof(unsigned char), 1, fileStream.get());
                    }
                }
                else if (bomType == havINIBOMType::UTF32LE)
                {
                    // Write UTF-32 Little Endian BOM at the beginning of the INI file
                    const unsigned char bomArray[4] = { 0xff, 0xfe, 0x00, 0x00 };

                    for (unsigned int index = 0; index < 4; ++index)
                    {
                        std::fwrite(&bomArray[index], sizeof(unsigned char), 1, fileStream.get());
                    }
                }
                else if (bomType == havINIBOMType::UTF32BE)
                {
                    // Write UTF-32 Big Endian BOM at the beginning of the INI file
                    const unsigned char bomArray[4] = { 0x00, 0x00, 0xfe, 0xff };

                    for (unsigned int index = 0; index < 4; ++index)
                    {
                        std::fwrite(&bomArray[index], sizeof(unsigned char), 1, fileStream.get());
                    }
                }
            }

            // Write INI file contents
            for (auto sectionIterator = mData.begin(); sectionIterator != mData.end(); ++sectionIterator)
            {
                std::string sectionName = "";

                const std::vector<havINIData>& sectionKeyValuePairs = (*sectionIterator).GetKeyValuePairs();

                if ((*sectionIterator).GetSectionName() != "HI_Global" &&
                    (*sectionIterator).GetSectionName() != "hi_global")
                {
                    std::string newline = ((sectionIterator == mData.begin()) ? "" : GetNewline());

                    // Prevent adding a new line, if the file or the global section is empty, however, we need to take possible BOM into consideration
                    if (bomType == havINIBOMType::None)
                    {
                        if (std::ftell(fileStream.get()) > 0)
                        {
                            sectionName += newline;
                        }
                    }
                    else if (bomType == havINIBOMType::UTF8)
                    {
                        if (std::ftell(fileStream.get()) > 3)
                        {
                            sectionName += newline;
                        }
                    }
                    else if (bomType == havINIBOMType::UTF16LE || bomType == havINIBOMType::UTF16BE)
                    {
                        if (std::ftell(fileStream.get()) > 2)
                        {
                            sectionName += newline;
                        }
                    }
                    else if (bomType == havINIBOMType::UTF32LE || bomType == havINIBOMType::UTF32BE)
                    {
                        if (std::ftell(fileStream.get()) > 4)
                        {
                            sectionName += newline;
                        }
                    }

                    sectionName += "[";
                    sectionName += ConvertToEscapedString((*sectionIterator).GetSectionName());
                    sectionName += "]";

                    if ((*sectionIterator).HasInlineComment() == true)
                    {
                        if (formatted == true)
                        {
                            sectionName += " ";
                        }
                        sectionName += GetCommentCharacter();
                        sectionName += " ";
                        sectionName += ConvertToEscapedString((*sectionIterator).GetInlineComment());
                    }

                    if (formatted == true && sectionKeyValuePairs.empty() == true)
                    {
                        sectionName += GetNewline();
                    }

                    WriteStringToFile(sectionName, fileStream.get(), bomType);
                }

                for (auto keyValuePairIterator = sectionKeyValuePairs.begin(); keyValuePairIterator != sectionKeyValuePairs.end(); ++keyValuePairIterator)
                {
                    std::string newline = (sectionName.empty() == true && keyValuePairIterator == sectionKeyValuePairs.begin()) ? "" : GetNewline();

                    std::string keyValuePair;

                    if ((*keyValuePairIterator).GetType() == havINIDataType::Empty)
                    {
                        keyValuePair = newline;
                    }
                    else if ((*keyValuePairIterator).GetType() == havINIDataType::Comment)
                    {
                        keyValuePair = newline;
                        keyValuePair += GetCommentCharacter();
                        keyValuePair += " ";
                        keyValuePair += ConvertToEscapedString((*keyValuePairIterator).GetValue());
                    }
                    else if ((*keyValuePairIterator).GetType() == havINIDataType::Array)
                    {
                        for (auto arrayIterator = (*keyValuePairIterator).ArrayCBegin(); arrayIterator != (*keyValuePairIterator).ArrayCEnd(); ++arrayIterator)
                        {
                            std::string value = ConvertToEscapedString((*arrayIterator).GetValue());

                            if ((*arrayIterator).GetAddQuotes() == true)
                            {
                                value = GetValueQuoteCharacter() + value + GetValueQuoteCharacter();
                            }

                            if ((*keyValuePairIterator).HasArrayIndex() == true)
                            {
                                keyValuePair = newline;
                                keyValuePair += ConvertToEscapedString((*keyValuePairIterator).GetKey());
                                keyValuePair += "[";
                                keyValuePair += ConvertToEscapedString((*arrayIterator).GetKey());
                                keyValuePair += "]";
                                if (formatted == true)
                                {
                                    keyValuePair += " ";
                                }
                                keyValuePair += GetKeyValuePairDelimiter();
                                if (formatted == true)
                                {
                                    keyValuePair += " ";
                                }
                                keyValuePair += value;
                            }
                            else
                            {
                                keyValuePair = newline;
                                keyValuePair += ConvertToEscapedString((*keyValuePairIterator).GetKey());
                                keyValuePair += "[]";
                                if (formatted == true)
                                {
                                    keyValuePair += " ";
                                }
                                keyValuePair += GetKeyValuePairDelimiter();
                                if (formatted == true)
                                {
                                    keyValuePair += " ";
                                }
                                keyValuePair += value;
                            }

                            if ((*arrayIterator).HasInlineComment() == true)
                            {
                                if (formatted == true)
                                {
                                    keyValuePair += " ";
                                }
                                keyValuePair += GetCommentCharacter();
                                keyValuePair += " ";
                                keyValuePair += ConvertToEscapedString((*arrayIterator).GetInlineComment());
                            }

                            if (formatted == true &&
                                arrayIterator + 1 == (*keyValuePairIterator).ArrayCEnd() &&
                                keyValuePairIterator + 1 == sectionKeyValuePairs.end())
                            {
                                keyValuePair += GetNewline();
                            }

                            WriteStringToFile(keyValuePair, fileStream.get(), bomType);
                        }

                        // All array entries have been written, so continue with the next key value pair
                        continue;
                    }
                    else
                    {
                        std::string value = ConvertToEscapedString((*keyValuePairIterator).GetValue());

                        if ((*keyValuePairIterator).GetAddQuotes() == true)
                        {
                            value = GetValueQuoteCharacter() + value + GetValueQuoteCharacter();
                        }

                        keyValuePair = newline;
                        keyValuePair += ConvertToEscapedString((*keyValuePairIterator).GetKey());
                        if (formatted == true)
                        {
                            keyValuePair += " ";
                        }
                        keyValuePair += GetKeyValuePairDelimiter();
                        if (formatted == true)
                        {
                            keyValuePair += " ";
                        }
                        keyValuePair += value;
                    }

                    if ((*keyValuePairIterator).HasInlineComment() == true)
                    {
                        if (formatted == true)
                        {
                            keyValuePair += " ";
                        }
                        keyValuePair += GetCommentCharacter();
                        keyValuePair += " ";
                        keyValuePair += ConvertToEscapedString((*keyValuePairIterator).GetInlineComment());
                    }

                    if (formatted == true &&
                        keyValuePairIterator + 1 == sectionKeyValuePairs.end() &&
                        (*keyValuePairIterator).GetType() != havINIDataType::Empty)
                    {
                        keyValuePair += GetNewline();
                    }

                    WriteStringToFile(keyValuePair, fileStream.get(), bomType);
                }
            }

            return true;
        }

        bool SetEmptyLine(std::string sectionName, const havINIPosition& position, std::optional<std::string> keyName = std::nullopt)
        {
            if (sectionName.empty() == true)
            {
                sectionName = "HI_Global";
            }

#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
#endif

            auto sectionEntry = std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == sectionName; } );

            if (sectionEntry != mData.end())
            {
                std::string emptyLineKeyStart = "HI_EL_";

#ifndef HAVINI_CASE_SENSITIVE
                emptyLineKeyStart = havUtils::ToLower(emptyLineKeyStart, mLocale);

                if (keyName.has_value() == true)
                {
                    keyName = havUtils::ToLower(keyName.value(), mLocale);
                }
#endif

                sectionEntry->SetEmptyLine(emptyLineKeyStart + std::to_string(sectionEntry->GetEmptyLineCount()), position, keyName);

                return true;
            }

            return false;
        }

        std::vector<std::string> GetEmptyLineKeyNames(std::string sectionName)
        {
            if (sectionName.empty() == true)
            {
                sectionName = "HI_Global";
            }

#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
#endif

            auto sectionEntry = std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == sectionName; } );

            if (sectionEntry != mData.end())
            {
                std::string emptyLineKeyStart = "HI_EL_";

#ifndef HAVINI_CASE_SENSITIVE
                emptyLineKeyStart = havUtils::ToLower(emptyLineKeyStart, mLocale);
#endif

                return sectionEntry->GetEmptyLineKeyNames(emptyLineKeyStart);
            }

            return {};
        }

        bool RemoveEmptyLine(std::string sectionName, std::string keyName)
        {
            if (sectionName.empty() == true)
            {
                sectionName = "HI_Global";
            }

#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
#endif

            auto sectionEntry = std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == sectionName; } );

            if (sectionEntry != mData.end())
            {
#ifndef HAVINI_CASE_SENSITIVE
                keyName = havUtils::ToLower(keyName, mLocale);
#endif

                return sectionEntry->RemoveEmptyLine(keyName);
            }

            return false;
        }

        bool AddSection(std::string sectionName)
        {
#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
#endif

            auto sectionEntry = std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == sectionName; } );

            if (sectionEntry == mData.end())
            {
#ifdef HAVINI_CASE_SENSITIVE
                mData.emplace_back(sectionName);
#else
                mData.emplace_back(mLocale, sectionName);
#endif

                return true;
            }

            return false;
        }

        std::string GetValue(std::string sectionName, std::string keyName, const std::string& defaultValue)
        {
#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
#endif

            auto sectionEntry = std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == sectionName; } );

            if (sectionEntry != mData.end())
            {
                const std::vector<havINIData>& keyValuePairs = sectionEntry->GetKeyValuePairs();

#ifndef HAVINI_CASE_SENSITIVE
                keyName = havUtils::ToLower(keyName, mLocale);
#endif

                auto keyValuePair = sectionEntry->GetKeyValuePair(keyName);

                if (keyValuePair != keyValuePairs.end())
                {
                    return keyValuePair->GetValue();
                }
            }

            return defaultValue;
        }

        bool SetValue(std::string sectionName, std::string keyName, const std::string& value, bool addQuotes)
        {
#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
            keyName = havUtils::ToLower(keyName, mLocale);
#endif

            auto sectionEntry = std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == sectionName; } );

            if (sectionEntry != mData.end())
            {
                const std::vector<havINIData>& keyValuePairs = sectionEntry->GetKeyValuePairs();

                auto keyValuePair = sectionEntry->GetKeyValuePair(keyName);

                if (keyValuePair != keyValuePairs.end())
                {
                    keyValuePair->SetValue(value);
                    keyValuePair->SetAddQuotes(addQuotes);

                    return true;
                }
                else
                {
                    sectionEntry->SetKeyValuePair(keyName, value, addQuotes);

                    return true;
                }
            }
            else
            {
#ifdef HAVINI_CASE_SENSITIVE
                havINISection newSection(sectionName);
#else
                havINISection newSection(mLocale, sectionName);
#endif
                newSection.SetKeyValuePair(keyName, value, addQuotes);
                mData.push_back(newSection);

                return true;
            }

            return false;
        }

        bool SetComment(std::string sectionName, const std::string& comment, const havINIPosition& position, std::optional<std::string> keyName = std::nullopt)
        {
            if (sectionName.empty() == true)
            {
                sectionName = "HI_Global";
            }

#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);

            if (keyName.has_value() == true)
            {
                keyName = havUtils::ToLower(keyName.value(), mLocale);
            }
#endif

            auto sectionEntry = std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == sectionName; } );

            if (sectionEntry != mData.end())
            {
#ifdef HAVINI_CASE_SENSITIVE
                std::string commentKeyStart = "HI_C_";
#else
                std::string commentKeyStart = "hi_c_";
#endif

                sectionEntry->SetComment(commentKeyStart + std::to_string(sectionEntry->GetCommentLineCount()), comment, position, keyName);

                return true;
            }

            return false;
        }

        std::vector<std::string> GetCommentKeyNames(std::string sectionName)
        {
            if (sectionName.empty() == true)
            {
                sectionName = "HI_Global";
            }

#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
#endif

            auto sectionEntry = std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == sectionName; } );

            if (sectionEntry != mData.end())
            {
#ifdef HAVINI_CASE_SENSITIVE
                std::string commentKeyStart = "HI_C_";
#else
                std::string commentKeyStart = "hi_c_";
#endif

                return sectionEntry->GetCommentKeyNames(commentKeyStart);
            }

            return {};
        }

        bool RemoveComment(std::string sectionName, std::string keyName)
        {
            if (sectionName.empty() == true)
            {
                sectionName = "HI_Global";
            }

#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
            keyName = havUtils::ToLower(keyName, mLocale);
#endif

            auto sectionEntry = std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == sectionName; } );

            if (sectionEntry != mData.end())
            {
                return sectionEntry->RemoveComment(keyName);
            }

            return false;
        }

        bool SetInlineComment(std::string sectionName, std::string keyName, const std::string& inlineComment)
        {
#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
            keyName = havUtils::ToLower(keyName, mLocale);
#endif

            auto sectionEntry = std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == sectionName; } );

            if (sectionEntry != mData.end())
            {
                const std::vector<havINIData>& keyValuePairs = sectionEntry->GetKeyValuePairs();

                auto keyValuePair = sectionEntry->GetKeyValuePair(keyName);

                if (keyValuePair != keyValuePairs.end())
                {
                    keyValuePair->SetInlineComment(inlineComment);

                    return true;
                }
            }

            return false;
        }

        bool RemoveKey(std::string sectionName, std::string keyName)
        {
#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
            keyName = havUtils::ToLower(keyName, mLocale);
#endif

            auto sectionEntry = std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == sectionName; } );

            if (sectionEntry != mData.end())
            {
                const std::vector<havINIData>& keyValuePairs = sectionEntry->GetKeyValuePairs();

                auto keyValuePair = sectionEntry->GetKeyValuePair(keyName);

                if (keyValuePair != keyValuePairs.end())
                {
                    sectionEntry->RemoveKeyValuePair(keyValuePair);

                    return true;
                }
            }

            return false;
        }

        bool RemoveSection(std::string sectionName)
        {
#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
#endif

            auto sectionEntry = std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == sectionName; } );

            if (sectionEntry != mData.end())
            {
                mData.erase(sectionEntry);

                return true;
            }

            return false;
        }

        bool RenameKey(std::string sectionName, std::string oldKeyName, std::string newKeyName)
        {
#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
            oldKeyName = havUtils::ToLower(oldKeyName, mLocale);
            newKeyName = havUtils::ToLower(newKeyName, mLocale);
#endif

            auto sectionEntry = std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == sectionName; } );

            if (sectionEntry != mData.end())
            {
                const std::vector<havINIData>& keyValuePairs = sectionEntry->GetKeyValuePairs();

                if (std::find_if(keyValuePairs.begin(), keyValuePairs.end(), [&](const havINIData& data) { return data.GetKey() == newKeyName; } ) == keyValuePairs.end())
                {
                    auto keyValuePair = sectionEntry->GetKeyValuePair(oldKeyName);

                    if (keyValuePair != keyValuePairs.end())
                    {
                        sectionEntry->SetKey(newKeyName, keyValuePair);

                        return true;
                    }
                }
            }

            return false;
        }

        bool RenameSection(std::string oldSectionName, std::string newSectionName)
        {
#ifndef HAVINI_CASE_SENSITIVE
            oldSectionName = havUtils::ToLower(oldSectionName, mLocale);
            newSectionName = havUtils::ToLower(newSectionName, mLocale);
#endif

            if (std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == newSectionName; } ) == mData.end())
            {
                auto sectionEntry = std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == oldSectionName; } );

                if (sectionEntry != mData.end())
                {
                    sectionEntry->SetSectionName(newSectionName);

                    return true;
                }
            }

            return false;
        }

        bool SetSectionInlineComment(std::string sectionName, const std::string& inlineComment)
        {
#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
#endif

            auto sectionEntry = std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == sectionName; } );

            if (sectionEntry != mData.end())
            {
                sectionEntry->SetInlineComment(inlineComment);

                return true;
            }

            return false;
        }

        std::vector<havINIData>::size_type GetNumberOfKeys(std::string sectionName)
        {
#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
#endif

            auto sectionEntry = std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == sectionName; } );

            if (sectionEntry == mData.end())
            {
                return 0;
            }

            return sectionEntry->GetNumberOfKeys();
        }

        std::vector<havINISection>::size_type GetNumberOfSections()
        {
            return mData.size();
        }

        bool HasKey(std::string sectionName, std::string keyName)
        {
#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
            keyName = havUtils::ToLower(keyName, mLocale);
#endif

            auto sectionEntry = std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == sectionName; } );

            if (sectionEntry != mData.end())
            {
                return sectionEntry->HasKey(keyName);
            }

            return false;
        }

        bool HasSection(std::string sectionName)
        {
#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
#endif

            return std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == sectionName; } ) != mData.end();
        }

        bool ClearSection(std::string sectionName)
        {
#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
#endif

            auto sectionEntry = std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == sectionName; } );

            if (sectionEntry != mData.end())
            {
                sectionEntry->Clear();

                return true;
            }

            return false;
        }

        void SetNewline(const std::string& newline)
        {
            if (newline != "\n" && newline != "\r" && newline != "\r\n")
            {
                throw std::runtime_error("Only \"\", \"\" or \"\" are allowed as new line!");
            }

            mNewline = newline;
        }

        void SetCommentCharacter(char commentCharacter)
        {
            if (commentCharacter != ';' && commentCharacter != '#')
            {
                throw std::runtime_error("Only \";\" or \"#\" are allowed as comment character!");
            }

            mCommentCharacter = commentCharacter;
        }

        void SetValueQuoteCharacter(char valueQuoteCharacter)
        {
            if (valueQuoteCharacter != '\"' && valueQuoteCharacter != '\'')
            {
                throw std::runtime_error("Only \"\"\" or \"\'\" are allowed as quote character!");
            }

            mValueQuoteCharacter = valueQuoteCharacter;
        }

        void SetKeyValuePairDelimiter(char keyValuePairDelimiter)
        {
            if (keyValuePairDelimiter != '=' && keyValuePairDelimiter != ':')
            {
                throw std::runtime_error("Only \"=\" or \":\" are allowed as key value pair delimiter!");
            }

            mKeyValuePairDelimiter = keyValuePairDelimiter;
        }

        void SetLocale(const std::locale& value)
        {
            mLocale = value;
        }

        const std::string& GetNewline() const { return mNewline; }
        char GetCommentCharacter() const { return mCommentCharacter; }
        char GetValueQuoteCharacter() const { return mValueQuoteCharacter; }
        char GetKeyValuePairDelimiter() const { return mKeyValuePairDelimiter; }
        std::locale GetLocale() const { return mLocale; }

#ifdef _WIN32
        std::wstring ConvertStringToWString(const std::string& value)
        {
            int numOfChars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.c_str(), -1, nullptr, 0);

            std::wstring wstr(numOfChars, 0);

            numOfChars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.c_str(), -1, &wstr[0], numOfChars);

            if (numOfChars > 0)
            {
                return wstr;
            }

            return std::wstring();
        }
#endif

    private:
        template<class internT, class externT, class stateT>
        struct havINICodeCvt : std::codecvt<internT, externT, stateT>
        {
            ~havINICodeCvt() {}
        };

        std::vector<havINISection>::iterator GetSection(std::string sectionName)
        {
#ifndef HAVINI_CASE_SENSITIVE
            sectionName = havUtils::ToLower(sectionName, mLocale);
#endif

            return std::find_if(mData.begin(), mData.end(), [&](const havINISection& section) { return section.GetSectionName() == sectionName; } );
        }

        void WriteStringToFile(const std::string& value, std::FILE* fileStream, havINIBOMType bomType)
        {
            if (bomType == havINIBOMType::None || bomType == havINIBOMType::UTF8)
            {
                std::fwrite(value.data(), sizeof(char), value.size(), fileStream);
            }
            else if (bomType == havINIBOMType::UTF16LE)
            {
                std::u16string u16Conv = std::wstring_convert<havINICodeCvt<char16_t, char, std::mbstate_t>, char16_t>{}.from_bytes(value);
                std::fwrite(u16Conv.data(), sizeof(char16_t), u16Conv.size(), fileStream);
            }
            else if (bomType == havINIBOMType::UTF16BE)
            {
                std::u16string u16Conv = std::wstring_convert<havINICodeCvt<char16_t, char, std::mbstate_t>, char16_t>{}.from_bytes(value);
                std::u16string u16ConvBE;
                for (char16_t currentChar : u16Conv)
                {
                    u16ConvBE += (((currentChar & 0x00ff) << 8) | ((currentChar & 0xff00) >> 8));
                }
                std::fwrite(u16ConvBE.data(), sizeof(char16_t), u16ConvBE.size(), fileStream);
            }
            else if (bomType == havINIBOMType::UTF32LE)
            {
                std::u32string u32Conv = std::wstring_convert<havINICodeCvt<char32_t, char, std::mbstate_t>, char32_t>{}.from_bytes(value);
                std::fwrite(u32Conv.data(), sizeof(char32_t), u32Conv.size(), fileStream);
            }
            else if (bomType == havINIBOMType::UTF32BE)
            {
                std::u32string u32Conv = std::wstring_convert<havINICodeCvt<char32_t, char, std::mbstate_t>, char32_t>{}.from_bytes(value);
                std::u32string u32ConvBE;
                for (char32_t currentChar : u32Conv)
                {
                    u32ConvBE += (((currentChar & 0x000000ff) << 24) |
                                  ((currentChar & 0x0000ff00) << 8) |
                                  ((currentChar & 0x00ff0000) >> 8) |
                                  ((currentChar & 0xff000000) >> 24));
                }
                std::fwrite(u32ConvBE.data(), sizeof(char32_t), u32ConvBE.size(), fileStream);
            }
        }

        // Default INI newline, characters and delimiter
        std::string mNewline = "\r\n";
        char mCommentCharacter = ';';
        char mValueQuoteCharacter = '\"';
        char mKeyValuePairDelimiter = '=';

        std::locale mLocale = std::locale();

        // Notes:
        // HI_EL_x / hi_el_x - Indicates that we're dealing with an empty line (x = number)
        // HI_C_x / hi_c_x - Indicates that we're dealing with a comment (x = number)
        std::vector<havINISection> mData; // A section contains key value pairs
    };
}

#endif
