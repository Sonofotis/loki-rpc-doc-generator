#include "loki-rpc-doc-parser.h"
#include <ctype.h>

int
main(int argc, char* argv[])
{
  parse_rpc_doc();
  return 0;
}

std::vector<Token> parse_rpc_doc() 
{
  FILE* core_rpc_doc = fopen("core_rpc_server_commands_defs.h", "rb");
  
  char* buf;
  
  if (fseek(core_rpc_doc, 0, SEEK_END) == 0)
  {
    long bufsize = ftell(core_rpc_doc);
    buf = (char*) malloc(sizeof(char)*(bufsize+1));
    fseek(core_rpc_doc, 0, SEEK_SET);
    fread(buf, sizeof(char), bufsize, core_rpc_doc);
    buf[bufsize] = '\0';
  }
  
  std::vector<Token> token_list;
  
  char *ptr = buf;
  
  while (!start_scanning(&ptr)) // Get to starting point of structures
  {
    ptr++;
  }
  
  bool kv_status = false;
  
  Token prev_token = {};
  
  while (*ptr)
  {
    next_token(&ptr);
    
    Token curr_token = {};
    
    if (start_kv_serialize(&ptr)) 
    {
      kv_status = true;
    }
    else if (end_kv_serialize(&ptr))
    {
      kv_status = false;
    }
    else if (stop_scanning(&ptr))
    {
      break;
    }
    else if (kv_status)
    {
      ptr++;
    }
    else if (left_curly_brace(&ptr)) 
    {
      curr_token.str = "{";
      curr_token.len = 1;
      curr_token.type = LeftCurlyBrace;
      token_list.push_back(curr_token);
    }    
    else if (right_curly_brace(&ptr)) 
    {
      curr_token.str = "}";
      curr_token.len = 1;
      curr_token.type = RightCurlyBrace;
      token_list.push_back(curr_token);
    }
    else if (assignment(&ptr) || semicolon(&ptr)) 
    {
    }    
    else if (type_definition(&ptr))
    {
      curr_token.str = "typedef";
      curr_token.len = 7;
      curr_token.type = TypeDefStart;
      token_list.push_back(curr_token);
      prev_token = curr_token;
    }
    else if (enumer(&ptr))
    {
      curr_token.str = "enumer";
      curr_token.len = 6;
      curr_token.type = EnumStart;
      token_list.push_back(curr_token);
      prev_token = curr_token;
    }
    else if (structure(&ptr)) 
    {
      curr_token.str = "struct";
      curr_token.len = 6;
      curr_token.type = StructStart;
      token_list.push_back(curr_token);
      prev_token = curr_token;
    }
    else if (comment_start(&ptr)) 
    {
      next_token(&ptr);
      char *start = ptr;
      char *end = ptr;
      while (end[0] != '\n')
      {
        end++;
      }
      curr_token.str = start;
      curr_token.len = end - start;
      curr_token.type = CommentString;
      
      token_list.push_back(curr_token);
      
      prev_token = curr_token;
      
      ptr = end;
    }     
    else 
    {
      char *start = ptr;
      char *end = ptr;
      while(!isspace(end[0]) && end[0] != ';')
        end++;
        
      if (prev_token.type == StructStart)
      {
        curr_token.type = StructString;
      }
      else if (prev_token.type == StructString || prev_token.type == CommentString 
        || prev_token.type == VarString || prev_token.type == TypeDefStart)
      {
        curr_token.type = TypeString;
      }
      else if (prev_token.type == TypeString)
      {
        curr_token.type = VarString;
      }
      else if (prev_token.type == EnumStart)
      {
        curr_token.type = EnumString;
      }
      else if (prev_token.type == EnumString || prev_token.type == EnumVal)
      {
        curr_token.type = EnumDef;
      }
      else if (prev_token.type == EnumDef)
      {
        curr_token.type = EnumVal;
      }
      
      curr_token.len = end - start;
      curr_token.str = start;
      
      token_list.push_back(curr_token);
      
      prev_token = curr_token;
      
      ptr = end;
    }
  }
  
  
  
  FILE *out_fp = fopen("documentation.txt", "w");
  for (auto& token : token_list)
  {
    fprintf(out_fp, "%.*s - %u\n", token.len, token.str, token.type);
  }
  
  struct doc_struct main_struct = fill_struct(&token_list);

  generate_html_doc(main_struct);
  
  fclose(out_fp);
  fclose(core_rpc_doc);
  free(buf);
  
  return token_list;
}

doc_enum fill_enum(std::vector<Token>* tokens)
{
  struct doc_enum enumer;
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
      enumer.name = (tokens->at(0)).str;
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

doc_var fill_var(std::vector<Token>* tokens)
{
  struct doc_var var;
  while (tokens->size() > 0)
  {
    if ((tokens->at(0)).type == VarString)
    {
      var.name = (tokens->at(0)).str;
      var.name_len = (tokens->at(0)).len;
      tokens->erase(tokens->begin());
      if ((tokens->at(0)).type == CommentString)
      {
        var.has_comment = true;
        var.comment = (tokens->at(0)).str;
        var.comment_len = (tokens->at(0)).len;
        tokens->erase(tokens->begin());
        return var;
      }
      else 
      {
        var.has_comment = false;
        return var;
      }
    }
    else if ((tokens->at(0)).type == TypeString)
    {
      var.type = (tokens->at(0)).str;
      var.type_len = (tokens->at(0)).len;
      tokens->erase(tokens->begin());
    }
  }
  return var;
}

doc_struct fill_struct(std::vector<Token>* tokens)
{
  struct doc_struct structure;
  while (tokens->size() > 0)
  {
    if ((tokens->at(0)).type == LeftCurlyBrace) // DONE
    {
      tokens->erase(tokens->begin());
    }
    else if ((tokens->at(0)).type == RightCurlyBrace) // DONE
    {
      tokens->erase(tokens->begin());
      return structure;
    }
    
    else if ((tokens->at(0)).type == StructStart) // DONE
    {
      tokens->erase(tokens->begin());
      doc_struct new_struct = fill_struct(tokens);
      structure.inner_structs.push_back(new_struct);
    }
    else if ((tokens->at(0)).type == StructString) // DONE
    {
      structure.name = (tokens->at(0)).str;
      structure.name_len = (tokens->at(0)).len;
      tokens->erase(tokens->begin());
    }
    
    else if ((tokens->at(0)).type == CommentString) // DONE
    {
      tokens->erase(tokens->begin());
    }
    else if ((tokens->at(0)).type == TypeString)
    {
      structure.variables.push_back(fill_var(tokens));
    }
    
    else if ((tokens->at(0)).type == TypeDefStart) // DONE
    {
      tokens->erase(tokens->begin());
    }
    
    else if ((tokens->at(0)).type == EnumStart)
    {
      tokens->erase(tokens->begin());
      structure.enums.push_back(fill_enum(tokens));
    }
  }
  return structure;
}

void next_token(char *s[])
{
  while (isspace(*s[0]))
  {
    *s = *s + 1;
  }
}

bool match(char *s[], const char *token_str)
{
  if (strncmp(*s, token_str, strlen(token_str)) == 0)
  {
    *s = *s + strlen(token_str);
    return true;
  }
  return false;
}

bool start_scanning(char *s[])
{
  char *original = *s;
  next_token(s);
  
  if (match(s, "GENERATE_LOKI_DOCS"))
  {
    return true;
  }
  *s = original;
  return false;
}

bool stop_scanning(char *s[])
{
  char *original = *s;
  next_token(s);
  
  if (match(s, "STOP_GEN_LOKI_DOCS"))
  {
    return true;
  }
  *s = original;
  return false;
}

bool start_kv_serialize(char *s[])
{
  char *original = *s;
  next_token(s);
  
  if (match(s, "BEGIN_KV_SERIALIZE_MAP()"))
  {
    return true;
  }
  *s = original;
  return false;  
}

bool end_kv_serialize(char *s[])
{
  char *original = *s;
  next_token(s);
  
  if (match(s, "END_KV_SERIALIZE_MAP()"))
  {
    return true;
  }
  *s = original;
  return false;  
}

bool left_curly_brace(char *s[]) 
{
  char *original = *s;
  next_token(s);
  
  if (match(s, "{"))
  {
    return true;
  }
  *s = original;
  return false;
}

bool right_curly_brace(char *s[]) 
{
  char *original = *s;
  next_token(s);
  
  if (match(s, "}"))
  {
    return true;
  }
  *s = original;
  return false;
}

bool assignment(char *s[]) 
{
  char *original = *s;
  next_token(s);
  
  if (match(s, "="))
  {
    return true;
  }
  *s = original;
  return false;
}

bool semicolon(char *s[])
{
  char *original = *s;
  next_token(s);
  
  if (match(s, ";"))
  {
    return true;
  }
  *s = original;
  return false;
}

bool comment_start(char *s[])
{
  char *original = *s;
  next_token(s);
  
  if (match(s, "//"))
  {
    return true;
  }
  *s = original;
  return false;
}

bool structure(char *s[])
{
  char *original = *s;
  next_token(s);
  
  if (match(s, "struct"))
  {
    return true;
  }
  *s = original;
  return false;
}

bool enumer(char *s[])
{
  char *original = *s;
  next_token(s);
  
  if (match(s, "enum"))
  {
    return true;
  }
  *s = original;
  return false;
}

bool type_definition(char *s[])
{
  char *original = *s;
  next_token(s);
  
  if (match(s, "typedef"))
  {
    return true;
  }
  *s = original;
  return false;
}

void generate_html_doc(doc_struct structure)
{
  int curr_max = 1000000;
  char* output = (char*) malloc(sizeof(char)*curr_max);
  strcpy(output, 
    "<!DOCTYPE html>\n<html>\n<head></head>\n<body>\n\n"
    "<h2>Introduction</h2>\n\n" 
    "<p>This is a list of the lokid daemon RPC calls, their inputs and outputs, and examples of each.\n\n" 
    "Many RPC calls use the daemon's JSON RPC interface while others use their own interfaces, as demonstrated below.\n\n" 
    "Note: \"atomic units\" refer to the smallest fraction of 1 LOKI according to the lokid implementation. 1 LOKI = 1e12 atomic units.</p>\n\n"
    "<h3>RPC Methods</h3>\n\n"
    "<ul>\n");
  
  for (auto& struc : structure.inner_structs)
  {
    char* add_str = (char*) malloc(sizeof(char) * (struc.name_len + 9));
    sprintf(add_str, "  <li>%.*s</li>\n", struc.name_len, struc.name);
    if (strstr(add_str, "COMMAND_RPC") != NULL)
    {
      strcat(output, add_str);
    }
  }
  strcat(output, "</ul>\n\n");
  
  for (auto& struc : structure.inner_structs) 
  {
    char* outer_name = (char*) malloc(sizeof(char) * (struc.name_len + 11));
    sprintf(outer_name, "<h3>%.*s</h3>\n\n", struc.name_len, struc.name);
    strcat(output, outer_name);
    
    strcat(output, "<p>Inputs: ");    
    for (auto& inner : struc.inner_structs)
    {
      char* inner_name = (char*) malloc(sizeof(char) * inner.name_len);
      sprintf(inner_name, "%.*s", inner.name_len, inner.name);
      if (strcmp(inner_name, "request") == 0)
      {
        if (inner.variables.size() > 0)
        {
          strcat(output, "</p>\n<ul>\n");
          for (auto& var : inner.variables)
          {
            char* curr_type = (char*) malloc(sizeof(char) * var.type_len);
            char* curr_vname = (char*) malloc(sizeof(char) * var.name_len);
            char* curr_comm = (char*) malloc(sizeof(char) * var.comment_len);
            char* entry_str = (char*) malloc(sizeof(char) * (var.comment_len + var.name_len + var.type_len + 17));
            
            sprintf(curr_type, "%.*s", var.type_len, var.type);
            sprintf(curr_vname, "%.*s", var.name_len, var.name);
            sprintf(curr_comm, "%.*s", var.comment_len, var.comment);
            
            if (var.has_comment)
            {
              sprintf(entry_str, "  <li>%s - %s; %s</li>\n", curr_vname, curr_type, curr_comm);
            }
            else 
            {
              sprintf(entry_str, "  <li>%s - %s;</li>\n", curr_vname, curr_type);
            }
            
            strcat(output, entry_str);
          }
          strcat(output, "</ul>\n\n");
          strcat(output, "<p>Outputs: ");
        }
        else
        {
          strcat(output, "None.</p>\n\n");
          strcat(output, "<p>Outputs: ");
        }    
      }
      if (strcmp(inner_name, "response") == 0)
      {
        if (inner.variables.size() > 0)
        {
          strcat(output, "</p>\n<ul>\n");
          for (auto& var : inner.variables)
          {
            char* curr_type = (char*) malloc(sizeof(char) * var.type_len);
            char* curr_vname = (char*) malloc(sizeof(char) * var.name_len);
            char* curr_comm = (char*) malloc(sizeof(char) * var.comment_len);
            char* entry_str = (char*) malloc(sizeof(char) * (var.comment_len + var.name_len + var.type_len + 17));
            
            sprintf(curr_type, "%.*s", var.type_len, var.type);
            sprintf(curr_vname, "%.*s", var.name_len, var.name);
            sprintf(curr_comm, "%.*s", var.comment_len, var.comment);
            
            if (var.has_comment)
            {
              sprintf(entry_str, "  <li>%s - %s; %s</li>\n", curr_vname, curr_type, curr_comm);
            }
            else 
            {
              sprintf(entry_str, "  <li>%s - %s;</li>\n", curr_vname, curr_type);
            }
            
            strcat(output, entry_str);
          }
          strcat(output, "</ul>\n\n");
        }
        else
        {
          strcat(output, "None.</p>\n\n");
        }    
      }
    }
  }
  
  strcat(output, "</body>\n</html>");
  
  FILE *out = fopen("docs.html", "w");
  fprintf(out, "%s", output);
  
  fclose(out);
  free(output);
}