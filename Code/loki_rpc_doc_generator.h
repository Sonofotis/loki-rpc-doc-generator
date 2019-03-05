//
// Copyright 2019, Loki Project
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <assert.h>
#include <ctype.h>
#include <vector>

template <typename proc_t>
struct defer
{
    proc_t p;
    defer(proc_t proc) : p(proc) {}
    ~defer() { p(); }
};

struct defer_helper
{
    template <typename proc_t>
    defer<proc_t> operator+(proc_t p) { return defer<proc_t>(p); }
};

#define TOKEN_COMBINE2(x, y) x ## y
#define TOKEN_COMBINE(x, y) TOKEN_COMBINE2(x, y)
#define DEFER const auto TOKEN_COMBINE2(defer_lambda_, __COUNTER__) = defer_helper() + [&]()

#define ARRAY_COUNT(array) sizeof(array)/sizeof(array[0])
#define CHAR_COUNT(str) (ARRAY_COUNT(str) - 1)
#define STRING_LIT(str) {str, CHAR_COUNT(str)}

enum struct token_type
{
    invalid,
    left_curly_brace,
    right_curly_brace,
    identifier,
    fwd_slash,
    semicolon,
    colon,
    namespace_colon,
    comment,
    less_than,
    greater_than,
    end_of_stream,
};

struct string_lit
{
    string_lit() = default;
    string_lit(char *str_, int len_) : str(str_), len(len_) {}
    char     *str;
    int       len;
};

struct decl_var
{
  string_lit type;
  string_lit name;
  string_lit comment;
};

struct decl_enum
{
  const char *name;
  int name_len;
  std::vector<const char*> names;
  std::vector<int> names_lens;
  std::vector<const char*> vals;
  std::vector<int> vals_lens;
};

enum decl_struct_type
{
    rpc_command,
    request,
    response,
    helper,
};

struct decl_struct
{
  decl_struct_type         type;
  string_lit               name;
  std::vector<decl_struct> inner_structs;
  std::vector<decl_var>    variables;
  std::vector<decl_enum>   enums;
};

struct token_t
{
  char      *str;
  int        len;
  token_type type;
};

struct tokeniser_t
{
    token_t *tokens;
    size_t tokens_index;
    int    indent_level;
};
