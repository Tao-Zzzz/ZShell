#include <stdlib.h>

enum cursor_direction { cursor_up, cursor_down, cursor_left, cursor_right };
// token类型
enum token {
  CMD = 1,           // 命令
  PIPE_CMD,          // 管道后的命令
  PIPE,              // 管道符号 '|'
  AMPAMP,            // 逻辑与 '&&'
  AMP_CMD,           // '&' 后的命令
  GREATGREAT,        // 追加输出 '>>'
  GREAT,             // 输出重定向 '>'
  LESS,              // 输入重定向 '<'
  AMP_GREAT,         // 重定向标准输出和错误输出 '>&'
  AMP_GREATGREAT,    // 追加标准输出和错误输出 '>>&'
  WHITESPACE,        // 空白字符
  ASTERISK,          // 通配符 '*'
  QUESTION,          // 通配符 '?'
  ARG,               // 参数
  ENUM_LEN           // token 类型数量
};

typedef struct wildcard_groups {
  char* wildcard_arg;
  int start;
  int end;
} wildcard_groups;

typedef struct {
  int len;
  wildcard_groups* arr;
} wildcard_groups_arr;

typedef struct token_index {
  int start;
  int end;
  enum token token;
} token_index;

typedef struct token_index_arr {
  token_index* arr;
  int len;
} token_index_arr;

typedef struct regex_loop_struct {
  char fill_char;
  int loop_start;
  int token_index_inc;
} regex_loop_struct;

typedef struct {
  char* output_filename;
  char* input_filename;
  char* stderr_filename;
  char* merge_filename;
  int output_append;
  int stderr_append;
  int merge_append;
} file_redirection_data;

enum logger_type {
  integer,
  string,
  character,
};

enum autocomplete_type {
  command,
  file_or_dir,
};

enum color_decorations { standard = 0, bold = 1, underline = 4, reversed = 7 };

// 坐标,存储终端大小
typedef struct coordinates {
  int x;
  int y;
} coordinates;

typedef struct {
  int len;
  char** values;
} string_array;

typedef struct {
  int len;
  char** values;
  enum token* token_arr;
} string_array_token;

typedef struct {
  string_array array;
  int appending_index;
} autocomplete_array;

typedef struct {
  int one;
  int second;
} integer_tuple;

typedef struct {
  char* removed_sub;
  char* current_dir;
} file_string_tuple;

typedef struct {
  int format_width;
  int col_size;
  int row_size;
  int cursor_height_diff;
  int cursor_row;
  int line_row_count_with_autocomplete;
  coordinates* cursor_pos;
  coordinates terminal_size;
} render_objects;

typedef struct {
  char* line;
  char c;
  int* i;
  int prompt_len;
  int cursor_row;
  int line_row_count_with_autocomplete;
  size_t size;
} line_data;

typedef struct {
  string_array sessions_command_history;
  string_array global_command_history;
  int history_index;
} history_data;

typedef struct {
  char* possible_autocomplete;
  int autocomplete;
  size_t size;
} autocomplete_data;

typedef struct {
  char* line;
  int shifted;
} fuzzy_result;

typedef struct {
  const char* name;
  void (*func)();
} function_by_name;

typedef struct {
  int len;
  function_by_name* array;
} builtins_array;


typedef struct {
  int successful;
  int continue_loop;
} tab_completion;

typedef struct {
  char** var_names;
  char** values;
  int len;
} env_var_arr;
