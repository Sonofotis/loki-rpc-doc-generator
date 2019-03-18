#define _CRT_SECURE_NO_WARNINGS

#include "loki_rpc_doc_generator.h"

type_conversion const TYPE_CONVERSION_TABLE[] =
{
    {STRING_LIT("std::string"), STRING_LIT("string")},
    {STRING_LIT("uint64_t"),    STRING_LIT("uint64")},
    {STRING_LIT("uint32_t"),    STRING_LIT("uint32")},
    {STRING_LIT("uint16_t"),    STRING_LIT("uint16")},
    {STRING_LIT("uint8_t"),     STRING_LIT("uint8")},
    {STRING_LIT("int64_t"),     STRING_LIT("int64")},
    {STRING_LIT("blobdata"),    STRING_LIT("string")},
    {STRING_LIT("crypto::hash"),STRING_LIT("string[64]")},
    {STRING_LIT("difficulty_type"),STRING_LIT("uint64")}, // TODO(doyle): Hmm should be derived from codebase?
};

bool string_lit_cmp(string_lit a, string_lit b)
{
    bool result = (a.len == b.len && (strncmp(a.str, b.str, a.len) == 0));
    return result;
}

string_lit token_to_string_lit(token_t token)
{
    string_lit result = {};
    result.str        = token.str;
    result.len        = token.len;
    return result;
}

string_lit trim_whitespace_around(string_lit in)
{
    string_lit result = in;
    while (result.str && isspace(result.str[0]))
    {
        result.str++;
        result.len--;
    }

    while (result.str && isspace(result.str[result.len - 1]))
        result.len--;

    return result;
}

char *read_entire_file(char const *file_path, ptrdiff_t *file_size)
{
    char *result = nullptr;
    FILE *handle = fopen(file_path, "rb");
    if (!handle)
        return result;

    DEFER { fclose(handle); };
    fseek(handle, 0, SEEK_END);
    *file_size = ftell(handle);
    rewind(handle);

    result = static_cast<char *>(malloc(sizeof(char) * ((*file_size) + 1)));
    if (!result)
        return result;

    size_t elements_read = fread(result, *file_size, 1, handle);
    if (elements_read != 1)
    {
        free(result);
        return nullptr;
    }

    result[*file_size] = '\0';
    return result;
}

char *str_multi_find(char *start, string_lit const *find, int find_len, int *find_index)
{
    for (char *ptr = start; ptr && ptr[0]; ptr++)
    {
        for (int i = 0; i < find_len; ++i)
        {
            string_lit const *check = find + i;
            if (strncmp(ptr, check->str, check->len) == 0)
            {
                if (find_index) *find_index = i;
                return ptr;
            }
        }
    }

    return nullptr;
}

char *str_find(char *start, string_lit find)
{
    for (char *ptr = start; ptr && ptr[0]; ptr++)
    {
        if (strncmp(ptr, find.str, find.len) == 0)
            return ptr;
    }

    return nullptr;
}

bool char_is_alpha     (char ch) { return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'); }
bool char_is_digit     (char ch) { return (ch >= '0' && ch <= '9'); }
bool char_is_alphanum  (char ch) { return char_is_alpha(ch) || char_is_digit(ch); }

char char_to_lower(char ch)
{
   if (ch >= 'A' && ch <= 'Z')
     ch += ('a' - 'A');
   return ch;
}

#if 0
decl_enum fill_enum(std::vector<token_t> *tokens)
{
    struct decl_enum enumer;
    while (tokens->size() > 0)
    {
        if ((tokens->at(0)).type == LeftCurlyBrace) // DONE
        {
            tokens->erase(tokens->begin());
        }
        else if ((tokens->at(0)).type == RightCurlyBrace) // DONE
        {
            tokens->erase(tokens->begin());
            return enumer;
        }
        else if ((tokens->at(0)).type == EnumString)
        {
            enumer.name     = (tokens->at(0)).str;
            enumer.name_len = (tokens->at(0)).len;
            tokens->erase(tokens->begin());
        }
        else if ((tokens->at(0)).type == EnumVal)
        {
            enumer.vals.push_back((tokens->at(0)).str);
            tokens->erase(tokens->begin());
        }
        else if ((tokens->at(0)).type == EnumDef)
        {
            enumer.names.push_back((tokens->at(0)).str);
            tokens->erase(tokens->begin());
        }
    }
    return enumer;
}
#endif

token_t tokeniser_peek_token(tokeniser_t *tokeniser)
{
    token_t result = tokeniser->tokens[tokeniser->tokens_index];
    return result;
}

void tokeniser_rewind_token(tokeniser_t *tokeniser)
{
    tokeniser->tokens[--tokeniser->tokens_index];
}

token_t tokeniser_prev_token(tokeniser_t *tokeniser)
{
    token_t result = {};
    result.type = token_type::end_of_stream;
    if (tokeniser->tokens_index > 0)
        result = tokeniser->tokens[tokeniser->tokens_index - 1];
    return result;
}

token_t tokeniser_next_token(tokeniser_t *tokeniser, int amount = 1)
{
    token_t result = tokeniser->tokens[tokeniser->tokens_index];
    if (result.type != token_type::end_of_stream)
    {
        for (int i = 0; i < amount; i++)
        {
            result = tokeniser->tokens[tokeniser->tokens_index++];
            if (result.type      == token_type::left_curly_brace)  tokeniser->indent_level++;
            else if (result.type == token_type::right_curly_brace) tokeniser->indent_level--;
            assert(tokeniser->indent_level >= 0);

            if (result.type == token_type::end_of_stream)
                break;
        }
    }
    return result;
}

token_t tokeniser_advance_until_token(tokeniser_t *tokeniser, token_type type)
{
    token_t result = {};
    for (result = tokeniser_next_token(tokeniser);
         result.type != token_type::end_of_stream && result.type != type;
         result = tokeniser_next_token(tokeniser)
        )
    {
    }
    return result;
}

bool is_identifier_token(token_t token, string_lit expect_str)
{
    bool result = (token.type == token_type::identifier) && (string_lit_cmp(token_to_string_lit(token), expect_str));
    return result;
}

bool tokeniser_accept_token_if_identifier(tokeniser_t *tokeniser, string_lit expect, token_t *token = nullptr)
{
    token_t check = tokeniser_peek_token(tokeniser);
    if (is_identifier_token(check, expect))
    {
        if (token) *token = check;
        tokeniser_next_token(tokeniser);
        return true;
    }

    return false;
}

bool tokeniser_accept_token_if_type(tokeniser_t *tokeniser, token_type type, token_t *token = nullptr)
{
    token_t check = tokeniser_peek_token(tokeniser);
    if (check.type == type)
    {
        if (token) *token = check;
        tokeniser_next_token(tokeniser);
        return true;
    }

    return false;
}

bool parse_type_and_var_decl(tokeniser_t *tokeniser, decl_var *variable)
{
    token_t token = tokeniser_peek_token(tokeniser);

    bool member_function = false;
    char *type_decl_start = token.str;
    token_t var_decl = {};
    for (token_t sub_token = tokeniser_peek_token(tokeniser);; sub_token = tokeniser_peek_token(tokeniser))
    {
        // Member function decl
        if (sub_token.type == token_type::open_paren)
        {
          sub_token       = tokeniser_next_token(tokeniser);
          member_function = true;

          for (sub_token = tokeniser_next_token(tokeniser);; sub_token = tokeniser_next_token(tokeniser))
          {
            if (sub_token.type == token_type::semicolon)
              break;

            // Inline function decl
            if (sub_token.type == token_type::left_curly_brace)
            {
              int orig_scope_level = tokeniser->indent_level - 1;
              assert(orig_scope_level >= 0);

              while (tokeniser->indent_level != orig_scope_level)
                sub_token = tokeniser_next_token(tokeniser);
              break;
            }
          }

        }

        if (sub_token.type == token_type::semicolon || sub_token.type == token_type::equal)
        {
            var_decl = tokeniser_prev_token(tokeniser);
            break;
        }
        sub_token = tokeniser_next_token(tokeniser);
    }

    if (member_function)
      return false;

    char *type_decl_end = var_decl.str - 1;
    variable->type.str   = type_decl_start;
    variable->type.len   = static_cast<int>(type_decl_end - type_decl_start);
    variable->name       = token_to_string_lit(var_decl);
    variable->type       = trim_whitespace_around(variable->type);

    for (int i = 0; i < variable->type.len; ++i)
    {
        if (variable->type.str[i] == '<')
        {
            variable->template_expr.str = variable->type.str + (++i);
            for (int j = ++i; j < variable->type.len; ++j)
            {
                if (variable->type.str[j] == '>')
                {
                    char const *template_expr_end = variable->type.str + j;
                    variable->template_expr.len    = static_cast<int>(template_expr_end - variable->template_expr.str);
                    break;
                }
            }
            break;
        }
    }

    token = tokeniser_next_token(tokeniser);
    if (token.type != token_type::semicolon)
       token = tokeniser_advance_until_token(tokeniser, token_type::semicolon);

    token = tokeniser_peek_token(tokeniser);
    if (token.type == token_type::comment)
    {
        variable->comment = token_to_string_lit(token);
        token = tokeniser_next_token(tokeniser);
    }

    return true;
}

static string_lit const COMMAND_RPC_PREFIX = STRING_LIT("COMMAND_RPC_");
decl_struct fill_struct(tokeniser_t *tokeniser)
{
    decl_struct result = {};
    if (!tokeniser_accept_token_if_identifier(tokeniser, STRING_LIT("struct")))
        return result;

    token_t token = {};
    if (!tokeniser_accept_token_if_type(tokeniser, token_type::identifier, &token))
        return result;

    int original_indent_level = tokeniser->indent_level;
    result.name               = token_to_string_lit(token);
    if (result.name.len > COMMAND_RPC_PREFIX.len &&
        strncmp(result.name.str, COMMAND_RPC_PREFIX.str, COMMAND_RPC_PREFIX.len) == 0)
    {
        result.type = decl_struct_type::rpc_command;
    }
    else if (string_lit_cmp(result.name, STRING_LIT("request")))
    {
        result.type = decl_struct_type::request;
    }
    else if (string_lit_cmp(result.name, STRING_LIT("response")))
    {
        result.type = decl_struct_type::response;
    }
    else
    {
        result.type = decl_struct_type::helper;
    }

    if (!tokeniser_accept_token_if_type(tokeniser, token_type::left_curly_brace, &token))
        return result;

    for (token       = tokeniser_peek_token(tokeniser);
         token.type != token_type::end_of_stream && tokeniser->indent_level != original_indent_level;
         token       = tokeniser_peek_token(tokeniser))
    {
        if (token.type == token_type::identifier)
        {
            string_lit token_lit = token_to_string_lit(token);
            if (string_lit_cmp(token_lit, STRING_LIT("struct")))
            {
                decl_struct decl = fill_struct(tokeniser);
                result.inner_structs.push_back(decl);
                continue;
            }
            else if (string_lit_cmp(token_lit, STRING_LIT("typedef")))
            {
              token = tokeniser_next_token(tokeniser);
              decl_struct decl = {};
              decl_var variable = {};
              if (parse_type_and_var_decl(tokeniser, &variable))
              {
                decl.name = variable.name;

                if (string_lit_cmp(variable.name, STRING_LIT("response")))
                {
                  decl.type = decl_struct_type::response;
                }
                else if (string_lit_cmp(variable.name, STRING_LIT("request")))
                {
                  decl.type = decl_struct_type::request;
                }
                else
                {
                  continue;
                }

                decl.variables.push_back(variable);
                result.inner_structs.push_back(decl);
              }
            }
            else
            {
                decl_var variable = {};
                if (parse_type_and_var_decl(tokeniser, &variable))
                  result.variables.push_back(variable);
            }
        }
        else
        {
            token = tokeniser_next_token(tokeniser);
        }
    }

    return result;
}

string_lit const *convert_cpp_type_with_conversion_table(string_lit const type_name)
{
  string_lit const *result = nullptr;
  for (int i = 0; i < ARRAY_COUNT(TYPE_CONVERSION_TABLE); ++i)
  {
      type_conversion const *conversion = TYPE_CONVERSION_TABLE + i;
      if (conversion->from.len < type_name.len) continue;

      if (string_lit_cmp(conversion->from, type_name))
      {
          result = &conversion->to;
          break;
      }
  }

  return result;
};

void fprint_variable(std::vector<decl_struct const *> *global_helper_structs, std::vector<decl_struct const *> *rpc_helper_structs, decl_var const *variable, int indent_level = 0)
{
    bool is_array              = variable->template_expr.len > 0;
    string_lit const *var_type = &variable->type;
    if (is_array) var_type     = &variable->template_expr;

    string_lit const *converted_type = convert_cpp_type_with_conversion_table(*var_type);
    if (converted_type)
      var_type = converted_type;

    for (int i = 0; i < indent_level * 2; i++)
      fprintf(stdout, " ");

    fprintf(stdout, " * `%.*s - %.*s", variable->name.len, variable->name.str, var_type->len, var_type->str);
    if (is_array) fprintf(stdout, "[]");
    fprintf(stdout, "`");

    if (variable->comment.len > 0) fprintf(stdout, ": %.*s", variable->comment.len, variable->comment.str);
    fprintf(stdout, "\n");

    if (!converted_type)
    {
        decl_struct const *resolved_decl = nullptr;
        for (decl_struct const *decl : *global_helper_structs)
        {
            if (string_lit_cmp(*var_type, decl->name))
            {
              resolved_decl = decl;
              break;
            }
        }

        if (!resolved_decl)
        {
          for (decl_struct const *decl : *rpc_helper_structs)
          {
              if (string_lit_cmp(*var_type, decl->name))
              {
                resolved_decl = decl;
                break;
              }
          }
        }

        if (resolved_decl)
        {
            ++indent_level;
            for (decl_var const &inner_variable : resolved_decl->variables)
                fprint_variable(global_helper_structs, rpc_helper_structs, &inner_variable, indent_level);

        }
    }
}

void generate_markdown(std::vector<decl_struct_wrapper> const *declarations)
{
    fprintf(stdout,
           "# Introduction\n\n"
           "This is a list of the lokid daemon RPC calls, their inputs and outputs, and examples of each.\n\n"
           "Many RPC calls use the daemon's JSON RPC interface while others use their own interfaces, as demonstrated below.\n\n"
           "Note: \"atomic units\" refer to the smallest fraction of 1 LOKI according to the lokid implementation. 1 LOKI = 1e12 atomic units.\n\n"
           "## RPC Methods\n\n"
           );

    for (decl_struct_wrapper const &wrapper : (*declarations))
    {
        decl_struct const &decl = wrapper.decl;
        if (decl.type == decl_struct_type::rpc_command)
        {
            fprintf(stdout, " - [%.*s](#", decl.name.len, decl.name.str);
            for (int i = 0; i < decl.name.len; ++i)
            {
              char ch = char_to_lower(decl.name.str[i]);
              fprintf(stdout, "%c", ch);
            }
            fprintf(stdout, ")\n");
        }
    }
    fprintf(stdout, "\n\n");

    std::vector<decl_struct const *> global_helper_structs;
    std::vector<decl_struct const *> rpc_helper_structs;

    for (decl_struct_wrapper const &wrapper : (*declarations))
    {
        decl_struct const &global_decl = wrapper.decl;
        if (global_decl.type == decl_struct_type::invalid)
        {
            continue;
        }

        if (global_decl.type == decl_struct_type::helper)
        {
            global_helper_structs.push_back(&global_decl);
            continue;
        }


        if (global_decl.type != decl_struct_type::rpc_command)
        {
            // TODO(doyle): Warning, unexpected non-rpc command in global scope
            continue;
        }

        decl_struct const *request  = nullptr;
        decl_struct const *response = nullptr;
        rpc_helper_structs.clear();
        for (auto &inner_decl : global_decl.inner_structs)
        {
            switch(inner_decl.type)
            {
                case decl_struct_type::request: request   = &inner_decl; break;
                case decl_struct_type::response: response = &inner_decl; break;
                case decl_struct_type::helper: rpc_helper_structs.push_back(&inner_decl); break;
                default: break; // TODO(doyle): Warning unexpected decl inside rpc declaration
            }
        }

        if (!(request && response))
        {
            // TODO(doyle): Warning
            continue;
        }

        fprintf(stdout, "### %.*s\n\n", global_decl.name.len, global_decl.name.str);
        if (wrapper.aliases.size() > 0 || wrapper.pre_decl_comments.size())
        {
            fprintf(stdout, "```\n");
            if (wrapper.aliases.size() > 0)
            {
              fprintf(stdout, "Endpoints: ");
              for(int i = 0; i < wrapper.aliases.size(); i++)
              {
                string_lit const &alias = wrapper.aliases[i];
                fprintf(stdout, "%.*s", alias.len, alias.str);

                if (i < (wrapper.aliases.size() - 1))
                  fprintf(stdout, ", ");
              }
              fprintf(stdout, "\n");

              if (wrapper.pre_decl_comments.size() > 0)
                fprintf(stdout, "\n");
            }

            if (wrapper.pre_decl_comments.size() > 0)
            {
              for (string_lit const &comment : wrapper.pre_decl_comments)
                fprintf(stdout, "%.*s\n", comment.len, comment.str);
            }
            fprintf(stdout, "```\n");
        }


        fprintf(stdout, "**Inputs:**\n");
        fprintf(stdout, "\n");
        for (decl_var const &variable : request->variables)
            fprint_variable(&global_helper_structs, &rpc_helper_structs, &variable);
        fprintf(stdout, "\n");

        fprintf(stdout, "**Outputs:**\n");
        fprintf(stdout, "\n");
        for (decl_var const &variable : response->variables)
            fprint_variable(&global_helper_structs, &rpc_helper_structs, &variable);
        fprintf(stdout, "\n\n");
    }

    fprintf(stdout, "\n");
}

char *next_token(char *src)
{
    char *result = src;
    while (result && isspace(result[0])) result++;
    return result;
}

int main(int argc, char *argv[])
{
    std::vector<decl_struct_wrapper> declarations;
    declarations.reserve(128);

    struct name_to_alias
    {
      string_lit name;
      string_lit alias;
    };
    std::vector<name_to_alias> struct_rpc_aliases;
    struct_rpc_aliases.reserve(128);

    std::vector<token_t> token_list;
    token_list.reserve(16384);
    for (int arg_index = 1; arg_index < argc; arg_index++, token_list.clear())
    {
        ptrdiff_t buf_size = 0;
        char *buf          = read_entire_file(argv[arg_index], &buf_size);
        //
        // Lex File into token_list
        //
        {
            token_type const LEX_MARKERS_TYPE[] =
            {
              token_type::introspect_marker,
              token_type::uri_json_rpc_marker,
              token_type::uri_json_rpc_marker,
              token_type::uri_binary_rpc_marker,
              token_type::json_rpc_marker,
              token_type::json_rpc_marker,
            };

            string_lit const LEX_MARKERS[] =
            {
              STRING_LIT("LOKI_RPC_DOC_INTROSPECT"),
              STRING_LIT("MAP_URI_AUTO_JON2_IF"), // TODO(doyle): Length dependant, longer lines must come first because the checking is naiive and doesn't check that the strings are delimited
              STRING_LIT("MAP_URI_AUTO_JON2"),
              STRING_LIT("MAP_URI_AUTO_BIN2"),
              STRING_LIT("MAP_JON_RPC_WE_IF"),
              STRING_LIT("MAP_JON_RPC_WE"),
            };
            static_assert(ARRAY_COUNT(LEX_MARKERS) == ARRAY_COUNT(LEX_MARKERS_TYPE), "Mismatched enum to string mapping");

            char *ptr            = buf;
            int lex_marker_index = 0;
            for (ptr = str_multi_find(ptr, LEX_MARKERS, ARRAY_COUNT(LEX_MARKERS), &lex_marker_index);
                 ptr;
                 ptr = str_multi_find(ptr, LEX_MARKERS, ARRAY_COUNT(LEX_MARKERS), &lex_marker_index))
            {

              string_lit const *found_marker = LEX_MARKERS + lex_marker_index;
              token_t lexing_marker_token    = {};
              lexing_marker_token.str        = ptr;
              lexing_marker_token.len        = found_marker->len;
              lexing_marker_token.type       = LEX_MARKERS_TYPE[lex_marker_index];
              token_list.push_back(lexing_marker_token);
              ptr += lexing_marker_token.len;

              bool started_parsing_scope = false;
              int scope_level = 0;
              for (;ptr && *ptr;)
              {
                  if (started_parsing_scope && scope_level == 0)
                  {
                    break;
                  }

                  token_t curr_token = {};
                  curr_token.str   = next_token(ptr);
                  curr_token.len   = 1;
                  ptr              = curr_token.str + 1;
                  switch(curr_token.str[0])
                  {
                      case '{': curr_token.type = token_type::left_curly_brace; break;
                      case '}': curr_token.type = token_type::right_curly_brace; break;
                      case ';': curr_token.type = token_type::semicolon; break;
                      case '=': curr_token.type = token_type::equal; break;
                      case '<': curr_token.type = token_type::less_than; break;
                      case '>': curr_token.type = token_type::greater_than; break;
                      case '(': curr_token.type = token_type::open_paren; break;
                      case ')': curr_token.type = token_type::close_paren; break;
                      case ',': curr_token.type = token_type::comma; break;

                      case '"':
                      {
                        curr_token.type = token_type::string;
                        curr_token.str  = ptr++;
                        while(ptr[0] != '"') ptr++;
                        curr_token.len  = static_cast<int>(ptr - curr_token.str);
                        ptr++;
                      }
                      break;

                      case ':':
                      {
                          curr_token.type = token_type::colon;
                          if (ptr[0] == ':')
                          {
                              ptr++;
                              curr_token.type = token_type::namespace_colon;
                              curr_token.len  = 2;
                          }
                      }
                      break;

                      case '/':
                      {
                          curr_token.type = token_type::fwd_slash;
                          if (ptr[0] == '/' || ptr[0] == '*')
                          {
                              curr_token.type = token_type::comment;
                              if (ptr[0] == '/')
                              {
                                  ptr++;
                                  if (ptr[0] == ' ' && ptr[1] && ptr[1] == ' ')
                                  {
                                    curr_token.str = ptr++;
                                  }
                                  else
                                  {
                                    while (ptr[0] == ' ' || ptr[0] == '\t') ptr++;
                                    curr_token.str = ptr;
                                  }

                                  while (ptr[0] != '\n' && ptr[0] != '\r') ptr++;
                                  curr_token.len = static_cast<int>(ptr - curr_token.str);
                              }
                              else
                              {
                                  curr_token.str = ++ptr;
                                  for (;;)
                                  {
                                      while (ptr[0] != '*') ptr++;
                                      ptr++;
                                      if (ptr[0] == '/')
                                      {
                                          curr_token.len = static_cast<int>(ptr - curr_token.str - 2);
                                          ptr++;
                                          break;
                                      }
                                  }
                              }

                          }
                      }
                      break;

                      default:
                      {
                          curr_token.type = token_type::identifier;
                          if (char_is_alpha(ptr[0]) || ptr[0] == '_' || ptr[0] == '!')
                          {
                              ptr++;
                              while (char_is_alphanum(ptr[0]) || ptr[0] == '_') ptr++;
                          }
                          curr_token.len = static_cast<int>(ptr - curr_token.str);
                      }
                      break;
                  }

                  if (lexing_marker_token.type == token_type::introspect_marker)
                  {
                      if (curr_token.type == token_type::left_curly_brace)
                      {
                        started_parsing_scope = true;
                        scope_level++;
                      }
                      else if (curr_token.type == token_type::right_curly_brace)
                      {
                        scope_level--;
                      }
                  }
                  else
                  {
                      if (curr_token.type == token_type::open_paren)
                      {
                        started_parsing_scope = true;
                        scope_level++;
                      }
                      else if (curr_token.type == token_type::close_paren)
                      {
                        scope_level--;
                      }
                  }

                  string_lit token_lit = token_to_string_lit(curr_token);
                  if (curr_token.type == token_type::identifier)
                  {
                      if (string_lit_cmp(token_lit, STRING_LIT("BEGIN_KV_SERIALIZE_MAP")))
                      {
                          string_lit const END_STR = STRING_LIT("END_KV_SERIALIZE_MAP()");
                          ptr                      = str_find(ptr, END_STR);
                          ptr                     += END_STR.len;
                          continue;
                      }
                  }

                  assert(curr_token.type != token_type::invalid);
                  if (curr_token.len > 0)
                      token_list.push_back(curr_token);

              }
            }

            token_t sentinel_token = {};
            sentinel_token.type  = token_type::end_of_stream;
            token_list.push_back(sentinel_token);
        }

        //
        // Parse lexed tokens into declarations
        //
        {
            decl_struct_wrapper struct_wrapper = {};
            tokeniser_t tokeniser = {};
            tokeniser.tokens   = token_list.data();
            for (token_t token = tokeniser_peek_token(&tokeniser);
                 token.type   != token_type::end_of_stream;
                 token         = tokeniser_peek_token(&tokeniser), struct_wrapper.pre_decl_comments.clear())
            {
                if (token.type == token_type::end_of_stream)
                    break;

                bool handled = false;
                if (token.type == token_type::introspect_marker)
                {

                  token = tokeniser_next_token(&tokeniser); // accept marker
                  for (token = tokeniser_peek_token(&tokeniser); token.type == token_type::comment; token = tokeniser_peek_token(&tokeniser))
                  {
                    string_lit comment_lit = token_to_string_lit(token);
                    struct_wrapper.pre_decl_comments.push_back(comment_lit);
                    token = tokeniser_next_token(&tokeniser);
                  }

                  if (token.type == token_type::identifier)
                  {
                      string_lit token_lit = token_to_string_lit(token);
                      if (string_lit_cmp(token_lit, STRING_LIT("struct")))
                      {
                          struct_wrapper.decl = fill_struct(&tokeniser);
                          declarations.push_back(struct_wrapper);
                          handled = true;
                      }
                      else if (string_lit_cmp(token_lit, STRING_LIT("enum")))
                      {
                          // decl_struct doc = fill_struct(&tokeniser);
                      }
                  }
                }
                else if (token.type == token_type::uri_json_rpc_marker || token.type == token_type::uri_binary_rpc_marker || token.type == token_type::json_rpc_marker)
                {
                    token = tokeniser_next_token(&tokeniser); // accept marker
                    if (tokeniser_accept_token_if_type(&tokeniser, token_type::open_paren) &&
                        tokeniser_accept_token_if_type(&tokeniser, token_type::string, &token))
                    {
                        string_lit rpc_alias = token_to_string_lit(token);
                        if (tokeniser_accept_token_if_type(&tokeniser, token_type::comma) &&
                            tokeniser_accept_token_if_type(&tokeniser, token_type::identifier) &&
                            tokeniser_accept_token_if_type(&tokeniser, token_type::comma) &&
                            tokeniser_accept_token_if_type(&tokeniser, token_type::identifier, &token))
                        {
                            string_lit struct_name   = token_to_string_lit(token);
                            name_to_alias name_alias = {};
                            name_alias.name          = struct_name;
                            name_alias.alias         = rpc_alias;

                            struct_rpc_aliases.push_back(name_alias);
                            handled = true;
                        }
                    }
                }

                if (!handled)
                    token = tokeniser_next_token(&tokeniser);
            }
        }
    }

    //
    // Resolve aliases to struct declarations
    //
    for (name_to_alias &name_alias : struct_rpc_aliases)
    {
        for(decl_struct_wrapper &decl_wrapper : declarations)
        {
            if (string_lit_cmp(decl_wrapper.decl.name, name_alias.name))
            {
                decl_wrapper.aliases.push_back(name_alias.alias);
                break;
            }
        }
    }
    generate_markdown(&declarations);

    return 0;
}
