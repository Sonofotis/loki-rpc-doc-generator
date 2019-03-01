#include <cstdlib>
#include <iostream> 
#include <vector> 
#include <fstream>

enum TokenType
{
  LeftCurlyBrace, // 0
  RightCurlyBrace, // 1
  
  StructStart, // 2
  StructString, // 3
  
  EnumString, // 4
  VarString, // 5
  CommentString, // 6
  TypeString, // 7
  
  TypeDefStart, // 8
  
  EnumVal, // 9
  EnumDef, // 10
  EnumStart // 11
  
};

struct doc_var
{
  const char *type;
  const char *name;
  const char *comment;
};

struct doc_enum
{
  const char *name;
  std::vector<const char*> names;
  std::vector<const char*> vals;
};

struct doc_struct
{
  const char *name;
  std::vector<doc_struct> inner_structs;
  std::vector<doc_var> variables;
  std::vector<doc_enum> enums;
};

struct Token
{
  const char *str;
  int len;
  TokenType type;
}; 

std::vector<Token> parse_rpc_doc();
void next_token(char *s[]);
bool match(char *s[], const char *token_str);

bool start_scanning(char *s[]);
bool stop_scanning(char *s[]);
bool start_kv_serialize(char *s[]);
bool end_kv_serialize(char *s[]);

bool left_curly_brace(char *s[]);
bool right_curly_brace(char *s[]);
bool assignment(char *s[]);
bool semicolon(char *s[]);
bool comment_start(char *s[]);
bool structure(char *s[]);
bool enumer(char *s[]);
bool type_definition(char *s[]);

doc_enum fill_enum(std::vector<Token>* tokens);
doc_struct fill_struct(std::vector<Token>* tokens);