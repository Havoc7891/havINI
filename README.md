# havINI

Havoc's single-file INI library for C++.

## Table of Contents

- [Features](#features)
- [Getting Started](#getting-started)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)

## Features

- Reading and writing INI files
  - Also supports generating INI files from scratch
- Support for both case-insensitive and case-sensitive section names and keys (Default is case-insensitive)
- Supports the following escape characters and sequences for newlines:
  - `\r\n`
  - `\r`
  - `\n`
- Comments can be initiated with a number sign (`#`) or semicolon (`;`)
- Inline comments after section names and key-value pairs are supported
- Key-value pairs can be separated by an equal sign (`=`) or colon (`:`)
- Pretty print support when saving an INI file
- Whitespaces are removed while parsing the INI file, except in (inline) comments
- Arrays are supported
- Empty lines are supported
- Empty sections and key-value pairs/arrays without actual values are supported
- Global arrays, key-value pairs, comments, and empty lines are supported
- Values with quotes are supported (Double quote (`"`) or single quote (`'`))
- Support for renaming section names and keys
- Support for removing sections, key-value pairs, arrays, empty lines, and (inline) comments
- Support for clearing sections and arrays
- Unicode support

## Getting Started

This library requires C++17. The library should be cross-platform, but has only been tested under Windows so far.

### Installation

Copy the header file into your project folder and include the file like this:

```cpp
#include "havINI.hpp"
```

By default, the library uses case-insensitive section names and keys. However, you can enable case-sensitive support by defining `HAVINI_CASE_SENSITIVE` before including the header file:

```cpp
#define HAVINI_CASE_SENSITIVE
#include "havINI.hpp"
```

### Usage

#### Change default settings of INI library

```cpp
havINI::havINIStream mIniParser;

// Get locale
std::locale loc = mIniParser.GetLocale();

// Set locale
mIniParser.SetLocale(loc);

// Change default newline, characters and delimiter
mIniParser.SetNewline("\n");
mIniParser.SetCommentCharacter('#');
mIniParser.SetValueQuoteCharacter('\'');
mIniParser.SetKeyValuePairDelimiter(':');
```

#### Read INI file

```cpp
havINI::havINIStream mIniParser;

if (mIniParser.ParseFile("test.ini") == false)
{
    return false;
}
```

#### Write INI file

```cpp
havINI::havINIStream mIniParser;

mIniParser["Test"]["Test"] = "Test";

if (mIniParser.WriteFile("test.ini", false, havINI::havINIBOMType::UTF16LE) == false)
{
    return false;
}
```

#### Add section

```cpp
havINI::havINIStream mIniParser;

mIniParser.AddSection("Test");
```

#### Check if section and key exist

```cpp
havINI::havINIStream mIniParser;

if (mIniParser.HasSection("Test") == true && mIniParser.HasKey("Test", "Foo") == true)
{
    std::string value = mIniParser.GetValue("Test", "Foo", "Empty");

    std::cout << "Value of 'Foo' in section 'Test': " << value << std::endl;
}
```

#### Create an array and an array entry

```cpp
havINI::havINIStream mIniParser;

mIniParser["Test"].SetArrayEntry("array", "", false, false);
mIniParser["Test"]["array"].SetArrayEntry("", "Test", false, false);
```

#### Iterate over an array

```cpp
havINI::havINIStream mIniParser;

for (auto it = mIniParser["Test"]["array"].ArrayBegin(); it != mIniParser["Test"]["array"].ArrayEnd(); ++it)
{
    std::cout << it->GetKey() << " " << mIniParser.GetKeyValuePairDelimiter() << " " << it->GetValue() << std::endl;
}
```

#### Create comments and inline comments

```cpp
havINI::havINIStream mIniParser;

// Add section inline comment
mIniParser.SetSectionInlineComment("Test", "Some comment");

// Add inline comment
mIniParser.SetInlineComment("Test", "Foo", "Some comment");

// Add comment at the start of the "Test" section
mIniParser.SetComment("Test", "Some comment", havINI::havINIPosition::Start);

// Add comment below "Test" key-value pair
mIniParser.SetComment("Test", "Some comment", havINI::havINIPosition::Below, "Test");

// Add inline comment after "Foo" key-value pair
mIniParser["Test"]["Foo"].SetInlineComment("Setting the bar");
```

#### Get value with "," delimiter

```cpp
havINI::havINIStream mIniParser;

std::vector<std::string> values = mIniParser["Test"]["Foo"].Split(",");
```

#### Create value with "," delimiter from a vector of strings

```cpp
havINI::havINIStream mIniParser;

mIniParser["Test"]["Foo"].Join(values, ",");
```

#### Rename sections and keys

```cpp
havINI::havINIStream mIniParser;

mIniParser.RenameSection("Test", "Foo");

mIniParser.RenameKey("Test", "Foo", "Bar");
```

#### Add quotes to a value

```cpp
havINI::havINIStream mIniParser;

// Add quotes to a new value
mIniParser.SetValue("Test", "Foo", "Bar", true);

// Add quotes to an existing value
mIniParser["Test"]["Foo"].SetAddQuotes(true);
```

#### Add (global) empty lines

```cpp
havINI::havINIStream mIniParser;

mIniParser.SetEmptyLine("", havINI::havINIPosition::End);

mIniParser.SetEmptyLine("Test", havINI::havINIPosition::Above, "Foo");
```

#### Remove (global) empty lines and comments

```cpp
havINI::havINIStream mIniParser;

auto globalEmptyLineKeyNames = mIniParser.GetEmptyLineKeyNames("");
if (globalEmptyLineKeyNames.size() > 0)
{
    mIniParser.RemoveEmptyLine("", globalEmptyLineKeyNames.at(0));
}

auto globalCommentKeyNames = mIniParser.GetCommentKeyNames("");
if (globalCommentKeyNames.size() > 0)
{
    mIniParser.RemoveComment("", globalCommentKeyNames.at(0));
}

auto commentKeyNames = mIniParser.GetCommentKeyNames("Test");
if (commentKeyNames.size() > 0)
{
    mIniParser.RemoveComment("Test", commentKeyNames.at(0));
}
```

## Contributing

Feel free to suggest features or report issues. However, please note that pull requests will not be accepted.

## License

Copyright &copy; 2024-2025 Ren&eacute; Nicolaus

This library is released under the [MIT license](/LICENSE).
