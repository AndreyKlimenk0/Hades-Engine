#include <stdio.h>
#include "test.h"
#include "../sys/sys_local.h"

#include "../libs/ds/stack.h"
#include "win_time.h"
#include "../libs/os/file.h"
#include "../libs/os/path.h"
#include "../libs/enum_helper.h"


static bool is_alpha(char c) {
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

static bool is_number(char c)
{
	return (c >= '0' && c <= '9');
}

static bool is_end_char(char c)
{
	return (c == '\0');
}

enum Token_Type {
	TOKEN_TYPE_UNKNOWN,
	TOKEN_TYPE_EOF,
	TOKEN_TYPE_SPACE,
	TOKEN_TYPE_NEW_LINE,
	TOKEN_TYPE_NODE,
	TOKEN_TYPE_VARIABLE,
	TOKEN_TYPE_INT_VALUE,
	TOKEN_TYPE_FLOAT_VALUE,
	TOKEN_TYPE_STRING_VALUE,
	TOKEN_TYPE_VECTOR3_VALUE,
};

struct Token {
	Token() {}
	Token(Token_Type type);
	Token(Token_Type type, u32 line, u32 column);

	Token_Type type;
	u32 line;
	u32 column;
	String string;
};

Token::Token(Token_Type type) : type(type)
{
}

Token::Token(Token_Type type, u32 line, u32 column) : type(type), line(line), column(column)
{
}

struct Map_Scanner {
	u32 line;
	u32 column;
	u32 next_char_index;
	u32 current_char_index;
	String map_data;

	void reset_state();
	void reset_advance();
	void go_to_next_char();

	char advance();
	char next_char();
	char current_char();

	bool is_node();
	bool is_variable();
	bool is_int_value();
	bool is_float_value();
	
	bool is_new_line(char character);
	bool load_map(const char *map_name);

	Token get_token();
	Token make_token(Token_Type type);

	void get_line(u32 line_number, String *line_string);
};

bool Map_Scanner::is_node()
{
	char character = advance();
	while (is_alpha(character) || (character == ':') || (character == '_')) {
		if (character == ':') {
			return true;
		}
		character = advance();
	}
	reset_advance();
	return false;
}

bool Map_Scanner::is_variable()
{
	char character = advance();
	while (is_alpha(character)) {
		if (next_char() == ' ') {
			return true;
		}
		character = advance();
	}
	reset_advance();
	return false;
}

bool Map_Scanner::is_int_value()
{
	char character = advance();
	while (is_number(character)) {
		character = advance();
	}
	return true;
}

bool Map_Scanner::is_float_value()
{
	return false;
}

bool Map_Scanner::is_new_line(char character)
{
	if (character == '\n') {
		go_to_next_char();
		return true;
	}
	if ((character == '\r') && (map_data.data[current_char_index + 1])) {
		go_to_next_char();
		go_to_next_char();
		return true;
	}
	return false;
}

void Map_Scanner::reset_state()
{
	line = 1;
	column = 1;
	current_char_index = 0;
	next_char_index = 0;
	map_data.free();
}

void Map_Scanner::reset_advance()
{
	next_char_index = current_char_index;
}

void Map_Scanner::go_to_next_char()
{
	current_char_index++;
	next_char_index = current_char_index;
}

char Map_Scanner::advance()
{
	return map_data.data[next_char_index++];
}

char Map_Scanner::next_char()
{
	return map_data.data[next_char_index];
}

char Map_Scanner::current_char()
{
	return map_data.data[current_char_index];
}

bool Map_Scanner::load_map(const char *map_name)
{
	reset_state();

	String path_to_map_file;
	build_full_path_to_map_file(map_name, path_to_map_file);

	File map_file;
	if (!map_file.open(path_to_map_file, FILE_MODE_READ, FILE_OPEN_EXISTING)) {	
		print("Map_Scanner::load_map: Failed to load map {}.", map_name);
		return false;
	}
	
	map_data.allocate(map_file.file_size + 1);
	map_file.read((void *)map_data.data, map_file.file_size);
	map_data.place_end_char();
	return true;
}

Token Map_Scanner::get_token()
{
	char character = current_char();

	if (is_alpha(character)) {

		if (is_node()) {
			return make_token(TOKEN_TYPE_NODE);
		}

		if (is_variable()) {
			return make_token(TOKEN_TYPE_VARIABLE);
		}
	}

	if (is_new_line(character)) {
		u32 prev_column = column;
		column = 0;
		return Token(TOKEN_TYPE_NEW_LINE, line++, prev_column);
	}

	if (is_number(character)) {
		if (is_int_value()) {
			return make_token(TOKEN_TYPE_INT_VALUE);
		}
	}

	if (character == ' ') {
		go_to_next_char();
		return Token(TOKEN_TYPE_SPACE, line, column++);
	}

	if (is_end_char(character)) {
		return Token(TOKEN_TYPE_EOF);
	}
	
	go_to_next_char();
	Token token;
	token.type = TOKEN_TYPE_UNKNOWN;
	token.string = String(character);
	return token;
}

Token Map_Scanner::make_token(Token_Type type)
{
	Token token;
	if ((type == TOKEN_TYPE_NODE)) {
		token.string = String(map_data, current_char_index, next_char_index - 1);
	} else {
		token.string = String(map_data, current_char_index, next_char_index);
	}
	token.line = line;
	token.column = column;
	token.type = type;
	
	column += next_char_index - current_char_index;
	current_char_index = next_char_index;
	
	return token;
}

void Map_Scanner::get_line(u32 line_number, String *line_string)
{
	u32 start_index = 0;
	u32 current_line = 1;
	
	for (u32 char_index = 0; char_index < map_data.len; char_index++) {
		char character = map_data.data[char_index];

		if ((line_number == current_line ) && ((character == '\n') || ((character == '\r') && (map_data.data[char_index + 1] == '\n')))) {
			*line_string = String(map_data.data, start_index, char_index - 1);
			line_string->copy(map_data.data, start_index, char_index - 1);
			break;
		}

		if (character == '\n') {
			start_index = char_index + 1;
			current_line++;
		}

		if ((character == '\r') && (map_data.data[char_index + 1] == '\n')) {
			start_index = char_index + 2;
			current_line++;
		}
	}
}

struct Map_Parser {
	Enum_Helper<Token_Type> *enum_helper = NULL;
	String path_to_map_file;
	Map_Scanner scanner;
	Array<String> error_messages;

	Map_Parser();
	~Map_Parser();
	
	void debug_token(Token *token);
	void parse(const char *map_name);
	void parse_game_world_node();

	void report_error(Token_Type expecting_token, Token_Type actual_token, u32 line, u32 column);
	void print_erros();
};

Map_Parser::Map_Parser()
{
	set_simple_enum_formatting();
	enum_helper = MAKE_ENUM_HELPER(Token_Type, TOKEN_TYPE_UNKNOWN, TOKEN_TYPE_EOF, TOKEN_TYPE_SPACE, TOKEN_TYPE_NEW_LINE, TOKEN_TYPE_NODE, 
								   TOKEN_TYPE_VARIABLE, TOKEN_TYPE_INT_VALUE, TOKEN_TYPE_FLOAT_VALUE, TOKEN_TYPE_STRING_VALUE, TOKEN_TYPE_VECTOR3_VALUE);
}

Map_Parser::~Map_Parser()
{
	DELETE_PTR(enum_helper);
}

void Map_Parser::debug_token(Token *token)
{
	static u32 last_printed_line = 0;
	static u32 token_index = 0;

	String str_token = enum_helper->to_string(token->type);

	if (last_printed_line != token->line) {
		last_printed_line = token->line;
		if (token_index < 10) {
			print("[0{}] {} {} {}", token_index, token->line, token->column, str_token);
		} else {
			print("[{}] {} {} {}", token_index, token->line, token->column, str_token);
		}
	} else {
		if (token_index < 10) {
			print("[0{}] | {} {}", token_index, token->column, str_token);
		} else {
			print("[{}] | {} {}", token_index, token->column, str_token);
		}
	}
	token_index++;
}

#define ARRAY_SIZE(x) (u32)(sizeof(x) / sizeof(x[0]))

Token_Type node_rules[] = { TOKEN_TYPE_NEW_LINE, TOKEN_TYPE_SPACE, TOKEN_TYPE_SPACE, TOKEN_TYPE_SPACE, TOKEN_TYPE_SPACE };

void Map_Parser::parse(const char *map_name)
{
	build_full_path_to_map_file(map_name, path_to_map_file);

	print("\n[Info] Map_Parser::parse: is starting to parse {}.", map_name);
	if (!scanner.load_map(map_name)) {
		return;
	}

	while (true) {
		Token token = scanner.get_token();

		debug_token(&token);

		if (token.type == TOKEN_TYPE_NODE) {
			for (u32 i = 0; i < ARRAY_SIZE(node_rules); i++) {
				Token next_token = scanner.get_token();

				if (next_token.type != node_rules[i]) {
					//String line_with_error;
					//scanner.get_line(next_token.line, &line_with_error);
					//print("File '{}', line {}", path_to_map_file, next_token.line);
					//print(line_with_error);
					//String test;
					//for (int i = 0; i < next_token.column; i++) {
					//	test.append("-");
					//}
					//test.append("^");
					//print(test);
					//print("Token Error: Expecting token is {}, Actual token is {}.", enum_helper->to_string(node_rules[i]), enum_helper->to_string(next_token.type));
					break;
				}
			}
		}


		if (token.type == TOKEN_TYPE_EOF) {
			break;
		}
	}
	print("");
}

void Map_Parser::report_error(Token_Type expecting_token, Token_Type actual_token, u32 line, u32 column)
{

	String line_with_error;
	scanner.get_line(line, &line_with_error);
	String error_arrow;
	for (u32 i = 0; i < column; i++) {
		error_arrow.append("-");
	}
	error_arrow.append("^");
	char *error_message = format("File '{}', line {}\n{}\n{}\nToken Error: Expecting token is {}, Actual token is {}.", 
								  path_to_map_file, line, line_with_error, error_arrow, enum_helper->to_string(expecting_token), enum_helper->to_string(actual_token));

	error_messages.push(error_message);
	free_string(error_message);
}

void test()
{

	Map_Parser parser;
	parser.parse("base.map");
}