#include "readline.h"

// 构造并初始化 line_data 结构体，用于保存当前输入行的相关信息
line_data* lineDataConstructor(int directory_len) {
  // 分配 line_data 结构体内存
  line_data* line_info = calloc(1, sizeof(line_data));
  // 初始化结构体各字段
  *line_info = (line_data){
      .line = calloc(BUFFER, sizeof(char)),         // 分配输入行缓冲区
      .i = calloc(1, sizeof(int)),                  // 分配光标位置指针
      .prompt_len = directory_len + 4,              // 提示符长度（目录长度+4）
      .line_row_count_with_autocomplete = 0,        // 自动补全时的行数
      .cursor_row = 0,                              // 当前光标所在行
      .size = BUFFER * sizeof(char),                // 缓冲区大小
  };
  *line_info->i = 0; // 初始化光标位置为0

  return line_info;  // 返回初始化后的结构体指针
}

// 初始化补齐结构体
autocomplete_data* autocompleteDataConstructor() {
  autocomplete_data* autocomplete_info = calloc(1, sizeof(autocomplete_data));
  *autocomplete_info = (autocomplete_data){
      .possible_autocomplete = calloc(BUFFER, sizeof(char)),
      .autocomplete = false,
      .size = BUFFER * sizeof(char),
  };

  return autocomplete_info;
}

// 构造并初始化历史数据结构体, 本次会话的命令历史和全局命令历史
history_data* historyDataConstructor(string_array* command_history, string_array global_command_history) {
  history_data* history_info = calloc(1, sizeof(history_data));
  *history_info = (history_data){
      .history_index = 0,
      .sessions_command_history = *command_history,
      .global_command_history = global_command_history,
  };

  return history_info;
}

// 合并两个 string_array 数组，返回一个新的 string_array，包含所有元素
string_array concatenateArrays(const string_array one, const string_array two) {
  // 如果两个数组都为空，直接返回空数组
  if (one.len == 0 && two.len == 0)
    return (string_array){.len = 0, .values = NULL};

  // 分配新数组的内存，长度为两个数组之和
  string_array concatenated = {.values = calloc((one.len + two.len), sizeof(char*))};
  int i = 0;

  // 拷贝第一个数组的所有字符串到新数组
  for (int k = 0; k < one.len; k++) {
    concatenated.values[i] = calloc(strlen(one.values[k]) + 1, sizeof(char));
    strcpy(concatenated.values[i], one.values[k]);
    i++;
  }
  // 拷贝第二个数组的所有字符串到新数组
  for (int j = 0; j < two.len; j++) {
    concatenated.values[i] = calloc(strlen(two.values[j]) + 1, sizeof(char));
    strcpy(concatenated.values[i], two.values[j]);
    i++;
  }
  // 设置新数组的长度
  concatenated.len = i;

  // 返回合并后的数组
  return concatenated;
}

// 判断当前补全位置前是否存在通配符（* 或 ?），用于自动补全逻辑
bool wildcardInCompletion(token_index_arr token, int line_index) {
  int current_token_index = 0;
  // 遍历 token 数组，找到光标所在的 token 索引
  for (int i = 0; i < token.len; i++) {
    if (line_index >= token.arr[i].start && line_index <= token.arr[i].end) {
      current_token_index = i;
    }
  }
  
  // 从当前 token 向前遍历，查找是否有通配符
  for (int j = current_token_index; j > 0; j--) {
    // 遇到空白 token 停止查找
    if (token.arr[j].token == WHITESPACE) {
      break;
    }
    // 如果遇到 * 或 ?，说明存在通配符，返回 true
    else if (token.arr[j].token == ASTERISK || token.arr[j].token == QUESTION) {
      return true;
    }
  }
  // 未找到通配符，返回 false
  return false;
}

// 查找第一个非空白 token 的起始位置
int firstNonWhitespaceToken(token_index_arr token_line) {
  for (int i = 0; i < token_line.len; i++) {
    // 如果当前 token 不是空白符，则返回其起始位置
    if (token_line.arr[i].token != WHITESPACE) {
      return token_line.arr[i].start;
    }
  }
  // 如果没有找到非空白 token，返回 INT_MAX
  return INT_MAX;
}

// 判断光标是否在第一个单词（非空白 token）之前，用于 TAB 自动补全逻辑
bool tabCompBeforeFirstWord(token_index_arr tokenized_line, int line_index) {
  // tokenized_line 非空且光标位置在第一个非空白 token 之前，返回 true
  return tokenized_line.len > 0 && line_index <= firstNonWhitespaceToken(tokenized_line);
}

// TAB 键自动补全处理函数
void tab(line_data* line_info, coordinates* cursor_pos, string_array PATH_BINS, coordinates terminal_size) {
  // 如果当前输入行为空，直接返回
  if (strlen(line_info->line) <= 0)
    return;

  // 对当前输入行进行分词，获取 token 数组
  token_index_arr tokenized_line = tokenizeLine(line_info->line);

  // 获取当前光标所在的 token, 默认返回的是最后一个token
  /* 这里也应该在单词末尾时获取 token */
  token_index current_token = getCurrentToken(*line_info->i, tokenized_line);

  // 如果当前 token 不是命令且存在通配符，直接返回，不进行补全
  if (current_token.token != CMD && wildcardInCompletion(tokenized_line, *line_info->i)) {
    return;
  }

  // 如果光标不在第一个单词前，且补全成功，则更新 token 信息和光标位置
  if (!tabCompBeforeFirstWord(tokenized_line, *line_info->i) &&
      tabLoop(line_info, cursor_pos, PATH_BINS, terminal_size, current_token)) {
    // 补全成功后，重新分词并获取新的 token
    free(tokenized_line.arr);
    tokenized_line = tokenizeLine(line_info->line);
    current_token =
        getCurrentToken(*line_info->i + 1, tokenized_line); /* 在单词末尾时无法识别 token */
    // 将光标移动到补全后单词的末尾
    *line_info->i = current_token.end;
    // 重置当前输入字符
    line_info->c = -1;
  }
  // 释放 token 数组内存
  free(tokenized_line.arr);
}

void upArrowPress(char* line, history_data* history_info) {
  if (history_info->history_index < history_info->sessions_command_history.len) {
    history_info->history_index += 1;
    memset(line, 0, strlen(line));
    strcpy(line, history_info->sessions_command_history.values[history_info->history_index - 1]);
  };
}

void downArrowPress(char* line, history_data* history_info) {
  if (history_info->history_index > 1) {
    history_info->history_index -= 1;
    memset(line, 0, strlen(line));
    strcpy(line, history_info->sessions_command_history.values[history_info->history_index - 1]);

  } else if (history_info->history_index > 0) {
    history_info->history_index -= 1;
    memset(line, 0, strlen(line));
  };
}

bool typedLetter(line_data* line_info) {
  bool cursor_moved = false;
  if ((line_info->c < 27 || line_info->c > 127) || (strlen(line_info->line) == 0 && line_info->c == TAB)) {
    return false;
  } else if ((strlen(line_info->line) * sizeof(char) + 1) >= line_info->size) {
    char* tmp;
    if ((tmp = realloc(line_info->line, 1.5 * line_info->size)) == NULL) {
      perror("zsh:");
    } else {
      line_info->line = tmp;
      line_info->size *= 1.5;
    }
  }

  if (*line_info->i == strlen(line_info->line)) {
    (line_info->line)[*line_info->i] = line_info->c;
    (line_info->line)[*line_info->i + 1] = '\0';
    cursor_moved = true;
  } else if (insertCharAtPos(line_info->line, *line_info->i, line_info->c)) {
    cursor_moved = true;
  }

  return cursor_moved;
}

void arrowPress(line_data* line_info, history_data* history_info, autocomplete_data* autocomplete_info) {
  getch();
  int value = getch();
  switch (value) {
  case 'A':
    upArrowPress(line_info->line, history_info);

    *line_info->i = strlen(line_info->line);
    break;

  case 'B':
    downArrowPress(line_info->line, history_info);

    *line_info->i = strlen(line_info->line);
    break;

  case 'C': { // right-arrow
    if (autocomplete_info->autocomplete &&
        strncmp(line_info->line, autocomplete_info->possible_autocomplete, strlen(line_info->line)) == 0) {
      memset(line_info->line, 0, strlen(line_info->line));
      if (((strlen(autocomplete_info->possible_autocomplete) + 1) * sizeof(char)) >= line_info->size) {
        line_info->line =
            realloc(line_info->line, (strlen(autocomplete_info->possible_autocomplete) + 1) * sizeof(char));
        line_info->size = (strlen(autocomplete_info->possible_autocomplete) + 1) * sizeof(char);
      }
      strcpy(line_info->line, autocomplete_info->possible_autocomplete);
      *line_info->i = strlen(line_info->line);
    } else {
      *line_info->i = (*line_info->i < strlen(line_info->line)) ? *line_info->i + 1 : *line_info->i;
    }
    break;
  }

  case 'D': { // left-arrow
    *line_info->i = (*line_info->i > 0) ? (*line_info->i) - 1 : *line_info->i;
    break;
  }
  }
}

void ctrlFPress(string_array all_time_command_history, coordinates terminal_size, coordinates* cursor_pos,
                line_data* line_info) {
  fuzzy_result popup_result = popupFuzzyFinder(all_time_command_history, terminal_size, cursor_pos->y,
                                               line_info->line_row_count_with_autocomplete);

  if (strcmp(popup_result.line, "") != 0) {
    if (((strlen(popup_result.line) + 1) * sizeof(char)) >= line_info->size) {
      line_info->line = realloc(line_info->line, (strlen(popup_result.line) + 1) * sizeof(char));
      line_info->size = (strlen(popup_result.line) + 1) * sizeof(char);
    }
    strcpy(line_info->line, popup_result.line);
    *line_info->i = strlen(line_info->line);
  }
  free(popup_result.line);

  if (popup_result.shifted) {
    cursor_pos->y = (terminal_size.y * 0.85) - 3 - line_info->line_row_count_with_autocomplete;
    moveCursor(*cursor_pos);
  } else {
    moveCursor(*cursor_pos);
  }
}

bool filterHistoryForMatchingAutoComplete(const string_array all_time_commands, char* line,
                                          autocomplete_data* autocomplete_info) {

  for (int i = 0; i < all_time_commands.len; i++) {
    if (strlen(line) > 0 && (strncmp(line, all_time_commands.values[i], strlen(line)) == 0)) {
      if (strlen(all_time_commands.values[i]) >= autocomplete_info->size) {
        autocomplete_info->possible_autocomplete = realloc(
            autocomplete_info->possible_autocomplete, (strlen(all_time_commands.values[i]) + 1) * sizeof(char));
        autocomplete_info->size = strlen(all_time_commands.values[i]) + 1;
      }
      strcpy(autocomplete_info->possible_autocomplete, all_time_commands.values[i]);

      return true;
    }
  }

  return false;
}

// 处理用户输入并更新行数据、自动补全、历史记录等状态
bool update(line_data* line_info, autocomplete_data* autocomplete_info, history_data* history_info,
      coordinates terminal_size, string_array PATH_BINS, coordinates* cursor_pos) {

  // 合并本会话历史和全局历史，便于自动补全和历史搜索
  string_array all_time_command_history =
    concatenateArrays(history_info->sessions_command_history, history_info->global_command_history);
  bool loop = true;

  // TAB 键：触发自动补全
  if (line_info->c == TAB) {
    tab(line_info, cursor_pos, PATH_BINS, terminal_size);
  }
  // 退格键：删除字符
  if (line_info->c == BACKSPACE) {
    backspaceLogic(line_info->line, line_info->i);
  }
  // ESC 键：处理方向键（上下左右）
  else if (line_info->c == ESCAPE) {
    arrowPress(line_info, history_info, autocomplete_info);
  }
  // 回车键：输入结束，退出循环
  else if (line_info->c == '\n') {
    free_string_array(&all_time_command_history);
    return false;
  }
  // Ctrl+F：弹出模糊查找历史命令
  else if ((int)line_info->c == CONTROL_F) {
    ctrlFPress(all_time_command_history, terminal_size, cursor_pos, line_info);
  }
  // 普通字符输入：插入字符并移动光标
  else if (line_info->c != -1 && typedLetter(line_info)) {
    (*line_info->i)++;
  }

  // 自动补全逻辑：查找历史命令中是否有匹配项
  autocomplete_info->autocomplete =
    filterHistoryForMatchingAutoComplete(all_time_command_history, line_info->line, autocomplete_info);

  // 计算当前行（含自动补全）占用的行数
  int line_len = (autocomplete_info->autocomplete) ? strlen(autocomplete_info->possible_autocomplete)
                           : strlen(line_info->line);
  line_info->line_row_count_with_autocomplete = calculateRowCount(terminal_size, line_info->prompt_len, line_len);

  // 计算光标当前所在的行数
  line_info->cursor_row = calculateRowCount(terminal_size, line_info->prompt_len, *line_info->i);

  // 释放合并后的历史命令数组
  free_string_array(&all_time_command_history);

  return loop;
}

bool shiftLineIfOverlap(int current_cursor_height, int terminal_height, int line_row_count_with_autocomplete) {
  if ((current_cursor_height + line_row_count_with_autocomplete) < terminal_height)
    return false;

  for (int i = terminal_height; i < (current_cursor_height + line_row_count_with_autocomplete); i++) {
    printf("\n");
  }
  return true;
}

void stringToLower(char* string) {
  for (int i = 0; i < strlen(string); i++) {
    string[i] = tolower(string[i]);
  }
}

bool isInPath(char* line, string_array PATH_BINS) {
  bool result = false;
  char* line_copy = calloc(strlen(line) + 1, sizeof(char));
  strcpy(line_copy, line);
  stringToLower(line_copy);

  char* bin_copy;
  for (int i = 0; i < PATH_BINS.len; i++) {
    bin_copy = calloc(strlen(PATH_BINS.values[i]) + 1, sizeof(char));
    strcpy(bin_copy, PATH_BINS.values[i]);
    stringToLower(bin_copy);

    if (strcmp(bin_copy, line_copy) == 0) {
      result = true;
    }
    free(bin_copy);
  }
  free(line_copy);
  return result;
}

void printTokenizedLine(line_data line_info, token_index_arr tokenized_line, builtins_array BUILTINS,
                        string_array PATH_BINS) {
  for (int i = 0; i < tokenized_line.len; i++) {
    int token_start = tokenized_line.arr[i].start;
    int token_end = tokenized_line.arr[i].end;
    char* substring = calloc(token_end - token_start + 1, sizeof(char));
    strncpy(substring, &line_info.line[token_start], token_end - token_start);

    switch (tokenized_line.arr[i].token) {
    case (AMP_CMD):
    case (PIPE_CMD):
    case (CMD): {
      bool in_path = isInPath(substring, PATH_BINS);
      bool is_builtin = isBuiltin(substring, BUILTINS) != -1 ? true : false;
      bool is_exec = isExec(substring);
      in_path || is_builtin || is_exec ? printColor(substring, GREEN, standard) : printColor(substring, RED, bold);
      break;
    }
    case (ARG): {
      char* copy_sub = calloc(strlen(substring) + 1, sizeof(char));
      strcpy(copy_sub, substring);
      replaceAliasesString(&copy_sub);
      removeEscapesString(copy_sub);

      autocomplete_array autocomplete = fileComp(copy_sub);
      if (autocomplete.array.len > 0 && !wildcardInCompletion(tokenized_line, *line_info.i)) {
        printColor(substring, WHITE, underline);
      } else if (substring[0] == '\'' && substring[strlen(substring) - 1] == '\'') {
        printColor(substring, YELLOW, standard);
      } else {
        printf("%s", substring);
      }
      free(copy_sub);
      free_string_array(&(autocomplete.array));
      break;
    }
    case (WHITESPACE): {
      printf("%s", substring);
      break;
    }
    case (GREAT):
    case (GREATGREAT):
    case (LESS):
    case (AMP_GREAT):
    case (AMP_GREATGREAT):
    case (PIPE):
    case (AMPAMP): {
      printColor(substring, YELLOW, standard);
      break;
    }
    case (QUESTION):
    case (ASTERISK): {
      printColor(substring, BLUE, bold);
      break;
    }
    default: {
      fprintf(stderr, "zsh: invalid input\n");
      break;
    }
    }
    free(substring);
  }
}

void printLine(line_data line_info, builtins_array BUILTINS, string_array PATH_BINS) {
  token_index_arr tokenized_line = tokenizeLine(line_info.line);
  printTokenizedLine(line_info, tokenized_line, BUILTINS, PATH_BINS);
  free(tokenized_line.arr);
}

// 渲染当前输入行和自动补全提示
void render(line_data* line_info, autocomplete_data* autocomplete_info, const string_array PATH_BINS,
      char* directories, coordinates* starting_cursor_pos, coordinates terminal_size,
      builtins_array BUILTINS) {
  // 如果当前行内容超出终端高度，则调整起始光标位置，避免内容被遮挡
  if (shiftLineIfOverlap(starting_cursor_pos->y, terminal_size.y, line_info->line_row_count_with_autocomplete)) {
  starting_cursor_pos->y -=
    (starting_cursor_pos->y + line_info->line_row_count_with_autocomplete) - terminal_size.y;
  }
  // 移动光标到起始位置
  moveCursor(*starting_cursor_pos);

  // 清除当前行和光标以下内容，准备重新渲染
  CLEAR_LINE;
  CLEAR_BELOW_CURSOR;
  printf("\r");
  // 打印提示符
  printPrompt(directories, CYAN);

  // 如果当前输入行非空，打印输入内容
  if (strlen(line_info->line) > 0) {
  printLine(*line_info, BUILTINS, PATH_BINS);

  // 如果存在自动补全，打印补全部分（高亮显示）
  if (autocomplete_info->autocomplete) {
    printf("%s", &autocomplete_info->possible_autocomplete[strlen(line_info->line)]);
  }
  }
  // 计算补全后光标的新位置
  coordinates new_cursor_pos = calculateCursorPos(
    terminal_size, (coordinates){.x = 0, .y = starting_cursor_pos->y}, line_info->prompt_len, *line_info->i);

  // 移动光标到新位置
  moveCursor(new_cursor_pos);
  // 更新起始光标位置
  starting_cursor_pos->x = new_cursor_pos.x;
}

// 读取一行用户输入，支持自动补全、历史记录等功能
char* readLine(string_array PATH_BINS, char* directories, string_array* command_history,
         const string_array global_command_history, builtins_array BUILTINS) {

  // 获取终端尺寸
  const coordinates terminal_size = getTerminalSize();

  // 初始化行数据结构体
  line_data* line_info = lineDataConstructor(strlen(directories));

  // 初始化自动补全数据结构体
  autocomplete_data* autocomplete_info = autocompleteDataConstructor();

  // 初始化历史记录数据结构体
  history_data* history_info = historyDataConstructor(command_history, global_command_history);

  // 获取当前光标位置
  coordinates* cursor_pos = calloc(1, sizeof(coordinates));
  *cursor_pos = getCursorPos();

  int loop = true;

  // 主循环，读取用户输入并实时更新界面
  while (loop && (line_info->c = getch()) != -1) {
    // 处理输入并更新状态
    loop = update(line_info, autocomplete_info, history_info, terminal_size, PATH_BINS, cursor_pos);

    // 渲染当前行和自动补全提示
    render(line_info, autocomplete_info, PATH_BINS, directories, cursor_pos, terminal_size, BUILTINS);
  }

  // 输入结束后，将光标移动到下一行
  moveCursor((coordinates){
    1000, cursor_pos->y + calculateRowCount(terminal_size, line_info->prompt_len, strlen(line_info->line))});

  // 释放自动补全相关内存
  free(autocomplete_info->possible_autocomplete);
  free(autocomplete_info);

  // 释放历史记录结构体内存
  free(history_info);

  // 释放光标位置结构体内存
  free(cursor_pos);

  // 拷贝最终输入结果
  char* result = calloc(strlen(line_info->line) + 1, sizeof(char));
  strcpy(result, line_info->line);

  // 释放行数据结构体相关内存
  free(line_info->line);
  free(line_info->i);
  free(line_info);

  // 输出换行符
  printf("\n");
  return result;
}
