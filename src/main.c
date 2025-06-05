#include "main.h"


string_array splitString(const char* string_to_split, char delimeter) {
  int start = 0;
  int j = 0;
  char** splitted_strings = (char**)calloc(strlen(string_to_split), sizeof(char*));
  string_array result;
  if (isOnlyDelimeter(string_to_split, delimeter)) {
    splitted_strings[0] = calloc(strlen(string_to_split) + 1, sizeof(char));
    strcpy(splitted_strings[0], string_to_split);
    return (string_array){.len = 1, .values = splitted_strings};
  }

  for (int i = 0;; i++) {
    if (string_to_split[i] == delimeter || string_to_split[i] == '\0') {
      splitted_strings[j] = (char*)calloc(i - start + 1, sizeof(char));
      memcpy(splitted_strings[j], &string_to_split[start], i - start);
      start = i + 1;
      j++;
    }
    if (string_to_split[i] == '\0')
      break;
  }
  result.len = j;
  result.values = splitted_strings;
  return result;
}

// 获取当前工作目录的最后两个目录名
char* getLastTwoDirs(char* cwd) {
  int i = 1;
  int last_slash_pos = 0;
  int second_to_last_slash = 0;

  // 遍历字符串，记录最后两个斜杠的位置
  for (; cwd[i] != '\n' && cwd[i] != '\0'; i++) {
    if (cwd[i] == '/') {
      second_to_last_slash = last_slash_pos; // 记录倒数第二个斜杠位置
      last_slash_pos = i + 1;                // 记录最后一个斜杠后的位置
    }
  }

  // 分配内存并拷贝最后两个目录名
  char* last_two_dirs = (char*)calloc(i - second_to_last_slash + 1, sizeof(char));
  // 将最后两个目录名从第二个斜杠开始拷贝到新字符串
  strncpy(last_two_dirs, &cwd[second_to_last_slash], i - second_to_last_slash);

  return last_two_dirs;
}

// 移除字符串数组中的 ".", "..", "./", "../" 元素
string_array removeDots(string_array* array) {
  int j = 0;
  bool remove_index;
  // 不允许出现的点目录
  char* not_allowed_dots[] = {".", "..", "./", "../"};
  string_array no_dots_array;
  // 为新数组分配内存
  no_dots_array.values = calloc(array->len, sizeof(char*));
  no_dots_array.len = 0;

  // 遍历原数组
  for (int i = 0; i < array->len; i++) {
    remove_index = false;
    // 检查当前元素是否为不允许的点目录
    for (int k = 0; k < 4; k++) {
      if (strcmp(array->values[i], not_allowed_dots[k]) == 0) {
        remove_index = true;
      }
    }
    // 如果不是点目录，则添加到新数组
    if (!remove_index) {
      no_dots_array.values[j] = calloc(strlen(array->values[i]) + 1, sizeof(char));
      strcpy(no_dots_array.values[j], array->values[i]);
      no_dots_array.len += 1;
      j++;
    }
  }
  // 释放原数组内存
  free_string_array(array);
  return no_dots_array;
}

// 拼接文件路径
char* joinFilePath(char* home_dir, char* destination_file) {
  // 分配足够的内存来存储 home_dir 和 destination_file 的总长度
  char* home_dir_copied = calloc(strlen(home_dir) + strlen(destination_file) + 1, sizeof(char));
  strcpy(home_dir_copied, home_dir);

  // 拼接文件路径
  char* file_path = strcat(home_dir_copied, destination_file);

  return file_path;
}

void removeEscapesArr(string_array* splitted) {
  for (int i = 0; i < splitted->len; i++) {
    removeEscapesString(splitted->values[i]);
  }
}

// 分组通配符（* 或 ?）及其相关参数
wildcard_groups_arr groupWildcards(char* line, token_index_arr token) {
  wildcard_groups* result = calloc(token.len, sizeof(wildcard_groups));
  int wildcard_index = 0;

  for (int i = 0; i < token.len;) {
    // 如果当前 token 是 * 或 ?
    if (token.arr[i].token == ASTERISK || token.arr[i].token == QUESTION) {
      // 计算通配符起始位置
      int start = token.arr[i - 1].token == ARG ? token.arr[i - 1].start : token.arr[i].start;
      int j = i;

      // 向后查找连续的 ARG、*、?，确定通配符结束位置
      for (; (j + 1) < token.len && (token.arr[j + 1].token == ARG || token.arr[j + 1].token == ASTERISK ||
                                     token.arr[j + 1].token == QUESTION);
           j++)
        ;
      int end_index = token.arr[j].end;

      // 提取通配符字符串
      result[wildcard_index].wildcard_arg = calloc(end_index - start + 1, sizeof(char));
      strncpy(result[wildcard_index].wildcard_arg, &line[start], end_index - start);
      result[wildcard_index].start = start;
      result[wildcard_index].end = start + strlen(result[wildcard_index].wildcard_arg);

      wildcard_index++;
      i = j + 1;
    }
    i++;
  }
  // 返回所有通配符分组
  return (wildcard_groups_arr){.arr = result, .len = wildcard_index};
}

// 判断行中是否还有 * 或 ?，用于判断是否为最后一个重定向
bool isLastRedirectionInLine(char* line) {
  for (int i = 0; i < strlen(line); i++) {
    if (line[i] == '*' || line[i] == '?')
      return false;
  }
  return true;
}

// 将通配符字符串转换为正则表达式
char* createRegex(char* regex, char* start, char* end) {
  char* regex_start = regex;
  *regex++ = '^'; // 匹配行首
  while (start < end) {
    if (*start == '*') {
      *regex++ = '.';
      *regex++ = '*';
      start++;
    } else if (*start == '?') {
      *regex++ = '.';
      start++;
    } else if (*start == '.') {
      *regex++ = '\\';
      *regex++ = '.';
      start++;
    } else {
      *regex++ = *start++;
    }
  }
  *regex++ = '$'; // 匹配行尾

  return regex_start;
}

// 计算通配符结束索引（遇到 / 或 空格为止）
int calculateEndIndex(wildcard_groups_arr wildcard_groups, int j, int i) {
  int end_index = j + 1;

  for (; end_index < strlen(wildcard_groups.arr[i].wildcard_arg) &&
         wildcard_groups.arr[i].wildcard_arg[end_index] != '/' &&
         wildcard_groups.arr[i].wildcard_arg[end_index] != ' ';
       end_index++)
    ;

  return end_index;
}

// 如果目录项匹配正则，则插入到通配符结果中
void insertIfMatch(wildcard_groups_arr* wildcard_groups, char* prefix, DIR* current_dir, regex_t* re,
                   int concat_index, bool is_dotfile, int i) {
  struct dirent* dir;
  int start_index = 0;

  while ((dir = readdir(current_dir)) != NULL) {
    // 如果文件名匹配正则表达式
    if (regexec(re, dir->d_name, 0, NULL, 0) == 0) {
      // 如果不是点文件但匹配到点文件，则跳过
      if (dir->d_name[0] == '.' && !is_dotfile) {
        continue;
      }
      // 空格前插入转义符
      for (int j = 0; j < strlen(dir->d_name); j++) {
        if (dir->d_name[j] == ' ') {
          // insert escape \\ in front of whitespace
          insertCharAtPos(dir->d_name, j, '\\');
          j++;
        }
      }
      // 拼接前缀和匹配到的文件名
      char* prev_copy = calloc(strlen(prefix) + strlen(dir->d_name) + 1, sizeof(char));
      strcpy(prev_copy, prefix);
      char* match = strcat(&prev_copy[concat_index], dir->d_name);

      // 如果当前通配符参数为空，直接赋值，否则插入到指定位置
      if (strlen(wildcard_groups->arr[i].wildcard_arg) == 0) {
        wildcard_groups->arr[i].wildcard_arg =
            realloc(wildcard_groups->arr[i].wildcard_arg, (strlen(match) + 1) * sizeof(char));
        strcpy(wildcard_groups->arr[i].wildcard_arg, match);
      } else {
        insertStringAtPos(&wildcard_groups->arr[i].wildcard_arg, match, start_index);
      }

      // 如果是最后一个重定向，则在末尾插入空格
      if (isLastRedirectionInLine(wildcard_groups->arr[i].wildcard_arg)) {
        start_index = strlen(wildcard_groups->arr[i].wildcard_arg);
        wildcard_groups->arr[i].wildcard_arg =
            realloc(wildcard_groups->arr[i].wildcard_arg,
                    (strlen(wildcard_groups->arr[i].wildcard_arg) + 2) * sizeof(char));
        insertCharAtPos(wildcard_groups->arr[i].wildcard_arg, start_index, ' ');
        start_index++;
      }
      free(prev_copy);
    }
  }
}

// 计算前缀拼接索引（用于拼接通配符匹配结果）
int calculateConcatIndex(char* prefix) {
  int concat_index = 0;
  for (; concat_index < strlen(prefix) && prefix[concat_index] != '/'; concat_index++)
    ;
  concat_index += (prefix[0] == '/') ? 0 : 1;

  return concat_index;
}

// 计算通配符参数中前缀的结束位置
int calculatePrefixEnd(wildcard_groups_arr wildcard_groups, int j, int i) {
  int prefix_end = j;
  for (; prefix_end > 0 && wildcard_groups.arr[i].wildcard_arg[prefix_end - 1] != '/'; prefix_end--)
    ;
  return prefix_end;
}

// 获取通配符参数的前缀部分
void getPrefix(wildcard_groups_arr wildcard_groups, char* prefix, int i, int prefix_end) {
  if (wildcard_groups.arr[i].wildcard_arg[0] == '/') {
    strncpy(prefix, wildcard_groups.arr[i].wildcard_arg, prefix_end);
  } else {
    strcpy(prefix, "./");
    strncpy(&prefix[2], wildcard_groups.arr[i].wildcard_arg, prefix_end);
  }
}

// 展开所有通配符分组，查找匹配的文件名
wildcard_groups_arr expandWildcardgroups(wildcard_groups_arr wildcard_groups) {
  for (int i = 0; i < wildcard_groups.len; i++) {
    for (int j = 0; j < strlen(wildcard_groups.arr[i].wildcard_arg); j++) {
      if (wildcard_groups.arr[i].wildcard_arg[j] == '*' || wildcard_groups.arr[i].wildcard_arg[j] == '?') {
        int prefix_end = calculatePrefixEnd(wildcard_groups, j, i);
        char* prefix = calloc(prefix_end + 3, sizeof(char));
        getPrefix(wildcard_groups, prefix, i, prefix_end);

        char* start = &wildcard_groups.arr[i].wildcard_arg[prefix_end];
        bool is_dotfile = start[0] == '.' ? true : false;

        int end_index = calculateEndIndex(wildcard_groups, j, i);
        char* end = &wildcard_groups.arr[i].wildcard_arg[end_index];

        char* regex = calloc(((end - start) * 2) + 3, sizeof(char));
        regex = createRegex(regex, start, end);
        regex_t re;
        if (regcomp(&re, regex, REG_EXTENDED) != 0) {
          perror("regex");
        }

        removeSlice(&wildcard_groups.arr[i].wildcard_arg, 0, end_index);
        int concat_index = calculateConcatIndex(prefix);

        DIR* current_dir = opendir(prefix);
        if (current_dir == NULL) {
          // 如果目录无法打开，将通配符参数置空，保证 foundAllWildcards 返回 false
          fprintf(stderr, "zsh: couldn't open directory: %s\n", prefix);
          strcpy(wildcard_groups.arr[0].wildcard_arg, "");
          free(regex);
          free(prefix);
          regfree(&re);
          closedir(current_dir);
          return wildcard_groups;
        }

        insertIfMatch(&wildcard_groups, prefix, current_dir, &re, concat_index, is_dotfile, i);

        free(regex);
        free(prefix);
        regfree(&re);
        closedir(current_dir);
      }
    }
  }
  return wildcard_groups;
}

// 用通配符匹配结果替换原始命令行中的通配符部分
void replaceLineWithWildcards(char** line, wildcard_groups_arr wildcard_matches) {
  int verschoben_len = 0;
  for (int i = 0; i < wildcard_matches.len; i++) {
    removeSlice(line, wildcard_matches.arr[i].start + verschoben_len,
                wildcard_matches.arr[i].end + verschoben_len);
    insertStringAtPos(line, wildcard_matches.arr[i].wildcard_arg, wildcard_matches.arr[i].start + verschoben_len);
    verschoben_len += strlen(wildcard_matches.arr[i].wildcard_arg) -
                      (wildcard_matches.arr[i].end - wildcard_matches.arr[i].start);
  }
}

// 将命令行推入历史记录（头插法，保持最新在前）
void push(char* line, string_array* command_history) {
  if (command_history->len > 0) {
    for (int i = command_history->len; i > 0; i--) {
      if (i <= HISTORY_SIZE) {
        command_history->values[i] = command_history->values[i - 1];
      }
    }
  }
  (command_history->len <= HISTORY_SIZE) ? command_history->len++ : command_history->len;
  command_history->values[0] = calloc(strlen(line) + 1, sizeof(char));
  strcpy(command_history->values[0], line);
}

// 比较两个字符串数组是否完全相等
bool arrCmp(string_array arr1, string_array arr2) {
  if (arr1.len != arr2.len) {
    return false;
  }
  for (int i = 0; i < arr1.len; i++) {
    if (strcmp(arr1.values[i], arr2.values[i]) != 0) {
      return false;
    }
  }
  return true;
}

string_array getAllHistoryCommands() {
  size_t size = BUFFER * sizeof(char*); // 初始化分配的内存大小
  string_array result = {.len = 0, .values = calloc(BUFFER, sizeof(char*))}; // 初始化结果数组

  char* file_path = joinFilePath(getenv("HOME"), "/.zsh_history"); // 拼接历史文件路径
  FILE* file_to_read = fopen(file_path, "r"); // 打开历史文件
  free(file_path); // 释放路径字符串内存
  
  char* buf = NULL; // 行缓冲区
  int line_len; // 行长度
  unsigned long buf_size; // 缓冲区大小
  int i = 0; // 行计数器

  // 文件不存在直接返回空结果
  if (file_to_read == NULL) { 
    return result;
  }

  // 逐行读取历史文件
  while ((line_len = getline(&buf, &buf_size, file_to_read)) != -1) {
    // 如果超出当前分配的内存，扩容
    if ((i * sizeof(char*)) >= size) {
      char** tmp;
      if ((tmp = realloc(result.values, size * 1.5)) == NULL) { // 扩容1.5倍
        perror("zsh:");
      } else {
        result.values = tmp;
        size *= 1.5;
      }
    }

    result.values[i] = calloc(strlen(buf), sizeof(char)); // 为每一行分配内存
    strncpy(result.values[i], buf, strlen(buf) - 1); // 拷贝内容（去掉换行符）
    i++;
  }
  result.len = i; // 设置历史命令数量

  free(buf); // 释放行缓冲区
  fclose(file_to_read); // 关闭文件

  return result; // 返回历史命令数组
}

// 将命令写入全局历史文件（~/.zsh_history），避免重复
void writeCommandToGlobalHistory(char* cmd, string_array global_history) {
  char* file_path = joinFilePath(getenv("HOME"), "/.zsh_history");
  FILE* file_to_write = fopen(file_path, "a");
  free(file_path);

  if (file_to_write == NULL) {
    fprintf(stderr, "zsh: can't open ~/.zsh_history\n");
    return;
  }

  // 只有不在历史中的命令才写入
  if (!inArray(cmd, global_history)) {
    fprintf(file_to_write, "%s\n", cmd);
  }

  fclose(file_to_write);
}

// 内置cd命令实现
void cd(string_array splitted_line) {
  if (splitted_line.len == 2) {
    if (chdir(splitted_line.values[1]) == -1) {
      fprintf(stderr, "cd: %s: no such file or directory\n", splitted_line.values[1]);
    }
  } else if (splitted_line.len > 2) {
    fprintf(stderr, "cd: too many arguments\n");
  } else {
    printf("usage: cd [path]\n");
  }
}

// 调用内置命令，根据索引调用对应函数
bool callBuiltin(string_array splitted_line, function_by_name builtins[], int function_index) {
  if (strcmp(splitted_line.values[0], "exit") == 0) {
    return false;
  }
  (*builtins[function_index].func)(splitted_line);
  return true;
}

// 推送命令到历史记录（避免重复）
void pushToCommandHistory(char* line, string_array* command_history) {
  if (command_history->len == 0 || strcmp(command_history->values[0], line) != 0) {
    push(line, command_history);
  }
}

// 移除token数组中的空白符token
void removeWhitespaceTokens(token_index_arr* tokenized_line) {
  for (int i = 0; i < tokenized_line->len;) {
    if (tokenized_line->arr[i].token == WHITESPACE) {
      for (int j = i; j < tokenized_line->len - 1; j++) {
        tokenized_line->arr[j] = tokenized_line->arr[j + 1];
      }
      tokenized_line->len--;
    } else {
      i++;
    }
  }
}

// 将token数组转换为字符串（用于语法校验）
char* convertTokenToString(token_index_arr tokenized_line) {
  char* result = calloc(tokenized_line.len * 2 + 1, sizeof(char));
  int string_index = 0;
  for (int i = 0; i < tokenized_line.len; i++) {
    // 忽略通配符
    if (tokenized_line.arr[i].token != ASTERISK && tokenized_line.arr[i].token != QUESTION) {
      sprintf(&result[string_index], "%d", tokenized_line.arr[i].token);
      tokenized_line.arr[i].token >= 10 ? string_index += 2 : string_index++;
    }
  }
  return result;
}

// 校验命令行token序列是否合法
bool isValidSyntax(token_index_arr tokenized_line) {
  char* string_rep = convertTokenToString(tokenized_line);
  bool result = false;
  regex_t re;
  regmatch_t rm[1];
  // 有效语法的正则表达式（token序列）
  char* valid_syntax = "^((6|7|8|9|10)14)*(1((6|7|8|9|10)14)*(14)*((6|7|8|9|10)14)*((3((6|7|8|9|10)14)*2)(("
                       "6|7|8|9|10)14)*(14)*((6|7|8|9|10)14)*|((4((6|7|8|9|10)14)*5)|(4((6|7|8|9|10)14)+))"
                       "((6|7|8|9|10)14)*(14)*((6|7|8|9|10)14)*)*)?";

  if (regcomp(&re, valid_syntax, REG_EXTENDED) != 0) {
    perror("zsh:");
  }
  // 匹配正则表达式
  if (regexec(&re, string_rep, 1, rm, 0) == 0 && rm->rm_eo - rm->rm_so == strlen(string_rep)) {
    result = true;
  } else {
    result = false;
  }
  regfree(&re);

  free(string_rep);
  return result;
}

// 按管道/&&分割命令行，返回每个简单命令
string_array_token splitLineIntoSimpleCommands(char* line, token_index_arr tokenized_line) {
  char** line_arr = calloc(tokenized_line.len, sizeof(char*));
  enum token* token_arr = calloc(tokenized_line.len, sizeof(enum token));
  int j = 0;
  bool found_split = true;
  int start = 0;
  int i;
  token_arr[0] = CMD;
  for (i = 0; i < tokenized_line.len; i++) {
    if (found_split) {
      start = tokenized_line.arr[i].start;
      found_split = false;
    }
    if (tokenized_line.arr[i].token == PIPE) {
      int end = tokenized_line.arr[i].start;
      line_arr[j] = calloc(end - start + 1, sizeof(char));
      strncpy(line_arr[j], &line[start], end - start);
      token_arr[j + 1] = PIPE_CMD;
      j++;
      found_split = true;
    } else if (tokenized_line.arr[i].token == AMPAMP) {
      int end = tokenized_line.arr[i].start;
      line_arr[j] = calloc(end - start + 1, sizeof(char));
      strncpy(line_arr[j], &line[start], end - start);
      token_arr[j + 1] = AMP_CMD;
      j++;
      found_split = true;
    }
  }
  int end = strlen(line);
  line_arr[j] = calloc(end - start + 1, sizeof(char));
  strncpy(line_arr[j], &line[start], end - start);

  string_array_token result = {.values = line_arr, .len = j + 1, .token_arr = token_arr};
  return result;
}

// 按 token 分割命令行字符串，返回字符串数组
string_array splitByTokens(char* line, token_index_arr token) {
  char** line_arr = calloc(token.len + 1, sizeof(char*));

  int start;
  int end;
  for (int i = 0; i < token.len; i++) {
    // 如果是带引号的参数，去除首尾引号
    if (token.arr[i].token == ARG && line[token.arr[i].start] == '\'' && line[token.arr[i].end - 1] == '\'') {
      start = token.arr[i].start + 1;
      end = token.arr[i].end - 1;
    } else {
      start = token.arr[i].start;
      end = token.arr[i].end;
    }
    line_arr[i] = calloc(end - start + 1, sizeof(char));
    strncpy(line_arr[i], &line[start], end - start);
  }
  line_arr[token.len] = NULL;
  string_array result = {.values = line_arr, .len = token.len};
  // free(token.arr);
  return result;
}

// 解析重定向文件名，返回重定向信息结构体
file_redirection_data parseForRedirectionFiles(string_array simple_command, token_index_arr token) {
  char* output_name = NULL;
  char* input_name = NULL;
  char* error_name = NULL;
  char* merge_name = NULL;
  bool output_append = false;
  bool error_append = false;
  bool merge_append = false;
  bool found_output = false;
  bool found_input = false;
  bool found_stderr = false;
  bool found_merge = false;

  // 从后往前查找重定向符号，优先处理靠后的重定向
  for (int j = token.len - 2; j >= 0; j--) {
    // 输入重定向 <
    if (!found_input && token.arr[j].token == LESS) {
      input_name = calloc(strlen(simple_command.values[j + 1]) + 1, sizeof(char));
      strcpy(input_name, simple_command.values[j + 1]);
      removeEscapesString(input_name);
      found_input = true;
    }
    // 合并输出和错误重定向 &> 或 &>>
    if (!found_stderr && !found_output && !found_merge &&
        (token.arr[j].token == AMP_GREAT || token.arr[j].token == AMP_GREATGREAT)) {
      merge_name = calloc(strlen(simple_command.values[j + 1]) + 1, sizeof(char));
      strcpy(merge_name, simple_command.values[j + 1]);
      removeEscapesString(merge_name);
      found_stderr = true;
      found_output = true;
      found_merge = true;
      merge_append = token.arr[j].token == AMP_GREATGREAT ? true : false;
    }
    // 输出重定向 > >> 或 错误重定向 2> 2>>
    if (token.arr[j].token == GREAT || token.arr[j].token == GREATGREAT) {
      // 错误重定向 2> 2>>
      if (!found_stderr && simple_command.values[j][0] == '2') {
        error_name = calloc(strlen(simple_command.values[j + 1]) + 1, sizeof(char));
        strcpy(error_name, simple_command.values[j + 1]);
        removeEscapesString(error_name);
        found_stderr = true;
        error_append = token.arr[j].token == GREATGREAT ? true : false;
      }
      // 普通输出重定向 > >>
      if (!found_output && simple_command.values[j][0] != '2') {
        output_name = calloc(strlen(simple_command.values[j + 1]) + 1, sizeof(char));
        strcpy(output_name, simple_command.values[j + 1]);
        removeEscapesString(output_name);
        found_output = true;
        output_append = token.arr[j].token == GREATGREAT ? true : false;
      }
    }
  }

  return (file_redirection_data){.output_filename = output_name,
                                 .input_filename = input_name,
                                 .output_append = output_append,
                                 .stderr_filename = error_name,
                                 .stderr_append = error_append,
                                 .merge_filename = merge_name,
                                 .merge_append = merge_append};
}

// 判断文件是否存在
bool fileExists(char* name) {
  if (access(name, 0) == 0) {
    return true;
  } else {
    return false;
  }
}

// 移除字符串数组中的某个元素
void removeArrayElement(string_array* splitted, int pos) {
  for (int i = pos; i < splitted->len; i++) {
    splitted->values[i] = splitted->values[i + 1];
  }
  splitted->len--;
}

// 移除命令参数中的重定向符号及其对应的文件名
void stripRedirections(string_array* splitted_line, token_index_arr token) {
  int j = 0;
  int len = splitted_line->len;
  for (int i = 0; i < len;) {
    if (token.arr[i].token >= GREATGREAT && token.arr[i].token <= AMP_GREATGREAT) {
      removeArrayElement(splitted_line, j); // removes redirection
      removeArrayElement(splitted_line, j); // removes filename
      i += 2;
    } else {
      j++;
      i++;
    }
  }
}

// 替换命令别名
void replaceAliases(char** line, int len) {
  for (int i = 0; i < len; i++) {
    replaceAliasesString(&line[i]);
  }
}

// 释放通配符分组结构体的内存
void free_wildcard_groups(wildcard_groups_arr arr) {
  for (int i = 0; i < arr.len; i++) {
    free(arr.arr[i].wildcard_arg);
  }
  free(arr.arr);
}

// 判断所有通配符是否都匹配到了文件
bool foundAllWildcards(wildcard_groups_arr arr) {
  for (int i = 0; i < arr.len; i++) {
    if (strcmp(arr.arr[i].wildcard_arg, "") == 0) {
      free_wildcard_groups(arr);
      return false;
    }
  }
  free_wildcard_groups(arr);
  return true;
}

// 替换命令行中的所有通配符（*、?），返回是否所有通配符都匹配到了文件
bool replaceWildcards(char** line) {
  token_index_arr token = tokenizeLine(*line); // 对命令行进行分词
  wildcard_groups_arr groups = groupWildcards(*line, token); // 分组通配符
  wildcard_groups_arr wildcard_matches = expandWildcardgroups(groups); // 展开通配符，查找匹配文件
  replaceLineWithWildcards(line, wildcard_matches); // 用匹配结果替换原命令行

  free(token.arr);

  return foundAllWildcards(wildcard_matches); // 判断是否所有通配符都匹配到了文件
}

// 释放 string_array_token 结构体占用的内存
void free_string_array_token(string_array_token simple_commands) {
  for (int i = 0; i < simple_commands.len; i++) {
    free(simple_commands.values[i]);
  }
  free(simple_commands.values);
  free(simple_commands.token_arr);
}

// 释放 file_redirection_data 结构体占用的内存
void free_file_info(file_redirection_data file_info) {
  free(file_info.input_filename);
  free(file_info.output_filename);
  free(file_info.stderr_filename);
  free(file_info.merge_filename);
}

// 重置标准输入输出错误流到原始状态
void resetIO(int* tmpin, int* tmpout, int* tmperr) {
  dup2(*tmpin, 0);
  dup2(*tmpout, 1);
  dup2(*tmperr, 2);
  close(*tmpin);
  close(*tmpout);
  close(*tmperr);
  *tmpin = dup(0);
  *tmpout = dup(1);
  *tmperr = dup(2);
}

// 处理输出重定向（>、>> 或管道），设置 fdout
void outputRedirection(file_redirection_data file_info, int pd[2], int* fdout, int* fdin, int tmpout, int i,
                       string_array_token simple_commands_arr) {
  if (file_info.output_filename != NULL) {
    // 输出重定向到文件
    *fdout = file_info.output_append ? open(file_info.output_filename, O_RDWR | O_CREAT | O_APPEND, 0666)
                                     : open(file_info.output_filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
  } else if (i < simple_commands_arr.len - 1 && simple_commands_arr.token_arr[i + 1] == PIPE_CMD) {
    // 管道重定向
    pipe(pd);
    *fdout = pd[1];
    *fdin = pd[0];
  } else {
    // 默认输出
    *fdout = dup(tmpout);
  }
}

// 处理错误输出重定向（2>、2>>），设置 fderr
void errorRedirection(file_redirection_data file_info, int* fderr, int tmperr) {
  if (file_info.stderr_filename != NULL) {
    *fderr = file_info.stderr_append ? open(file_info.stderr_filename, O_RDWR | O_CREAT | O_APPEND, 0666)
                                     : open(file_info.stderr_filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
  } else {
    *fderr = dup(tmperr);
  }
}

// 处理合并输出和错误重定向（&>、&>>），同时重定向 stdout 和 stderr
void mergeRedirection(file_redirection_data file_info, int* fdout) {
  *fdout = file_info.merge_append ? open(file_info.merge_filename, O_RDWR | O_CREAT | O_APPEND, 0666)
                                  : open(file_info.merge_filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
  dup2(*fdout, STDOUT_FILENO);
  dup2(*fdout, STDERR_FILENO);
  close(*fdout);
}

// 获取 PATH 环境变量下所有可执行文件的路径（去重、去点目录）
string_array getPathBins() {
  string_array PATH_ARR = splitString(getenv("PATH"), ':'); // 按 : 分割 PATH
  // 读取PATH环境变量文件夹中的所有文件
  string_array all_files_in_dir = getAllFilesInDir(&PATH_ARR);
  // 释放PATH_ARR内存
  free_string_array(&PATH_ARR);
  // 移除掉带点的文件
  string_array removed_dots = removeDots(&all_files_in_dir);

  return removeDuplicates(&removed_dots);
}

// 处理通配符逻辑，如果有通配符则尝试替换，失败则输出错误并重定向输出
bool wildcardLogic(string_array_token simple_commands_arr, int* fdout, int* fderr, int tmpout, int tmperr, int i) {
  if (strchr(simple_commands_arr.values[i], '*') != NULL || strchr(simple_commands_arr.values[i], '?') != NULL) {
    if (!replaceWildcards(&simple_commands_arr.values[i])) {
      *fdout = dup(tmpout);
      *fderr = dup(tmperr);
      dup2(*fderr, STDERR_FILENO);
      close(*fderr);
      dup2(*fdout, STDOUT_FILENO);
      close(*fdout);
      fprintf(stderr, "zsh: no wildcard matches found\n");
      return false;
    }
  }
  return true;
}

// 判断命令是否为内置命令，并返回其索引
bool foundBuiltin(string_array splitted_line, builtins_array BUILTINS, int* builtin_index) {
  return (splitted_line.len > 0 && (*builtin_index = isBuiltin(splitted_line.values[0], BUILTINS)) != -1) ? true
                                                                                                          : false;
}

// 解析环境变量名（取等号前的部分）
char* parseVarName(char* buf) {
  char* result = calloc(strlen(buf) + 1, sizeof(char));
  for (int i = 0; i < strlen(buf) && buf[i] != '='; i++) {
    result[i] = buf[i];
  }
  return result;
}

// 解析 ~/.zshrc 文件中的环境变量
env_var_arr parseRcFileForEnv() {
  // 构造配置文件路径
  char* file_path = joinFilePath(getenv("HOME"), "/.zshrc");
  FILE* file_to_read = fopen(file_path, "r");

  // 如果文件不存在，提示用户是否创建
  if (file_to_read == NULL) {
    char answer;
    fprintf(stderr, "zsh: no ~/.zshrc file found. Want to create one? [y/n]: ");
    answer = getchar();
    getchar(); // 跳过回车
    if (answer == 'y') {
      FILE* file_created = fopen(file_path, "w");
      // 写入默认环境变量
      fprintf(file_created, "TERM=\"linux\"\nPATH=\"/usr/local/bin/$\"\n");
      fclose(file_created);
      file_to_read = fopen(file_path, "r");
    } else {
      exit(0);
    }
  }

  size_t len = 0;
  char* buf = NULL;
  size_t line_len;
  // 预分配变量名和值的数组
  char** var_names = calloc(64, sizeof(char*));
  char** values = calloc(64, sizeof(char*));

  int i = 0;
  // 逐行读取配置文件
  for (; (line_len = getline(&buf, &len, file_to_read)) != -1; i++) {
    // 解析变量名
    var_names[i] = parseVarName(buf);
    // 解析变量值（去除变量名、等号和引号）
    values[i] = calloc(strlen(buf), sizeof(char));
    strncpy(values[i], &buf[strlen(var_names[i]) + 2], strlen(buf) - (strlen(var_names[i]) + 4));
  }
  free(file_path);
  fclose(file_to_read);
  free(buf);

  // 返回环境变量数组
  return (env_var_arr){.len = i, .var_names = var_names, .values = values};
}

void setRcVars(env_var_arr envs) {
  for (int i = 0; i < envs.len; i++) {
    if (envs.values[i][strlen(envs.values[i]) - 1] == '$') {
      // $ Means concat with already existant VAR
      envs.values[i][strlen(envs.values[i]) - 1] = '\0';
      insertCharAtPos(envs.values[i], 0, ':');
      char* joined_envs = joinFilePath(getenv(envs.var_names[i]), envs.values[i]);
      setenv(envs.var_names[i], joined_envs, 1);
      free(joined_envs);
    } else {
      // can overwrite VAR
      setenv(envs.var_names[i], envs.values[i], 1);
    }
  }
}

void free_env_var_arr(env_var_arr arr) {
  for (int i = 0; i < arr.len; i++) {
    free(arr.values[i]);
    free(arr.var_names[i]);
  }
  free(arr.values);
  free(arr.var_names);
}

volatile sig_atomic_t ctrlc_hit = false;

void ctrlCHandler(int sig) { ctrlc_hit = true; }

#ifndef TEST

int main(int argc, char* argv[]) {
  char *line;         // 命令行输入
  char dir[PATH_MAX]; // 当前目录缓冲区
  bool loop = true;   // 主循环控制
  // 解析并设置环境变量
  env_var_arr ENV = parseRcFileForEnv();
  setRcVars(ENV);
  free_env_var_arr(ENV);
  // 初始化命令历史记录
  string_array command_history = {.len = 0, .values = calloc(HISTORY_SIZE, sizeof(char*))};
  // 获取所有文件路径 PATH里的
  string_array PATH_BINS = getPathBins();
  // 获取全局命令历史记录
  string_array global_command_history = getAllHistoryCommands();

  // 传入长度看是否足够存放
  char* current_dir = getcwd(dir, sizeof(dir));
  // 获取当前目录的最后两个目录
  char* last_two_dirs = getLastTwoDirs(current_dir);

  // 定义内置函数
  function_by_name builtin_funcs[] = {
      {"exit", NULL},
      {"cd", cd},
  };

  // 定义内置函数数组
  builtins_array BUILTINS = {
      .array = builtin_funcs,
      .len = sizeof(builtin_funcs) / sizeof(builtin_funcs[0]),
  };

  CLEAR_SCREEN;

  signal(SIGHUP, SIG_DFL);

  // 设置 SIGINT 信号处理函数
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));

  while (loop) {
    ctrlc_hit = false;
    printf("\n");
    // 打印终端样式
    printPrompt(last_two_dirs, CYAN);

    // 设置 SIGINT 信号处理函数为自定义 ctrlCHandler，确保即使在阻塞函数中也能响应 Ctrl+C
    sa.sa_flags = 0;
    sa.sa_handler = ctrlCHandler;
    sigaction(SIGINT, &sa, NULL);

    // 读取用户输入的命令行
    line = readLine(PATH_BINS, last_two_dirs, &command_history, global_command_history, BUILTINS);

    // 如果检测到 Ctrl+C 被按下，释放输入行内存并跳过本次循环
    if (ctrlc_hit) {
      free(line);
      continue;
    }

    // 恢复 SIGINT 信号的默认行为（用于子进程），并设置 SA_RESTART 以自动重启被中断的系统调用
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = ctrlCHandler;
    sigaction(SIGINT, &sa, NULL);

    // 对输入行进行分词和去除空白符 token
    token_index_arr tokenized_line = tokenizeLine(line);
    removeWhitespaceTokens(&tokenized_line);

    if (tokenized_line.len > 0 && isValidSyntax(tokenized_line)) {
      // 按管道分割命令
      string_array_token simple_commands_arr = splitLineIntoSimpleCommands(line, tokenized_line);
      replaceAliases(simple_commands_arr.values, simple_commands_arr.len);

      // 定义用于管道和文件描述符的变量
      int pd[2];
      int tmpin = dup(0);   // 备份标准输入
      int tmpout = dup(1);  // 备份标准输出
      int tmperr = dup(2);  // 备份标准错误

      int fdout;
      int fderr;
      pid_t pid;
      int fdin = -1;

      // 遍历每个简单命令（管道/&&分割的）
      for (int i = 0; i < simple_commands_arr.len; i++) {
        // 处理通配符逻辑，若失败则跳过本次命令
        if (!wildcardLogic(simple_commands_arr, &fdout, &fderr, tmpout, tmperr, i)) {
          continue;
        }
        // 对当前命令分词并去除空白符
        token_index_arr token = tokenizeLine(simple_commands_arr.values[i]);
        removeWhitespaceTokens(&token);
        // 按token分割为字符串数组
        string_array splitted_line = splitByTokens(simple_commands_arr.values[i], token);
        // 解析重定向信息
        file_redirection_data file_info = parseForRedirectionFiles(splitted_line, token);
        // 移除重定向相关参数
        stripRedirections(&splitted_line, token);
        free(token.arr);
        // 移除转义字符
        removeEscapesArr(&splitted_line);
        int builtin_index;

        // 处理输入重定向
        if (file_info.input_filename != NULL) {
          if (fileExists(file_info.input_filename)) {
            fdin = open(file_info.input_filename, O_RDONLY);
          } else {
            fprintf(stderr, "zsh: no such file %s\n", file_info.input_filename);
            free_string_array(&splitted_line);
            free_file_info(file_info);
            continue;
          }
        } else if (simple_commands_arr.token_arr[i] == AMP_CMD) {
          // 处理 && 命令，重置IO
          resetIO(&tmpin, &tmpout, &tmperr);
        }

        // 判断是否为内置命令
        if (foundBuiltin(splitted_line, BUILTINS, &builtin_index)) {
          // 调用内置命令
          if (!callBuiltin(splitted_line, BUILTINS.array, builtin_index)) {
            free_string_array(&splitted_line);
            free_file_info(file_info);
            loop = false;
            break;
          }
          // 更新当前目录显示
          current_dir = getcwd(dir, sizeof(dir));
          free(last_two_dirs);
          last_two_dirs = getLastTwoDirs(current_dir);

          // 推送命令到历史记录
          pushToCommandHistory(line, &command_history);

        } else {
          int w_status;

          // 设置输入重定向
          dup2(fdin, STDIN_FILENO);
          close(fdin);
          if (file_info.merge_filename != NULL) {
            // 合并输出和错误重定向
            mergeRedirection(file_info, &fdout);
          } else {
            // 处理输出和错误重定向
            outputRedirection(file_info, pd, &fdout, &fdin, tmpout, i, simple_commands_arr);
            errorRedirection(file_info, &fderr, tmperr);

            dup2(fderr, STDERR_FILENO);
            close(fderr);
            dup2(fdout, STDOUT_FILENO);
            close(fdout);
          }

          // 如果命令不为空，fork子进程执行外部命令
          if (splitted_line.len > 0) {
            if ((pid = fork()) == 0) {
              int error = execvp(splitted_line.values[0], splitted_line.values);
              if (error) {
                fprintf(stderr, "zsh: couldn't find command %s\n", splitted_line.values[0]);
              }
            }

            // 父进程等待子进程结束
            if (waitpid(pid, &w_status, WUNTRACED | WCONTINUED) == -1) {
              exit(EXIT_FAILURE);
            }
          }
        }
        // 释放本轮命令相关内存
        free_string_array(&splitted_line);
        free_file_info(file_info);
      }
      // 恢复标准输入输出错误流
      dup2(tmpin, 0);
      dup2(tmpout, 1);
      dup2(tmperr, 2);
      close(tmpin);
      close(tmpout);
      close(tmperr);
      free_string_array_token(simple_commands_arr);
    } else {
      // 语法错误提示
      if (!isOnlyDelimeter(line, ' ')) {
        fprintf(stderr, "zsh: syntax error\n");
      }
    }
        // 写入全局历史文件并推送到历史记录
        writeCommandToGlobalHistory(line, global_command_history);
        pushToCommandHistory(line, &command_history);
        free(tokenized_line.arr);
        free(line);
  }
      // 释放全局资源
      free_string_array(&global_command_history);
      free_string_array(&command_history);
      free_string_array(&PATH_BINS);
      free(last_two_dirs);
}

#endif /* !TEST */
