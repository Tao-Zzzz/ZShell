#include "tab_complete.h"

int maxWidthTerm(int width, int terminal_width) {
  if (width > terminal_width - 2) {
    width = terminal_width - 2;
  }
  return width;
}

// 渲染Tab补全列表
void tabRender(string_array possible_tabcomplete, int tab_index, int col_size, int format_width,
         int terminal_width) {
  int j = 0;
  char* complete;

  // 遍历所有补全项，按行列输出
  while (j < possible_tabcomplete.len) {
    printf("\n"); // 每行开始换行
    for (int x = 0; x < col_size; x++) {
      if (j >= possible_tabcomplete.len)
        break;
      // 如果补全项太长则截断显示
      complete = shortenIfTooLong(possible_tabcomplete.values[j], terminal_width);

      int diff_len = strlen(complete) - format_width;
      // 高亮当前选中的补全项
      if (tab_index == j) {
        printColor(complete, GREEN, reversed);
        printf("%-*s", diff_len, "");
      } else {
        // 目录用不同颜色显示
        if (possible_tabcomplete.values[j][strlen(possible_tabcomplete.values[j]) - 1] == '/') {
          printColor(complete, CYAN, bold);
          printf("%-*s", diff_len, "");
        } else {
          // 普通补全项正常显示
          printf("%-*s", format_width, complete);
        }
      }
      j++;
      free(complete); // 释放内存
    }
  }
}

// 在空格前插入转义字符
void escapeWhitespace(string_array* arr) {
  for (int i = 0; i < arr->len; i++) {
    for (int j = 0; j < strlen(arr->values[i]); j++) {
      if (arr->values[i][j] == ' ') {
        arr->values[i] = realloc(arr->values[i], (strlen(arr->values[i]) + 2) * sizeof(char));
        
        insertCharAtPos(arr->values[i], j, '\\');
        j++;
      }
    }
  }
}

autocomplete_array checkForAutocomplete(char* current_word, enum token current_token,
                                        const string_array PATH_BINS) {

  autocomplete_array possible_autocomplete = {.array.len = 0};

  // 命令类
  if (current_token == CMD || current_token == PIPE_CMD || current_token == AMP_CMD) {
    string_array filtered = filterMatching(current_word, PATH_BINS);

    possible_autocomplete = (autocomplete_array){
        .array.values = filtered.values, .array.len = filtered.len, .appending_index = strlen(current_word)};
  } else {
    // 文件类
    possible_autocomplete = fileComp(current_word);
    escapeWhitespace(&possible_autocomplete.array);
  }

  return possible_autocomplete;
}

// 如果补全列表显示会导致光标溢出终端底部，则调整光标位置
void moveCursorIfShifted(render_objects* render_data) {
  // 如果补全列表高度大于或等于光标到底部的距离，或者距离为0，则上移光标
  if (render_data->cursor_height_diff <= render_data->row_size || render_data->cursor_height_diff == 0) {
    render_data->cursor_pos->y -= render_data->row_size - render_data->cursor_height_diff;
    moveCursor(*render_data->cursor_pos);
  } else {
    // 否则直接移动光标到当前位置
    moveCursor(*render_data->cursor_pos);
  }
  // 最后再根据当前行数调整光标位置
  render_data->cursor_pos->y -= render_data->cursor_row;
}

// 渲染补全列表到终端
void renderCompletion(autocomplete_array possible_tabcomplete, int tab_index, render_objects* render_data) {
  // 先将光标y坐标加上当前行数
  render_data->cursor_pos->y += render_data->cursor_row;
  // 计算补全列表底部的y坐标
  int bottom_line_y =
      render_data->cursor_pos->y - render_data->cursor_row + render_data->line_row_count_with_autocomplete;
  // 计算光标到底部的距离
  render_data->cursor_height_diff = render_data->terminal_size.y - bottom_line_y;

  // 将光标移动到终端最右侧的底部行，防止内容被截断
  moveCursor((coordinates){1000, bottom_line_y}); // have to move cursor to end of
                                                  // line to not cut off in middle
  CLEAR_BELOW_CURSOR; // 清除光标以下内容
  // 渲染补全项
  tabRender(possible_tabcomplete.array, tab_index, render_data->col_size, render_data->format_width,
            render_data->terminal_size.x);
  // 渲染后根据需要调整光标位置
  moveCursorIfShifted(render_data);
}

bool dontShowMatches(char answer, render_objects* render_data, autocomplete_array possible_tabcomplete) {
  bool result = false;

  if (answer != 'y') {
    result = true;
  } else if (render_data->row_size >= render_data->terminal_size.y) {
    renderCompletion(possible_tabcomplete, -1, render_data);
    printf("\n\n");

    for (int i = 0; i < render_data->line_row_count_with_autocomplete; i++) {
      printf("\n");
    }
    render_data->cursor_pos->y =
        render_data->terminal_size.y + render_data->cursor_row - render_data->line_row_count_with_autocomplete;
    result = true;
  }
  return result;
}

// 判断可能的补全项是否“太多”，如果太多就询问用户是否要全部显示。
bool tooManyMatches(render_objects* render_data, autocomplete_array possible_tabcomplete) {
  char answer;
  char* prompt_sentence = "\nThe list of possible matches is %d lines. Do you want to print all of them? (y/n) ";
  // 计算底部行的y坐标
  int bottom_line_y = render_data->cursor_pos->y + render_data->line_row_count_with_autocomplete;

  moveCursor((coordinates){1000, bottom_line_y});

  // 太长或者超过终端则不显示
  if (!(render_data->row_size > 10 || render_data->row_size > render_data->terminal_size.y))
    return false;


  printf(prompt_sentence, render_data->row_size);
  answer = getch();

  // 计算光标行位置  
  int prompt_row_count = calculateRowCount(render_data->terminal_size, 0, strlen(prompt_sentence) - 1) + 1;
  
  // render_data是当前tab补齐的渲染数据
  // 计算是否会溢出终端底部, 溢出就把光标位置往上移
  int diff = prompt_row_count + bottom_line_y - render_data->terminal_size.y;
  if (diff > 0) {
    render_data->cursor_pos->y -= diff;
  }

  return dontShowMatches(answer, render_data, possible_tabcomplete);
}

// 处理Tab键按下的逻辑
bool tabPress(autocomplete_array possible_tabcomplete, int* tab_index, line_data* line_info,
        token_index current_token) {
  // 如果只有一个补全项，直接补全
  if (possible_tabcomplete.array.len == 1) {
    removeSlice(&line_info->line, *line_info->i, current_token.end);
    insertStringAtPos(&line_info->line,
              &(possible_tabcomplete.array.values[0])[possible_tabcomplete.appending_index],
              *line_info->i);

    line_info->size = (strlen(line_info->line) + 1) * sizeof(char);
    return true;
  } 
  // 如果有多个补全项，循环高亮下一个
  else if (possible_tabcomplete.array.len > 1) {
    if (*tab_index < possible_tabcomplete.array.len - 1) {
      *tab_index += 1;
    } else {
      *tab_index = 0;
    }
  }
  return false;
}

// 处理Shift+Tab键按下的逻辑
void shiftTabPress(autocomplete_array possible_tabcomplete, int* tab_index) {
  getch(); // 读取ESC
  if (getch() == 'Z') { // Shift-Tab
    if (*tab_index > 0) {
      *tab_index -= 1;
    } else {
      *tab_index = possible_tabcomplete.array.len - 1;
    }
  }
}

// 处理回车键按下的逻辑，执行补全
void enterPress(autocomplete_array possible_tabcomplete, line_data* line_info, int tab_index,
        token_index current_token) {
  removeSlice(&line_info->line, *line_info->i, current_token.end);
  // remove-slice is correct
  // have to count escapes upto line_index
  insertStringAtPos(&line_info->line,
          &(possible_tabcomplete.array.values[tab_index])[possible_tabcomplete.appending_index],
          *line_info->i);

  line_info->size = (strlen(line_info->line) + 1) * sizeof(char);
}

// 处理补全交互的主逻辑，根据用户输入更新补全状态
tab_completion updateCompletion(autocomplete_array possible_tabcomplete, char* c, line_data* line_info,
                int* tab_index, token_index current_token) {
              
  tab_completion result = {.successful = false, .continue_loop = true};

  // Tab键：补全或高亮下一个
  if (*c == TAB) {
  if (tabPress(possible_tabcomplete, tab_index, line_info, current_token)) {
    result.successful = true;
    result.continue_loop = false;
  }

  } 
  // Shift-Tab：高亮上一个
  else if (*c == ESCAPE) {
  shiftTabPress(possible_tabcomplete, tab_index);

  } 
  // 回车键：执行补全
  else if (*c == '\n') {
  enterPress(possible_tabcomplete, line_info, *tab_index, current_token);
  result.successful = true;
  result.continue_loop = false;

  } 
  // 其他按键：退出补全
  else {
  result.successful = false;
  result.continue_loop = false;
  }

  return result;
}

// 初始化渲染相关的数据结构
render_objects initializeRenderObjects(coordinates terminal_size, autocomplete_array possible_tabcomplete,
                     coordinates* cursor_pos, int cursor_row,
                     int line_row_count_with_autocomplete) {

  // 计算每列的最大宽度（最长单词宽度+2，防止显示不全），不超过终端宽度
  int format_width = maxWidthTerm(getLongestWordInArray(possible_tabcomplete.array), terminal_size.x) + 2;
  // 计算一行可以显示多少列
  int col_size = terminal_size.x / format_width;
  // 计算需要多少行来显示所有补全项，向上取整
  int row_size = ceil(possible_tabcomplete.array.len / (float)col_size);
  // 计算光标距离终端底部的行数
  int cursor_height_diff = terminal_size.y - cursor_pos->y;

  // 构造并返回render_objects结构体
  return (render_objects){
    .format_width = format_width,
    .col_size = col_size,
    .row_size = row_size,
    .cursor_height_diff = cursor_height_diff,
    .cursor_pos = cursor_pos,
    .terminal_size = terminal_size,
    .cursor_row = cursor_row,
    .line_row_count_with_autocomplete = line_row_count_with_autocomplete,
  };
}

// 移除以.开头的隐藏文件
autocomplete_array removeDotFiles(autocomplete_array* tabcomp) {
  autocomplete_array new;
  new.array.values = calloc(tabcomp->array.len, sizeof(char*));
  
  int j = 0;
  for (int i = 0; i < tabcomp->array.len; i++) {
    // 如果不是以.开头则保留
    if (tabcomp->array.values[i][0] != '.') {
      new.array.values[j] = calloc(strlen(tabcomp->array.values[i]) + 1, sizeof(char));
      strcpy(new.array.values[j], tabcomp->array.values[i]);
      j++;
    }
  }
  new.appending_index = tabcomp->appending_index;
  new.array.len = j;
  free_string_array(&tabcomp->array);
  return new;
}

void removeDotFilesIfnecessary(char* current_word, autocomplete_array* possible_tabcomplete) {
  int appending_index;
  char* removed_sub;

  // 如果当前单词包含斜杆，则获取斜杠后的子串
  if ((appending_index = getAppendingIndex(current_word, '/')) != -1) {
    removed_sub = &(current_word[strlen(current_word) - getAppendingIndex(current_word, '/')]);
  } else {
    removed_sub = current_word;
  }


  if (removed_sub[0] != '.') {
    *possible_tabcomplete = removeDotFiles(possible_tabcomplete);
  }
}

// 获取当前需要补全的单词
char* getCurrentWord(char* line, int line_index, token_index current_token) {
  // 如果token无效，返回NULL
  if (current_token.start == -1 && current_token.end == -1) {
    return NULL;
  } 
  // 如果当前token是空白，返回空字符串
  else if (current_token.token == WHITESPACE) {
    char* result = calloc(2, sizeof(char));
    strcpy(result, "");
    return result;
  } 
  // 否则，提取当前token对应的子串
  else {
    char* result = calloc(line_index - current_token.start + 1, sizeof(char));
    strncpy(result, &line[current_token.start], line_index - current_token.start);
    return result;
  }
}

// tabLoop函数用于处理Tab补全的主循环
bool tabLoop(line_data* line_info, coordinates* cursor_pos, const string_array PATH_BINS,
       const coordinates terminal_size, token_index current_token) {

  int tab_index = -1; // 当前高亮的补全项索引，初始为-1
  // 获取当前需要补全的单词
  char* current_word = getCurrentWord(line_info->line, *line_info->i, current_token);
  
  // 移除转义字符
  // removeEscapesString(&current_word);
  removeEscapesString(current_word);

  // 获取所有可能的补全项
  autocomplete_array possible_tabcomplete = checkForAutocomplete(current_word, current_token.token, PATH_BINS);

  // 如有必要，移除以.开头的隐藏文件
  removeDotFilesIfnecessary(current_word, &possible_tabcomplete); 

  // 初始化渲染相关的数据结构
  render_objects render_data =
    initializeRenderObjects(terminal_size, possible_tabcomplete, cursor_pos, line_info->cursor_row,
                line_info->line_row_count_with_autocomplete);

  // 如果没有补全项，或者当前单词为空，则释放资源并返回false
  tab_completion completion_result;

  // 如果没有补全项，或者补全项太多需要用户确认，则释放资源并返回false
  if (possible_tabcomplete.array.len <= 0 
      || tooManyMatches(&render_data, possible_tabcomplete)) {
    free_string_array(&(possible_tabcomplete.array));
    free(current_word);
    return false;
  }

  // 主循环，处理用户输入，直到补全完成或用户退出
  do {
      
    if ((completion_result =
        updateCompletion(possible_tabcomplete, &line_info->c, line_info, &tab_index, current_token))
        .continue_loop) {
      renderCompletion(possible_tabcomplete, tab_index, &render_data); // 渲染补全列表
    }

  } while (completion_result.continue_loop && (line_info->c = getch())); // 持续读取用户输入

  free_string_array(&(possible_tabcomplete.array)); // 释放补全项数组
  free(current_word); // 释放当前单词字符串

  return completion_result.successful; // 返回补全是否成功
}
