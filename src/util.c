#include "util.h"
#include <regex.h>

int getch() {
  struct termios oldattr, newattr;
  int ch;
  tcgetattr(STDIN_FILENO, &oldattr);
  newattr = oldattr;
  newattr.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
  return ch;
}

// 打印带颜色和装饰的字符串
void printColor(const char *string, color color, enum color_decorations decor)
{
  // 设置样式
  printf("\e[%d;%d;%dm", decor, color.fg, color.bg);

  // 打印文本
  printf("%s", string);

  // 恢复默认
  printf("\e[0m");
}

bool isOnlyDelimeter(const char* string, char delimeter) {
  for (int i = 0; i < strlen(string); i++) {
    if (string[i] != delimeter) {
      return false;
    }
  }
  return true;
}

char* removeMultipleWhitespaces(char* string) {
  int i = 0;
  for (; i < strlen(string) && string[i] == ' '; i++)
    ;
  char* new_string = calloc(strlen(string) + 1, sizeof(char));
  strcpy(new_string, &string[i]);
  free(string);
  bool is_first = true;
  for (int j = 0; j < strlen(new_string);) {
    if (new_string[j] == ' ' && is_first) {
      is_first = false;
      j++;
    } else if (new_string[j] == ' ' && !is_first) {
      new_string = removeCharAtPos(new_string, j);
    } else {
      is_first = true;
      j++;
    }
  }
  return new_string;
}

void free_string_array(string_array* arr) {
  if (arr->values == NULL)
    return;
  for (int i = 0; i < arr->len; i++) {
    free(arr->values[i]);
    arr->values[i] = NULL;
  }
  free(arr->values);
  arr->values = NULL;
}

// 将光标的位置放到指定的坐标
void moveCursor(coordinates new_pos) { printf("\033[%d;%dH", new_pos.y, new_pos.x); }

coordinates getTerminalSize() {
  coordinates size;
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  size.x = w.ws_col;
  size.y = w.ws_row;

  return size;
}

bool inArray(char* value, string_array array) {
  for (int i = 0; i < array.len; i++) {
    if (strcmp(value, array.values[i]) == 0) {
      return true;
    }
  }
  return false;
}

string_array removeDuplicates(string_array* matching_commands) {
  int j = 0;
  string_array no_dup_array;
  no_dup_array.values = calloc(matching_commands->len, sizeof(char*));
  no_dup_array.len = 0;

  for (int i = 0; i < matching_commands->len; i++) {
    if (!inArray(matching_commands->values[i], no_dup_array)) {
      no_dup_array.values[j] = calloc(strlen(matching_commands->values[i]) + 1, sizeof(char));
      strcpy(no_dup_array.values[j], matching_commands->values[i]);
      no_dup_array.len += 1;
      j++;
    }
  }
  free_string_array(matching_commands);

  return no_dup_array;
}

// 直接从字符串中删除指定位置的字符, 后面的往前插入
char* removeCharAtPos(char* line, int x_pos) {
  for (int i = x_pos - 1; i < strlen(line); i++) {
    line[i] = line[i + 1];
  }
  return line;
}

void backspaceLogic(char* line, int* i) {
  if (strlen(line) > 0 && *i >= 0) {
    line = removeCharAtPos(line, *i);

    if (*i > 0) {
      *i -= 1;
    }
  }
}

void logger(enum logger_type type, void* message) {
  FILE* logfile = fopen("log.txt", "a");

  switch (type) {
  case integer: {
    fprintf(logfile, "%d", *((int*)message));
    break;
  }
  case string: {
    fprintf(logfile, "%s", (char*)message);
    break;
  }
  case character: {
    fprintf(logfile, "%c", *(char*)message);
    break;
  }
  default: {
    break;
  }
  }
  fclose(logfile);
}

// 获取当前光标在终端中的位置
coordinates getCursorPos() {
  char buf[30] = {0}; // 用于存储终端返回的光标位置字符串
  int ret, i, pow;
  char ch;
  coordinates cursor_pos = {.x = 0, .y = 0}; // 初始化光标坐标

  struct termios term, restore;

  // 获取当前终端属性并保存，用于后续恢复
  tcgetattr(0, &term);
  tcgetattr(0, &restore);
  // 设置终端为非规范模式且关闭回显
  term.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(0, TCSANOW, &term);

  // 发送转义序列请求光标位置
  write(1, "\033[6n", 4);

  // 读取终端返回的光标位置响应，直到遇到'R'为止
  for (i = 0, ch = 0; ch != 'R'; i++) {
    ret = read(0, &ch, 1);
    if (!ret) {
      // 读取失败，恢复终端属性并报错
      tcsetattr(0, TCSANOW, &restore);
      fprintf(stderr, "zsh: error parsing cursor-position\n");
      return cursor_pos;
    }
    buf[i] = ch;
  }

  // 如果返回内容长度异常，直接恢复终端属性并返回
  if (i < 2) {
    tcsetattr(0, TCSANOW, &restore);
    return cursor_pos;
  }

  // 解析列号（x 坐标），从字符串末尾往前找到';'
  for (i -= 2, pow = 1; buf[i] != ';'; i--, pow *= 10)
    cursor_pos.x += (buf[i] - '0') * pow;

  // 解析行号（y 坐标），继续往前找到'['
  for (i--, pow = 1; buf[i] != '['; i--, pow *= 10)
    cursor_pos.y += (buf[i] - '0') * pow;

  // 恢复终端属性
  tcsetattr(0, TCSANOW, &restore);
  return cursor_pos;
}

string_array getAllFilesInDir(string_array* directory_array) {
  struct dirent* file;
  string_array all_path_files;
  char** files = calloc(BUFFER, sizeof(char*));
  int j = 0;
  size_t size = BUFFER * sizeof(char*);


  for (int i = 0; i < directory_array->len; i++) {
    if (isDirectory(directory_array->values[i])) {
      DIR* dr = opendir(directory_array->values[i]);

      while ((file = readdir(dr)) != NULL) {
        // 扩充
        if ((j * sizeof(char*)) >= size) {
          char** tmp;
          if ((tmp = realloc(files, size * 1.5)) == NULL) {
            perror("realloc");
          } else {
            files = tmp;
            size *= 1.5;
          }
        }

        // 将该文件夹中的所有文件名存入数组
        files[j] = (char*)calloc(strlen(file->d_name) + 1, sizeof(char));
        strcpy(files[j], file->d_name);
        j++;
      }
      closedir(dr);
    }
  }
  all_path_files.values = files;
  all_path_files.len = j;

  // free_string_array(directory_array);
  return all_path_files;
}

// 过滤出所有以 line(当前字符串) 为前缀的字符串，返回匹配的字符串数组
string_array filterMatching(char* line, const string_array PATH_BINS) {
  int buf_size = 64; // 初始缓冲区大小
  size_t size = buf_size * sizeof(char*);
  char** matching_binaries = calloc(buf_size, sizeof(char*)); // 存储匹配结果
  string_array result;
  int j = 0; // 匹配结果计数器

  // 遍历 PATH_BINS 中的每个字符串
  for (int i = 0; i < PATH_BINS.len; i++) {
    // 如果当前字符串以 line 为前缀
    if (strncmp(PATH_BINS.values[i], line, strlen(line)) == 0) {
      // 检查是否需要扩容
      if ((j * sizeof(char*)) >= size) {
        char** tmp;
        if ((tmp = realloc(matching_binaries, size * 1.5)) == NULL) {
          perror("realloc");
        } else {
          matching_binaries = tmp;
          size *= 1.5;
        }
      }
      // 分配空间并复制匹配的字符串
      matching_binaries[j] = calloc(strlen(PATH_BINS.values[i]) + 1, sizeof(char));
      strcpy(matching_binaries[j], PATH_BINS.values[i]);
      j++;
    }
  }
  // 构造结果数组
  result.values = matching_binaries;
  result.len = j;

  return result;
}

// removed_sub是当前输入的文件名的最后一个斜杠后的子串
string_array getAllMatchingFiles(char* current_dir_sub, char* removed_sub) {
  char* temp_sub = calloc(strlen(current_dir_sub) + 1, sizeof(char));
  strcpy(temp_sub, current_dir_sub);

  string_array current_dir_array = {.len = 1, .values = &temp_sub};
  // 获取当前目录下的所有文件
  string_array all_files_in_dir = getAllFilesInDir(&current_dir_array);

  // 用当前目录和被移除的子串去过滤所有文件
  string_array filtered = filterMatching(removed_sub, all_files_in_dir);

  free_string_array(&all_files_in_dir);
  free(temp_sub);

  return filtered;
}

// 在指定位置插入字符
bool insertCharAtPos(char* line, int index, char c) {
  int len = strlen(line);
  if (index >= 0 && index <= strlen(line)) {
    // 从最后开始插即可,不要忘了重新分配内存
    for (int i = strlen(line) - 1; i >= index; i--) {
      line[i + 1] = line[i];
    }
    line[index] = c;
    line[len + 1] = '\0';
  } else {
    return false;
  }
  return true;
}

void insertStringAtPos(char** line, char* insert_string, int position) {
  if (strcmp(insert_string, "") == 0)
    return;
  char* tmp;
  if ((tmp = realloc(*line, (strlen(*line) + strlen(insert_string) + 2) * sizeof(char))) == NULL) {
    perror("realloc");
  } else {
    *line = tmp;
  }
  insertCharAtPos(*line, position, '%');
  insertCharAtPos(*line, position + 1, 's');

  char* new_line = calloc(strlen(*line) + strlen(insert_string) + 1, sizeof(char));
  sprintf(new_line, *line, insert_string);
  strcpy(*line, new_line);
  free(new_line);
}

int isDirectory(const char* path) {
  struct stat statbuf;
  // stat函数获取文件状态信息. 确定给定路径是否存在以及是否为目录
  if (stat(path, &statbuf) != 0)
    return 0;

  return S_ISDIR(statbuf.st_mode);
}

string_array copyStringArray(string_array arr) {
  string_array copy = {.len = arr.len, .values = calloc(arr.len, sizeof(char*))};
  for (int i = 0; i < arr.len; i++) {
    copy.values[i] = calloc(strlen(arr.values[i]) + 1, sizeof(char));
    strcpy(copy.values[i], arr.values[i]);
  }

  return copy;
}

// 解析当前单词和路径，返回文件路径相关信息
// 也就是将当前单词转换为绝对路径，并获取最后一个斜杠后的子串
file_string_tuple getFileStrings(char* current_word, char* current_path) {
  char* current_dir;
  char* removed_sub;

  switch (current_word[0]) {
  case '/': {
    // 如果以绝对路径开头
    current_dir = current_word;
    if (strlen(current_dir) == 1) {
      // 仅为根目录
      removed_sub = "";
    } else {
      // 获取最后一个斜杠后的子串
      // 获取最后一个斜杠后的子串, 用取地址的方式获取
      removed_sub = &(current_dir[strlen(current_dir) - getAppendingIndex(current_dir, '/')]); // c_e
    }
    break;
  }
  case '~': {
    // 以 ~ 开头，替换为 HOME 路径
    char* home_path = getenv("HOME"); // 获取用户主目录
    strcpy(current_path, home_path);

    // 拼接剩余路径,这样取地址类似substr的用法
    current_dir = strcat(current_path, &(current_word[1])); // 例如 ~/documents
    removed_sub = &(current_dir[strlen(current_dir) - getAppendingIndex(current_dir, '/')]); // documents
    break;
  }
  default: {
    // 相对路径处理
    current_dir = strcat(current_path, current_word); // 例如 documents/coding/c_e
    // 获取最后一个斜杠后的子串
    removed_sub = &(current_dir[strlen(current_dir) - getAppendingIndex(current_dir, '/')]); // c_e
    break;
  }
  }

  // 返回结构体，包含当前目录和被移除的子串
  return (file_string_tuple){.removed_sub = removed_sub, .current_dir = current_dir};
}

// 截取最后一个delimeter后的子串的起始位置
int getAppendingIndex(char* line, char delimeter) {
  int j = 0;
  for (int i = strlen(line) - 1; i >= 0; i--) {
    if (line[i] == delimeter)
      return j;
    j++;
  }
  return -1;
}

// filtered是当前目录下所有匹配的文件名数组
// fileDirArray函数会检查每个匹配项是否为目录，如果是，则在末尾添加斜杠
void fileDirArray(string_array* filtered, char* current_dir_sub) {
  int longest_word = getLongestWordInArray(*filtered);

  // 分配内存
  char *current_dir_sub_copy =
      calloc(strlen(current_dir_sub) + longest_word + 2, sizeof(char));    
  char* temp;
  char copy[strlen(current_dir_sub) + longest_word + 2];
  
  // 当前目录子路径后面加上斜杠
  strcat(current_dir_sub, "/");

  // 检测每个匹配项是否为目录
  for (int i = 0; i < filtered->len; i++) {
    strcpy(current_dir_sub_copy, current_dir_sub);

    // 因为是当前目录的子路径，所以需要在后面加上匹配的文件名
    temp = strcpy(copy, strcat(current_dir_sub_copy, filtered->values[i]));

    // 如果是则加上斜杠
    if (isDirectory(temp)) {
      filtered->values[i] = realloc(filtered->values[i], strlen(filtered->values[i]) + 2);
      filtered->values[i][strlen(filtered->values[i]) + 1] = '\0';
      filtered->values[i][strlen(filtered->values[i])] = '/';
    }
    // 重置内容
    memset(copy, 0, strlen(copy));
    memset(temp, 0, strlen(temp));
    memset(current_dir_sub_copy, 0, strlen(current_dir_sub_copy));
  }
  free(current_dir_sub_copy);
  free(current_dir_sub);
}

int getCurrentWordPosInLine(string_array command_line, char* word) {
  for (int i = 0; i < command_line.len; i++) {
    if (strncmp(command_line.values[i], word, strlen(word)) == 0) {
      return i;
    }
  }

  return -1;
}

// 计算字符串中的空格数量
int countWhitespace(char* string) {
  int result = 0;
  for (int i = 0; i < strlen(string); i++) {
    if (string[i] == ' ')
      result++;
  }
  return result;
}

// 文件路径自动补全函数，根据当前输入的单词返回可补全的文件/目录列表及补全起始位置
autocomplete_array fileComp(char* current_word) {
  // 获取当前工作目录，并拼接斜杠
  char cd[PATH_MAX];
  file_string_tuple file_strings = getFileStrings(current_word, strcat(getcwd(cd, sizeof(cd)), "/"));

  // 分配空间，准备存储当前目录的子路径
  char* current_dir_sub = calloc(strlen(file_strings.current_dir) + 2, sizeof(char));

  // 截取当前目录路径（去除最后一个斜杠及其后内容）
  strncpy(current_dir_sub, file_strings.current_dir,
          strlen(file_strings.current_dir) - getAppendingIndex(file_strings.current_dir, '/'));

  // 获取当前目录下所有与输入前缀匹配的文件/目录名
  string_array filtered = getAllMatchingFiles(current_dir_sub, file_strings.removed_sub);

  // 检查每个匹配项是否为目录，是则在末尾添加斜杠
  fileDirArray(&filtered, current_dir_sub);

  // 输入的时候空格变成\ + 空格, 所以要再加上一次空格的数量
  int offset = countWhitespace(file_strings.removed_sub);

  // 返回自动补全结果，包括匹配项数组和补全起始下标
  return (autocomplete_array){.array.values = filtered.values,
                              .array.len = filtered.len,
                              .appending_index = strlen(file_strings.removed_sub) + offset};
}


// 计算光标在终端中的实际位置
coordinates calculateCursorPos(coordinates terminal_size, coordinates cursor_pos, int prompt_len, int i)
{
  // 计算光标在当前行的绝对位置（包括提示符长度）
  int line_pos = i + prompt_len;

  // 如果光标还在第一行（未超过终端宽度）
  if (line_pos < terminal_size.x)
  {
    // x: 当前列，y: 当前行
    return (coordinates){.x = line_pos, .y = cursor_pos.y};
  }
  // 如果光标正好在某一行的末尾（整除终端宽度）
  else if (line_pos % terminal_size.x == 0)
  {
    // x: 最后一列，y: 行数加上已经换行的行数
    return (coordinates){.x = terminal_size.x, .y = cursor_pos.y + ((line_pos - 1) / terminal_size.x)};
  }
  // 光标在多行输入的某一行中间
  else
  {
    // x: 当前列（对终端宽度取余），y: 行数加上已经换行的行数
    return (coordinates){.x = line_pos % terminal_size.x, .y = cursor_pos.y + (line_pos / terminal_size.x)};
  }
}

// 计算输入内容占用了多少行（从0开始计数）
int calculateRowCount(coordinates terminal_size, int prompt_len, int i)
{
  // 计算光标最终所在的行数（y），即输入内容占用的总行数
  return calculateCursorPos(terminal_size, (coordinates){0, 0}, prompt_len, i).y;
}

// 如果字符串长度超过终端宽度，则进行截断并在末尾添加省略号
char* shortenIfTooLong(char* word, int terminal_width) {
  // 分配足够的空间存储结果字符串
  char* result = calloc(strlen(word) + 1, sizeof(char));
  strcpy(result, word); // 复制原始字符串

  // 如果字符串长度超出终端宽度限制
  if (strlen(word) > terminal_width - 2) {
    result[terminal_width - 2] = '\0'; // 截断字符串
    // 在末尾添加省略号（...），表示被截断
    for (int j = terminal_width - 3; j > (terminal_width - 6); j--) {
      result[j] = '.';
    }
  }
  return result; // 返回处理后的字符串
}

int firstNonDelimeterIndex(string_array splitted_line) {
  for (int i = 0; i < splitted_line.len; i++) {
    if (strcmp(splitted_line.values[i], "") != 0) {
      return i;
    }
  }
  return 0;
}

// 获取最长string的长度
int getLongestWordInArray(const string_array array) {
  int longest = 0;
  int current_len = 0;

  for (int i = 0; i < array.len; i++) {
    current_len = strlen(array.values[i]);
    if (current_len > longest) {
      longest = current_len;
    }
  }

  return longest;
}

bool isExec(char* file) {
  if (access(file, F_OK | X_OK) == 0 && !isDirectory(file)) {
    return true;
  }
  return false;
}

// 获取当前光标所在的 token 信息
token_index getCurrentToken(int line_index, token_index_arr tokenized_line) {
  // 默认返回最后一个 token 的起止位置
  token_index result = {.start = tokenized_line.arr[tokenized_line.len - 1].start,
                        .end = tokenized_line.arr[tokenized_line.len - 1].end};
  // 遍历所有 token，查找包含 line_index 的 token
  for (int i = 0; i < tokenized_line.len; i++) {
    // 如果 line_index 落在当前 token 的范围内，则返回该 token
    if (line_index >= tokenized_line.arr[i].start && line_index <= tokenized_line.arr[i].end) {
      result = tokenized_line.arr[i];
      break;
    }
  }
  // 返回找到的 token（或最后一个 token）
  return result;
}

// 移除字符串中的转义字符（反斜杠）
// 参数：string - 指向字符串指针的指针
void removeEscapes(char* str) {
    if (!str) return; // 安全防护
    
    char *src = str; // 原始字符串指针
    char *dst = str; // 目标字符串指针
    
    for (; *src; ++src) {
        if (*src == '\\') {
            // 跳过反斜杠，保留下一个字符
            continue;
        }
        *dst++ = *src; // 复制字符到新位置
    }
    *dst = '\0'; // 结束新字符串
    
    // 如有需要，可在此处重新分配内存以释放多余空间
}

void removeSlice(char** line, int start, int end) {
  for (int i = start; i < end; i++) {
    *line = removeCharAtPos(*line, start + 1);
  }
}

// 按起始位置排序
int tokenCmp(const void* a, const void* b) {
  token_index cast_a = *(token_index*)a;
  token_index cast_b = *(token_index*)b;
  return cast_a.start - cast_b.start;
}

// 对输入行进行分词，返回每个 token 的起止位置和类型
token_index_arr tokenizeLine(char* line) {
  // 复制输入行，避免修改原始内容
  char* copy = calloc(strlen(line) + 1, sizeof(char));
  strcpy(copy, line);

  int retval = 0;
  regex_t re;
  regmatch_t rm[ENUM_LEN];

  // 定义各种正则表达式，用于匹配不同类型的 token
  char *quoted_args = "(\'[^\']*\')"; // 单引号内的字符串
  char *whitespace = "(\\\\[ ])|([ ]+)"; // 空格或转义空格
  char* filenames = "([12]?>{2}|[12]?>|<|&>|&>>)[\t]*([^\n\t><]+)";
  char* redirection = "([12]?>{2})|([12]?>)|(<)|(&>)|(&>>)";
  char* line_token = "^[\n\t]*([^\n\t\f|&&]+)|\\|[\t\n]*([^\n\t\f|&&]+)|(\\|)|(&&)|&&["
                     "\t\n]*([^\n\t\f|&&]+)";
  char* wildcards = "(\\*)|(\\?)";
  char* only_args = "([^\t\n\f]+)";

  // 将所有正则表达式放入数组，便于循环处理
  char* regexes[] = {quoted_args, whitespace, filenames, redirection, line_token, wildcards, only_args};

  // 每种正则的处理参数, 填充字符, token类型, 循环开始位置
  regex_loop_struct regex_info[] = {
    {.fill_char = '\n', .loop_start = 1, .token_index_inc = 13}, // 单引号内的字符串
    {.fill_char = '\t', .loop_start = 1, .token_index_inc = 9},  // 空格或转义空格
    {.fill_char = '\n', .loop_start = 2, .token_index_inc = 12}, // filenames(args)
    {.fill_char = '\n', .loop_start = 1, .token_index_inc = 5},  // redirection
    {.fill_char = '\f', .loop_start = 1, .token_index_inc = 0},  // line-token
    {.fill_char = '\t', .loop_start = 1, .token_index_inc = 11}, // wildcards
    {.fill_char = '\t', .loop_start = 1, .token_index_inc = 13}  // args
  };

  char* start;
  char* end;
  int j = 0;

  // 结果数组，存储所有 token 的起止位置和类型
  token_index* result_arr = calloc(strlen(line), sizeof(token_index));

  // 依次用每种正则表达式处理输入行
  for (int k = 0; k < (sizeof(regexes) / sizeof(regexes[0])); k++) {
    // 编译正则表达式
    if (regcomp(&re, regexes[k], REG_EXTENDED) != 0) {
      perror("Error in compiling regex\n");
    }
    if (k == 2) {
      // 针对 filenames 匹配前，将之前转义的空格还原
      for (int l = 0; l < strlen(copy); l++) {
        if (copy[l] == '\f') {
          removeCharAtPos(copy, l + 1);
          removeCharAtPos(copy, l + 1);
          insertStringAtPos(&copy, "\\ ", l);
        }
      }
    }
    // 在输入行中查找所有匹配项
    for (; j < strlen(copy); j++) {
      if ((retval = regexec(&re, copy, ENUM_LEN, rm, 0)) == 0) {
        // 遍历所有捕获组
        for (int i = regex_info[k].loop_start; i < ENUM_LEN; i++) {
          if (rm[i].rm_so != -1) {
            start = copy + rm[i].rm_so;
            end = copy + rm[i].rm_eo;
            if (k == 1 && i == 1) {
              // 匹配到转义空格，做特殊处理
              while (start < end) {
                *start++ = '\f'; // 用特殊字符覆盖，便于后续识别
              }
              j--;
            } else {
              // 记录 token 的起止位置和类型
              result_arr[j].start = rm[i].rm_so;
              result_arr[j].end = rm[i].rm_eo;
              result_arr[j].token = i + regex_info[k].token_index_inc;
              // 用特殊字符覆盖已匹配内容，避免重复匹配
              while (start < end) {
                *start++ = regex_info[k].fill_char;
              }
            }
            break;
          }
        }
      } else {
        break;
      }
    }
    regfree(&re); // 释放正则资源
  }
  free(copy);

  // 按 token 起始位置排序
  qsort(result_arr, j, sizeof(token_index), tokenCmp);
  token_index_arr result = {.arr = result_arr, .len = j};

  return result;
}

int isBuiltin(char* command, builtins_array builtins) {
  for (int i = 0; i < builtins.len; i++) {
    if (strcmp(command, builtins.array[i].name) == 0) {
      return i;
    }
  }
  return -1;
}

void replaceAliasesString(char** line) {
  for (int j = 0; j < strlen(*line); j++) {
    if ((*line)[j] == '~') {
      char* home_path = getenv("HOME");
      char* prior_line = calloc(strlen(*line) + 1, sizeof(char));
      strcpy(prior_line, *line);
      removeCharAtPos(prior_line, j + 1);
      *line = realloc(*line, strlen(home_path) + strlen(*line) + 10);
      strcpy(*line, prior_line);
      insertStringAtPos(line, home_path, j);
      free(prior_line);
    }
  }
}

void printPrompt(const char* dir, color color) {
  // 使用指定颜色和加粗样式打印当前目录
  printColor(dir, color, bold);
  // 打印一个空格分隔
  printf(" ");
  // 使用绿色和标准样式打印提示符（右尖括号）
  printColor("\u2771 ", GREEN, standard);
}
